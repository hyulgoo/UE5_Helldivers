// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HDShipMenu.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDShipMenuTest, "Helldivers.FrontEnd.ShipMenu", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDShipMenuTest
{
    FHDShipMenuItem MakeItem(const TCHAR* Id, EHDShipActionType ConfirmAction, bool bEnabled = true, bool bAdjustable = false)
    {
        FHDShipMenuItem Item;
        Item.Id = FName(Id);
        Item.Label = Id;
        Item.bEnabled = bEnabled;
        Item.bAdjustable = bAdjustable;
        Item.ConfirmAction = ConfirmAction;
        return Item;
    }

    // A(Select) · B(Select, 꺼짐) · C(Deploy)
    TArray<FHDShipMenuItem> MakeItems3()
    {
        return {
            MakeItem(TEXT("A"), EHDShipActionType::Select),
            MakeItem(TEXT("B"), EHDShipActionType::Select, false),
            MakeItem(TEXT("C"), EHDShipActionType::Deploy),
        };
    }

    // bAdjustable 참인 한 줄
    TArray<FHDShipMenuItem> MakeAdj()
    {
        return { MakeItem(TEXT("Adj"), EHDShipActionType::Select, true, true) };
    }

    bool IsAction(const FHDShipMenuAction& Action, EHDShipActionType Type, const TCHAR* Id = nullptr, int32 Value = 0)
    {
        if (Action.Type != Type || Action.Value != Value)
        {
            return false;
        }

        return Id == nullptr || Action.Id == FName(Id);
    }

    bool IsNone(const FHDShipMenuAction& Action)
    {
        return Action.Type == EHDShipActionType::None;
    }
}

