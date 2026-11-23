// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipScreenPainter.h"
#include "UI/HDShipUICanvas.h"
#include "UI/HDShipUIStyle.h"
#include "UI/HDShipUILayout.h"
#include "UI/HDShipMenu.h"
#include "UI/HDShipMenuView.h"
#include "Ship/HDShipHubRules.h"
#include "Ship/HDWarTableMap.h"
#include "Game/HDMissionSettings.h"

namespace HDShipScreenPainterPrivate
{
    using namespace HDShipUIStyle;

    // 위 바(설계 단위): 제목 줄 가운데 y 44. 자원 줄은 마지막 미션 줄이 있으면 y 30, 그 줄은 y 68
    constexpr float     TopCenterY          = 44.f;
    constexpr float     ResourceRowY        = 30.f;
    constexpr float     LastMissionY        = 68.f;
    constexpr float     ResourceTileSize    = 30.f;
    constexpr float     ResourceTileGap     = 8.f;
    constexpr float     ResourceItemGap     = 24.f;
    constexpr float     TitleResourceGap    = 32.f;

    // 아래 바: 가운데 y 1048, 키 상자와 동작 이름 사이 8, 안내 사이 32
    constexpr float     BottomCenterY       = DesignHeight - BottomBarHeight * 0.5f;
    constexpr float     HintInnerGap        = 8.f;
    constexpr float     HintGap             = 32.f;

    // 줄 버튼 안 글자 왼쪽 여백, 상세 패널 안쪽 여백과 머리 높이
    constexpr float     RowTextPadding      = 20.f;
    constexpr float     DetailPadding       = 24.f;
    constexpr float     DetailHeaderHeight  = 40.f;
    constexpr int32     DetailMaxLines      = 8;

    // 출격 대기(설계 y): 머리 330, 숫자 520, 취소 안내 720
    constexpr float     CountdownHeaderY    = 330.f;
    constexpr float     CountdownNumberY    = 520.f;
    constexpr float     CountdownCancelY    = 720.f;

    // 함선 안 단말기 태그(설계 단위): 좌우 여백 12, 높이 수동 28·활성 40. 키 상자 폭 = 글자 + 20(KeyCap 양옆 여백 10)
    constexpr float     TagPadding          = 12.f;
    constexpr float     PassiveTagHeight    = 28.f;
    constexpr float     ActiveTagHeight     = 40.f;
    constexpr float     KeyCapPaddingTotal  = 20.f;
    const TCHAR* const  InteractKeyText     = TEXT("E");

    const TCHAR* const  CountdownHeaderText = TEXT("HELLPOD LAUNCH");
    const TCHAR* const  CancelKeyText       = TEXT("BACKSPACE");
    const TCHAR* const  CancelActionText    = TEXT("CANCEL");
    const TCHAR* const  ShipMenuKeyText     = TEXT("P");
    const TCHAR* const  ShipMenuActionText  = TEXT("SHIP MENU");

    // 설계 점 → 캔버스 점
    FVector2D ToPoint(const FHDShipUICanvas& Canvas, float X, float Y)
    {
        return HDShipUILayout::ToScreen(Canvas.GetFrame(), FBox2D(FVector2D(X, Y), FVector2D(X, Y))).Min;
    }

    FBox2D ToRect(const FHDShipUICanvas& Canvas, float X0, float Y0, float X1, float Y1)
    {
        return HDShipUILayout::ToScreen(Canvas.GetFrame(), FBox2D(FVector2D(X0, Y0), FVector2D(X1, Y1)));
    }

    // 키 상자 + 동작 이름(Small, TextDim)을 Left 에서 시작해 그린다. 합친 상자를 돌려준다
    FBox2D PaintKeyHint(FHDShipUICanvas& Canvas, const FString& Key, const FString& Action, float Left, float CenterY, float MaxRight)
    {
        const FBox2D KeyBox = Canvas.KeyCap(Key, FVector2D(Left, CenterY));
        const float ActionLeft = static_cast<float>(KeyBox.Max.X) + Canvas.Px(HintInnerGap);
        const FBox2D ActionBox = Canvas.Text(Action, EHDShipFont::Small, FVector2D(ActionLeft, CenterY), EHDShipTextAlign::Left, TextDim, FMath::Max(0.f, MaxRight - ActionLeft));

        FBox2D Combined = KeyBox;
        if (ActionBox.bIsValid)
        {
            Combined += ActionBox;
        }
        return Combined;
    }
}

void FHDShipScreenPainter::PaintBackdrop(FHDShipUICanvas& Canvas)
{
    Canvas.FillScreen(HDShipUIStyle::Backdrop);
}

