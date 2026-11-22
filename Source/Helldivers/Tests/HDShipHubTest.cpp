// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Ship/HDShipHubRules.h"
#include "UI/HDShipMenu.h"
#include "Progression/HDProgressionSettings.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Stratagem/HDStratagemSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDShipHubTest, "Helldivers.FrontEnd.ShipHub", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDShipHubTest
{
    FHDShipMenuItem MakeItem(const TCHAR* Id, EHDShipActionType ConfirmAction, EHDShipScreen TargetScreen = EHDShipScreen::Bridge)
    {
        FHDShipMenuItem Item;
        Item.Id             = FName(Id);
        Item.Label          = Id;
        Item.ConfirmAction  = ConfirmAction;
        Item.TargetScreen   = TargetScreen;
        return Item;
    }

    FString JoinIds(const TArray<FHDShipMenuItem>& Items)
    {
        TArray<FString> Parts;
        for (const FHDShipMenuItem& Item : Items)
        {
            Parts.Add(Item.Id.ToString());
        }
        return FString::Join(Parts, TEXT(","));
    }

    bool IsAction(const FHDShipMenuAction& Action, EHDShipActionType Type, const TCHAR* Id = nullptr)
    {
        if (Action.Type != Type)
        {
            return false;
        }

        return Id == nullptr || Action.Id == FName(Id);
    }

    FHDShipTerminalInfo MakeFakeTerminal(const TCHAR* Id, const FVector& LocalLocation)
    {
        FHDShipTerminalInfo Terminal;
        Terminal.Id             = FName(Id);
        Terminal.Label          = Id;
        Terminal.LocalLocation  = LocalLocation;
        return Terminal;
    }
}