bool FHDShipMenuTest::RunTest(const FString& Parameters)
{
    using namespace HDShipMenuTest;

    // M1 키
    {
        struct FKeyCase
        {
            const TCHAR*        Message;
            FKey                Key;
            EHDShipMenuKey      Expected;
        };

        const FKeyCase KeyCases[] =
        {
            { TEXT("M1: Up pressed is Up"),             EKeys::Up,          EHDShipMenuKey::Up },
            { TEXT("M1: Down pressed is Down"),         EKeys::Down,        EHDShipMenuKey::Down },
            { TEXT("M1: Left pressed is Left"),         EKeys::Left,        EHDShipMenuKey::Left },
            { TEXT("M1: Right pressed is Right"),       EKeys::Right,       EHDShipMenuKey::Right },
            { TEXT("M1: Enter pressed is Confirm"),     EKeys::Enter,       EHDShipMenuKey::Confirm },
            { TEXT("M1: BackSpace pressed is Back"),    EKeys::BackSpace,   EHDShipMenuKey::Back },
            { TEXT("M1: Escape pressed is Back"),       EKeys::Escape,      EHDShipMenuKey::Back },
        };

        for (const FKeyCase& KeyCase : KeyCases)
        {
            // 기대값과 다른 값에서 시작해 실제로 채웠는지 본다
            EHDShipMenuKey MenuKey = KeyCase.Expected == EHDShipMenuKey::Up ? EHDShipMenuKey::Back : EHDShipMenuKey::Up;
            const bool bResult = HDShipMenu::TryGetMenuKey(KeyCase.Key, IE_Pressed, MenuKey);
            TestTrue(KeyCase.Message, bResult && MenuKey == KeyCase.Expected);
        }

        EHDShipMenuKey MenuKey = EHDShipMenuKey::Up;
        TestFalse(TEXT("M1: Enter released ignored"), HDShipMenu::TryGetMenuKey(EKeys::Enter, IE_Released, MenuKey));
        TestFalse(TEXT("M1: W ignored"), HDShipMenu::TryGetMenuKey(EKeys::W, IE_Pressed, MenuKey));
        TestFalse(TEXT("M1: SpaceBar ignored"), HDShipMenu::TryGetMenuKey(EKeys::SpaceBar, IE_Pressed, MenuKey));
        TestFalse(TEXT("M1: P ignored"), HDShipMenu::TryGetMenuKey(EKeys::P, IE_Pressed, MenuKey));
    }

    // M2 타이틀
    {
        FHDShipMenu Menu;
        TestTrue(TEXT("M2: starts at Title"), Menu.GetScreen() == EHDShipScreen::Title);

        const TArray<FHDShipMenuItem> TitleItems = HDShipMenu::BuildItems(EHDShipScreen::Title, FHDShipMenuContext());

        TestTrue(TEXT("M2: Down on Title is None"), IsNone(Menu.HandleKey(EHDShipMenuKey::Down, TitleItems)));
        TestTrue(TEXT("M2: still Title after Down"), Menu.GetScreen() == EHDShipScreen::Title);
        TestTrue(TEXT("M2: Back on Title is None"), IsNone(Menu.HandleKey(EHDShipMenuKey::Back, TitleItems)));
        TestTrue(TEXT("M2: still Title after Back"), Menu.GetScreen() == EHDShipScreen::Title);

        const FHDShipMenuAction Action = Menu.HandleKey(EHDShipMenuKey::Confirm, TitleItems);
        TestTrue(TEXT("M2: Confirm on Title opens Bridge"), IsAction(Action, EHDShipActionType::OpenScreen, TEXT("Bridge")));
        TestTrue(TEXT("M2: screen is Bridge after Confirm"), Menu.GetScreen() == EHDShipScreen::Bridge);
        TestEqual(TEXT("M2: cursor 0 after Confirm"), Menu.GetCursor(), 0);
    }

    // M3 커서
    {
        FHDShipMenu Menu(EHDShipScreen::Bridge);
        const TArray<FHDShipMenuItem> Items3 = MakeItems3();

        bool bAllNone = true;
        bAllNone &= IsNone(Menu.HandleKey(EHDShipMenuKey::Down, Items3));
        TestEqual(TEXT("M3: Down -> 1"), Menu.GetCursor(), 1);
        bAllNone &= IsNone(Menu.HandleKey(EHDShipMenuKey::Down, Items3));
        TestEqual(TEXT("M3: Down -> 2"), Menu.GetCursor(), 2);
        bAllNone &= IsNone(Menu.HandleKey(EHDShipMenuKey::Down, Items3));
        TestEqual(TEXT("M3: Down -> 0 (wraps)"), Menu.GetCursor(), 0);
        bAllNone &= IsNone(Menu.HandleKey(EHDShipMenuKey::Up, Items3));
        TestEqual(TEXT("M3: Up -> 2 (wraps)"), Menu.GetCursor(), 2);
        TestTrue(TEXT("M3: cursor moves are None"), bAllNone);
    }

    // M4 확정
    {
        FHDShipMenu Menu(EHDShipScreen::Bridge);
        const TArray<FHDShipMenuItem> Items3 = MakeItems3();

        Menu.SetCursor(0, Items3.Num());
        TestTrue(TEXT("M4: cursor 0 confirm is Select A"), IsAction(Menu.HandleKey(EHDShipMenuKey::Confirm, Items3), EHDShipActionType::Select, TEXT("A")));

        Menu.SetCursor(1, Items3.Num());
        TestEqual(TEXT("M4: cursor set to 1"), Menu.GetCursor(), 1);
        TestTrue(TEXT("M4: disabled row confirm is None"), IsNone(Menu.HandleKey(EHDShipMenuKey::Confirm, Items3)));

        Menu.SetCursor(2, Items3.Num());
        TestEqual(TEXT("M4: cursor set to 2"), Menu.GetCursor(), 2);
        TestTrue(TEXT("M4: cursor 2 confirm is Deploy C"), IsAction(Menu.HandleKey(EHDShipMenuKey::Confirm, Items3), EHDShipActionType::Deploy, TEXT("C")));
        TestTrue(TEXT("M4: screen stays Bridge"), Menu.GetScreen() == EHDShipScreen::Bridge);
    }

    // M5 줄이 줄어듦
    {
        FHDShipMenu Menu(EHDShipScreen::Bridge);
        const TArray<FHDShipMenuItem> Items3 = MakeItems3();
        Menu.SetCursor(2, Items3.Num());

        // A(Select) · C(Deploy): 커서 2 가 1 로 잘리면 C 를 확정한다
        const TArray<FHDShipMenuItem> Items2 = { Items3[0], Items3[2] };
        const FHDShipMenuAction Action = Menu.HandleKey(EHDShipMenuKey::Confirm, Items2);
        TestEqual(TEXT("M5: cursor clamped to 1"), Menu.GetCursor(), 1);
        TestTrue(TEXT("M5: confirm uses the clamped row"), IsAction(Action, EHDShipActionType::Deploy, TEXT("C")));
    }

    // M6 좌우
    {
        FHDShipMenu Menu(EHDShipScreen::Bridge);
        const TArray<FHDShipMenuItem> Items3 = MakeItems3();

        TestTrue(TEXT("M6: Left on fixed row is None"), IsNone(Menu.HandleKey(EHDShipMenuKey::Left, Items3)));
        TestTrue(TEXT("M6: Right on fixed row is None"), IsNone(Menu.HandleKey(EHDShipMenuKey::Right, Items3)));

        const TArray<FHDShipMenuItem> Adj = MakeAdj();
        TestTrue(TEXT("M6: Right on adjustable row is Adjust +1"), IsAction(Menu.HandleKey(EHDShipMenuKey::Right, Adj), EHDShipActionType::Adjust, TEXT("Adj"), 1));
        TestTrue(TEXT("M6: Left on adjustable row is Adjust -1"), IsAction(Menu.HandleKey(EHDShipMenuKey::Left, Adj), EHDShipActionType::Adjust, TEXT("Adj"), -1));

        TestTrue(TEXT("M6: Back on Bridge is None"), IsNone(Menu.HandleKey(EHDShipMenuKey::Back, Items3)));
        TestTrue(TEXT("M6: screen stays Bridge after Back"), Menu.GetScreen() == EHDShipScreen::Bridge);
    }

    // M7 함선 줄
    {
        const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Bridge, FHDShipMenuContext());
        TestTrue(TEXT("M7: Bridge items not empty"), Items.Num() > 0);

        if (Items.Num() > 0)
        {
            TestEqual(TEXT("M7: first id is Deploy"), Items[0].Id, FName(TEXT("Deploy")));
            TestTrue(TEXT("M7: first confirms Deploy"), Items[0].ConfirmAction == EHDShipActionType::Deploy);
            TestEqual(TEXT("M7: last id is Quit"), Items.Last().Id, FName(TEXT("Quit")));
            TestTrue(TEXT("M7: last confirms Quit"), Items.Last().ConfirmAction == EHDShipActionType::Quit);

            TSet<FName> Ids;
            bool bAllEnabled = true;
            for (const FHDShipMenuItem& Item : Items)
            {
                Ids.Add(Item.Id);
                bAllEnabled &= Item.bEnabled;
            }
            TestEqual(TEXT("M7: ids are distinct"), Ids.Num(), Items.Num());
            TestTrue(TEXT("M7: all enabled"), bAllEnabled);
        }

        TestTrue(TEXT("M7: Title items empty"), HDShipMenu::BuildItems(EHDShipScreen::Title, FHDShipMenuContext()).IsEmpty());
    }

    // M8 자원 줄
    {
        FHDProfileData Profile;
        Profile.Requisition = 500;
        Profile.Samples.Common = 10;
        Profile.Samples.Rare = 5;
        Profile.Samples.Super = 2;
        TestEqual(TEXT("M8: resources line"), HDShipMenu::FormatResources(Profile), FString(TEXT("REQ 500   COMMON 10   RARE 5   SUPER 2")));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