float FHDShipScreenPainter::PaintResources(FHDShipUICanvas& Canvas, const FHDResourceView& Resources)
{
    using namespace HDShipScreenPainterPrivate;

    struct FResourceEntry
    {
        // 아이콘 표 Id(HDShipIcons)
        const TCHAR*    IconId;
        int32           Count;
    };
    const TArray<FResourceEntry> Entries = {
        { TEXT("Requisition"),  Resources.Requisition },
        { TEXT("SampleCommon"), Resources.Samples.Common },
        { TEXT("SampleRare"),   Resources.Samples.Rare },
        { TEXT("SampleSuper"),  Resources.Samples.Super },
    };

    const bool bHasLastMission = Resources.LastMission.IsEmpty() == false;
    const float RowY = bHasLastMission ? ResourceRowY : TopCenterY;
    const int32 BodyPx = GetFontPx(EHDShipFont::Body, Canvas.GetScale());

    // 오른쪽 끝에서 폭을 모두 더해 왼쪽 끝을 구한 뒤 왼쪽부터 그린다
    float TotalWidth = 0.f;
    TArray<float> NumberWidths;
    for (const FResourceEntry& Entry : Entries)
    {
        const float NumberWidth = Canvas.Measure(FString::FromInt(Entry.Count), BodyPx, IsBold(EHDShipFont::Body));
        NumberWidths.Add(NumberWidth);
        TotalWidth += Canvas.Px(ResourceTileSize + ResourceTileGap) + NumberWidth;
    }
    TotalWidth += Canvas.Px(ResourceItemGap) * (Entries.Num() - 1);

    const FVector2D RightPoint = ToPoint(Canvas, DesignWidth - Margin, RowY);
    const float Left = static_cast<float>(RightPoint.X) - TotalWidth;
    const float CenterY = static_cast<float>(RightPoint.Y);
    const float TileSize = Canvas.Px(ResourceTileSize);

    float X = Left;
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        const FResourceEntry& Entry = Entries[Index];
        Canvas.Icon(FName(Entry.IconId), FBox2D(FVector2D(X, CenterY - TileSize * 0.5f), FVector2D(X + TileSize, CenterY + TileSize * 0.5f)), false);
        X += TileSize + Canvas.Px(ResourceTileGap);

        Canvas.Text(FString::FromInt(Entry.Count), EHDShipFont::Body, FVector2D(X, CenterY), EHDShipTextAlign::Left, Text, 0.f);
        X += NumberWidths[Index] + Canvas.Px(ResourceItemGap);
    }

    if (bHasLastMission)
    {
        const FVector2D LastPoint = ToPoint(Canvas, DesignWidth - Margin, LastMissionY);
        Canvas.Text(Resources.LastMission, EHDShipFont::Small, LastPoint, EHDShipTextAlign::Right, TextDim, Canvas.Px(DesignWidth * 0.5f));
    }

    return Left;
}

void FHDShipScreenPainter::PaintTopBar(FHDShipUICanvas& Canvas, const FString& Title, const FHDResourceView& Resources)
{
    using namespace HDShipScreenPainterPrivate;

    const FHDShipUIFrame& Frame = Canvas.GetFrame();
    Canvas.Fill(Frame.TopBar, Panel);
    Canvas.Fill(FBox2D(FVector2D(Frame.TopBar.Min.X, Frame.TopBar.Max.Y - Canvas.Px(BorderThin)), Frame.TopBar.Max), Line);

    Canvas.SetRegion(Frame.TopBar);
    const float ResourcesLeft = PaintResources(Canvas, Resources);

    const FVector2D TitlePoint = ToPoint(Canvas, Margin, TopCenterY);
    const float TitleMaxWidth = FMath::Max(0.f, ResourcesLeft - Canvas.Px(TitleResourceGap) - static_cast<float>(TitlePoint.X));
    Canvas.Text(Title, EHDShipFont::Title, TitlePoint, EHDShipTextAlign::Left, Accent, TitleMaxWidth);
    Canvas.SetRegion(FBox2D(ForceInit));
}

void FHDShipScreenPainter::PaintBottomBar(FHDShipUICanvas& Canvas, const TArray<FHDKeyHint>& Hints)
{
    using namespace HDShipScreenPainterPrivate;

    const FHDShipUIFrame& Frame = Canvas.GetFrame();
    Canvas.Fill(Frame.BottomBar, Panel);
    Canvas.Fill(FBox2D(Frame.BottomBar.Min, FVector2D(Frame.BottomBar.Max.X, Frame.BottomBar.Min.Y + Canvas.Px(BorderThin))), Line);

    Canvas.SetRegion(Frame.BottomBar);
    const FVector2D Start = ToPoint(Canvas, Margin, BottomCenterY);
    const float MaxRight = static_cast<float>(ToPoint(Canvas, DesignWidth - Margin, BottomCenterY).X);

    float X = static_cast<float>(Start.X);
    for (const FHDKeyHint& Hint : Hints)
    {
        const FBox2D HintBox = PaintKeyHint(Canvas, Hint.Key, Hint.Action, X, static_cast<float>(Start.Y), MaxRight);
        if (Hint.bClickable)
        {
            Canvas.HitBox(HintBox, HDShipUILayout::MakeHitBoxName(HDShipUILayout::EHDShipHitKind::Key, static_cast<int32>(Hint.MenuKey)));
        }
        X = static_cast<float>(HintBox.Max.X) + Canvas.Px(HintGap);
    }
    Canvas.SetRegion(FBox2D(ForceInit));
}

