// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HDShipMenu.h"
#include "UI/HDShipMenuNav.h"
#include "UI/HDShipMenuView.h"
#include "UI/HDHoldConfirm.h"
#include "Progression/HDUpgradeCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDUpgradeScreenTest, "Helldivers.FrontEnd.UpgradeScreen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDUpgradeScreenTest
{
    // A: 분야 AIR, 이글 재무장 2단계 / B: 분야 COMMAND, 증원 1단계 / C: 분야 없음, 스팀·수류탄 1단계
    const TCHAR* const TestCatalogJson = TEXT(R"([
        { "Name": "A", "DisplayName": "ALPHA", "Description": "Alpha.", "Group": "AIR", "Effect": "EagleRearmScale",
          "Levels": [ { "Requisition": 100, "Common": 5, "Value": 0.8 }, { "Requisition": 200, "Common": 10, "Rare": 3, "Super": 1, "Value": 0.6 } ] },
        { "Name": "B", "DisplayName": "BRAVO", "Description": "Bravo.", "Group": "COMMAND", "Effect": "ExtraReinforcements",
          "Levels": [ { "Requisition": 150, "Common": 8, "Value": 1 } ] },
        { "Name": "C", "DisplayName": "CHARLIE", "Description": "Charlie.", "Effect": "ExtraStimsAndGrenades",
          "Levels": [ { "Requisition": 50, "Value": 1 } ] }
    ])");

    const FName NameA(TEXT("A"));
    const FName NameB(TEXT("B"));
    const FName NameC(TEXT("C"));

    FHDShipMenuContext MakeContext(const FHDUpgradeCatalog& Catalog, int32 Requisition, int32 Common, int32 LevelA, int32 LevelB = 0)
    {
        FHDShipMenuContext Context;
        Context.Upgrades                    = &Catalog;
        Context.Profile.Requisition         = Requisition;
        Context.Profile.Samples.Common      = Common;
        if (LevelA > 0)
        {
            Context.Profile.UpgradeLevels.Add(NameA, LevelA);
        }
        if (LevelB > 0)
        {
            Context.Profile.UpgradeLevels.Add(NameB, LevelB);
        }
        return Context;
    }

    FString CellText(const TArray<FHDNavCell>& Cells, int32 Index)
    {
        if (Cells.IsValidIndex(Index) == false)
        {
            return TEXT("missing");
        }
        const FHDNavCell& Cell = Cells[Index];
        return FString::Printf(TEXT("(%d,%d)"), Cell.Col, Cell.Row);
    }

    FString Hints(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items)
    {
        return HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(Menu, Items, false));
    }

    FString CostText(const FHDUpgradeDetail& Detail)
    {
        TArray<FString> Parts;
        for (const FHDCostPart& Part : Detail.Cost)
        {
            Parts.Add(FString::Printf(TEXT("%s %d %s"), *Part.IconId.ToString(), Part.Amount, Part.bEnough ? TEXT("ok") : TEXT("short")));
        }
        return FString::Join(Parts, TEXT(", "));
    }

    bool IsAction(const FHDShipMenuAction& Action, EHDShipActionType Type, FName Id = NAME_None)
    {
        return Action.Type == Type && (Id.IsNone() || Action.Id == Id);
    }
}