bool FHDShipHubTest::RunTest(const FString& Parameters)
{
    using namespace HDShipHubTest;

    const float Range = HDShipHub::DefaultInteractRange;
    const TArray<FHDShipTerminalInfo>& Terminals = HDShipHub::GetTerminals();
    const bool bHasThreeTerminals = Terminals.Num() == 3;

    // H1 단말기 표
    {
        TestEqual(TEXT("H1: three terminals"), Terminals.Num(), 3);
        TestEqual(TEXT("H1: interact range 300"), Range, 300.f);

        if (bHasThreeTerminals)
        {
            TestEqual(TEXT("H1: id 0 WarTable"), Terminals[0].Id, FName(TEXT("WarTable")));
            TestEqual(TEXT("H1: id 1 ShipManagement"), Terminals[1].Id, FName(TEXT("ShipManagement")));
            TestEqual(TEXT("H1: id 2 Armory"), Terminals[2].Id, FName(TEXT("Armory")));

            TestEqual(TEXT("H1: label 0"), Terminals[0].Label, FString(TEXT("WAR TABLE")));
            TestEqual(TEXT("H1: label 1"), Terminals[1].Label, FString(TEXT("SHIP MANAGEMENT")));
            TestEqual(TEXT("H1: label 2"), Terminals[2].Label, FString(TEXT("ARMORY")));

            TestTrue(TEXT("H1: root 0 WarTable"), Terminals[0].RootScreen == EHDShipScreen::WarTable);
            TestTrue(TEXT("H1: root 1 Upgrades"), Terminals[1].RootScreen == EHDShipScreen::Upgrades);
            TestTrue(TEXT("H1: root 2 Armory"), Terminals[2].RootScreen == EHDShipScreen::Armory);

            TestTrue(TEXT("H1: terminal enums"), Terminals[0].Terminal == EHDShipTerminal::WarTable
                && Terminals[1].Terminal == EHDShipTerminal::ShipManagement
                && Terminals[2].Terminal == EHDShipTerminal::Armory);

            TestTrue(TEXT("H1: FindTerminal ShipManagement"), HDShipHub::FindTerminal(FName(TEXT("ShipManagement"))) == &Terminals[1]);
            TestTrue(TEXT("H1: FindTerminal unknown"), HDShipHub::FindTerminal(FName(TEXT("NoSuch"))) == nullptr);
        }

        TSet<FName> Ids;
        for (const FHDShipTerminalInfo& Terminal : Terminals)
        {
            Ids.Add(Terminal.Id);
        }
        TestEqual(TEXT("H1: ids are distinct"), Ids.Num(), Terminals.Num());

        // 두 단말기 범위가 겹치지 않는다
        bool bApart = true;
        for (int32 First = 0; First < Terminals.Num(); ++First)
        {
            for (int32 Second = First + 1; Second < Terminals.Num(); ++Second)
            {
                bApart &= FVector::Dist2D(Terminals[First].LocalLocation, Terminals[Second].LocalLocation) > 2.f * Range;
            }
        }
        TestTrue(TEXT("H1: terminals are more than 2 x range apart"), bApart);

        const FVector Spawn = HDShipHub::GetCrewSpawnLocation();
        TestTrue(TEXT("H1: crew spawn (-1500, 0, 100)"), Spawn.Equals(FVector(-1500.f, 0.f, 100.f)));
        TestEqual(TEXT("H1: crew spawn yaw 0"), HDShipHub::GetCrewSpawnYaw(), 0.f);

        bool bSpawnOutside = true;
        for (const FHDShipTerminalInfo& Terminal : Terminals)
        {
            bSpawnOutside &= FVector::Dist2D(Spawn, Terminal.LocalLocation) > Range;
        }
        TestTrue(TEXT("H1: crew spawn is outside every terminal range"), bSpawnOutside);

        const FVector HalfExtent = HDShipHub::GetRoomHalfExtent();
        TestTrue(TEXT("H1: room half extent (2000, 1200, 700)"), HalfExtent.Equals(FVector(2000.f, 1200.f, 700.f)));

        bool bInside = true;
        for (const FHDShipTerminalInfo& Terminal : Terminals)
        {
            bInside &= FMath::Abs(Terminal.LocalLocation.X) < HalfExtent.X - 100.f;
            bInside &= FMath::Abs(Terminal.LocalLocation.Y) < HalfExtent.Y - 100.f;
        }
        TestTrue(TEXT("H1: terminals are inside the room"), bInside);
    }

    // H2 범위
    {
        if (bHasThreeTerminals)
        {
            const FVector WarTable = Terminals[0].LocalLocation;
            const FVector ShipManagement = Terminals[1].LocalLocation;

            TestEqual(TEXT("H2: war table + (200, 0, 0) -> 0"), HDShipHub::FindTerminalInRange(WarTable + FVector(200.f, 0.f, 0.f), Terminals, Range), 0);
            TestEqual(TEXT("H2: ship management + (0, 250, 0) -> 1"), HDShipHub::FindTerminalInRange(ShipManagement + FVector(0.f, 250.f, 0.f), Terminals, Range), 1);
            TestEqual(TEXT("H2: war table + (0, 0, 500) -> 0 (Z ignored)"), HDShipHub::FindTerminalInRange(WarTable + FVector(0.f, 0.f, 500.f), Terminals, Range), 0);
            TestEqual(TEXT("H2: range 0 -> none"), HDShipHub::FindTerminalInRange(WarTable, Terminals, 0.f), static_cast<int32>(INDEX_NONE));
        }

        TestEqual(TEXT("H2: crew spawn -> none"), HDShipHub::FindTerminalInRange(HDShipHub::GetCrewSpawnLocation(), Terminals, Range), static_cast<int32>(INDEX_NONE));

        const TArray<FHDShipTerminalInfo> Fake = { MakeFakeTerminal(TEXT("A"), FVector(0.f, 0.f, 0.f)), MakeFakeTerminal(TEXT("B"), FVector(400.f, 0.f, 0.f)) };
        TestEqual(TEXT("H2: fake (150, 0, 0) -> 0"), HDShipHub::FindTerminalInRange(FVector(150.f, 0.f, 0.f), Fake, Range), 0);
        TestEqual(TEXT("H2: fake (250, 0, 0) -> 1 (nearer)"), HDShipHub::FindTerminalInRange(FVector(250.f, 0.f, 0.f), Fake, Range), 1);
        TestEqual(TEXT("H2: empty table -> none"), HDShipHub::FindTerminalInRange(FVector::ZeroVector, TArray<FHDShipTerminalInfo>(), Range), static_cast<int32>(INDEX_NONE));
    }

    // H3 키·글자
    {
        TestTrue(TEXT("H3: E pressed is interact"), HDShipHub::IsInteractKey(EKeys::E, IE_Pressed));
        TestFalse(TEXT("H3: E released is not interact"), HDShipHub::IsInteractKey(EKeys::E, IE_Released));
        TestFalse(TEXT("H3: F pressed is not interact"), HDShipHub::IsInteractKey(EKeys::F, IE_Pressed));

        if (bHasThreeTerminals)
        {
            TestEqual(TEXT("H3: prompt"), HDShipHub::FormatInteractPrompt(Terminals[0]), FString(TEXT("[E] WAR TABLE")));
        }

        TestEqual(TEXT("H3: countdown 3.2"), HDShipHub::FormatCountdown(3.2f), FString(TEXT("HELLPOD LAUNCH IN 4")));
        TestEqual(TEXT("H3: countdown 0"), HDShipHub::FormatCountdown(0.f), FString(TEXT("HELLPOD LAUNCH IN 0")));
    }

    // H4 카운트다운
    {
        FHDLaunchCountdown Countdown;
        TestFalse(TEXT("H4: new is not active"), Countdown.IsActive());
        TestEqual(TEXT("H4: new remaining -1"), Countdown.GetRemaining(), -1.f);
        TestEqual(TEXT("H4: new display 0"), Countdown.GetDisplaySeconds(), 0);

        TestTrue(TEXT("H4: Start(5) accepted"), Countdown.Start(5.f));
        TestTrue(TEXT("H4: active after Start"), Countdown.IsActive());
        TestEqual(TEXT("H4: remaining 5"), Countdown.GetRemaining(), 5.f);
        TestEqual(TEXT("H4: display 5"), Countdown.GetDisplaySeconds(), 5);

        TestFalse(TEXT("H4: second Start refused"), Countdown.Start(5.f));
        TestEqual(TEXT("H4: remaining still 5"), Countdown.GetRemaining(), 5.f);

        TestFalse(TEXT("H4: Tick(1.2) does not fire"), Countdown.Tick(1.2f));
        TestEqual(TEXT("H4: display 4 after 1.2s"), Countdown.GetDisplaySeconds(), 4);

        TestTrue(TEXT("H4: Cancel while active"), Countdown.Cancel());
        TestFalse(TEXT("H4: off after Cancel"), Countdown.IsActive());
        TestEqual(TEXT("H4: remaining -1 after Cancel"), Countdown.GetRemaining(), -1.f);
        TestFalse(TEXT("H4: second Cancel false"), Countdown.Cancel());

        TestTrue(TEXT("H4: Start(5) again"), Countdown.Start(5.f));
        TestFalse(TEXT("H4: Tick(4.9) does not fire"), Countdown.Tick(4.9f));
        TestTrue(TEXT("H4: Tick(0.2) fires"), Countdown.Tick(0.2f));
        TestFalse(TEXT("H4: off after fire"), Countdown.IsActive());
        TestFalse(TEXT("H4: Tick(1) after fire does not fire again"), Countdown.Tick(1.f));

        TestTrue(TEXT("H4: Start(-3) accepted"), Countdown.Start(-3.f));
        TestEqual(TEXT("H4: Start(-3) clamps to 0"), Countdown.GetRemaining(), 0.f);
        TestTrue(TEXT("H4: Tick(0) fires at 0"), Countdown.Tick(0.f));
    }

    // H5 메뉴 뿌리
    {
        const TArray<FHDShipMenuItem> FakeItems = { MakeItem(TEXT("A"), EHDShipActionType::Select), MakeItem(TEXT("B"), EHDShipActionType::OpenScreen, EHDShipScreen::Loadout) };

        FHDShipMenu Menu;
        Menu.OpenTerminal(EHDShipScreen::WarTable);
        TestTrue(TEXT("H5: screen WarTable"), Menu.GetScreen() == EHDShipScreen::WarTable);
        TestEqual(TEXT("H5: cursor 0"), Menu.GetCursor(), 0);
        TestTrue(TEXT("H5: root WarTable"), Menu.GetRootScreen() == EHDShipScreen::WarTable);

        Menu.HandleKey(EHDShipMenuKey::Down, FakeItems);
        TestEqual(TEXT("H5: Down -> cursor 1"), Menu.GetCursor(), 1);

        TestTrue(TEXT("H5: Confirm B opens screen"), IsAction(Menu.HandleKey(EHDShipMenuKey::Confirm, FakeItems), EHDShipActionType::OpenScreen, TEXT("B")));
        TestTrue(TEXT("H5: screen Loadout"), Menu.GetScreen() == EHDShipScreen::Loadout);
        TestEqual(TEXT("H5: cursor 0 on Loadout"), Menu.GetCursor(), 0);

        TestTrue(TEXT("H5: Back from Loadout opens WarTable"), IsAction(Menu.HandleKey(EHDShipMenuKey::Back, FakeItems), EHDShipActionType::OpenScreen, TEXT("WarTable")));
        TestTrue(TEXT("H5: screen back to WarTable"), Menu.GetScreen() == EHDShipScreen::WarTable);
        TestEqual(TEXT("H5: cursor back to 1"), Menu.GetCursor(), 1);

        TestTrue(TEXT("H5: Back at root closes terminal"), IsAction(Menu.HandleKey(EHDShipMenuKey::Back, FakeItems), EHDShipActionType::CloseTerminal, TEXT("WarTable")));
        TestTrue(TEXT("H5: screen stays WarTable after close"), Menu.GetScreen() == EHDShipScreen::WarTable);

        Menu.OpenTerminal(EHDShipScreen::Upgrades);
        TestTrue(TEXT("H5: root Upgrades"), Menu.GetRootScreen() == EHDShipScreen::Upgrades);
        TestEqual(TEXT("H5: cursor 0 on Upgrades terminal"), Menu.GetCursor(), 0);
        TestTrue(TEXT("H5: Back at Upgrades root closes terminal"), IsAction(Menu.HandleKey(EHDShipMenuKey::Back, FakeItems), EHDShipActionType::CloseTerminal, TEXT("Upgrades")));

        // 옛 동작: 타이틀 → Bridge, Bridge 에서 Back 은 None(M6 과 같다)
        FHDShipMenu TitleMenu;
        TestTrue(TEXT("H5: Title Confirm opens Bridge"), IsAction(TitleMenu.HandleKey(EHDShipMenuKey::Confirm, TArray<FHDShipMenuItem>()), EHDShipActionType::OpenScreen, TEXT("Bridge")));
        TestTrue(TEXT("H5: Back on Bridge root is None"), TitleMenu.HandleKey(EHDShipMenuKey::Back, FakeItems).Type == EHDShipActionType::None);
        TestTrue(TEXT("H5: still Bridge"), TitleMenu.GetScreen() == EHDShipScreen::Bridge);
    }

    // H6 줄
    {
        FHDShipMenuContext WarContext;
        WarContext.Profile.Difficulty   = 2;
        WarContext.Difficulties         = GetDefault<UHDProgressionSettings>()->DifficultyLevels;

        const TArray<FHDShipMenuItem> WarItems = HDShipMenu::BuildItems(EHDShipScreen::WarTable, WarContext);
        TestEqual(TEXT("H6: WarTable ids"), JoinIds(WarItems), FString(TEXT("Difficulty,StartMission")));
        if (WarItems.Num() == 2)
        {
            TestEqual(TEXT("H6: difficulty label"), WarItems[0].Label, FString(TEXT("DIFFICULTY  < 2  NORMAL >")));
            TestTrue(TEXT("H6: difficulty adjustable"), WarItems[0].bAdjustable);
            TestEqual(TEXT("H6: start mission label"), WarItems[1].Label, FString(TEXT("START MISSION")));
            TestTrue(TEXT("H6: start mission opens Loadout"), WarItems[1].ConfirmAction == EHDShipActionType::OpenScreen && WarItems[1].TargetScreen == EHDShipScreen::Loadout);
        }

        FHDShipMenuContext NoTableContext;
        NoTableContext.Profile.Difficulty = 2;
        TestEqual(TEXT("H6: WarTable without difficulty table"), JoinIds(HDShipMenu::BuildItems(EHDShipScreen::WarTable, NoTableContext)), FString(TEXT("StartMission")));

        // 기본 문맥 = 새 프로필 + 프로젝트 카탈로그 + 설정 기본 로드아웃(컨트롤러가 만드는 것과 같은 모양)
        FHDShipMenuContext LoadoutContext;
        LoadoutContext.Stratagems       = &FHDStratagemCatalog::GetProjectCatalog();
        LoadoutContext.DefaultLoadout   = GetDefault<UHDStratagemSettings>()->Loadout;

        const TArray<FHDShipMenuItem> LoadoutItems = HDShipMenu::BuildItems(EHDShipScreen::Loadout, LoadoutContext);
        if (TestTrue(TEXT("H6: Loadout has 4 slots and READY"), LoadoutItems.Num() >= 5))
        {
            bool bSlotIds = true;
            for (int32 Index = 0; Index < 4; ++Index)
            {
                bSlotIds &= LoadoutItems[Index].Id == FName(*FString::Printf(TEXT("Slot%d"), Index + 1));
            }
            TestTrue(TEXT("H6: first 4 ids are Slot1..Slot4"), bSlotIds);

            const FHDShipMenuItem& ReadyItem = LoadoutItems.Last();
            TestEqual(TEXT("H6: last id Ready"), ReadyItem.Id, FName(TEXT("Ready")));
            TestEqual(TEXT("H6: last label READY"), ReadyItem.Label, FString(TEXT("READY")));
            TestTrue(TEXT("H6: READY confirms Launch"), ReadyItem.ConfirmAction == EHDShipActionType::Launch);
        }

        const TArray<FHDShipMenuItem> SystemItems = HDShipMenu::BuildItems(EHDShipScreen::System, FHDShipMenuContext());
        TestEqual(TEXT("H6: System ids"), JoinIds(SystemItems), FString(TEXT("Resume,Quit")));
        if (SystemItems.Num() == 2)
        {
            TestTrue(TEXT("H6: RESUME closes terminal"), SystemItems[0].ConfirmAction == EHDShipActionType::CloseTerminal);
            TestTrue(TEXT("H6: QUIT GAME quits"), SystemItems[1].ConfirmAction == EHDShipActionType::Quit);
        }

        TestEqual(TEXT("H6: screen name WarTable"), FString(HDShipMenu::GetScreenName(EHDShipScreen::WarTable)), FString(TEXT("WarTable")));
        TestEqual(TEXT("H6: screen name System"), FString(HDShipMenu::GetScreenName(EHDShipScreen::System)), FString(TEXT("System")));
        TestEqual(TEXT("H6: screen title WarTable"), FString(HDShipMenu::GetScreenTitle(EHDShipScreen::WarTable)), FString(TEXT("WAR TABLE")));
        TestEqual(TEXT("H6: screen title System"), FString(HDShipMenu::GetScreenTitle(EHDShipScreen::System)), FString(TEXT("SHIP MENU")));
        TestEqual(TEXT("H6: action name close"), FString(HDShipMenu::GetActionName(EHDShipActionType::CloseTerminal)), FString(TEXT("close")));
        TestEqual(TEXT("H6: action name launch"), FString(HDShipMenu::GetActionName(EHDShipActionType::Launch)), FString(TEXT("launch")));
    }

    // H7 Bridge 그대로
    {
        FHDShipMenuContext BridgeContext;
        BridgeContext.Difficulties      = GetDefault<UHDProgressionSettings>()->DifficultyLevels;
        BridgeContext.Stratagems        = &FHDStratagemCatalog::GetProjectCatalog();
        BridgeContext.DefaultLoadout    = GetDefault<UHDStratagemSettings>()->Loadout;

        TestEqual(TEXT("H7: Bridge ids"), JoinIds(HDShipMenu::BuildItems(EHDShipScreen::Bridge, BridgeContext)), FString(TEXT("Deploy,Difficulty,Loadout,Upgrades,Armory,Quit")));
        TestTrue(TEXT("H7: Title items empty"), HDShipMenu::BuildItems(EHDShipScreen::Title, BridgeContext).IsEmpty());
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