void FHDShipScreenPainter::PaintList(FHDShipUICanvas& Canvas, const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items)
{
    using namespace HDShipScreenPainterPrivate;

    const FHDShipUIFrame& Frame = Canvas.GetFrame();
    const bool bCentered = Menu.GetScreen() == EHDShipScreen::System;
    const int32 Cursor = Items.IsEmpty() ? INDEX_NONE : FMath::Clamp(Menu.GetCursor(), 0, Items.Num() - 1);

    Canvas.SetContentLayer(true);

    const TArray<FBox2D> Rows = HDShipUILayout::ComputeListRows(Frame, Items.Num(), bCentered);
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        const FHDShipMenuItem& Item = Items[Index];
        const FBox2D& Row = Rows[Index];
        const bool bSelected = Index == Cursor;

        Canvas.Fill(Row, Card);
        if (bSelected)
        {
            Canvas.Fill(Row, AccentFill);
            Canvas.Border(Row, BorderSelected, Accent);
        }
        else
        {
            Canvas.Border(Row, BorderThin, Line);
        }

        const FLinearColor LabelColor = Item.bEnabled == false ? TextDisabled : (bSelected ? Accent : Text);
        const float TextLeft = static_cast<float>(Row.Min.X) + Canvas.Px(RowTextPadding);
        Canvas.SetRegion(Row);
        Canvas.Text(Item.Label, EHDShipFont::Body, FVector2D(TextLeft, Row.GetCenter().Y), EHDShipTextAlign::Left, LabelColor, static_cast<float>(Row.GetSize().X) - Canvas.Px(RowTextPadding) * 2.f);
        Canvas.HitBox(Row, HDShipUILayout::MakeHitBoxName(HDShipUILayout::EHDShipHitKind::Item, Index));
    }

    // 상세 패널: 고른 줄의 이름과 설명
    if (bCentered == false)
    {
        Canvas.Fill(Frame.Detail, Panel);
        Canvas.Border(Frame.Detail, BorderThin, Line);

        if (Items.IsValidIndex(Cursor))
        {
            const FHDShipMenuItem& Item = Items[Cursor];
            const float Padding = Canvas.Px(DetailPadding);
            const float InnerWidth = static_cast<float>(Frame.Detail.GetSize().X) - Padding * 2.f;
            const FVector2D HeaderPoint(Frame.Detail.Min.X + Padding, Frame.Detail.Min.Y + Padding + Canvas.Px(DetailHeaderHeight) * 0.5f);

            Canvas.SetRegion(Frame.Detail);
            Canvas.Text(Item.Label, EHDShipFont::Header, HeaderPoint, EHDShipTextAlign::Left, Item.bEnabled ? Accent : TextDisabled, InnerWidth);

            const FBox2D BodyRect(
                FVector2D(Frame.Detail.Min.X + Padding, Frame.Detail.Min.Y + Padding + Canvas.Px(DetailHeaderHeight + Gap)),
                FVector2D(Frame.Detail.Max.X - Padding, Frame.Detail.Max.Y - Padding));
            Canvas.Paragraph(Item.Detail, EHDShipFont::Body, BodyRect, Text, DetailMaxLines);
        }
    }

    Canvas.SetRegion(FBox2D(ForceInit));
    Canvas.SetContentLayer(false);
}

void FHDShipScreenPainter::PaintCountdown(FHDShipUICanvas& Canvas, int32 DisplaySeconds)
{
    using namespace HDShipScreenPainterPrivate;

    Canvas.FillScreen(Backdrop);

    const float CenterX = static_cast<float>(ToPoint(Canvas, DesignWidth * 0.5f, 0.f).X);
    Canvas.Text(CountdownHeaderText, EHDShipFont::Header, FVector2D(CenterX, ToPoint(Canvas, 0.f, CountdownHeaderY).Y), EHDShipTextAlign::Center, TextDim, 0.f);
    Canvas.Text(FString::FromInt(DisplaySeconds), EHDShipFont::Countdown, FVector2D(CenterX, ToPoint(Canvas, 0.f, CountdownNumberY).Y), EHDShipTextAlign::Center, Accent, 0.f);

    // 키 상자 + CANCEL 을 가운데로: 폭을 먼저 잰다(KeyCap 은 글자 양옆 여백 10)
    const int32 KeyPx = GetFontPx(EHDShipFont::KeyCap, Canvas.GetScale());
    const int32 SmallPx = GetFontPx(EHDShipFont::Small, Canvas.GetScale());
    const float Width = Canvas.Measure(CancelKeyText, KeyPx, true) + Canvas.Px(20.f + HintInnerGap) + Canvas.Measure(CancelActionText, SmallPx, IsBold(EHDShipFont::Small));
    const float CancelY = static_cast<float>(ToPoint(Canvas, 0.f, CountdownCancelY).Y);
    const FBox2D HintBox = PaintKeyHint(Canvas, CancelKeyText, CancelActionText, CenterX - Width * 0.5f, CancelY, CenterX + Width);
    Canvas.HitBox(HintBox, HDShipUILayout::MakeHitBoxName(HDShipUILayout::EHDShipHitKind::Key, static_cast<int32>(EHDShipMenuKey::Back)));
}