bool FHDUpgradeScreenTest::RunTest(const FString& Parameters)
{
    using namespace HDUpgradeScreenTest;

    const FHDUpgradeCatalog Catalog = FHDUpgradeCatalog::FromJsonString(TestCatalogJson);

    // G1 분야
    {
        for (const FString& Problem : Catalog.GetProblems()) { AddInfo(FString::Printf(TEXT("problem: %s"), *Problem)); }
        TestEqual(TEXT("G1: test catalog no problems"), Catalog.GetProblems().Num(), 0);
        const FHDUpgradeDefinition* A = Catalog.Find(NameA);
        const FHDUpgradeDefinition* B = Catalog.Find(NameB);
        const FHDUpgradeDefinition* C = Catalog.Find(NameC);
        TestTrue(TEXT("G1: rows found"), A && B && C);
        if (A && B && C)
        {
            TestEqual(TEXT("G1: A group"), A->Group, FString(TEXT("AIR")));
            TestEqual(TEXT("G1: B group"), B->Group, FString(TEXT("COMMAND")));
            TestEqual(TEXT("G1: C group empty"), C->Group, FString());
        }
        TestEqual(TEXT("G1: test groups"), FString::Join(HDShipMenuView::GetUpgradeGroups(Catalog), TEXT(", ")), FString(TEXT("AIR, COMMAND, OTHER")));

        const FHDUpgradeCatalog& Project = FHDUpgradeCatalog::GetProjectCatalog();
        for (const FString& Problem : Project.GetProblems()) { AddInfo(FString::Printf(TEXT("project problem: %s"), *Problem)); }
        TestEqual(TEXT("G1: project no problems"), Project.GetProblems().Num(), 0);
        TestEqual(TEXT("G1: project groups"), FString::Join(HDShipMenuView::GetUpgradeGroups(Project), TEXT(", ")), FString(TEXT("AIR SUPPORT, ORBITAL, LOGISTICS, COMMAND")));

        const TCHAR* const ExpectedGroups[][2] = {
            { TEXT("EaglePayloadBay"),      TEXT("AIR SUPPORT") },
            { TEXT("EagleFastRearm"),       TEXT("AIR SUPPORT") },
            { TEXT("OrbitalCoolant"),       TEXT("ORBITAL") },
            { TEXT("LogisticsPriority"),    TEXT("LOGISTICS") },
            { TEXT("FieldKit"),             TEXT("LOGISTICS") },
            { TEXT("TargetingUplink"),      TEXT("COMMAND") },
            { TEXT("ReinforcementBudget"),  TEXT("COMMAND") } };
        TestEqual(TEXT("G1: project 7 rows"), Project.Num(), 7);
        for (const auto& Row : ExpectedGroups)
        {
            const FHDUpgradeDefinition* Definition = Project.Find(FName(Row[0]));
            TestEqual(FString::Printf(TEXT("G1: %s group"), Row[0]), Definition ? Definition->Group : FString(TEXT("missing")), FString(Row[1]));
        }
    }

    // G2 줄
    {
        const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 0, 0, 0));
        if (TestEqual(TEXT("G2: 3 rows"), Items.Num(), 3))
        {
            const TCHAR* const Groups[3] = { TEXT("AIR"), TEXT("COMMAND"), TEXT("OTHER") };
            const int32 SubCounts[3] = { 2, 1, 1 };
            for (int32 Index = 0; Index < 3; ++Index)
            {
                TestEqual(FString::Printf(TEXT("G2: row %d group"), Index), Items[Index].Group, FString(Groups[Index]));
                TestEqual(FString::Printf(TEXT("G2: row %d SubCount"), Index), Items[Index].SubCount, SubCounts[Index]);
                TestEqual(FString::Printf(TEXT("G2: row %d ActiveSub"), Index), Items[Index].ActiveSub, 0);
                TestTrue(FString::Printf(TEXT("G2: row %d hold"), Index), Items[Index].bHoldToConfirm);
                TestFalse(FString::Printf(TEXT("G2: row %d disabled"), Index), Items[Index].bEnabled);
            }
            TestEqual(TEXT("G2: label"), Items[0].Label, FString(TEXT("ALPHA  0/2")));
            TestTrue(TEXT("G2: detail NEXT"), Items[0].Detail.StartsWith(TEXT("NEXT")));
        }
    }

    // G3 상태
    {
        const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 1000, 50, 1));
        const TArray<FHDShipMenuItem> Fresh = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 1000, 50, 0));
        const TArray<FHDShipMenuItem> Maxed = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 1000, 50, 1, 1));
        if (Items.Num() == 3 && Fresh.Num() == 3 && Maxed.Num() == 3)
        {
            TestTrue(TEXT("G3: (A,0) Owned"), HDShipMenuView::GetTierState(Items[0], 0) == EHDTierState::Owned);
            TestTrue(TEXT("G3: (A,1) Insufficient"), HDShipMenuView::GetTierState(Items[0], 1) == EHDTierState::Insufficient);
            TestTrue(TEXT("G3: (B,0) Available"), HDShipMenuView::GetTierState(Items[1], 0) == EHDTierState::Available);
            TestTrue(TEXT("G3: A=0 (A,1) NeedsPrevious"), HDShipMenuView::GetTierState(Fresh[0], 1) == EHDTierState::NeedsPrevious);
            TestTrue(TEXT("G3: B=1 (B,0) Owned"), HDShipMenuView::GetTierState(Maxed[1], 0) == EHDTierState::Owned);

            TestEqual(TEXT("G3: label OWNED"), HDShipMenuView::GetTierStateLabel(Items[0], 0), FString(TEXT("OWNED")));
            TestEqual(TEXT("G3: label AVAILABLE"), HDShipMenuView::GetTierStateLabel(Items[1], 0), FString(TEXT("AVAILABLE")));
            TestEqual(TEXT("G3: label NOT ENOUGH"), HDShipMenuView::GetTierStateLabel(Items[0], 1), FString(TEXT("NOT ENOUGH")));
            TestEqual(TEXT("G3: label REQUIRES"), HDShipMenuView::GetTierStateLabel(Fresh[0], 1), FString(TEXT("REQUIRES TIER 1")));
        }
        else
        {
            AddError(TEXT("G3: expected 3 rows"));
        }
    }

    // G4 칸
    {
        TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 0, 0, 0));
        const TArray<FHDNavCell> Cells = HDShipMenuNav::MakeUpgradeCells(Items);
        TestEqual(TEXT("G4: 4 cards"), Cells.Num(), 4);
        TestEqual(TEXT("G4: (A,0)"), CellText(Cells, HDShipMenuNav::FindCell(Cells, 0, 0)), FString(TEXT("(0,0)")));
        TestEqual(TEXT("G4: (A,1)"), CellText(Cells, HDShipMenuNav::FindCell(Cells, 0, 1)), FString(TEXT("(1,0)")));
        TestEqual(TEXT("G4: (B,0)"), CellText(Cells, HDShipMenuNav::FindCell(Cells, 1, 0)), FString(TEXT("(2,0)")));
        TestEqual(TEXT("G4: (C,0)"), CellText(Cells, HDShipMenuNav::FindCell(Cells, 2, 0)), FString(TEXT("(4,0)")));

        // 같은 분야 둘째 줄은 행 1
        if (Items.Num() == 3)
        {
            Items[1].Group = TEXT("AIR");
            const TArray<FHDNavCell> SameGroup = HDShipMenuNav::MakeUpgradeCells(Items);
            TestEqual(TEXT("G4: second row in group"), CellText(SameGroup, HDShipMenuNav::FindCell(SameGroup, 1, 0)), FString(TEXT("(0,1)")));
            TestEqual(TEXT("G4: C moves to group 1"), CellText(SameGroup, HDShipMenuNav::FindCell(SameGroup, 2, 0)), FString(TEXT("(2,0)")));
        }
    }

    // G5 키
    {
        const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 1000, 50, 0));
        FHDShipMenu Menu;
        Menu.OpenTerminal(EHDShipScreen::Upgrades);
        TestEqual(TEXT("G5: cursor 0"), Menu.GetCursor(), 0);
        TestEqual(TEXT("G5: sub 0"), Menu.GetSubCursor(), 0);
        TestTrue(TEXT("G5: Confirm selects A"), IsAction(Menu.HandleKey(EHDShipMenuKey::Confirm, Items), EHDShipActionType::Select, NameA));
        TestTrue(TEXT("G5: Right none"), IsAction(Menu.HandleKey(EHDShipMenuKey::Right, Items), EHDShipActionType::None));
        TestEqual(TEXT("G5: Right -> sub 1"), Menu.GetSubCursor(), 1);
        TestEqual(TEXT("G5: Right -> cursor 0"), Menu.GetCursor(), 0);
        TestTrue(TEXT("G5: Confirm on tier 2 none"), IsAction(Menu.HandleKey(EHDShipMenuKey::Confirm, Items), EHDShipActionType::None));
        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("G5: Right -> cursor 1"), Menu.GetCursor(), 1);
        TestEqual(TEXT("G5: Right -> sub 0"), Menu.GetSubCursor(), 0);
        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("G5: Right -> cursor 2"), Menu.GetCursor(), 2);
        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("G5: Right at end stays"), Menu.GetCursor(), 2);
        Menu.HandleKey(EHDShipMenuKey::Left, Items);
        Menu.HandleKey(EHDShipMenuKey::Left, Items);
        TestEqual(TEXT("G5: Left x2 -> cursor 0"), Menu.GetCursor(), 0);
        TestEqual(TEXT("G5: Left x2 -> sub 1"), Menu.GetSubCursor(), 1);
        Menu.HandleKey(EHDShipMenuKey::Down, Items);
        TestEqual(TEXT("G5: Down stays cursor"), Menu.GetCursor(), 0);
        TestEqual(TEXT("G5: Down stays sub"), Menu.GetSubCursor(), 1);
        TestTrue(TEXT("G5: Back closes"), IsAction(Menu.HandleKey(EHDShipMenuKey::Back, Items), EHDShipActionType::CloseTerminal, FName(TEXT("Upgrades"))));

        // 단계 없는 가짜 줄뿐이면 목록 규칙(감김)
        TArray<FHDShipMenuItem> Fake;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            FHDShipMenuItem& Item = Fake.AddDefaulted_GetRef();
            Item.Id = FName(*FString::Printf(TEXT("Fake%d"), Index));
        }
        FHDShipMenu FakeMenu;
        FakeMenu.OpenTerminal(EHDShipScreen::Upgrades);
        FakeMenu.HandleKey(EHDShipMenuKey::Up, Fake);
        TestEqual(TEXT("G5: fake Up wraps"), FakeMenu.GetCursor(), 2);
        FakeMenu.HandleKey(EHDShipMenuKey::Down, Fake);
        TestEqual(TEXT("G5: fake Down wraps"), FakeMenu.GetCursor(), 0);
    }

    // G6 길게 누르기 판정
    {
        const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 1000, 50, 0));
        FHDShipMenu Menu;
        Menu.OpenTerminal(EHDShipScreen::Upgrades);
        TestTrue(TEXT("G6: start needs hold"), HDShipMenu::NeedsHold(Menu, Items));
        TestEqual(TEXT("G6: NeedsHold leaves cursor"), Menu.GetCursor(), 0);
        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestFalse(TEXT("G6: tier 2 no hold"), HDShipMenu::NeedsHold(Menu, Items));

        const TArray<FHDShipMenuItem> Poor = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 0, 0, 0));
        FHDShipMenu PoorMenu;
        PoorMenu.OpenTerminal(EHDShipScreen::Upgrades);
        TestFalse(TEXT("G6: no requisition no hold"), HDShipMenu::NeedsHold(PoorMenu, Poor));

        const FHDShipMenuContext BridgeContext = MakeContext(Catalog, 1000, 50, 0);
        FHDShipMenu Bridge(EHDShipScreen::Bridge);
        TestFalse(TEXT("G6: Bridge no hold"), HDShipMenu::NeedsHold(Bridge, HDShipMenu::BuildItems(EHDShipScreen::Bridge, BridgeContext)));

        TArray<FHDShipMenuItem> NoHold = Items;
        NoHold[0].bHoldToConfirm = false;
        FHDShipMenu NoHoldMenu;
        NoHoldMenu.OpenTerminal(EHDShipScreen::Upgrades);
        TestFalse(TEXT("G6: bHoldToConfirm false no hold"), HDShipMenu::NeedsHold(NoHoldMenu, NoHold));
    }

    // G7 길게 누르기
    {
        FHDHoldConfirm Hold;
        TestTrue(TEXT("G7: Start"), Hold.Start(NameA, 0, 0.8f));
        TestTrue(TEXT("G7: active"), Hold.IsActive());
        TestEqual(TEXT("G7: id"), Hold.GetItemId(), NameA);
        TestEqual(TEXT("G7: progress 0"), Hold.GetProgress(), 0.f);
        TestFalse(TEXT("G7: Tick 0.4"), Hold.Tick(0.4f));
        TestEqual(TEXT("G7: progress 0.5"), Hold.GetProgress(), 0.5f, 0.001f);
        TestFalse(TEXT("G7: Start while running"), Hold.Start(NameB, 0, 0.8f));
        TestEqual(TEXT("G7: progress kept"), Hold.GetProgress(), 0.5f, 0.001f);
        TestEqual(TEXT("G7: id kept"), Hold.GetItemId(), NameA);
        TestFalse(TEXT("G7: Tick 0.39"), Hold.Tick(0.39f));
        TestTrue(TEXT("G7: Tick 0.02 completes"), Hold.Tick(0.02f));
        TestFalse(TEXT("G7: inactive after"), Hold.IsActive());
        TestFalse(TEXT("G7: only once"), Hold.Tick(1.f));

        TestTrue(TEXT("G7: Start again"), Hold.Start(NameA, 1, 0.8f));
        TestEqual(TEXT("G7: sub"), Hold.GetSubIndex(), 1);
        Hold.Cancel();
        TestFalse(TEXT("G7: cancelled"), Hold.IsActive());
        TestFalse(TEXT("G7: Tick after cancel"), Hold.Tick(1.f));
        TestFalse(TEXT("G7: Start 0 seconds"), Hold.Start(NameA, 0, 0.f));
        TestFalse(TEXT("G7: not active after 0"), Hold.IsActive());
    }

    // G8 상세
    {
        const FHDShipMenuContext Context = MakeContext(Catalog, 1000, 50, 1);
        const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, Context);
        FHDShipMenu Menu;
        Menu.OpenTerminal(EHDShipScreen::Upgrades);
        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("G8: sub 1"), Menu.GetSubCursor(), 1);

        const FHDUpgradeDetail Detail = HDShipMenuView::BuildUpgradeDetail(Menu, Items, Context, 0, 1);
        TestEqual(TEXT("G8: icon"), Detail.IconId, NameA);
        TestEqual(TEXT("G8: title"), Detail.Title, FString(TEXT("ALPHA")));
        TestEqual(TEXT("G8: group"), Detail.Group, FString(TEXT("AIR")));
        TestEqual(TEXT("G8: description"), Detail.Description, FString(TEXT("Alpha.")));
        TestEqual(TEXT("G8: tier"), Detail.TierLabel, FString(TEXT("TIER 2 / 2")));
        TestTrue(TEXT("G8: state Insufficient"), Detail.State == EHDTierState::Insufficient);
        TestEqual(TEXT("G8: state label"), Detail.StateLabel, FString(TEXT("NOT ENOUGH")));
        TestEqual(TEXT("G8: current"), Detail.Current, FString(TEXT("EAGLE REARM -20%")));
        TestEqual(TEXT("G8: this tier"), Detail.ThisTier, FString(TEXT("EAGLE REARM -40%")));
        TestEqual(TEXT("G8: cost"), CostText(Detail), FString(TEXT("Requisition 200 ok, SampleCommon 10 ok, SampleRare 3 short, SampleSuper 1 short")));

        const FHDShipMenuContext FreshContext = MakeContext(Catalog, 1000, 50, 0);
        const TArray<FHDShipMenuItem> Fresh = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, FreshContext);
        const FHDUpgradeDetail FreshDetail = HDShipMenuView::BuildUpgradeDetail(Menu, Fresh, FreshContext, 0, 1);
        TestEqual(TEXT("G8: A=0 state"), FreshDetail.StateLabel, FString(TEXT("REQUIRES TIER 1")));
        TestEqual(TEXT("G8: A=0 current"), FreshDetail.Current, FString(TEXT("NONE")));

        const FHDUpgradeDetail OwnedDetail = HDShipMenuView::BuildUpgradeDetail(Menu, Items, Context, 0, 0);
        TestEqual(TEXT("G8: tier 1 OWNED"), OwnedDetail.StateLabel, FString(TEXT("OWNED")));
        TestEqual(TEXT("G8: tier 1 label"), OwnedDetail.TierLabel, FString(TEXT("TIER 1 / 2")));

        const FHDUpgradeDetail BravoDetail = HDShipMenuView::BuildUpgradeDetail(Menu, Items, Context, 1, 0);
        TestEqual(TEXT("G8: B cost two parts"), CostText(BravoDetail), FString(TEXT("Requisition 150 ok, SampleCommon 8 ok")));
    }

    // G9 키 안내
    {
        const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, MakeContext(Catalog, 1000, 50, 0));
        FHDShipMenu Menu;
        Menu.OpenTerminal(EHDShipScreen::Upgrades);
        TestEqual(TEXT("G9: available"), Hints(Menu, Items), FString(TEXT("[ARROWS] MOVE   [HOLD ENTER] BUY   [BACKSPACE] CLOSE")));
        const TArray<FHDKeyHint> HintList = HDShipMenuView::GetKeyHints(Menu, Items, false);
        const FHDKeyHint* HoldHint = HintList.FindByPredicate([](const FHDKeyHint& Hint) { return Hint.Key == TEXT("HOLD ENTER"); });
        TestTrue(TEXT("G9: HOLD ENTER not clickable"), HoldHint && HoldHint->bClickable == false);

        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("G9: other state"), Hints(Menu, Items), FString(TEXT("[ARROWS] MOVE   [BACKSPACE] CLOSE")));

        const FHDShipMenuContext BridgeContext = MakeContext(Catalog, 1000, 50, 0);
        const TArray<FHDShipMenuItem> BridgeItems = HDShipMenu::BuildItems(EHDShipScreen::Bridge, BridgeContext);
        const int32 UpgradesIndex = BridgeItems.IndexOfByPredicate([](const FHDShipMenuItem& Item) { return Item.Id == FName(TEXT("Upgrades")); });
        FHDShipMenu Bridge(EHDShipScreen::Bridge);
        Bridge.SetCursor(UpgradesIndex, BridgeItems.Num());
        Bridge.HandleKey(EHDShipMenuKey::Confirm, BridgeItems);
        TestTrue(TEXT("G9: Bridge opens Upgrades"), Bridge.GetScreen() == EHDShipScreen::Upgrades);
        TestTrue(TEXT("G9: from Bridge ends BACK"), Hints(Bridge, Items).EndsWith(TEXT("[BACKSPACE] BACK")));
    }

    return true;
}

#endif
