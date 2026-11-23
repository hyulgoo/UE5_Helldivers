// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HDShipMenu.h"
#include "UI/HDShipMenuNav.h"
#include "UI/HDShipMenuView.h"
#include "Progression/HDArmoryRules.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Progression/HDWeaponCatalog.h"
#include "Stratagem/HDStratagemCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDLoadoutScreenTest, "Helldivers.FrontEnd.LoadoutScreen", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDLoadoutScreenTest
{
    const FName EagleAirstrike(TEXT("EagleAirstrike"));
    const FName OrbitalPrecisionStrike(TEXT("OrbitalPrecisionStrike"));
    const FName OrbitalGatlingBarrage(TEXT("OrbitalGatlingBarrage"));
    const FName MachineGunSentry(TEXT("MachineGunSentry"));
    const FName AntiMaterielRifle(TEXT("AntiMaterielRifle"));
    const FName Resupply(TEXT("Resupply"));

    FHDShipMenuContext MakeContext()
    {
        FHDShipMenuContext Context;
        Context.Stratagems          = &FHDStratagemCatalog::GetProjectCatalog();
        Context.Upgrades            = &FHDUpgradeCatalog::GetProjectCatalog();
        Context.Profile.Loadout     = { EagleAirstrike, OrbitalPrecisionStrike, MachineGunSentry, AntiMaterielRifle };
        Context.DefaultLoadout      = Context.Profile.Loadout;
        return Context;
    }

    // OpenTerminal(WarTable) → Down → Confirm 으로 연 Loadout
    FHDShipMenu MakeLoadoutMenu(const FHDShipMenuContext& Context)
    {
        FHDShipMenu Menu;
        Menu.OpenTerminal(EHDShipScreen::WarTable);
        const TArray<FHDShipMenuItem> WarItems = HDShipMenu::BuildItems(EHDShipScreen::WarTable, Context);
        Menu.HandleKey(EHDShipMenuKey::Down, WarItems);
        Menu.HandleKey(EHDShipMenuKey::Confirm, WarItems);
        return Menu;
    }

    FString CellText(const TArray<FHDNavCell>& Cells, int32 Index)
    {
        if (Cells.IsValidIndex(Index) == false)
        {
            return TEXT("missing");
        }
        const FHDNavCell& Cell = Cells[Index];
        return FString::Printf(TEXT("%d:(%d,%d) %dx%d"), Cell.ItemIndex, Cell.Col, Cell.Row, Cell.ColSpan, Cell.RowSpan);
    }

    FString Hints(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items, bool bLaunching = false)
    {
        return HDShipMenuView::JoinKeyHints(HDShipMenuView::GetKeyHints(Menu, Items, bLaunching));
    }

    const FHDDetailLine* FindLine(const FHDLoadoutDetail& Detail, const TCHAR* Label)
    {
        return Detail.Lines.FindByPredicate([Label](const FHDDetailLine& Line) { return Line.Label == Label; });
    }

    FString LineValue(const FHDLoadoutDetail& Detail, const TCHAR* Label)
    {
        const FHDDetailLine* Line = FindLine(Detail, Label);
        return Line ? Line->Value : FString(TEXT("missing"));
    }

    FString JoinLine(const FHDLoadoutDetail& Detail, int32 Index)
    {
        return Detail.Lines.IsValidIndex(Index) ? Detail.Lines[Index].Label + TEXT("  ") + Detail.Lines[Index].Value : FString(TEXT("missing"));
    }
}