void FHDShipScreenPainter::PaintInteriorOverlay(FHDShipUICanvas& Canvas, const FHDResourceView& Resources)
{
    using namespace HDShipScreenPainterPrivate;

    PaintResources(Canvas, Resources);

    const FVector2D HelpPoint = ToPoint(Canvas, Margin, BottomCenterY);
    PaintKeyHint(Canvas, ShipMenuKeyText, ShipMenuActionText, static_cast<float>(HelpPoint.X), static_cast<float>(HelpPoint.Y), static_cast<float>(ToPoint(Canvas, DesignWidth * 0.5f, 0.f).X));
}

FVector2D FHDShipScreenPainter::MeasurePromptTag(const FHDShipUICanvas& Canvas, const FString& Label, bool bActive)
{
    using namespace HDShipScreenPainterPrivate;

    const float Scale = Canvas.GetScale();
    if (bActive == false)
    {
        const float NameWidth = Canvas.Measure(Label, GetFontPx(EHDShipFont::Small, Scale), IsBold(EHDShipFont::Small));
        return FVector2D(NameWidth + Canvas.Px(TagPadding) * 2.f, Canvas.Px(PassiveTagHeight));
    }

    const float KeyWidth = Canvas.Measure(InteractKeyText, GetFontPx(EHDShipFont::KeyCap, Scale), true) + Canvas.Px(KeyCapPaddingTotal);
    const float NameWidth = Canvas.Measure(Label, GetFontPx(EHDShipFont::Body, Scale), IsBold(EHDShipFont::Body));
    return FVector2D(KeyWidth + Canvas.Px(HintInnerGap) + NameWidth + Canvas.Px(TagPadding) * 2.f, Canvas.Px(ActiveTagHeight));
}

void FHDShipScreenPainter::PaintPromptTags(FHDShipUICanvas& Canvas, const TArray<FHDPromptTag>& Tags, const TArray<FHDPromptTagInput>& Inputs, const TArray<FString>& Labels)
{
    using namespace HDShipScreenPainterPrivate;

    for (int32 Index = 0; Index < Tags.Num(); ++Index)
    {
        const FHDPromptTag& Tag = Tags[Index];
        if (Tag.bVisible == false || Inputs.IsValidIndex(Index) == false || Labels.IsValidIndex(Index) == false)
        {
            continue;
        }

        const FBox2D& Rect = Tag.Rect;
        const FVector2D Anchor = Inputs[Index].ScreenAnchor;

        // 투영점까지 선: 상자 아래 가운데에서 내려간 뒤 옆으로(1px)
        if (Tag.bEdgeClamped == false && Inputs[Index].bProjected && Anchor.Y > Rect.Max.Y)
        {
            const double CenterX = Rect.GetCenter().X;
            Canvas.Fill(FBox2D(FVector2D(CenterX, Rect.Max.Y), FVector2D(CenterX + 1.0, Anchor.Y)), Line);
            if (FMath::Abs(Anchor.X - CenterX) > 1.0)
            {
                Canvas.Fill(FBox2D(FVector2D(FMath::Min(CenterX, Anchor.X), Anchor.Y - 1.0), FVector2D(FMath::Max(CenterX, Anchor.X), Anchor.Y)), Line);
            }
        }

        const float CenterY = static_cast<float>(Rect.GetCenter().Y);
        const float InnerWidth = FMath::Max(0.f, static_cast<float>(Rect.GetSize().X) - Canvas.Px(TagPadding) * 2.f);
        if (Tag.bActive)
        {
            Canvas.Fill(Rect, Card);
            Canvas.Border(Rect, BorderThin, Accent);
            const FBox2D KeyBox = Canvas.KeyCap(InteractKeyText, FVector2D(Rect.Min.X + Canvas.Px(TagPadding), CenterY));
            const float NameLeft = static_cast<float>(KeyBox.Max.X) + Canvas.Px(HintInnerGap);
            Canvas.Text(Labels[Index], EHDShipFont::Body, FVector2D(NameLeft, CenterY), EHDShipTextAlign::Left, Text, FMath::Max(0.f, static_cast<float>(Rect.Max.X) - Canvas.Px(TagPadding) - NameLeft));
        }
        else
        {
            Canvas.Fill(Rect, Panel);
            Canvas.Text(Labels[Index], EHDShipFont::Small, FVector2D(Rect.GetCenter().X, CenterY), EHDShipTextAlign::Center, TextDim, InnerWidth);
        }
    }
}

namespace HDShipWarTablePrivate
{
    using namespace HDShipUIStyle;

