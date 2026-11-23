// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HDShipUIStyle.h"
#include "UI/HDShipUILayout.h"
#include "UI/HDShipMenuView.h"
#include "UI/HDShipMenu.h"
#include "Progression/HDProgressionSettings.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Stratagem/HDStratagemSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDShipUITest, "Helldivers.FrontEnd.ShipUI", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDShipUITest
{
    // 가짜 측정: 폭 = 글자 수 × 0.6 × 글꼴 px
    float FakeMeasure(const FString& Text, int32 FontPx)
    {
        return Text.Len() * 0.6f * FontPx;
    }

    bool IsInside(const FBox2D& Outer, const FBox2D& Inner, float Tolerance = 0.01f)
    {
        return Inner.Min.X >= Outer.Min.X - Tolerance && Inner.Min.Y >= Outer.Min.Y - Tolerance
            && Inner.Max.X <= Outer.Max.X + Tolerance && Inner.Max.Y <= Outer.Max.Y + Tolerance;
    }

    // 넓이가 있는 겹침
    bool Overlaps(const FBox2D& A, const FBox2D& B, float Tolerance = 0.01f)
    {
        const float Width = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
        const float Height = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
        return Width > Tolerance && Height > Tolerance;
    }

    HDShipUILayout::FHDDrawnText MakeText(const FBox2D& Rect, const TCHAR* Text, bool bTruncated = false, const FBox2D& Region = FBox2D(ForceInit))
    {
        HDShipUILayout::FHDDrawnText Drawn;
        Drawn.Rect          = Rect;
        Drawn.Region        = Region;
        Drawn.Text          = Text;
        Drawn.bTruncated    = bTruncated;
        return Drawn;
    }

    FBox2D MakeBox(float X0, float Y0, float X1, float Y1)
    {
        return FBox2D(FVector2D(X0, Y0), FVector2D(X1, Y1));
    }

    // A: 100 징발 + 일반 5 로 살 수 있는 1단계 / B: 150 징발 + 일반 8 이라 못 산다
    const TCHAR* const TestCatalogJson = TEXT(R"([
        { "Name": "A", "DisplayName": "ALPHA", "Description": "Alpha.", "Effect": "EagleRearmScale",
          "Levels": [ { "Requisition": 100, "Common": 5, "Value": 0.8 } ] },
        { "Name": "B", "DisplayName": "BRAVO", "Description": "Bravo.", "Effect": "ExtraReinforcements",
          "Levels": [ { "Requisition": 150, "Common": 8, "Value": 1 } ] }
    ])");
}

