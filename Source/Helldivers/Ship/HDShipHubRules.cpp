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
