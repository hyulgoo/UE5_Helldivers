// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HDShipMenu.h"
#include "UI/HDShipMenuView.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Progression/HDWeaponCatalog.h"
#include "Stratagem/HDStratagemCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDArmoryScreenTest, "Helldivers.FrontEnd.ArmoryScreen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDArmoryScreenTest
{
    const FName OrbitalGatlingBarrage(TEXT("OrbitalGatlingBarrage"));
    const FName Shotgun(TEXT("Shotgun"));

    FHDShipMenuContext MakeContext(int32 Requisition)
    {
        FHDShipMenuContext Context;
        Context.Stratagems              = &FHDStratagemCatalog::GetProjectCatalog();
        Context.Upgrades                = &FHDUpgradeCatalog::GetProjectCatalog();
        Context.Profile.Requisition     = Requisition;
        return Context;
    }

    FHDShipMenu MakeArmoryMenu()
    {
        FHDShipMenu Menu;
        Menu.OpenTerminal(EHDShipScreen::Armory);
        return Menu;
    }

    FString Hints(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items)
    {
        return HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(Menu, Items, false));
    }

    bool IsAction(const FHDShipMenuAction& Action, EHDShipActionType Type, FName Id = NAME_None)
    {
        return Action.Type == Type && (Id.IsNone() || Action.Id == Id);
    }

    FString LinesText(const FHDArmoryDetail& Detail)
    {
        TArray<FString> Parts;
        for (const FHDDetailLine& Line : Detail.Lines)
        {
            Parts.Add(Line.Label + TEXT(" ") + Line.Value);
        }
        return FString::Join(Parts, TEXT("|"));
    }

    FString CostText(const FHDArmoryDetail& Detail)
    {
        TArray<FString> Parts;
        for (const FHDCostPart& Part : Detail.Cost)
        {
            Parts.Add(FString::Printf(TEXT("%s %d %s"), *Part.IconId.ToString(), Part.Amount, Part.bEnough ? TEXT("ok") : TEXT("short")));
        }
        return FString::Join(Parts, TEXT(", "));
    }
}