bool FHDLoadoutScreenTest::RunTest(const FString& Parameters)
{
    using namespace HDLoadoutScreenTest;

    const FHDShipMenuContext Context = MakeContext();
    const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Loadout, Context);
    const int32 ItemCount = Items.Num();

    TArray<FString> ItemIds;
    for (const FHDShipMenuItem& Item : Items)
    {
        ItemIds.Add(Item.Id.ToString());
    }
    if (TestEqual(TEXT("rows"), FString::Join(ItemIds, TEXT(",")), FString(TEXT("Slot1,Slot2,Slot3,Slot4,Primary,Secondary,Grenade,Ready"))) == false)
    {
        return false;
    }

    // L1 칸
    {
        const TArray<FHDNavCell> Cells = HDShipMenuNav::MakeLoadoutCells(Items);
        TestEqual(TEXT("L1 cell count"), Cells.Num(), 8);
        const TCHAR* const Expected[8] = {
            TEXT("0:(0,0) 1x1"), TEXT("1:(1,0) 1x1"), TEXT("2:(2,0) 1x1"), TEXT("3:(3,0) 1x1"),
            TEXT("4:(0,1) 1x1"), TEXT("5:(1,1) 1x1"), TEXT("6:(2,1) 1x1"), TEXT("7:(4,0) 1x2") };
        for (int32 Index = 0; Index < 8; ++Index)
        {
            TestEqual(FString::Printf(TEXT("L1 cell %d"), Index), CellText(Cells, HDShipMenuNav::FindCell(Cells, Index)), FString(Expected[Index]));
        }

        TArray<FHDShipMenuItem> Unknown;
        Unknown.AddDefaulted_GetRef().Id = TEXT("A");
        Unknown.AddDefaulted_GetRef().Id = TEXT("B");
        const TArray<FHDNavCell> UnknownCells = HDShipMenuNav::MakeLoadoutCells(Unknown);
        TestEqual(TEXT("L1 unknown A"), CellText(UnknownCells, HDShipMenuNav::FindCell(UnknownCells, 0)), FString(TEXT("0:(0,2) 1x1")));
        TestEqual(TEXT("L1 unknown B"), CellText(UnknownCells, HDShipMenuNav::FindCell(UnknownCells, 1)), FString(TEXT("1:(1,2) 1x1")));
    }

    // L2 이동
    {
        FHDShipMenu Menu = MakeLoadoutMenu(Context);
        TestTrue(TEXT("L2 screen Loadout"), Menu.GetScreen() == EHDShipScreen::Loadout);
        bool bAllNone = true;
        auto Press = [&](EHDShipMenuKey Key, int32 Times = 1)
        {
            for (int32 Count = 0; Count < Times; ++Count)
            {
                bAllNone &= Menu.HandleKey(Key, Items).Type == EHDShipActionType::None;
            }
            return Menu.GetCursor();
        };

        TestEqual(TEXT("L2 Right x3"), Press(EHDShipMenuKey::Right, 3), 3);
        TestEqual(TEXT("L2 Right -> READY"), Press(EHDShipMenuKey::Right), 7);
        TestEqual(TEXT("L2 Right at READY stays"), Press(EHDShipMenuKey::Right), 7);
        TestEqual(TEXT("L2 Left from READY -> Slot4"), Press(EHDShipMenuKey::Left), 3);
        TestEqual(TEXT("L2 Down from Slot4 -> Grenade"), Press(EHDShipMenuKey::Down), 6);
        TestEqual(TEXT("L2 Right from Grenade -> READY"), Press(EHDShipMenuKey::Right), 7);
        TestEqual(TEXT("L2 Left -> Slot4"), Press(EHDShipMenuKey::Left), 3);
        TestEqual(TEXT("L2 Left x3"), Press(EHDShipMenuKey::Left, 3), 0);
        TestEqual(TEXT("L2 Left at Slot1 stays"), Press(EHDShipMenuKey::Left), 0);
        TestEqual(TEXT("L2 Down -> Primary"), Press(EHDShipMenuKey::Down), 4);
        TestEqual(TEXT("L2 Up -> Slot1"), Press(EHDShipMenuKey::Up), 0);
        TestTrue(TEXT("L2 all None (no Adjust)"), bAllNone);
    }

    // L3 선택지
    {
        const FHDShipMenuItem& Slot1 = Items[0];
        TArray<FString> Ids;
        TArray<FString> Categories;
        TArray<FString> Locked;
        for (const FHDShipMenuOption& Option : Slot1.Options)
        {
            Ids.Add(Option.Id.ToString());
            Categories.Add(Option.Category);
            if (Option.bUnlocked == false)
            {
                Locked.Add(FString::FromInt(Ids.Num() - 1));
            }
        }
        TestEqual(TEXT("L3 Slot1 ids"), FString::Join(Ids, TEXT(",")),
            FString(TEXT("EagleAirstrike,OrbitalPrecisionStrike,OrbitalGatlingBarrage,Orbital120mmBarrage,Resupply,MachineGunSentry,AntiMaterielRifle")));
        TestEqual(TEXT("L3 Slot1 categories"), FString::Join(Categories, TEXT(",")),
            FString(TEXT("EAGLE,ORBITAL,ORBITAL,ORBITAL,SUPPLY,DEFENSIVE,SUPPORT WEAPON")));
        TestEqual(TEXT("L3 Slot1 locked"), FString::Join(Locked, TEXT(",")), FString(TEXT("2,3")));

        if (TestEqual(TEXT("L3 Slot1 option count"), Slot1.Options.Num(), 7))
        {
            const int32 ExpectedSlots[7] = { INDEX_NONE, 1, INDEX_NONE, INDEX_NONE, INDEX_NONE, 2, 3 };
            for (int32 Index = 0; Index < 7; ++Index)
            {
                TestEqual(FString::Printf(TEXT("L3 EquippedSlot %d"), Index), Slot1.Options[Index].EquippedSlot, ExpectedSlots[Index]);
                TestEqual(FString::Printf(TEXT("L3 bCurrent %d"), Index), Slot1.Options[Index].bCurrent, Index == 0);
            }
        }

        const FHDShipMenuItem& Primary = Items[4];
        if (TestEqual(TEXT("L3 Primary option count"), Primary.Options.Num(), 2))
        {
            TestEqual(TEXT("L3 Primary 0"), Primary.Options[0].Id, FName(TEXT("AssaultRifle")));
            TestTrue(TEXT("L3 Primary 0 current unlocked"), Primary.Options[0].bCurrent && Primary.Options[0].bUnlocked);
            TestEqual(TEXT("L3 Primary 1"), Primary.Options[1].Id, FName(TEXT("Shotgun")));
            TestTrue(TEXT("L3 Primary 1 locked"), Primary.Options[1].bUnlocked == false && Primary.Options[1].bCurrent == false);
            TestEqual(TEXT("L3 Primary category"), Primary.Options[0].Category, FString(TEXT("PRIMARY")));
        }

        const FHDShipMenuItem& Grenade = Items[6];
        if (TestEqual(TEXT("L3 Grenade option count"), Grenade.Options.Num(), 2))
        {
            TestEqual(TEXT("L3 Grenade 0"), Grenade.Options[0].Id, FName(TEXT("FragGrenade")));
            TestEqual(TEXT("L3 Grenade 1"), Grenade.Options[1].Id, FName(TEXT("ImpactGrenade")));
            TestEqual(TEXT("L3 Grenade category"), Grenade.Options[1].Category, FString(TEXT("GRENADE")));
        }

        TestEqual(TEXT("L3 IconId Slot1"), Items[0].IconId, EagleAirstrike);
        TestEqual(TEXT("L3 IconId Primary"), Items[4].IconId, FName(TEXT("AssaultRifle")));
        TestTrue(TEXT("L3 IconId Ready none"), Items[7].IconId.IsNone());
        TestTrue(TEXT("L3 Ready has no options"), Items[7].Options.IsEmpty());
    }

    // L4 창
    {
        FHDShipMenu Menu = MakeLoadoutMenu(Context);
        FHDShipMenuAction Action = Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        TestTrue(TEXT("L4 OpenPicker Slot1"), Action.Type == EHDShipActionType::OpenPicker && Action.Id == FName(TEXT("Slot1")));
        TestTrue(TEXT("L4 picker open"), Menu.IsPickerOpen());
        TestEqual(TEXT("L4 picker cursor 0"), Menu.GetPickerCursor(), 0);

        const TArray<FHDNavCell> Cells = HDShipMenuNav::MakePickerCells(Items[0].Options, 4);
        const TCHAR* const Expected[7] = {
            TEXT("0:(0,0) 1x1"), TEXT("1:(0,1) 1x1"), TEXT("2:(1,1) 1x1"), TEXT("3:(2,1) 1x1"),
            TEXT("4:(0,2) 1x1"), TEXT("5:(0,3) 1x1"), TEXT("6:(0,4) 1x1") };
        TestEqual(TEXT("L4 picker cell count"), Cells.Num(), 7);
        for (int32 Index = 0; Index < 7; ++Index)
        {
            TestEqual(FString::Printf(TEXT("L4 picker cell %d"), Index), CellText(Cells, HDShipMenuNav::FindCell(Cells, Index)), FString(Expected[Index]));
        }

        Action = Menu.HandleKey(EHDShipMenuKey::Down, Items);
        TestEqual(TEXT("L4 Down -> 1"), Menu.GetPickerCursor(), 1);
        TestTrue(TEXT("L4 Down is None"), Action.Type == EHDShipActionType::None);
        Action = Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        TestTrue(TEXT("L4 IN SLOT 2 confirm is None"), Action.Type == EHDShipActionType::None);
        TestTrue(TEXT("L4 picker stays open"), Menu.IsPickerOpen());

        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("L4 Right -> 2"), Menu.GetPickerCursor(), 2);
        TestTrue(TEXT("L4 locked confirm is None"), Menu.HandleKey(EHDShipMenuKey::Confirm, Items).Type == EHDShipActionType::None);

        Menu.HandleKey(EHDShipMenuKey::Down, Items);
        TestEqual(TEXT("L4 Down -> 4"), Menu.GetPickerCursor(), 4);
        Action = Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        TestTrue(TEXT("L4 Equip Slot1 Resupply"), Action.Type == EHDShipActionType::Equip && Action.Id == FName(TEXT("Slot1")) && Action.OptionId == Resupply);
        TestFalse(TEXT("L4 picker closed after equip"), Menu.IsPickerOpen());
        TestEqual(TEXT("L4 cursor 0 after equip"), Menu.GetCursor(), 0);
    }

    // L5 닫기
    {
        FHDShipMenu Menu = MakeLoadoutMenu(Context);
        Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        FHDShipMenuAction Action = Menu.HandleKey(EHDShipMenuKey::Back, Items);
        TestTrue(TEXT("L5 Back ClosePicker Slot1"), Action.Type == EHDShipActionType::ClosePicker && Action.Id == FName(TEXT("Slot1")));
        TestTrue(TEXT("L5 screen Loadout"), Menu.GetScreen() == EHDShipScreen::Loadout);
        TestFalse(TEXT("L5 picker closed"), Menu.IsPickerOpen());
        TestEqual(TEXT("L5 cursor 0"), Menu.GetCursor(), 0);
        Action = Menu.HandleKey(EHDShipMenuKey::Back, Items);
        TestTrue(TEXT("L5 Back again opens WarTable"), Action.Type == EHDShipActionType::OpenScreen && Action.Id == FName(TEXT("WarTable")));

        FHDShipMenu CursorMenu = MakeLoadoutMenu(Context);
        CursorMenu.HandleKey(EHDShipMenuKey::Confirm, Items);
        CursorMenu.SetCursor(2, ItemCount);
        TestFalse(TEXT("L5 SetCursor closes picker"), CursorMenu.IsPickerOpen());
        TestEqual(TEXT("L5 SetCursor cursor 2"), CursorMenu.GetCursor(), 2);

        FHDShipMenu ScreenMenu = MakeLoadoutMenu(Context);
        ScreenMenu.HandleKey(EHDShipMenuKey::Confirm, Items);
        ScreenMenu.SetScreen(EHDShipScreen::Bridge);
        TestFalse(TEXT("L5 SetScreen closes picker"), ScreenMenu.IsPickerOpen());
    }

    // L6 현재·READY
    {
        FHDShipMenu Menu = MakeLoadoutMenu(Context);
        Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        FHDShipMenuAction Action = Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        TestTrue(TEXT("L6 current confirm is ClosePicker"), Action.Type == EHDShipActionType::ClosePicker);
        TestFalse(TEXT("L6 picker closed"), Menu.IsPickerOpen());

        Menu.SetCursor(7, ItemCount);
        TestTrue(TEXT("L6 READY Launch"), Menu.HandleKey(EHDShipMenuKey::Confirm, Items).Type == EHDShipActionType::Launch);

        Menu.SetCursor(4, ItemCount);
        Action = Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        TestTrue(TEXT("L6 OpenPicker Primary"), Action.Type == EHDShipActionType::OpenPicker && Action.Id == FName(TEXT("Primary")));
        TestEqual(TEXT("L6 Primary picker cursor 0"), Menu.GetPickerCursor(), 0);
        // 브리프 표는 Down 이지만 두 선택지가 같은 행(0,0)·(1,0)이라 Move 규칙상 Right 가 Shotgun 이다(「결과」 참고)
        Menu.HandleKey(EHDShipMenuKey::Right, Items);
        TestEqual(TEXT("L6 Right -> Shotgun"), Menu.GetPickerCursor(), 1);
        TestTrue(TEXT("L6 locked Shotgun confirm is None"), Menu.HandleKey(EHDShipMenuKey::Confirm, Items).Type == EHDShipActionType::None);
    }

    // L7 장착 규칙
    {
        const FHDStratagemCatalog& Catalog = *Context.Stratagems;
        const FHDProfileData Data;
        const TArray<FName> Base = { EagleAirstrike, OrbitalPrecisionStrike, MachineGunSentry, AntiMaterielRifle };

        TArray<FName> Loadout = Base;
        TestTrue(TEXT("L7 slot 0 <- Resupply"), HDArmory::TrySetSlot(Loadout, 0, Resupply, Data, Catalog));
        TestTrue(TEXT("L7 Resupply set"), Loadout == TArray<FName>({ Resupply, OrbitalPrecisionStrike, MachineGunSentry, AntiMaterielRifle }));

        Loadout = Base;
        TestFalse(TEXT("L7 slot 0 <- OPS (in slot 1)"), HDArmory::TrySetSlot(Loadout, 0, OrbitalPrecisionStrike, Data, Catalog));
        TestTrue(TEXT("L7 OPS unchanged"), Loadout == Base);
        TestFalse(TEXT("L7 slot 0 <- Gatling (locked)"), HDArmory::TrySetSlot(Loadout, 0, OrbitalGatlingBarrage, Data, Catalog));
        TestTrue(TEXT("L7 Gatling unchanged"), Loadout == Base);

        FHDProfileData Unlocked;
        Unlocked.UnlockedStratagems.Add(OrbitalGatlingBarrage);
        TestTrue(TEXT("L7 slot 0 <- Gatling (unlocked)"), HDArmory::TrySetSlot(Loadout, 0, OrbitalGatlingBarrage, Unlocked, Catalog));
        TestEqual(TEXT("L7 Gatling set"), Loadout[0], OrbitalGatlingBarrage);

        Loadout = Base;
        TestTrue(TEXT("L7 slot 1 <- itself"), HDArmory::TrySetSlot(Loadout, 1, OrbitalPrecisionStrike, Data, Catalog));
        TestTrue(TEXT("L7 itself unchanged"), Loadout == Base);
        TestFalse(TEXT("L7 slot 4"), HDArmory::TrySetSlot(Loadout, 4, Resupply, Data, Catalog));
        TestFalse(TEXT("L7 slot -1"), HDArmory::TrySetSlot(Loadout, -1, Resupply, Data, Catalog));
        TestFalse(TEXT("L7 Foo"), HDArmory::TrySetSlot(Loadout, 0, FName(TEXT("Foo")), Data, Catalog));
        TestTrue(TEXT("L7 rejects unchanged"), Loadout == Base);
    }

    // L8 키 안내
    {
        FHDShipMenu Menu = MakeLoadoutMenu(Context);
        TestEqual(TEXT("L8 card"), Hints(Menu, Items), FString(TEXT("[ARROWS] MOVE   [ENTER] CHANGE   [BACKSPACE] BACK")));
        const TArray<FHDKeyHint> CardHints = HDShipMenuView::GetKeyHints(Menu, Items, false);
        TestTrue(TEXT("L8 ARROWS not clickable"), CardHints.Num() > 0 && CardHints[0].bClickable == false);

        Menu.SetCursor(7, ItemCount);
        TestEqual(TEXT("L8 READY"), Hints(Menu, Items), FString(TEXT("[ARROWS] MOVE   [ENTER] READY   [BACKSPACE] BACK")));

        Menu.SetCursor(0, ItemCount);
        Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        TestEqual(TEXT("L8 picker current"), Hints(Menu, Items), FString(TEXT("[ARROWS] MOVE   [ENTER] KEEP   [BACKSPACE] CLOSE")));
        Menu.SetPickerCursor(4, Items[0].Options.Num());
        TestEqual(TEXT("L8 picker selectable"), Hints(Menu, Items), FString(TEXT("[ARROWS] MOVE   [ENTER] EQUIP   [BACKSPACE] CLOSE")));
        Menu.SetPickerCursor(2, Items[0].Options.Num());
        TestEqual(TEXT("L8 picker locked"), Hints(Menu, Items), FString(TEXT("[ARROWS] MOVE   [BACKSPACE] CLOSE")));
        Menu.SetPickerCursor(1, Items[0].Options.Num());
        TestEqual(TEXT("L8 picker other slot"), Hints(Menu, Items), FString(TEXT("[ARROWS] MOVE   [BACKSPACE] CLOSE")));
        TestEqual(TEXT("L8 launching"), Hints(Menu, Items, true), FString(TEXT("[BACKSPACE] CANCEL")));
    }

    // L9 상세
    {
        FHDShipMenu Menu = MakeLoadoutMenu(Context);
        FHDLoadoutDetail Detail = HDShipMenuView::BuildLoadoutDetail(Menu, Items, Context);
        TestEqual(TEXT("L9 title"), Detail.Title, FString(TEXT("EAGLE AIRSTRIKE")));
        TestEqual(TEXT("L9 category"), Detail.Category, FString(TEXT("EAGLE")));
        TestEqual(TEXT("L9 icon"), Detail.IconId, EagleAirstrike);
        TArray<FString> Lines;
        for (const FHDDetailLine& Line : Detail.Lines)
        {
            Lines.Add(Line.Label + TEXT("=") + Line.Value);
        }
        TestEqual(TEXT("L9 lines"), FString::Join(Lines, TEXT("|")), FString(TEXT("CALL-IN=3 S|COOLDOWN=10 S|USES=2|REARM=120 S")));
        TestTrue(TEXT("L9 command"), Detail.Command == TArray<EHDCommandInput>({ EHDCommandInput::Up, EHDCommandInput::Right, EHDCommandInput::Down, EHDCommandInput::Right }));
        TestEqual(TEXT("L9 status"), Detail.Status, FString(TEXT("EQUIPPED")));

        FHDShipMenuContext RearmContext = Context;
        RearmContext.Profile.UpgradeLevels.Add(FName(TEXT("EagleFastRearm")), 1);
        TestEqual(TEXT("L9 fast rearm"), LineValue(HDShipMenuView::BuildLoadoutDetail(Menu, Items, RearmContext), TEXT("REARM")), FString(TEXT("96 S")));

        FHDShipMenuContext UplinkContext = Context;
        UplinkContext.Profile.UpgradeLevels.Add(FName(TEXT("TargetingUplink")), 1);
        TestEqual(TEXT("L9 uplink"), LineValue(HDShipMenuView::BuildLoadoutDetail(Menu, Items, UplinkContext), TEXT("CALL-IN")), FString(TEXT("2.5 S")));

        Menu.HandleKey(EHDShipMenuKey::Confirm, Items);
        Menu.SetPickerCursor(1, Items[0].Options.Num());
        Detail = HDShipMenuView::BuildLoadoutDetail(Menu, Items, Context);
        TestEqual(TEXT("L9 picker 1 title"), Detail.Title, FString(TEXT("ORBITAL PRECISION STRIKE")));
        TestEqual(TEXT("L9 picker 1 uses"), LineValue(Detail, TEXT("USES")), FString(TEXT("UNLIMITED")));
        TestTrue(TEXT("L9 picker 1 no rearm"), FindLine(Detail, TEXT("REARM")) == nullptr);
        TestEqual(TEXT("L9 picker 1 status"), Detail.Status, FString(TEXT("IN SLOT 2")));

        Menu.SetPickerCursor(2, Items[0].Options.Num());
        TestEqual(TEXT("L9 picker 2 status"), HDShipMenuView::BuildLoadoutDetail(Menu, Items, Context).Status, FString(TEXT("LOCKED  150 REQ IN ARMORY")));

        Menu.SetCursor(4, ItemCount);
        Detail = HDShipMenuView::BuildLoadoutDetail(Menu, Items, Context);
        TestEqual(TEXT("L9 rifle title"), Detail.Title, FString(TEXT("ASSAULT RIFLE")));
        const FHDWeaponOption* Rifle = HDWeaponCatalog::Find(TEXT("AssaultRifle"));
        if (TestNotNull(TEXT("L9 rifle option"), Rifle))
        {
            const TArray<FHDStatBar> Expected = HDWeaponCatalog::MakeStatBars(*Rifle);
            TestEqual(TEXT("L9 rifle bar count"), Detail.Bars.Num(), 4);
            bool bSame = Detail.Bars.Num() == Expected.Num();
            for (int32 Index = 0; bSame && Index < Expected.Num(); ++Index)
            {
                bSame = Detail.Bars[Index].Label == Expected[Index].Label && Detail.Bars[Index].Value == Expected[Index].Value
                    && FMath::IsNearlyEqual(Detail.Bars[Index].Fraction, Expected[Index].Fraction);
            }
            TestTrue(TEXT("L9 rifle bars = MakeStatBars"), bSame);
        }
        TestTrue(TEXT("L9 rifle no command"), Detail.Command.IsEmpty());

        Menu.SetCursor(7, ItemCount);
        Detail = HDShipMenuView::BuildLoadoutDetail(Menu, Items, Context);
        TestEqual(TEXT("L9 READY title"), Detail.Title, FString(TEXT("READY")));
        TestEqual(TEXT("L9 READY lines"), Detail.Lines.Num(), 7);
        TestEqual(TEXT("L9 READY line 1"), JoinLine(Detail, 0), FString(TEXT("SLOT 1  EAGLE AIRSTRIKE")));
        TestEqual(TEXT("L9 READY line 5"), JoinLine(Detail, 4), FString(TEXT("PRIMARY  ASSAULT RIFLE")));
    }

    TestEqual(TEXT("FormatSeconds 10"), HDShipMenuView::FormatSeconds(10.f), FString(TEXT("10 S")));
    TestEqual(TEXT("FormatSeconds 2.5"), HDShipMenuView::FormatSeconds(2.5f), FString(TEXT("2.5 S")));

    return true;
}

#endif
