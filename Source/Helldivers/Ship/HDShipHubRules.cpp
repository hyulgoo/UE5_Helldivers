// Fill out your copyright notice in the Description page of Project Settings.

#include "Ship/HDShipHubRules.h"

namespace HDShipHubPrivate
{
    FHDShipTerminalInfo MakeTerminal(EHDShipTerminal Terminal, const TCHAR* Id, const TCHAR* Label, EHDShipScreen RootScreen, const FVector& LocalLocation, float LocalYaw)
    {
        FHDShipTerminalInfo Info;
        Info.Terminal       = Terminal;
        Info.Id             = FName(Id);
        Info.Label          = Label;
        Info.RootScreen     = RootScreen;
        Info.LocalLocation  = LocalLocation;
        Info.LocalYaw       = LocalYaw;
        return Info;
    }
}

const TArray<FHDShipTerminalInfo>& HDShipHub::GetTerminals()
{
    using namespace HDShipHubPrivate;

    // 작전 테이블은 방 가운데, 함선 관리·무기고는 뒤쪽 양옆 벽 앞에서 방 안쪽을 본다.
    // 가장 가까운 두 단말기도 약 1209 떨어져 범위(300)가 겹치지 않는다
    static const TArray<FHDShipTerminalInfo> Terminals =
    {
        MakeTerminal(EHDShipTerminal::WarTable,       TEXT("WarTable"),       TEXT("WAR TABLE"),       EHDShipScreen::WarTable, FVector(0.f, 0.f, 0.f),          0.f),
        MakeTerminal(EHDShipTerminal::ShipManagement, TEXT("ShipManagement"), TEXT("SHIP MANAGEMENT"), EHDShipScreen::Upgrades, FVector(-600.f, -1050.f, 0.f),   90.f),
        MakeTerminal(EHDShipTerminal::Armory,         TEXT("Armory"),         TEXT("ARMORY"),          EHDShipScreen::Armory,   FVector(-600.f, 1050.f, 0.f),    -90.f),
    };
    return Terminals;
}

const FHDShipTerminalInfo* HDShipHub::FindTerminal(FName Id)
{
    return GetTerminals().FindByPredicate([Id](const FHDShipTerminalInfo& Terminal) { return Terminal.Id == Id; });
}

FVector HDShipHub::GetRoomHalfExtent()
{
    return FVector(2000.f, 1200.f, 700.f);
}

FVector HDShipHub::GetCrewSpawnLocation()
{
    return FVector(-1500.f, 0.f, 100.f);
}

float HDShipHub::GetCrewSpawnYaw()
{
    return 0.f;
}

int32 HDShipHub::FindTerminalInRange(const FVector& LocalLocation, const TArray<FHDShipTerminalInfo>& Terminals, float Range)
{
    if (Range <= 0.f)
    {
        return INDEX_NONE;
    }

    int32 NearestIndex = INDEX_NONE;
    double NearestDistanceSquared = FMath::Square(static_cast<double>(Range));
    for (int32 Index = 0; Index < Terminals.Num(); ++Index)
    {
        // 높이는 보지 않는다(단말기 본체 높이와 폰 캡슐 가운데가 다르다)
        const double DistanceSquared = FVector::DistSquared2D(LocalLocation, Terminals[Index].LocalLocation);
        if (DistanceSquared <= NearestDistanceSquared)
        {
            NearestIndex = Index;
            NearestDistanceSquared = DistanceSquared;
        }
    }

    return NearestIndex;
}

bool HDShipHub::IsInteractKey(const FKey& Key, EInputEvent Event)
{
    return Event == IE_Pressed && Key == EKeys::E;
}

FString HDShipHub::FormatInteractPrompt(const FHDShipTerminalInfo& Terminal)
{
    return FString::Printf(TEXT("[E] %s"), *Terminal.Label);
}