bool FHDArmoryScreenTest::RunTest(const FString& Parameters)
{
    using namespace HDArmoryScreenTest;

    const FHDShipMenuContext Poor = MakeContext(0);
    const FHDShipMenuContext Rich = MakeContext(1000);
    const TArray<FHDShipMenuItem> PoorItems = HDShipMenu::BuildItems(EHDShipScreen::Armory, Poor);
    const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Armory, Rich);

    // R1 줄
    {
        TArray<FString> Ids, Groups;
        bool bIconIsId = true, bHold = true, bAllDisabled = true;
        for (const FHDShipMenuItem& Item : PoorItems)
        {
            Ids.Add(Item.Id.ToString());
            Groups.Add(Item.Group);
            bIconIsId &= Item.IconId == Item.Id;
            bHold &= Item.bHoldToConfirm;
            bAllDisabled &= Item.bEnabled == false;
        }
        TestEqual(TEXT("R1 ids"), FString::Join(Ids, TEXT(",")), FString(TEXT("OrbitalGatlingBarrage,Orbital120mmBarrage,Shotgun,MachinePistol,ImpactGrenade")));
        TestEqual(TEXT("R1 groups"), FString::Join(Groups, TEXT(", ")), FString(TEXT("STRATAGEMS, STRATAGEMS, PRIMARY, SECONDARY, GRENADES")));
        TestTrue(TEXT("R1 IconId = Id"), bIconIsId);
        TestTrue(TEXT("R1 hold"), bHold);
        TestTrue(TEXT("R1 all disabled"), bAllDisabled);
        if (TestEqual(TEXT("R1 count"), PoorItems.Num(), 5))
        {
            TestEqual(TEXT("R1 shotgun label"), PoorItems[2].Label, FString(TEXT("SHOTGUN  100 REQ")));
        }
    }
    if (Items.Num() != 5)
    {
        AddError(TEXT("armory rows missing"));
        return false;
    }

    // R2 탭
    {
        TestEqual(TEXT("R2 tabs"), FString::Join(HDShipMenu::GetArmoryTabs(), TEXT(", ")), FString(TEXT("STRATAGEMS, PRIMARY, SECONDARY, GRENADES")));
        FHDShipMenu Menu = MakeArmoryMenu();
        TestEqual(TEXT("R2 open tab"), Menu.GetActiveTab(), 0);
        TestEqual(TEXT("R2 open cursor"), Menu.GetCursor(), 0);

        TestTrue(TEXT("R2 next -> ChangeTab PRIMARY"), IsAction(Menu.HandleKey(EHDShipMenuKey::NextTab, Items), EHDShipActionType::ChangeTab, FName(TEXT("PRIMARY"))));
        TestEqual(TEXT("R2 tab 1"), Menu.GetActiveTab(), 1);
        TestEqual(TEXT("R2 cursor 2"), Menu.GetCursor(), 2);

        Menu.HandleKey(EHDShipMenuKey::NextTab, Items);
        Menu.HandleKey(EHDShipMenuKey::NextTab, Items);
        TestEqual(TEXT("R2 tab 3"), Menu.GetActiveTab(), 3);
        TestEqual(TEXT("R2 cursor 4"), Menu.GetCursor(), 4);

        TestTrue(TEXT("R2 next at end -> None"), IsAction(Menu.HandleKey(EHDShipMenuKey::NextTab, Items), EHDShipActionType::None));
        TestEqual(TEXT("R2 end tab 3"), Menu.GetActiveTab(), 3);
        TestEqual(TEXT("R2 end cursor 4"), Menu.GetCursor(), 4);

        TestTrue(TEXT("R2 prev -> ChangeTab"), IsAction(Menu.HandleKey(EHDShipMenuKey::PrevTab, Items), EHDShipActionType::ChangeTab, FName(TEXT("SECONDARY"))));
        TestEqual(TEXT("R2 prev tab 2"), Menu.GetActiveTab(), 2);
        TestEqual(TEXT("R2 prev cursor 3"), Menu.GetCursor(), 3);

        Menu.SetActiveTab(0, Items);
        TestEqual(TEXT("R2 SetActiveTab tab"), Menu.GetActiveTab(), 0);
        TestEqual(TEXT("R2 SetActiveTab cursor"), Menu.GetCursor(), 0);
    }

    // R3 탭 안 이동
    {
        FHDShipMenu Menu = MakeArmoryMenu();
        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("R3 right -> 1"), Menu.GetCursor(), 1);
        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("R3 right -> 1 (no wrap)"), Menu.GetCursor(), 1);
        Menu.HandleKey(EHDShipMenuKey::Left, Items);
        TestEqual(TEXT("R3 left -> 0"), Menu.GetCursor(), 0);
        Menu.HandleKey(EHDShipMenuKey::Down, Items);
        TestEqual(TEXT("R3 down -> 0"), Menu.GetCursor(), 0);

        FHDShipMenu Tab1 = MakeArmoryMenu();
        Tab1.HandleKey(EHDShipMenuKey::NextTab, Items);
        for (const EHDShipMenuKey Key : { EHDShipMenuKey::Up, EHDShipMenuKey::Down, EHDShipMenuKey::Left, EHDShipMenuKey::Right })
        {
            Tab1.HandleKey(Key, Items);
            TestEqual(FString::Printf(TEXT("R3 tab 1 %s stays 2"), HDShipMenu::GetKeyName(Key)), Tab1.GetCursor(), 2);
        }

        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        Menu.HandleKey(EHDShipMenuKey::NextTab, Items);
        TestEqual(TEXT("R3 tab change -> first tile"), Menu.GetCursor(), 2);
    }

    // R4 확정
    {
        FHDShipMenu PoorMenu = MakeArmoryMenu();
        TestTrue(TEXT("R4 poor confirm None"), IsAction(PoorMenu.HandleKey(EHDShipMenuKey::Confirm, PoorItems), EHDShipActionType::None));

        FHDShipMenu Menu = MakeArmoryMenu();
        TestTrue(TEXT("R4 rich NeedsHold"), HDShipMenu::NeedsHold(Menu, Items));
        TestTrue(TEXT("R4 rich confirm Select"), IsAction(Menu.HandleKey(EHDShipMenuKey::Confirm, Items), EHDShipActionType::Select, OrbitalGatlingBarrage));

        FHDShipMenuContext Owned = Rich;
        Owned.Profile.UnlockedStratagems.Add(OrbitalGatlingBarrage);
        const TArray<FHDShipMenuItem> OwnedItems = HDShipMenu::BuildItems(EHDShipScreen::Armory, Owned);
        FHDShipMenu OwnedMenu = MakeArmoryMenu();
        TestFalse(TEXT("R4 owned row disabled"), OwnedItems.IsValidIndex(0) && OwnedItems[0].bEnabled);
        TestFalse(TEXT("R4 owned NeedsHold"), HDShipMenu::NeedsHold(OwnedMenu, OwnedItems));
        TestTrue(TEXT("R4 owned confirm None"), IsAction(OwnedMenu.HandleKey(EHDShipMenuKey::Confirm, OwnedItems), EHDShipActionType::None));

        FHDShipMenu Tab1 = MakeArmoryMenu();
        Tab1.HandleKey(EHDShipMenuKey::NextTab, Items);
        TestTrue(TEXT("R4 tab 1 confirm Shotgun"), IsAction(Tab1.HandleKey(EHDShipMenuKey::Confirm, Items), EHDShipActionType::Select, Shotgun));
    }

    // R5 다른 화면·키
    {
        EHDShipMenuKey Key = EHDShipMenuKey::Up;
        TestTrue(TEXT("R5 Q pressed"), HDShipMenu::TryGetMenuKey(EKeys::Q, IE_Pressed, Key) && Key == EHDShipMenuKey::PrevTab);
        Key = EHDShipMenuKey::Up;
        TestTrue(TEXT("R5 E pressed"), HDShipMenu::TryGetMenuKey(EKeys::E, IE_Pressed, Key) && Key == EHDShipMenuKey::NextTab);
        TestFalse(TEXT("R5 Q released"), HDShipMenu::TryGetMenuKey(EKeys::Q, IE_Released, Key));
        TestFalse(TEXT("R5 W"), HDShipMenu::TryGetMenuKey(EKeys::W, IE_Pressed, Key));
        TestFalse(TEXT("R5 SpaceBar"), HDShipMenu::TryGetMenuKey(EKeys::SpaceBar, IE_Pressed, Key));
        TestFalse(TEXT("R5 P"), HDShipMenu::TryGetMenuKey(EKeys::P, IE_Pressed, Key));

        for (const EHDShipScreen Screen : { EHDShipScreen::Bridge, EHDShipScreen::WarTable, EHDShipScreen::Loadout, EHDShipScreen::Upgrades, EHDShipScreen::System, EHDShipScreen::Title })
        {
            const TArray<FHDShipMenuItem> ScreenItems = HDShipMenu::BuildItems(Screen, Rich);
            FHDShipMenu Menu;
            Menu.OpenTerminal(Screen);
            const int32 Cursor = Menu.GetCursor();
            for (const EHDShipMenuKey TabKey : { EHDShipMenuKey::PrevTab, EHDShipMenuKey::NextTab })
            {
                const FHDShipMenuAction Action = Menu.HandleKey(TabKey, ScreenItems);
                const FString Name = FString::Printf(TEXT("R5 %s %s"), HDShipMenu::GetScreenName(Screen), HDShipMenu::GetKeyName(TabKey));
                TestTrue(Name + TEXT(" None"), IsAction(Action, EHDShipActionType::None));
                TestTrue(Name + TEXT(" screen same"), Menu.GetScreen() == Screen);
                TestEqual(Name + TEXT(" cursor same"), Menu.GetCursor(), Cursor);
            }
        }
    }

    // R6 초기화
    {
        FHDShipMenu Menu = MakeArmoryMenu();
        Menu.HandleKey(EHDShipMenuKey::NextTab, Items);
        Menu.HandleKey(EHDShipMenuKey::NextTab, Items);
        TestEqual(TEXT("R6 tab 2"), Menu.GetActiveTab(), 2);
        Menu.SetScreen(EHDShipScreen::Bridge);
        Menu.SetScreen(EHDShipScreen::Armory);
        TestEqual(TEXT("R6 SetScreen -> tab 0"), Menu.GetActiveTab(), 0);

        Menu.HandleKey(EHDShipMenuKey::NextTab, Items);
        Menu.OpenTerminal(EHDShipScreen::Armory);
        TestEqual(TEXT("R6 OpenTerminal -> tab 0"), Menu.GetActiveTab(), 0);
        TestTrue(TEXT("R6 root Back -> CloseTerminal"), IsAction(Menu.HandleKey(EHDShipMenuKey::Back, Items), EHDShipActionType::CloseTerminal, FName(TEXT("Armory"))));
    }

    // R7 상세
    {
        const FHDShipMenuContext Context = MakeContext(100);
        const TArray<FHDShipMenuItem> DetailItems = HDShipMenu::BuildItems(EHDShipScreen::Armory, Context);
        const FHDShipMenu Menu = MakeArmoryMenu();

        FHDArmoryDetail Detail = HDShipMenuView::BuildArmoryDetail(Menu, DetailItems, Context, 0);
        TestEqual(TEXT("R7 gatling title"), Detail.Title, FString(TEXT("ORBITAL GATLING BARRAGE")));
        TestEqual(TEXT("R7 gatling category"), Detail.Category, FString(TEXT("ORBITAL")));
        TestEqual(TEXT("R7 gatling icon"), Detail.IconId, OrbitalGatlingBarrage);
        TestEqual(TEXT("R7 gatling lines"), LinesText(Detail), FString(TEXT("CALL-IN 1 S|COOLDOWN 60 S|USES UNLIMITED")));
        TestTrue(TEXT("R7 gatling command"), Detail.Command == TArray<EHDCommandInput>({ EHDCommandInput::Right, EHDCommandInput::Down, EHDCommandInput::Left, EHDCommandInput::Up, EHDCommandInput::Up }));
        TestEqual(TEXT("R7 gatling cost"), CostText(Detail), FString(TEXT("Requisition 150 short")));
        TestEqual(TEXT("R7 gatling state"), Detail.StateLabel, FString(TEXT("NOT ENOUGH")));

        Detail = HDShipMenuView::BuildArmoryDetail(Menu, DetailItems, Context, 2);
        TestEqual(TEXT("R7 shotgun title"), Detail.Title, FString(TEXT("SHOTGUN")));
        TestEqual(TEXT("R7 shotgun category"), Detail.Category, FString(TEXT("PRIMARY")));
        TestEqual(TEXT("R7 shotgun description"), Detail.Description, FString(TEXT("8 PELLETS   6 SHELLS   ARMOR PEN 1")));
        if (TestEqual(TEXT("R7 shotgun bars"), Detail.Bars.Num(), 4))
        {
            TestEqual(TEXT("R7 shotgun bar 0"), Detail.Bars[0].Value, FString(TEXT("50 X8")));
        }
        TestEqual(TEXT("R7 shotgun cost"), CostText(Detail), FString(TEXT("Requisition 100 ok")));
        TestEqual(TEXT("R7 shotgun state"), Detail.StateLabel, FString(TEXT("AVAILABLE")));

        FHDShipMenuContext Owned = Context;
        Owned.Profile.UnlockedWeapons.Add(Shotgun);
        Detail = HDShipMenuView::BuildArmoryDetail(Menu, HDShipMenu::BuildItems(EHDShipScreen::Armory, Owned), Owned, 2);
        TestEqual(TEXT("R7 owned state"), Detail.StateLabel, FString(TEXT("UNLOCKED")));
        TestEqual(TEXT("R7 owned cost empty"), Detail.Cost.Num(), 0);
    }

    // R8 키 안내
    {
        FHDShipMenu Menu = MakeArmoryMenu();
        TestEqual(TEXT("R8 rich"), Hints(Menu, Items), FString(TEXT("[Q/E] TAB   [ARROWS] MOVE   [HOLD ENTER] UNLOCK   [BACKSPACE] CLOSE")));
        TestEqual(TEXT("R8 poor"), Hints(Menu, PoorItems), FString(TEXT("[Q/E] TAB   [ARROWS] MOVE   [BACKSPACE] CLOSE")));
        for (const FHDKeyHint& Hint : HDShipMenuView::GetKeyHints(Menu, Items, false))
        {
            if (Hint.Key == TEXT("Q/E") || Hint.Key == TEXT("HOLD ENTER"))
            {
                TestFalse(FString::Printf(TEXT("R8 %s not clickable"), *Hint.Key), Hint.bClickable);
            }
        }

        FHDShipMenu Bridge(EHDShipScreen::Bridge);
        const TArray<FHDShipMenuItem> BridgeItems = HDShipMenu::BuildItems(EHDShipScreen::Bridge, Rich);
        const int32 ArmoryIndex = BridgeItems.IndexOfByPredicate([](const FHDShipMenuItem& Item) { return Item.Id == FName(TEXT("Armory")); });
        Bridge.SetCursor(ArmoryIndex, BridgeItems.Num());
        Bridge.HandleKey(EHDShipMenuKey::Confirm, BridgeItems);
        TestTrue(TEXT("R8 bridge -> armory"), Bridge.GetScreen() == EHDShipScreen::Armory);
        TestTrue(TEXT("R8 bridge ends BACK"), Hints(Bridge, Items).EndsWith(TEXT("[BACKSPACE] BACK")));
    }

    // R9 이름
    {
        TestEqual(TEXT("R9 PrevTab"), FString(HDShipMenu::GetKeyName(EHDShipMenuKey::PrevTab)), FString(TEXT("PrevTab")));
        TestEqual(TEXT("R9 NextTab"), FString(HDShipMenu::GetKeyName(EHDShipMenuKey::NextTab)), FString(TEXT("NextTab")));
        TestEqual(TEXT("R9 ChangeTab"), FString(HDShipMenu::GetActionName(EHDShipActionType::ChangeTab)), FString(TEXT("tab")));
        TestEqual(TEXT("R9 Back = 5"), static_cast<int32>(EHDShipMenuKey::Back), 5);
    }

    return true;
}

#endif