    // 작전 테이블 배치(설계 단위)
    constexpr float     CardColumnWidth     = 360.f;
    constexpr float     ColumnHeaderHeight  = 40.f;
    constexpr float     OperationCardHeight = 132.f;
    constexpr float     CardPadding         = 16.f;
    constexpr float     LineHeight          = 32.f;
    constexpr float     BulletSize          = 10.f;
    constexpr float     BulletGap           = 12.f;
    constexpr float     SectionGap          = 16.f;
    constexpr float     DifficultyRowHeight = 56.f;
    constexpr float     ArrowBoxSize        = 40.f;
    constexpr float     SegmentGap          = 6.f;
    constexpr float     SegmentHeight       = 20.f;
    constexpr float     SmallIconSize       = 30.f;
    constexpr float     IconTextGap         = 8.f;
    constexpr float     SampleItemGap       = 24.f;
    constexpr float     ButtonHeight        = 64.f;
    constexpr float     DetailInnerPadding  = 24.f;
    constexpr float     MapGridStep         = 2000.f;   // 월드 단위(20 m)
    constexpr int32     RingSegments        = 64;
    constexpr float     RingThickness       = 2.f;
    constexpr float     DiamondSize         = 14.f;
    constexpr float     MapLabelInset       = 12.f;

    const FLinearColor  GridColor(0.25f, 0.28f, 0.33f, 0.45f);
    const FLinearColor  RingColor(1.f, 0.85f, 0.2f, 0.6f);
    const FName         DifficultyItemId(TEXT("Difficulty"));
    const FName         StartMissionItemId(TEXT("StartMission"));

    // 선분을 사각형으로 자른다(Liang-Barsky). 다 밖이면 거짓
    bool ClipSegment(const FBox2D& Box, FVector2D& A, FVector2D& B)
    {
        const FVector2D D = B - A;
        double T0 = 0.0;
        double T1 = 1.0;
        const double P[4] = { -D.X, D.X, -D.Y, D.Y };
        const double Q[4] = { A.X - Box.Min.X, Box.Max.X - A.X, A.Y - Box.Min.Y, Box.Max.Y - A.Y };
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (FMath::IsNearlyZero(P[Index]))
            {
                if (Q[Index] < 0.0)
                {
                    return false;
                }
                continue;
            }

            const double T = Q[Index] / P[Index];
            if (P[Index] < 0.0)
            {
                T0 = FMath::Max(T0, T);
            }
            else
            {
                T1 = FMath::Min(T1, T);
            }
        }

        if (T0 > T1)
        {
            return false;
        }

        const FVector2D Start = A;
        A = Start + D * T0;
        B = Start + D * T1;
        return true;
    }

    void ClippedLine(FHDShipUICanvas& Canvas, const FBox2D& Clip, FVector2D A, FVector2D B, float Thickness, const FLinearColor& Color)
    {
        if (ClipSegment(Clip, A, B))
        {
            Canvas.Line(A, B, Thickness, Color);
        }
    }

    void PaintSelectable(FHDShipUICanvas& Canvas, const FBox2D& Rect, bool bSelected)
    {
        Canvas.Fill(Rect, Card);
        if (bSelected)
        {
            Canvas.Fill(Rect, AccentFill);
            Canvas.Border(Rect, BorderSelected, Accent);
        }
        else
        {
            Canvas.Border(Rect, BorderThin, Line);
        }
    }
}

