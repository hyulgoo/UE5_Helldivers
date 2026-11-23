// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipScreenPainter.h"
#include "UI/HDShipUICanvas.h"
#include "UI/HDShipUIStyle.h"
#include "UI/HDShipUILayout.h"
#include "UI/HDShipMenu.h"
#include "UI/HDShipMenuView.h"

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

    // 함선 안: 단말기 안내 가운데 y 900, 최대 폭 900
    constexpr float     PromptY             = 900.f;
    constexpr float     PromptMaxWidth      = 900.f;

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
        const TCHAR*    Abbrev;
        FLinearColor    Tint;
        int32           Count;
    };
    const TArray<FResourceEntry> Entries = {
        { TEXT("RQ"),   Requisition,    Resources.Requisition },
        { TEXT("C"),    SampleCommon,   Resources.Samples.Common },
        { TEXT("R"),    SampleRare,     Resources.Samples.Rare },
        { TEXT("S"),    SampleSuper,    Resources.Samples.Super },
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
        Canvas.LetterTile(Entry.Abbrev, FBox2D(FVector2D(X, CenterY - TileSize * 0.5f), FVector2D(X + TileSize, CenterY + TileSize * 0.5f)), Entry.Tint, false);
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

void FHDShipScreenPainter::PaintInteriorOverlay(FHDShipUICanvas& Canvas, const FHDResourceView& Resources, const FString& Prompt)
{
    using namespace HDShipScreenPainterPrivate;

    PaintResources(Canvas, Resources);

    const FVector2D HelpPoint = ToPoint(Canvas, Margin, BottomCenterY);
    PaintKeyHint(Canvas, ShipMenuKeyText, ShipMenuActionText, static_cast<float>(HelpPoint.X), static_cast<float>(HelpPoint.Y), static_cast<float>(ToPoint(Canvas, DesignWidth * 0.5f, 0.f).X));

    if (Prompt.IsEmpty() == false)
    {
        Canvas.Text(Prompt, EHDShipFont::Body, ToPoint(Canvas, DesignWidth * 0.5f, PromptY), EHDShipTextAlign::Center, Accent, Canvas.Px(PromptMaxWidth));
    }
}