FString HDShipHub::FormatCountdown(float RemainingSeconds)
{
    return FString::Printf(TEXT("HELLPOD LAUNCH IN %d"), FMath::CeilToInt(FMath::Max(0.f, RemainingSeconds)));
}

FVector HDShipHub::GetPromptAnchor(const FHDShipTerminalInfo& Terminal)
{
    const float AnchorZ = Terminal.Terminal == EHDShipTerminal::WarTable ? TableAnchorZ : ConsoleAnchorZ;
    return Terminal.LocalLocation + FVector(0.f, 0.f, AnchorZ);
}

namespace HDShipHubPromptPrivate
{
    // 활성이 투영되지 않았을 때 아래 변 = 안전 영역 아래 - 간격 × 이 값
    constexpr float     OffscreenBottomGaps = 8.f;
    constexpr float     Tolerance           = 0.01f;

    bool IsPointInside(const FBox2D& Rect, const FVector2D& Point)
    {
        return Point.X >= Rect.Min.X && Point.X <= Rect.Max.X && Point.Y >= Rect.Min.Y && Point.Y <= Rect.Max.Y;
    }

    // 넓이가 있는 겹침의 높이. 없으면 0
    double GetOverlapHeight(const FBox2D& A, const FBox2D& B)
    {
        const double Width = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
        const double Height = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
        return (Width > Tolerance && Height > Tolerance) ? Height : 0.0;
    }

    // 상자를 안전 영역 안으로 민다(크기는 그대로). 밀었으면 참
    bool ClampInto(const FBox2D& Safe, FBox2D& Rect)
    {
        FVector2D Delta = FVector2D::ZeroVector;
        if (Rect.Min.X < Safe.Min.X)        { Delta.X = Safe.Min.X - Rect.Min.X; }
        else if (Rect.Max.X > Safe.Max.X)   { Delta.X = Safe.Max.X - Rect.Max.X; }
        if (Rect.Min.Y < Safe.Min.Y)        { Delta.Y = Safe.Min.Y - Rect.Min.Y; }
        else if (Rect.Max.Y > Safe.Max.Y)   { Delta.Y = Safe.Max.Y - Rect.Max.Y; }

        Rect = Rect.ShiftBy(Delta);
        return Delta.IsNearlyZero(Tolerance) == false;
    }

    bool IsInside(const FBox2D& Safe, const FBox2D& Rect)
    {
        return Rect.Min.X >= Safe.Min.X - Tolerance && Rect.Min.Y >= Safe.Min.Y - Tolerance
            && Rect.Max.X <= Safe.Max.X + Tolerance && Rect.Max.Y <= Safe.Max.Y + Tolerance;
    }
}

