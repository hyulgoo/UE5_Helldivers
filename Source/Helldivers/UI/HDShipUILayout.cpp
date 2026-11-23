// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipUILayout.h"
#include "UI/HDShipUIStyle.h"

namespace HDShipUILayoutPrivate
{
    using HDShipUIStyle::DesignWidth;
    using HDShipUIStyle::DesignHeight;
    using HDShipUIStyle::Margin;
    using HDShipUIStyle::TopBarHeight;
    using HDShipUIStyle::BottomBarHeight;
    using HDShipUIStyle::Gap;
    using HDShipUIStyle::DetailWidth;

    const FString       Ellipsis(TEXT("..."));
    // 이보다 작게 겹치거나 넘치면 문제로 보지 않는다(반올림 오차)
    constexpr float     ProblemTolerance = 0.5f;

    // 설계 영역(안전 영역 기준 설계 좌표)
    const FBox2D        TopBarRect      (FVector2D(0.f, 0.f), FVector2D(DesignWidth, TopBarHeight));
    const FBox2D        ContentRect     (FVector2D(Margin, TopBarHeight + Gap), FVector2D(DesignWidth - Margin, DesignHeight - BottomBarHeight - Gap));
    const FBox2D        BottomBarRect   (FVector2D(0.f, DesignHeight - BottomBarHeight), FVector2D(DesignWidth, DesignHeight));
    const FBox2D        MainRect        (FVector2D(Margin, TopBarHeight + Gap), FVector2D(DesignWidth - Margin - DetailWidth - Gap, DesignHeight - BottomBarHeight - Gap));
    const FBox2D        DetailRect      (FVector2D(DesignWidth - Margin - DetailWidth, TopBarHeight + Gap), FVector2D(DesignWidth - Margin, DesignHeight - BottomBarHeight - Gap));

    // 뒤에서부터 글자를 잘라 "..." 를 붙여 MaxWidth 에 맞춘다. bForce 면 다 맞아도 "..." 를 붙인다. "..." 도 안 들어가면 빈 글자
    FString TruncateWithEllipsis(const FString& Text, float MaxWidth, int32 FontPx, bool bForce, HDShipUILayout::FMeasureFn Measure)
    {
        if (bForce == false && Measure(Text, FontPx) <= MaxWidth)
        {
            return Text;
        }

        if (Measure(Ellipsis, FontPx) > MaxWidth)
        {
            return FString();
        }

        FString Kept = Text;
        while (Kept.IsEmpty() == false)
        {
            const FString Candidate = Kept.TrimEnd() + Ellipsis;
            if (Measure(Candidate, FontPx) <= MaxWidth)
            {
                return Candidate;
            }
            Kept.LeftChopInline(1);
        }

        return Ellipsis;
    }

    // 넓이가 있는 겹침인가(두 축 모두 ProblemTolerance 넘게)
    bool HasAreaOverlap(const FBox2D& A, const FBox2D& B)
    {
        const double Width = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
        const double Height = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
        return Width > ProblemTolerance && Height > ProblemTolerance;
    }

    // Inner 가 Outer 밖으로 ProblemTolerance 넘게 나갔는가
    bool IsOutside(const FBox2D& Outer, const FBox2D& Inner)
    {
        return Inner.Min.X < Outer.Min.X - ProblemTolerance || Inner.Min.Y < Outer.Min.Y - ProblemTolerance
            || Inner.Max.X > Outer.Max.X + ProblemTolerance || Inner.Max.Y > Outer.Max.Y + ProblemTolerance;
    }

    const TCHAR* GetHitBoxPrefix(HDShipUILayout::EHDShipHitKind Kind)
    {
        switch (Kind)
        {
        case HDShipUILayout::EHDShipHitKind::Item:  return TEXT("ShipItem_");
        case HDShipUILayout::EHDShipHitKind::Card:  return TEXT("ShipCard_");
        case HDShipUILayout::EHDShipHitKind::Tab:   return TEXT("ShipTab_");
        case HDShipUILayout::EHDShipHitKind::Pick:  return TEXT("ShipPick_");
        case HDShipUILayout::EHDShipHitKind::Key:   return TEXT("ShipKey_");
        default:                                    return nullptr;
        }
    }

    // 숫자 글자만 있는 0 이상의 정수
    bool TryParseIndex(const FString& Text, int32& OutValue)
    {
        if (Text.IsEmpty() || Text.Len() > 9)
        {
            return false;
        }

        for (const TCHAR Char : Text)
        {
            if (FChar::IsDigit(Char) == false)
            {
                return false;
            }
        }

        OutValue = FCString::Atoi(*Text);
        return true;
    }
}