bool FHDShipUITest::RunTest(const FString& Parameters)
{
    using namespace HDShipUITest;
    using namespace HDShipUILayout;

    const auto Measure = [](const FString& Text, int32 FontPx) { return FakeMeasure(Text, FontPx); };

    // S1 배율·글꼴
    {
        TestEqual(TEXT("S1: 1280x720 scale"), HDShipUIStyle::ComputeScale(FVector2D(1280.f, 720.f)), 0.6667f, 0.001f);
        TestEqual(TEXT("S1: 1920x1080 scale"), HDShipUIStyle::ComputeScale(FVector2D(1920.f, 1080.f)), 1.f, 0.001f);
        TestEqual(TEXT("S1: 2560x1440 scale"), HDShipUIStyle::ComputeScale(FVector2D(2560.f, 1440.f)), 1.3333f, 0.001f);
        TestEqual(TEXT("S1: 1500x821 scale uses height"), HDShipUIStyle::ComputeScale(FVector2D(1500.f, 821.f)), 0.7602f, 0.001f);
        TestEqual(TEXT("S1: 800x450 clamps to 0.5"), HDShipUIStyle::ComputeScale(FVector2D(800.f, 450.f)), 0.5f, 0.001f);
        TestEqual(TEXT("S1: 5000x3000 clamps to 2"), HDShipUIStyle::ComputeScale(FVector2D(5000.f, 3000.f)), 2.f, 0.001f);

        TestEqual(TEXT("S1: Body at 0.6667 is 15"), HDShipUIStyle::GetFontPx(EHDShipFont::Body, 0.6667f), 15);
        TestEqual(TEXT("S1: Small at 0.6667 is 12"), HDShipUIStyle::GetFontPx(EHDShipFont::Small, 0.6667f), 12);
        TestEqual(TEXT("S1: Body at 1 is 22"), HDShipUIStyle::GetFontPx(EHDShipFont::Body, 1.f), 22);
        TestEqual(TEXT("S1: Body at 1.3333 is 29"), HDShipUIStyle::GetFontPx(EHDShipFont::Body, 1.3333f), 29);
        TestEqual(TEXT("S1: Small at 0.5 is 12 (min)"), HDShipUIStyle::GetFontPx(EHDShipFont::Small, 0.5f), 12);
    }

    // S2 틀
    {
        const TArray<FVector2D> Views = { FVector2D(1280.f, 720.f), FVector2D(1920.f, 1080.f), FVector2D(2560.f, 1440.f), FVector2D(1500.f, 821.f) };
        for (const FVector2D& ViewSize : Views)
        {
            const FString Tag = FString::Printf(TEXT("S2 %.0fx%.0f"), ViewSize.X, ViewSize.Y);
            const FHDShipUIFrame Frame = ComputeFrame(ViewSize);
            const FBox2D View(FVector2D::ZeroVector, ViewSize);

            TestTrue(Tag + TEXT(": view"), Frame.View.Min.Equals(View.Min) && Frame.View.Max.Equals(View.Max));
            TestTrue(Tag + TEXT(": top bar inside view"), Frame.TopBar.bIsValid && IsInside(View, Frame.TopBar));
            TestTrue(Tag + TEXT(": content inside view"), Frame.Content.bIsValid && IsInside(View, Frame.Content));
            TestTrue(Tag + TEXT(": bottom bar inside view"), Frame.BottomBar.bIsValid && IsInside(View, Frame.BottomBar));
            TestTrue(Tag + TEXT(": main inside view"), Frame.Main.bIsValid && IsInside(View, Frame.Main));
            TestTrue(Tag + TEXT(": detail inside view"), Frame.Detail.bIsValid && IsInside(View, Frame.Detail));

            TestFalse(Tag + TEXT(": top bar / content"), Overlaps(Frame.TopBar, Frame.Content));
            TestFalse(Tag + TEXT(": content / bottom bar"), Overlaps(Frame.Content, Frame.BottomBar));
            TestFalse(Tag + TEXT(": top bar / bottom bar"), Overlaps(Frame.TopBar, Frame.BottomBar));
            TestFalse(Tag + TEXT(": main / detail"), Overlaps(Frame.Main, Frame.Detail));

            const FVector2D SafeSize = Frame.Safe.GetSize();
            TestEqual(Tag + TEXT(": safe is 16:9"), SafeSize.X / FMath::Max(SafeSize.Y, 1.0), 16.0 / 9.0, 0.001);
            TestTrue(Tag + TEXT(": safe inside view"), IsInside(View, Frame.Safe));
            TestTrue(Tag + TEXT(": safe centered"), Frame.Safe.GetCenter().Equals(View.GetCenter(), 0.01));

            TestEqual(Tag + TEXT(": detail width = 520 x scale"), Frame.Detail.GetSize().X, 520.0 * Frame.Scale, 0.01);

            const TArray<FBox2D> Rows = ComputeListRows(Frame, 6, false);
            TestEqual(Tag + TEXT(": six rows"), Rows.Num(), 6);
            bool bRowsInside = true;
            bool bRowsApart = true;
            for (int32 Index = 0; Index < Rows.Num(); ++Index)
            {
                bRowsInside &= IsInside(Frame.Main, Rows[Index]);
                for (int32 Other = 0; Other < Index; ++Other)
                {
                    bRowsApart &= Overlaps(Rows[Index], Rows[Other]) == false;
                }
            }
            TestTrue(Tag + TEXT(": rows inside main"), bRowsInside);
            TestTrue(Tag + TEXT(": rows do not overlap"), bRowsApart);
        }

        const FHDShipUIFrame Odd = ComputeFrame(FVector2D(1500.f, 821.f));
        TestEqual(TEXT("S2 1500x821: safe width 1459.6"), Odd.Safe.GetSize().X, 1459.6, 0.1);
        TestEqual(TEXT("S2 1500x821: safe left 20.2"), Odd.Safe.Min.X, 20.2, 0.1);
    }

    // S3 글자 맞춤
    {
        const FHDFitResult Short = FitText(TEXT("ABCDEFGHIJ"), 300.f, 22, 12, Measure);
        TestEqual(TEXT("S3: short text unchanged"), Short.Text, FString(TEXT("ABCDEFGHIJ")));
        TestEqual(TEXT("S3: short text keeps 22px"), Short.FontPx, 22);
        TestFalse(TEXT("S3: short text not truncated"), Short.bTruncated);

        // 브리프의 40글자는 12px 에서 288 이라 폭 300 에 맞는다. 말줄임 분기를 보려고 50글자(12px 에서 360)로 쓴다
        const FString Long = FString::ChrN(50, TEXT('A'));
        const FHDFitResult LongFit = FitText(Long, 300.f, 22, 12, Measure);
        TestTrue(TEXT("S3: 50 chars truncated"), LongFit.bTruncated);
        TestTrue(TEXT("S3: 50 chars ends with ..."), LongFit.Text.EndsWith(TEXT("...")));
        TestEqual(TEXT("S3: 50 chars at min px"), LongFit.FontPx, 12);
        TestTrue(TEXT("S3: 50 chars measured <= 300"), FakeMeasure(LongFit.Text, LongFit.FontPx) <= 300.f);

        const FString Thirty = FString::ChrN(30, TEXT('B'));
        const FHDFitResult ThirtyFit = FitText(Thirty, 300.f, 22, 12, Measure);
        TestEqual(TEXT("S3: 30 chars unchanged"), ThirtyFit.Text, Thirty);
        TestFalse(TEXT("S3: 30 chars not truncated"), ThirtyFit.bTruncated);
        TestEqual(TEXT("S3: 30 chars shrinks to 16px"), ThirtyFit.FontPx, 16);
        TestTrue(TEXT("S3: 30 chars measured <= 300"), FakeMeasure(ThirtyFit.Text, ThirtyFit.FontPx) <= 300.f);

        TestEqual(TEXT("S3: empty stays empty"), FitText(FString(), 300.f, 22, 12, Measure).Text, FString());
        // "..." 는 12px 에서 21.6
        TestEqual(TEXT("S3: narrower than ... is empty"), FitText(TEXT("ABCDEF"), 20.f, 22, 12, Measure).Text, FString());

        // 22px 한 글자 13.2. 폭 150 이면 11글자까지
        const FString Sentence(TEXT("ALPHA BRAVO CHARLIE DELTA ECHO FOXTROT GOLF HOTEL INDIA JULIET"));
        const TArray<FString> Lines = WrapText(Sentence, 150.f, 22, 0, Measure);
        TestTrue(TEXT("S3: wrap makes several lines"), Lines.Num() > 1);
        bool bLinesFit = true;
        bool bWordsWhole = true;
        TArray<FString> Words;
        Sentence.ParseIntoArrayWS(Words);
        for (const FString& Line : Lines)
        {
            bLinesFit &= FakeMeasure(Line, 22) <= 150.f;
            TArray<FString> LineWords;
            Line.ParseIntoArrayWS(LineWords);
            for (const FString& Word : LineWords)
            {
                bWordsWhole &= Words.Contains(Word);
            }
        }
        TestTrue(TEXT("S3: wrapped lines fit"), bLinesFit);
        TestTrue(TEXT("S3: wrap breaks only between words"), bWordsWhole);
        TestEqual(TEXT("S3: wrap keeps every word"), FString::Join(Lines, TEXT(" ")), Sentence);

        const TArray<FString> Limited = WrapText(Sentence, 150.f, 22, 2, Measure);
        TestEqual(TEXT("S3: MaxLines 2"), Limited.Num(), 2);
        if (Limited.Num() == 2)
        {
            TestTrue(TEXT("S3: last line ends with ..."), Limited[1].EndsWith(TEXT("...")));
            TestTrue(TEXT("S3: limited lines fit"), FakeMeasure(Limited[0], 22) <= 150.f && FakeMeasure(Limited[1], 22) <= 150.f);
        }
    }

    // S4 키 안내
    {
        FHDShipMenuContext BridgeContext;
        BridgeContext.Difficulties = GetDefault<UHDProgressionSettings>()->DifficultyLevels;
        TestTrue(TEXT("S4: difficulty table is not empty"), BridgeContext.Difficulties.Num() > 0);

        const TArray<FHDShipMenuItem> BridgeItems = HDShipMenu::BuildItems(EHDShipScreen::Bridge, BridgeContext);
        FHDShipMenu Bridge(EHDShipScreen::Bridge);
        TestEqual(TEXT("S4: Bridge Deploy"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(Bridge, BridgeItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [ENTER] CONFIRM")));
        Bridge.SetCursor(1, BridgeItems.Num());
        TestEqual(TEXT("S4: Bridge Difficulty"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(Bridge, BridgeItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [LEFT/RIGHT] CHANGE")));

        FHDShipMenu System;
        System.OpenTerminal(EHDShipScreen::System);
        const TArray<FHDShipMenuItem> SystemItems = HDShipMenu::BuildItems(EHDShipScreen::System, BridgeContext);
        TestEqual(TEXT("S4: System"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(System, SystemItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [ENTER] CONFIRM   [ESC] CLOSE")));

        FHDShipMenuContext WarContext = BridgeContext;
        WarContext.Stratagems       = &FHDStratagemCatalog::GetProjectCatalog();
        WarContext.DefaultLoadout   = GetDefault<UHDStratagemSettings>()->Loadout;

        FHDShipMenu War;
        War.OpenTerminal(EHDShipScreen::WarTable);
        const TArray<FHDShipMenuItem> WarItems = HDShipMenu::BuildItems(EHDShipScreen::WarTable, WarContext);
        TestEqual(TEXT("S4: WarTable Difficulty"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(War, WarItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [LEFT/RIGHT] DIFFICULTY   [BACKSPACE] CLOSE")));
        War.HandleKey(EHDShipMenuKey::Down, WarItems);
        TestEqual(TEXT("S4: WarTable StartMission"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(War, WarItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [ENTER] CONFIRM   [BACKSPACE] CLOSE")));

        War.HandleKey(EHDShipMenuKey::Confirm, WarItems);
        TestTrue(TEXT("S4: START MISSION opens Loadout"), War.GetScreen() == EHDShipScreen::Loadout);
        const TArray<FHDShipMenuItem> LoadoutItems = HDShipMenu::BuildItems(EHDShipScreen::Loadout, WarContext);
        TestTrue(TEXT("S4: Loadout has slot rows"), LoadoutItems.Num() > 1);
        TestEqual(TEXT("S4: Loadout slot"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(War, LoadoutItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [LEFT/RIGHT] CHANGE   [BACKSPACE] BACK")));
        War.SetCursor(LoadoutItems.Num() - 1, LoadoutItems.Num());
        TestEqual(TEXT("S4: Loadout READY"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(War, LoadoutItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [ENTER] CONFIRM   [BACKSPACE] BACK")));
        TestEqual(TEXT("S4: launching"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(War, LoadoutItems, true)),
            FString(TEXT("[BACKSPACE] CANCEL")));
        TestEqual(TEXT("S4: launching on System"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(System, SystemItems, true)),
            FString(TEXT("[BACKSPACE] CANCEL")));

        const FHDUpgradeCatalog Catalog = FHDUpgradeCatalog::FromJsonString(TestCatalogJson);
        FHDShipMenuContext UpgradeContext;
        UpgradeContext.Upgrades                 = &Catalog;
        UpgradeContext.Profile.Requisition      = 100;
        UpgradeContext.Profile.Samples.Common   = 5;
        const TArray<FHDShipMenuItem> UpgradeItems = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, UpgradeContext);
        TestEqual(TEXT("S4: two upgrade rows"), UpgradeItems.Num(), 2);
        if (UpgradeItems.Num() == 2)
        {
            TestTrue(TEXT("S4: row A can be bought"), UpgradeItems[0].bEnabled);
            TestFalse(TEXT("S4: row B cannot be bought"), UpgradeItems[1].bEnabled);
        }

        FHDShipMenu Upgrades;
        Upgrades.OpenTerminal(EHDShipScreen::Upgrades);
        TestEqual(TEXT("S4: Upgrades can buy"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(Upgrades, UpgradeItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [ENTER] CONFIRM   [BACKSPACE] CLOSE")));
        Upgrades.SetCursor(1, UpgradeItems.Num());
        TestEqual(TEXT("S4: Upgrades cannot buy"), HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(Upgrades, UpgradeItems, false)),
            FString(TEXT("[UP/DOWN] SELECT   [BACKSPACE] CLOSE")));

        FHDShipMenuContext BridgeUpgradeContext = UpgradeContext;
        const TArray<FHDShipMenuItem> BridgeUpgradeRoot = HDShipMenu::BuildItems(EHDShipScreen::Bridge, BridgeUpgradeContext);
        const int32 UpgradesIndex = BridgeUpgradeRoot.IndexOfByPredicate([](const FHDShipMenuItem& Item) { return Item.Id == FName(TEXT("Upgrades")); });
        TestTrue(TEXT("S4: Bridge has Upgrades row"), BridgeUpgradeRoot.IsValidIndex(UpgradesIndex));
        FHDShipMenu BridgeUpgrades(EHDShipScreen::Bridge);
        BridgeUpgrades.SetCursor(UpgradesIndex, BridgeUpgradeRoot.Num());
        BridgeUpgrades.HandleKey(EHDShipMenuKey::Confirm, BridgeUpgradeRoot);
        TestTrue(TEXT("S4: Bridge opens Upgrades"), BridgeUpgrades.GetScreen() == EHDShipScreen::Upgrades);
        TestTrue(TEXT("S4: Upgrades from Bridge ends with BACK"),
            HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(BridgeUpgrades, UpgradeItems, false)).EndsWith(TEXT("[BACKSPACE] BACK")));

        const FHDShipMenu Title;
        TestEqual(TEXT("S4: Title has no hints"), HDShipMenuView::GetKeyHints(Title, TArray<FHDShipMenuItem>(), false).Num(), 0);
    }

    // S5 전환
    {
        FHDShipUITransition Fresh;
        TestFalse(TEXT("S5: new is not visible"), Fresh.IsVisible());
        TestEqual(TEXT("S5: new alpha 0"), Fresh.GetAlpha(), 0.f, 0.001f);

        FHDShipUITransition Transition;
        Transition.Open(0.15f);
        TestEqual(TEXT("S5: open alpha 0"), Transition.GetAlpha(), 0.f, 0.001f);
        TestEqual(TEXT("S5: open slide 1"), Transition.GetSlide(), 1.f, 0.001f);
        Transition.Tick(0.075f);
        TestEqual(TEXT("S5: open half"), Transition.GetAlpha(), 0.5f, 0.001f);
        Transition.Tick(0.1f);
        TestEqual(TEXT("S5: open full"), Transition.GetAlpha(), 1.f, 0.001f);
        TestEqual(TEXT("S5: open slide 0"), Transition.GetSlide(), 0.f, 0.001f);
        TestTrue(TEXT("S5: open settled"), Transition.IsSettled());

        Transition.Close(0.12f);
        Transition.Tick(0.06f);
        TestEqual(TEXT("S5: close half"), Transition.GetAlpha(), 0.5f, 0.001f);
        TestTrue(TEXT("S5: closing still visible"), Transition.IsVisible());

        // 닫는 중에 다시 열면 그 자리 투명도에서 올라간다
        FHDShipUITransition Reopen = Transition;
        Reopen.Open(0.15f);
        TestEqual(TEXT("S5: reopen starts from 0.5"), Reopen.GetAlpha(), 0.5f, 0.001f);
        Reopen.Tick(0.03f);
        TestEqual(TEXT("S5: reopen climbs"), Reopen.GetAlpha(), 0.7f, 0.001f);

        Transition.Tick(0.1f);
        TestEqual(TEXT("S5: close full"), Transition.GetAlpha(), 0.f, 0.001f);
        TestFalse(TEXT("S5: closed not visible"), Transition.IsVisible());

        FHDShipUITransition Change;
        Change.Open(0.f);
        TestEqual(TEXT("S5: instant open"), Change.GetAlpha(), 1.f, 0.001f);
        Change.ChangeScreen(0.1f);
        TestEqual(TEXT("S5: change content 0"), Change.GetContentAlpha(), 0.f, 0.001f);
        Change.Tick(0.05f);
        TestEqual(TEXT("S5: change content half"), Change.GetContentAlpha(), 0.5f, 0.001f);
        TestEqual(TEXT("S5: change frame alpha stays 1"), Change.GetAlpha(), 1.f, 0.001f);
        TestFalse(TEXT("S5: changing is not settled"), Change.IsSettled());
    }

    // S6 히트박스 이름
    {
        const FName ItemName = MakeHitBoxName(EHDShipHitKind::Item, 3);
        const FName CardName = MakeHitBoxName(EHDShipHitKind::Card, 2, 1);
        const FName TabName  = MakeHitBoxName(EHDShipHitKind::Tab, 1);
        const FName PickName = MakeHitBoxName(EHDShipHitKind::Pick, 5);
        const FName KeyName  = MakeHitBoxName(EHDShipHitKind::Key, static_cast<int32>(EHDShipMenuKey::Back));

        TestEqual(TEXT("S6: item name"), ItemName.ToString(), FString(TEXT("ShipItem_3")));
        TestEqual(TEXT("S6: card name"), CardName.ToString(), FString(TEXT("ShipCard_2_1")));
        TestEqual(TEXT("S6: tab name"), TabName.ToString(), FString(TEXT("ShipTab_1")));
        TestEqual(TEXT("S6: pick name"), PickName.ToString(), FString(TEXT("ShipPick_5")));
        TestEqual(TEXT("S6: key name"), KeyName.ToString(), FString(TEXT("ShipKey_5")));

        struct FCase { FName Name; EHDShipHitKind Kind; int32 A; int32 B; };
        const TArray<FCase> Cases = {
            { ItemName, EHDShipHitKind::Item, 3, 0 },
            { CardName, EHDShipHitKind::Card, 2, 1 },
            { TabName,  EHDShipHitKind::Tab,  1, 0 },
            { PickName, EHDShipHitKind::Pick, 5, 0 },
            { KeyName,  EHDShipHitKind::Key,  5, 0 },
        };
        for (const FCase& Case : Cases)
        {
            EHDShipHitKind Kind = EHDShipHitKind::None;
            int32 A = -1;
            int32 B = -1;
            const bool bParsed = ParseHitBoxName(Case.Name, Kind, A, B);
            TestTrue(FString::Printf(TEXT("S6: parse %s"), *Case.Name.ToString()), bParsed && Kind == Case.Kind && A == Case.A && B == Case.B);
        }

        for (const TCHAR* Bad : { TEXT("ShipItem_x"), TEXT("ShipCard_2"), TEXT("ShipTitle"), TEXT("ResultReturn"), TEXT("") })
        {
            EHDShipHitKind Kind = EHDShipHitKind::None;
            int32 A = 0;
            int32 B = 0;
            TestFalse(FString::Printf(TEXT("S6: '%s' is not a ship hit box"), Bad), ParseHitBoxName(FName(Bad), Kind, A, B));
        }
    }

    // S7 겹침 찾기
    {
        const FBox2D View = MakeBox(0.f, 0.f, 100.f, 100.f);

        const FHDTextProblems Overlap = FindTextProblems({ MakeText(MakeBox(0.f, 0.f, 10.f, 10.f), TEXT("A")), MakeText(MakeBox(9.f, 0.f, 20.f, 10.f), TEXT("B")) }, View);
        TestEqual(TEXT("S7: 1px overlap"), Overlap.Overlaps, 1);
        TestEqual(TEXT("S7: texts 2"), Overlap.Texts, 2);
        TestEqual(TEXT("S7: first problem is later text"), Overlap.FirstProblem, FString(TEXT("B")));

        const FHDTextProblems Touch = FindTextProblems({ MakeText(MakeBox(0.f, 0.f, 10.f, 10.f), TEXT("A")), MakeText(MakeBox(10.f, 0.f, 20.f, 10.f), TEXT("B")) }, View);
        TestEqual(TEXT("S7: touching edges"), Touch.Overlaps, 0);
        TestEqual(TEXT("S7: touching not clipped"), Touch.Clipped, 0);
        TestTrue(TEXT("S7: no first problem"), Touch.FirstProblem.IsEmpty());

        const FHDTextProblems OffScreen = FindTextProblems({ MakeText(MakeBox(91.f, 0.f, 101.f, 10.f), TEXT("EDGE")) }, View);
        TestEqual(TEXT("S7: 1px off screen"), OffScreen.Clipped, 1);
        TestEqual(TEXT("S7: off screen first problem"), OffScreen.FirstProblem, FString(TEXT("EDGE")));

        const FHDTextProblems OffRegion = FindTextProblems({ MakeText(MakeBox(45.f, 0.f, 51.f, 10.f), TEXT("R"), false, MakeBox(0.f, 0.f, 50.f, 50.f)) }, View);
        TestEqual(TEXT("S7: out of region"), OffRegion.Clipped, 1);

        const FHDTextProblems Truncated = FindTextProblems({
            MakeText(MakeBox(0.f, 0.f, 10.f, 10.f), TEXT("A..."), true),
            MakeText(MakeBox(0.f, 20.f, 10.f, 30.f), TEXT("B..."), true),
            MakeText(MakeBox(0.f, 40.f, 10.f, 50.f), TEXT("C")) }, View);
        TestEqual(TEXT("S7: truncated 2"), Truncated.Truncated, 2);
        TestEqual(TEXT("S7: truncated texts 3"), Truncated.Texts, 3);
        TestEqual(TEXT("S7: truncated no overlap"), Truncated.Overlaps, 0);
    }

    // S8 마우스
    {
        TestFalse(TEXT("S8: same point"), HasMouseMoved(FVector2D(100.f, 100.f), FVector2D(100.f, 100.f)));
        TestFalse(TEXT("S8: within 3px"), HasMouseMoved(FVector2D(100.f, 100.f), FVector2D(101.f, 102.f)));
        TestTrue(TEXT("S8: 4px moved"), HasMouseMoved(FVector2D(100.f, 100.f), FVector2D(104.f, 100.f)));
    }

    return true;
}

#endif