TArray<FHDPromptTag> HDShipHub::PlacePromptTags(const TArray<FHDPromptTagInput>& Inputs, const FBox2D& SafeRect, float Gap, float ShowDistance)
{
    using namespace HDShipHubPromptPrivate;

    TArray<FHDPromptTag> Tags;
    Tags.SetNum(Inputs.Num());

    // 1. 보일지 정한다
    TArray<int32> Order;
    for (int32 Index = 0; Index < Inputs.Num(); ++Index)
    {
        const FHDPromptTagInput& Input = Inputs[Index];
        FHDPromptTag& Tag = Tags[Index];
        Tag.Id      = Input.Id;
        Tag.bActive = Input.bInRange;

        const bool bPassiveVisible = Input.bProjected && IsPointInside(SafeRect, Input.ScreenAnchor) && Input.Distance2D <= ShowDistance;
        if (Tag.bActive || bPassiveVisible)
        {
            Order.Add(Index);
        }
    }

    // 2. 활성 먼저, 그다음 투영점 Y 가 큰 것부터. 같으면 입력 순서
    Order.Sort([&Inputs](int32 A, int32 B)
    {
        if (Inputs[A].bInRange != Inputs[B].bInRange)
        {
            return Inputs[A].bInRange;
        }
        if (Inputs[A].ScreenAnchor.Y != Inputs[B].ScreenAnchor.Y)
        {
            return Inputs[A].ScreenAnchor.Y > Inputs[B].ScreenAnchor.Y;
        }
        return A < B;
    });

    TArray<FBox2D> Placed;
    for (const int32 Index : Order)
    {
        const FHDPromptTagInput& Input = Inputs[Index];
        FHDPromptTag& Tag = Tags[Index];

        // 3. 처음 자리
        const bool bOffscreenActive = Tag.bActive && Input.bProjected == false;
        const double CenterX = bOffscreenActive ? SafeRect.GetCenter().X : Input.ScreenAnchor.X;
        const double Bottom = bOffscreenActive ? SafeRect.Max.Y - Gap * OffscreenBottomGaps : Input.ScreenAnchor.Y - Gap;
        FBox2D Rect(FVector2D(CenterX - Input.Size.X * 0.5, Bottom - Input.Size.Y), FVector2D(CenterX + Input.Size.X * 0.5, Bottom));

        const bool bClamped = ClampInto(SafeRect, Rect);
        Tag.bEdgeClamped = Tag.bActive && (bClamped || bOffscreenActive);

        // 4. 겹치면 위로, 위로 넘치면 안전 영역 안으로 자른 뒤 아래로
        bool bMoveDown = false;
        bool bFound = false;
        for (int32 Push = 0; Push <= MaxTagPushes; ++Push)
        {
            int32 Hit = INDEX_NONE;
            for (int32 Other = 0; Other < Placed.Num(); ++Other)
            {
                if (GetOverlapHeight(Rect, Placed[Other]) > 0.0)
                {
                    Hit = Other;
                    break;
                }
            }

            if (Hit == INDEX_NONE && IsInside(SafeRect, Rect))
            {
                bFound = true;
                break;
            }
            if (Push == MaxTagPushes || Hit == INDEX_NONE)
            {
                break;
            }

            if (bMoveDown == false)
            {
                Rect = Rect.ShiftBy(FVector2D(0.0, -(GetOverlapHeight(Rect, Placed[Hit]) + Gap)));
                if (Rect.Min.Y < SafeRect.Min.Y - Tolerance)
                {
                    Rect = Rect.ShiftBy(FVector2D(0.0, SafeRect.Min.Y - Rect.Min.Y));
                    bMoveDown = true;
                }
            }
            else
            {
                Rect = Rect.ShiftBy(FVector2D(0.0, Placed[Hit].Max.Y + Gap - Rect.Min.Y));
            }
        }

        // 활성은 늘 처음에 놓이므로 숨지 않는다
        if (bFound || Tag.bActive)
        {
            Tag.Rect        = Rect;
            Tag.bVisible    = true;
            Placed.Add(Rect);
        }
    }

    return Tags;
}

bool FHDLaunchCountdown::Start(float Seconds)
{
    if (bActive)
    {
        return false;
    }

    bActive     = true;
    Remaining   = FMath::Max(0.f, Seconds);
    return true;
}

bool FHDLaunchCountdown::Cancel()
{
    if (bActive == false)
    {
        return false;
    }

    bActive     = false;
    Remaining   = -1.f;
    return true;
}

bool FHDLaunchCountdown::Tick(float DeltaSeconds)
{
    if (bActive == false)
    {
        return false;
    }

    Remaining -= FMath::Max(0.f, DeltaSeconds);
    if (Remaining > 0.f)
    {
        return false;
    }

    // 한 번만 알리고 끈다
    bActive     = false;
    Remaining   = -1.f;
    return true;
}

bool FHDLaunchCountdown::IsActive() const
{
    return bActive;
}

float FHDLaunchCountdown::GetRemaining() const
{
    return bActive ? Remaining : -1.f;
}

int32 FHDLaunchCountdown::GetDisplaySeconds() const
{
    return bActive ? FMath::CeilToInt(Remaining) : 0;
}