FHDShipUIFrame HDShipUILayout::ComputeFrame(const FVector2D& ViewSize)
{
    using namespace HDShipUILayoutPrivate;

    FHDShipUIFrame Frame;
    Frame.View  = FBox2D(FVector2D::ZeroVector, ViewSize);
    Frame.Scale = HDShipUIStyle::ComputeScale(ViewSize);

    // 화면 안에 들어가는 가장 큰 16:9 를 가운데에
    const double Aspect = DesignWidth / DesignHeight;
    FVector2D SafeSize = ViewSize;
    if (ViewSize.Y > 0.0 && ViewSize.X / ViewSize.Y > Aspect)
    {
        SafeSize.X = ViewSize.Y * Aspect;
    }
    else
    {
        SafeSize.Y = ViewSize.X / Aspect;
    }
    const FVector2D SafeMin = (ViewSize - SafeSize) * 0.5;
    Frame.Safe = FBox2D(SafeMin, SafeMin + SafeSize);

    Frame.TopBar    = ToScreen(Frame, TopBarRect);
    Frame.Content   = ToScreen(Frame, ContentRect);
    Frame.BottomBar = ToScreen(Frame, BottomBarRect);
    Frame.Main      = ToScreen(Frame, MainRect);
    Frame.Detail    = ToScreen(Frame, DetailRect);
    return Frame;
}

FBox2D HDShipUILayout::ToScreen(const FHDShipUIFrame& Frame, const FBox2D& DesignRect)
{
    // 설계 영역(1920×1080 × 배율)을 안전 영역 가운데에 둔다. 배율이 잘리지 않았으면 안전 영역과 같다
    const FVector2D DesignSize(HDShipUIStyle::DesignWidth * Frame.Scale, HDShipUIStyle::DesignHeight * Frame.Scale);
    const FVector2D Origin = Frame.Safe.GetCenter() - DesignSize * 0.5;
    return FBox2D(Origin + DesignRect.Min * Frame.Scale, Origin + DesignRect.Max * Frame.Scale);
}

TArray<FBox2D> HDShipUILayout::ComputeListRows(const FHDShipUIFrame& Frame, int32 Count, bool bCentered)
{
    using namespace HDShipUIStyle;

    TArray<FBox2D> Rows;
    if (Count <= 0)
    {
        return Rows;
    }

    const float RowWidthPx  = RowWidth * Frame.Scale;
    const float RowHeightPx = RowHeight * Frame.Scale;
    const float StepPx      = (RowHeight + RowGap) * Frame.Scale;
    const float TotalHeight = RowHeightPx + StepPx * (Count - 1);

    FVector2D Start = Frame.Main.Min;
    if (bCentered)
    {
        const FVector2D Center = Frame.Content.GetCenter();
        Start = FVector2D(Center.X - RowWidthPx * 0.5f, Center.Y - TotalHeight * 0.5f);
    }

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FVector2D Min(Start.X, Start.Y + StepPx * Index);
        Rows.Add(FBox2D(Min, Min + FVector2D(RowWidthPx, RowHeightPx)));
    }
    return Rows;
}

HDShipUILayout::FHDFitResult HDShipUILayout::FitText(const FString& Text, float MaxWidth, int32 FontPx, int32 MinFontPx, FMeasureFn Measure)
{
    FHDFitResult Result;
    Result.FontPx = FontPx;
    if (Text.IsEmpty())
    {
        return Result;
    }

    const int32 SmallestPx = FMath::Min(MinFontPx, FontPx);
    for (int32 Px = FontPx; Px >= SmallestPx; --Px)
    {
        if (Measure(Text, Px) <= MaxWidth)
        {
            Result.Text     = Text;
            Result.FontPx   = Px;
            return Result;
        }
    }

    Result.FontPx       = SmallestPx;
    Result.Text         = HDShipUILayoutPrivate::TruncateWithEllipsis(Text, MaxWidth, SmallestPx, false, Measure);
    Result.bTruncated   = true;
    return Result;
}

TArray<FString> HDShipUILayout::WrapText(const FString& Text, float MaxWidth, int32 FontPx, int32 MaxLines, FMeasureFn Measure)
{
    TArray<FString> Words;
    Text.ParseIntoArrayWS(Words);

    TArray<FString> Lines;
    FString Current;
    for (const FString& Word : Words)
    {
        const FString Candidate = Current.IsEmpty() ? Word : Current + TEXT(" ") + Word;
        if (Current.IsEmpty() == false && Measure(Candidate, FontPx) > MaxWidth)
        {
            Lines.Add(Current);
            Current = Word;
            continue;
        }
        Current = Candidate;
    }
    if (Current.IsEmpty() == false)
    {
        Lines.Add(Current);
    }

    // 한 단어가 한 줄보다 길면 그 줄을 말줄임
    for (FString& Line : Lines)
    {
        Line = HDShipUILayoutPrivate::TruncateWithEllipsis(Line, MaxWidth, FontPx, false, Measure);
    }

    if (MaxLines > 0 && Lines.Num() > MaxLines)
    {
        Lines.SetNum(MaxLines);
        Lines.Last() = HDShipUILayoutPrivate::TruncateWithEllipsis(Lines.Last(), MaxWidth, FontPx, true, Measure);
    }

    return Lines;
}