void FHDShipScreenPainter::PaintWarTable(FHDShipUICanvas& Canvas, const FHDShipUIFrame& Frame, const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items, const FHDWarTableView& View, UTexture* MapTexture, const FVector2D& CenterXY, float HalfExtent)
{
    using namespace HDShipWarTablePrivate;

    const int32 Cursor = Items.IsEmpty() ? INDEX_NONE : FMath::Clamp(Menu.GetCursor(), 0, Items.Num() - 1);
    const int32 DifficultyIndex = Items.IndexOfByPredicate([](const FHDShipMenuItem& Item) { return Item.Id == DifficultyItemId; });
    const int32 StartIndex = Items.IndexOfByPredicate([](const FHDShipMenuItem& Item) { return Item.Id == StartMissionItemId; });
    const float LinePx = Canvas.Px(LineHeight);
    const float HeaderPx = Canvas.Px(ColumnHeaderHeight);

    Canvas.SetContentLayer(true);

    // 작전 카드 열
    const FBox2D Column(Frame.Main.Min, FVector2D(Frame.Main.Min.X + Canvas.Px(CardColumnWidth), Frame.Main.Max.Y));
    {
        const float Left = static_cast<float>(Column.Min.X);
        const float Width = static_cast<float>(Column.GetSize().X);
        Canvas.SetRegion(Column);
        Canvas.Text(TEXT("OPERATIONS"), EHDShipFont::Header, FVector2D(Left, Column.Min.Y + HeaderPx * 0.5f), EHDShipTextAlign::Left, TextDim, Width);

        const float CardTop = static_cast<float>(Column.Min.Y) + HeaderPx + Canvas.Px(Gap);
        const FBox2D CardRect(FVector2D(Left, CardTop), FVector2D(Column.Max.X, CardTop + Canvas.Px(OperationCardHeight)));
        Canvas.Fill(CardRect, Card);
        Canvas.Fill(CardRect, AccentFill);
        Canvas.Border(CardRect, BorderSelected, Accent);

        const float TextLeft = Left + Canvas.Px(CardPadding);
        const float TextWidth = Width - Canvas.Px(CardPadding) * 2.f;
        const float LineTop = CardTop + Canvas.Px(CardPadding);
        Canvas.SetRegion(CardRect);
        Canvas.Text(View.OperationName, EHDShipFont::Body, FVector2D(TextLeft, LineTop + LinePx * 0.5f), EHDShipTextAlign::Left, Accent, TextWidth);
        Canvas.Text(View.OperationType, EHDShipFont::Small, FVector2D(TextLeft, LineTop + LinePx * 1.5f), EHDShipTextAlign::Left, Text, TextWidth);
        if (View.bHasDifficulty)
        {
            Canvas.Text(TEXT("DIFFICULTY ") + View.DifficultyLabel, EHDShipFont::Small, FVector2D(TextLeft, LineTop + LinePx * 2.5f), EHDShipTextAlign::Left, TextDim, TextWidth);
        }
    }

    // 작전 지역(정사각형, 남은 폭 가운데)
    {
        const float AreaLeft = static_cast<float>(Column.Max.X) + Canvas.Px(Gap * 2.f);
        const float AreaWidth = static_cast<float>(Frame.Main.Max.X) - AreaLeft;
        const float AreaHeight = static_cast<float>(Frame.Main.GetSize().Y);
        const float Side = FMath::Max(0.f, FMath::Min(AreaWidth, AreaHeight));
        const FVector2D MapMin(AreaLeft + (AreaWidth - Side) * 0.5f, Frame.Main.Min.Y + (AreaHeight - Side) * 0.5f);
        const FBox2D MapRect(MapMin, MapMin + FVector2D(Side, Side));
        const float Extent = FMath::Max(HalfExtent, 1.f);
        const float Inset = Canvas.Px(MapLabelInset);

        Canvas.Fill(MapRect, Panel);
        Canvas.SetRegion(MapRect);
        if (MapTexture)
        {
            Canvas.Texture(MapTexture, MapRect);
        }
        else
        {
            // 중심을 지나는 2000 단위 격자. 축에 나란한 1px 띠로 채운다(Fill 은 투명도를 따른다)
            const int32 Steps = FMath::CeilToInt(Extent / MapGridStep);
            const double Thickness = FMath::Max(1.0, static_cast<double>(FMath::RoundToInt(Canvas.Px(BorderThin))));
            for (int32 Step = -Steps; Step <= Steps; ++Step)
            {
                const double StepOffset = Step * MapGridStep;
                const double RowY = HDWarTableMap::WorldToMap(CenterXY + FVector2D(StepOffset, 0.0), CenterXY, Extent, MapRect).Y;
                if (RowY >= MapRect.Min.Y && RowY + Thickness <= MapRect.Max.Y)
                {
                    Canvas.Fill(FBox2D(FVector2D(MapRect.Min.X, RowY), FVector2D(MapRect.Max.X, RowY + Thickness)), GridColor);
                }
                const double ColumnX = HDWarTableMap::WorldToMap(CenterXY + FVector2D(0.0, StepOffset), CenterXY, Extent, MapRect).X;
                if (ColumnX >= MapRect.Min.X && ColumnX + Thickness <= MapRect.Max.X)
                {
                    Canvas.Fill(FBox2D(FVector2D(ColumnX, MapRect.Min.Y), FVector2D(ColumnX + Thickness, MapRect.Max.Y)), GridColor);
                }
            }
            Canvas.Text(TEXT("1 GRID = 20 M"), EHDShipFont::Small, FVector2D(MapRect.Min.X + Inset, MapRect.Min.Y + Inset + LinePx * 0.5f), EHDShipTextAlign::Left, TextDim, Side * 0.5f);
        }

        // 목표 배치 띠 두 고리
        const UHDMissionSettings* MissionSettings = GetDefault<UHDMissionSettings>();
        const float RadiusMin = MissionSettings ? MissionSettings->ObjectiveRadiusMin : 8000.f;
        const float RadiusMax = MissionSettings ? MissionSettings->ObjectiveRadiusMax : 15000.f;
        for (const float Radius : { RadiusMin, RadiusMax })
        {
            const TArray<FVector2D> Ring = HDWarTableMap::MakeRing(CenterXY, Radius, RingSegments);
            for (int32 Index = 0; Index < Ring.Num(); ++Index)
            {
                const FVector2D A = HDWarTableMap::WorldToMap(Ring[Index], CenterXY, Extent, MapRect);
                const FVector2D B = HDWarTableMap::WorldToMap(Ring[(Index + 1) % Ring.Num()], CenterXY, Extent, MapRect);
                ClippedLine(Canvas, MapRect, A, B, RingThickness, RingColor);
            }
        }
        const FVector2D BandLabel = HDWarTableMap::WorldToMap(CenterXY + FVector2D((RadiusMin + RadiusMax) * 0.5f, 0.f), CenterXY, Extent, MapRect);
        Canvas.Text(TEXT("OBJECTIVE AREA"), EHDShipFont::Small, BandLabel, EHDShipTextAlign::Center, Accent, Side * 0.5f);

        // 가운데 강하 지점 마름모
        const FVector2D Drop = MapRect.GetCenter();
        const float Diamond = Canvas.Px(DiamondSize);
        const FVector2D Top(Drop.X, Drop.Y - Diamond);
        const FVector2D RightPoint(Drop.X + Diamond, Drop.Y);
        const FVector2D Bottom(Drop.X, Drop.Y + Diamond);
        const FVector2D LeftPoint(Drop.X - Diamond, Drop.Y);
        Canvas.Line(Top, RightPoint, RingThickness, Accent);
        Canvas.Line(RightPoint, Bottom, RingThickness, Accent);
        Canvas.Line(Bottom, LeftPoint, RingThickness, Accent);
        Canvas.Line(LeftPoint, Top, RingThickness, Accent);
        Canvas.Text(TEXT("DROP ZONE"), EHDShipFont::Small, FVector2D(Drop.X, Drop.Y + Diamond + LinePx * 0.5f), EHDShipTextAlign::Center, Text, Side * 0.4f);

        Canvas.Text(TEXT("EXTRACTION: CALLED DURING MISSION"), EHDShipFont::Small, FVector2D(MapRect.Min.X + Inset, MapRect.Max.Y - Inset - LinePx * 0.5f), EHDShipTextAlign::Left, TextDim, Side - Inset * 2.f);
        Canvas.Border(MapRect, BorderThin, Line);
    }

    // 상세 패널
    const FBox2D& Detail = Frame.Detail;
    Canvas.Fill(Detail, Panel);
    Canvas.Border(Detail, BorderThin, Line);
    Canvas.SetRegion(Detail);

    const float Padding = Canvas.Px(DetailInnerPadding);
    const float Left = static_cast<float>(Detail.Min.X) + Padding;
    const float Right = static_cast<float>(Detail.Max.X) - Padding;
    const float InnerWidth = Right - Left;
    float Y = static_cast<float>(Detail.Min.Y) + Padding;

    auto PaintHeader = [&](const TCHAR* Label)
    {
        Canvas.Text(Label, EHDShipFont::Header, FVector2D(Left, Y + HeaderPx * 0.5f), EHDShipTextAlign::Left, Accent, InnerWidth);
        Y += HeaderPx;
    };

    PaintHeader(TEXT("OBJECTIVES"));
    for (const FString& Objective : View.Objectives)
    {
        const float CenterY = Y + LinePx * 0.5f;
        const float Bullet = Canvas.Px(BulletSize);
        Canvas.Border(FBox2D(FVector2D(Left, CenterY - Bullet * 0.5f), FVector2D(Left + Bullet, CenterY + Bullet * 0.5f)), BorderThin, Text);
        const float TextLeft = Left + Bullet + Canvas.Px(BulletGap);
        Canvas.Text(Objective, EHDShipFont::Body, FVector2D(TextLeft, CenterY), EHDShipTextAlign::Left, Text, Right - TextLeft);
        Y += LinePx;
    }
    Y += Canvas.Px(SectionGap);

    if (View.bHasDifficulty)
    {
        PaintHeader(TEXT("DIFFICULTY"));

        const bool bSelected = DifficultyIndex != INDEX_NONE && Cursor == DifficultyIndex;
        const FBox2D Row(FVector2D(Left, Y), FVector2D(Right, Y + Canvas.Px(DifficultyRowHeight)));
        PaintSelectable(Canvas, Row, bSelected);

        // 막대 칸: 양옆 화살표 상자 자리를 비운다. 현재 칸과 그 아래 칸은 Accent
        const float Arrow = Canvas.Px(ArrowBoxSize);
        const float ArrowMargin = (static_cast<float>(Row.GetSize().Y) - Arrow) * 0.5f;
        const float BarLeft = Left + ArrowMargin * 2.f + Arrow;
        const float BarRight = Right - ArrowMargin * 2.f - Arrow;
        const int32 NumSegments = View.Segments.Num();
        const int32 CurrentIndex = View.Segments.IndexOfByPredicate([](const FHDDifficultySegment& Segment) { return Segment.bCurrent; });
        const float SegmentWidth = NumSegments > 0 ? (BarRight - BarLeft - Canvas.Px(SegmentGap) * (NumSegments - 1)) / NumSegments : 0.f;
        const float SegmentTop = static_cast<float>(Row.GetCenter().Y) - Canvas.Px(SegmentHeight) * 0.5f;
        for (int32 Index = 0; Index < NumSegments; ++Index)
        {
            const float X = BarLeft + Index * (SegmentWidth + Canvas.Px(SegmentGap));
            Canvas.Fill(FBox2D(FVector2D(X, SegmentTop), FVector2D(X + SegmentWidth, SegmentTop + Canvas.Px(SegmentHeight))), Index <= CurrentIndex ? Accent : Line);
        }

        // 줄 히트박스는 화살표 상자와 겹치지 않게 막대 폭만
        if (DifficultyIndex != INDEX_NONE)
        {
            Canvas.HitBox(FBox2D(FVector2D(BarLeft, Row.Min.Y), FVector2D(BarRight, Row.Max.Y)), HDShipUILayout::MakeHitBoxName(HDShipUILayout::EHDShipHitKind::Item, DifficultyIndex));
        }

        if (bSelected)
        {
            const float ArrowTop = static_cast<float>(Row.Min.Y) + ArrowMargin;
            const FBox2D ArrowBoxes[2] = {
                FBox2D(FVector2D(Left + ArrowMargin, ArrowTop), FVector2D(Left + ArrowMargin + Arrow, ArrowTop + Arrow)),
                FBox2D(FVector2D(Right - ArrowMargin - Arrow, ArrowTop), FVector2D(Right - ArrowMargin, ArrowTop + Arrow))
            };
            const EHDShipMenuKey ArrowKeys[2] = { EHDShipMenuKey::Left, EHDShipMenuKey::Right };
            const TCHAR* const ArrowTexts[2] = { TEXT("<"), TEXT(">") };
            for (int32 Index = 0; Index < 2; ++Index)
            {
                Canvas.Fill(ArrowBoxes[Index], Panel);
                Canvas.Border(ArrowBoxes[Index], BorderThin, Accent);
                Canvas.SetRegion(ArrowBoxes[Index]);
                Canvas.Text(ArrowTexts[Index], EHDShipFont::KeyCap, ArrowBoxes[Index].GetCenter(), EHDShipTextAlign::Center, Accent, 0.f);
                Canvas.HitBox(ArrowBoxes[Index], HDShipUILayout::MakeHitBoxName(HDShipUILayout::EHDShipHitKind::Key, static_cast<int32>(ArrowKeys[Index])));
            }
            Canvas.SetRegion(Detail);
        }
        Y = static_cast<float>(Row.Max.Y);

        Canvas.Text(View.DifficultyLabel, EHDShipFont::Body, FVector2D(Left, Y + LinePx * 0.5f), EHDShipTextAlign::Left, bSelected ? Accent : Text, InnerWidth);
        Y += LinePx + Canvas.Px(SectionGap);

        PaintHeader(TEXT("REWARDS"));
        Canvas.Text(TEXT("REQUISITION ") + View.RewardMultiplier, EHDShipFont::Body, FVector2D(Left, Y + LinePx * 0.5f), EHDShipTextAlign::Left, Text, InnerWidth);
        Y += LinePx;

        const float SuccessY = Y + LinePx * 0.5f;
        Canvas.Text(TEXT("SUCCESS"), EHDShipFont::Small, FVector2D(Left, SuccessY), EHDShipTextAlign::Left, TextDim, InnerWidth * 0.5f);
        const FBox2D AmountBox = Canvas.Text(FString::FromInt(View.SuccessRequisition), EHDShipFont::Body, FVector2D(Right, SuccessY), EHDShipTextAlign::Right, Requisition, InnerWidth * 0.4f);
        const float IconSize = Canvas.Px(SmallIconSize);
        const float IconRight = (AmountBox.bIsValid ? static_cast<float>(AmountBox.Min.X) : Right) - Canvas.Px(IconTextGap);
        Canvas.Icon(FName(TEXT("Requisition")), FBox2D(FVector2D(IconRight - IconSize, SuccessY - IconSize * 0.5f), FVector2D(IconRight, SuccessY + IconSize * 0.5f)), false);
        Y += LinePx + Canvas.Px(SectionGap);
    }

    PaintHeader(TEXT("SAMPLES ON MAP"));
    {
        const float CenterY = Y + LinePx * 0.5f;
        const float IconSize = Canvas.Px(SmallIconSize);
        const int32 BodyPx = GetFontPx(EHDShipFont::Body, Canvas.GetScale());
        const TCHAR* const SampleIds[3] = { TEXT("SampleCommon"), TEXT("SampleRare"), TEXT("SampleSuper") };
        const int32 SampleCounts[3] = { View.MapSamples.Common, View.MapSamples.Rare, View.MapSamples.Super };
        float X = Left;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            Canvas.Icon(FName(SampleIds[Index]), FBox2D(FVector2D(X, CenterY - IconSize * 0.5f), FVector2D(X + IconSize, CenterY + IconSize * 0.5f)), false);
            X += IconSize + Canvas.Px(IconTextGap);
            const FString Count = FString::FromInt(SampleCounts[Index]);
            Canvas.Text(Count, EHDShipFont::Body, FVector2D(X, CenterY), EHDShipTextAlign::Left, Text, 0.f);
            X += Canvas.Measure(Count, BodyPx, IsBold(EHDShipFont::Body)) + Canvas.Px(SampleItemGap);
        }
    }

    // 맨 아래 START MISSION
    if (StartIndex != INDEX_NONE)
    {
        const FBox2D Button(FVector2D(Left, Detail.Max.Y - Padding - Canvas.Px(ButtonHeight)), FVector2D(Right, Detail.Max.Y - Padding));
        const bool bSelected = Cursor == StartIndex;
        PaintSelectable(Canvas, Button, bSelected);
        Canvas.SetRegion(Button);
        Canvas.Text(TEXT("START MISSION"), EHDShipFont::Header, Button.GetCenter(), EHDShipTextAlign::Center, bSelected ? Accent : Text, InnerWidth - Padding);
        Canvas.HitBox(Button, HDShipUILayout::MakeHitBoxName(HDShipUILayout::EHDShipHitKind::Item, StartIndex));
    }

    Canvas.SetRegion(FBox2D(ForceInit));
    Canvas.SetContentLayer(false);
}