HDShipUILayout::FHDTextProblems HDShipUILayout::FindTextProblems(const TArray<FHDDrawnText>& Texts, const FBox2D& View)
{
    using namespace HDShipUILayoutPrivate;

    FHDTextProblems Problems;
    Problems.Texts = Texts.Num();

    for (int32 Index = 0; Index < Texts.Num(); ++Index)
    {
        const FHDDrawnText& Drawn = Texts[Index];
        bool bProblem = false;

        if (Drawn.bTruncated)
        {
            ++Problems.Truncated;
        }

        if (IsOutside(View, Drawn.Rect) || (Drawn.Region.bIsValid && IsOutside(Drawn.Region, Drawn.Rect)))
        {
            ++Problems.Clipped;
            bProblem = true;
        }

        for (int32 Other = 0; Other < Index; ++Other)
        {
            if (HasAreaOverlap(Drawn.Rect, Texts[Other].Rect))
            {
                ++Problems.Overlaps;
                bProblem = true;
            }
        }

        if (bProblem && Problems.FirstProblem.IsEmpty())
        {
            Problems.FirstProblem = Drawn.Text;
        }
    }

    return Problems;
}

FName HDShipUILayout::MakeHitBoxName(EHDShipHitKind Kind, int32 A, int32 B)
{
    const TCHAR* Prefix = HDShipUILayoutPrivate::GetHitBoxPrefix(Kind);
    if (Prefix == nullptr)
    {
        return NAME_None;
    }

    return Kind == EHDShipHitKind::Card
        ? FName(*FString::Printf(TEXT("%s%d_%d"), Prefix, A, B))
        : FName(*FString::Printf(TEXT("%s%d"), Prefix, A));
}

bool HDShipUILayout::ParseHitBoxName(FName Name, EHDShipHitKind& OutKind, int32& OutA, int32& OutB)
{
    const FString Text = Name.ToString();
    for (const EHDShipHitKind Kind : { EHDShipHitKind::Item, EHDShipHitKind::Card, EHDShipHitKind::Tab, EHDShipHitKind::Pick, EHDShipHitKind::Key })
    {
        const FString Prefix = HDShipUILayoutPrivate::GetHitBoxPrefix(Kind);
        if (Text.StartsWith(Prefix, ESearchCase::CaseSensitive) == false)
        {
            continue;
        }

        const FString Rest = Text.RightChop(Prefix.Len());
        int32 A = 0;
        int32 B = 0;
        if (Kind == EHDShipHitKind::Card)
        {
            FString AText;
            FString BText;
            if (Rest.Split(TEXT("_"), &AText, &BText) == false
                || HDShipUILayoutPrivate::TryParseIndex(AText, A) == false
                || HDShipUILayoutPrivate::TryParseIndex(BText, B) == false)
            {
                return false;
            }
        }
        else if (HDShipUILayoutPrivate::TryParseIndex(Rest, A) == false)
        {
            return false;
        }

        OutKind = Kind;
        OutA    = A;
        OutB    = B;
        return true;
    }

    return false;
}

bool HDShipUILayout::HasMouseMoved(const FVector2D& Anchor, const FVector2D& Now, float Threshold)
{
    return FVector2D::Distance(Anchor, Now) > Threshold;
}

void FHDShipUITransition::Open(float Seconds)
{
    TargetAlpha     = 1.f;
    ContentAlpha    = 1.f;
    ContentRate     = 0.f;
    if (Seconds <= 0.f)
    {
        Alpha       = 1.f;
        AlphaRate   = 0.f;
        return;
    }
    AlphaRate       = 1.f / Seconds;
}

void FHDShipUITransition::Close(float Seconds)
{
    TargetAlpha     = 0.f;
    if (Seconds <= 0.f)
    {
        Alpha       = 0.f;
        AlphaRate   = 0.f;
        return;
    }
    AlphaRate       = 1.f / Seconds;
}

void FHDShipUITransition::ChangeScreen(float Seconds)
{
    if (Seconds <= 0.f)
    {
        ContentAlpha    = 1.f;
        ContentRate     = 0.f;
        return;
    }
    ContentAlpha        = 0.f;
    ContentRate         = 1.f / Seconds;
}

void FHDShipUITransition::Tick(float DeltaSeconds)
{
    if (DeltaSeconds <= 0.f)
    {
        return;
    }

    Alpha = TargetAlpha > Alpha
        ? FMath::Min(TargetAlpha, Alpha + AlphaRate * DeltaSeconds)
        : FMath::Max(TargetAlpha, Alpha - AlphaRate * DeltaSeconds);

    ContentAlpha = FMath::Min(1.f, ContentAlpha + ContentRate * DeltaSeconds);
}

float FHDShipUITransition::GetAlpha() const
{
    return Alpha;
}

float FHDShipUITransition::GetContentAlpha() const
{
    return ContentAlpha;
}

float FHDShipUITransition::GetSlide() const
{
    return TargetAlpha > 0.f ? 1.f - Alpha : 0.f;
}

bool FHDShipUITransition::IsVisible() const
{
    return Alpha > 0.f;
}

bool FHDShipUITransition::IsSettled() const
{
    return Alpha >= 1.f && TargetAlpha >= 1.f && ContentAlpha >= 1.f;
}
