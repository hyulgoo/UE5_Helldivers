// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Progression/HDArmoryRules.h"
#include "Progression/HDProgressionSettings.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "UI/HDShipMenu.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDArmoryTest, "Helldivers.Progression.Armory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDArmoryTest
{
    const FName EagleAirstrike(TEXT("EagleAirstrike"));
    const FName OrbitalPrecisionStrike(TEXT("OrbitalPrecisionStrike"));
    const FName OrbitalGatlingBarrage(TEXT("OrbitalGatlingBarrage"));
    const FName Orbital120mmBarrage(TEXT("Orbital120mmBarrage"));
    const FName Resupply(TEXT("Resupply"));
    const FName MachineGunSentry(TEXT("MachineGunSentry"));
    const FName AntiMaterielRifle(TEXT("AntiMaterielRifle"));
    const FName NoSuch(TEXT("NoSuch"));

    // UHDStratagemSettings 기본값(Stratagem/HDStratagemSettings.cpp)
    TArray<FName> MakeDefaultLoadout()
    {
        return { EagleAirstrike, Resupply, MachineGunSentry, AntiMaterielRifle };
    }

    FString JoinNames(const TArray<FName>& Names)
    {
        TArray<FString> Parts;
        for (const FName& Name : Names)
        {
            Parts.Add(Name.ToString());
        }
        return FString::Join(Parts, TEXT(","));
    }

    // 다르면 두 목록을 메시지에 함께 남긴다
    bool TestNames(FAutomationTestBase& Test, const TCHAR* Message, const TArray<FName>& Actual, const TArray<FName>& Expected)
    {
        return Test.TestEqual(Message, JoinNames(Actual), JoinNames(Expected));
    }

    FHDProfileData MakeGatlingProfile()
    {
        FHDProfileData Data;
        Data.UnlockedStratagems.Add(OrbitalGatlingBarrage);
        return Data;
    }
}

bool FHDArmoryTest::RunTest(const FString& Parameters)
{
    using namespace HDArmoryTest;

    const FHDStratagemCatalog& Catalog = FHDStratagemCatalog::GetProjectCatalog();
    const TArray<FName> DefaultLoadout = MakeDefaultLoadout();

    // 아래 기대값은 이 행 순서를 전제로 한다
    TestNames(*this, TEXT("Catalog: project file row order"), Catalog.GetNames(),
        { EagleAirstrike, OrbitalPrecisionStrike, OrbitalGatlingBarrage, Orbital120mmBarrage, Resupply, MachineGunSentry, AntiMaterielRifle });

    // A1 해금 가격
    {
        for (const FName& Name : Catalog.GetNames())
        {
            const FHDStratagemDefinition* Definition = Catalog.Find(Name);
            const int32 Expected = Name == OrbitalGatlingBarrage ? 150 : (Name == Orbital120mmBarrage ? 250 : 0);
            TestEqual(*FString::Printf(TEXT("A1: %s unlock cost"), *Name.ToString()), Definition ? Definition->UnlockCost : -1, Expected);
        }

        const FHDProfileData Empty;
        int32 UnlockedCount = 0;
        for (const FName& Name : Catalog.GetNames())
        {
            UnlockedCount += HDArmory::IsUnlocked(Empty, Catalog, Name) ? 1 : 0;
        }
        TestEqual(TEXT("A1: empty profile unlocks 5"), UnlockedCount, 5);

        bool bDefaultUnlocked = true;
        for (const FName& Name : DefaultLoadout)
        {
            bDefaultUnlocked &= HDArmory::IsUnlocked(Empty, Catalog, Name);
        }
        TestTrue(TEXT("A1: default loadout is unlocked"), bDefaultUnlocked);
        TestFalse(TEXT("A1: gatling locked on empty profile"), HDArmory::IsUnlocked(Empty, Catalog, OrbitalGatlingBarrage));
        TestFalse(TEXT("A1: unknown name is not unlocked"), HDArmory::IsUnlocked(Empty, Catalog, NoSuch));
    }

    // A2 해금
    {
        FHDProfileData Poor;
        Poor.Requisition = 100;
        TestTrue(TEXT("A2: 100 req gatling cannot afford"), HDArmory::TryUnlock(Poor, Catalog, OrbitalGatlingBarrage) == EHDUnlockResult::CannotAfford);
        TestEqual(TEXT("A2: cannot afford keeps requisition"), Poor.Requisition, 100);
        TestTrue(TEXT("A2: cannot afford keeps unlock list"), Poor.UnlockedStratagems.IsEmpty());

        FHDProfileData Rich;
        Rich.Requisition = 400;
        TestTrue(TEXT("A2: 400 req gatling unlocked"), HDArmory::TryUnlock(Rich, Catalog, OrbitalGatlingBarrage) == EHDUnlockResult::Unlocked);
        TestEqual(TEXT("A2: requisition 400 -> 250"), Rich.Requisition, 250);
        TestNames(*this, TEXT("A2: unlock list has gatling"), Rich.UnlockedStratagems, { OrbitalGatlingBarrage });
        TestTrue(TEXT("A2: gatling is unlocked after"), HDArmory::IsUnlocked(Rich, Catalog, OrbitalGatlingBarrage));

        TestTrue(TEXT("A2: gatling again already unlocked"), HDArmory::TryUnlock(Rich, Catalog, OrbitalGatlingBarrage) == EHDUnlockResult::AlreadyUnlocked);
        TestTrue(TEXT("A2: free row already unlocked"), HDArmory::TryUnlock(Rich, Catalog, Resupply) == EHDUnlockResult::AlreadyUnlocked);
        TestNames(*this, TEXT("A2: free row not added to list"), Rich.UnlockedStratagems, { OrbitalGatlingBarrage });
        TestEqual(TEXT("A2: already unlocked keeps requisition"), Rich.Requisition, 250);

        TestTrue(TEXT("A2: unknown name"), HDArmory::TryUnlock(Rich, Catalog, NoSuch) == EHDUnlockResult::Unknown);
        TestEqual(TEXT("A2: unknown keeps requisition"), Rich.Requisition, 250);
    }

    // A3 로드아웃 정리
    {
        const FHDProfileData Empty;
        TestNames(*this, TEXT("A3: locked, duplicate and unknown are dropped"),
            HDArmory::ValidateLoadout({ OrbitalGatlingBarrage, Resupply, Resupply, NoSuch }, Empty, Catalog, DefaultLoadout),
            { Resupply, EagleAirstrike, MachineGunSentry, AntiMaterielRifle });

        const FHDProfileData Gatling = MakeGatlingProfile();
        TestNames(*this, TEXT("A3: unlocked gatling is kept and filled"),
            HDArmory::ValidateLoadout({ OrbitalGatlingBarrage, Resupply }, Gatling, Catalog, DefaultLoadout),
            { OrbitalGatlingBarrage, Resupply, EagleAirstrike, MachineGunSentry });

        TestNames(*this, TEXT("A3: empty wanted is the fallback"),
            HDArmory::ValidateLoadout(TArray<FName>(), Empty, Catalog, DefaultLoadout),
            DefaultLoadout);

        TestNames(*this, TEXT("A3: five unlocked names cut to four"),
            HDArmory::ValidateLoadout({ EagleAirstrike, OrbitalPrecisionStrike, Resupply, MachineGunSentry, AntiMaterielRifle }, Empty, Catalog, DefaultLoadout),
            { EagleAirstrike, OrbitalPrecisionStrike, Resupply, MachineGunSentry });
    }

    // A4 칸 돌리기
    {
        const FHDProfileData Empty;
        TestEqual(TEXT("A4: slot 2 +1"), HDArmory::CycleSlot(DefaultLoadout, 1, 1, Empty, Catalog), OrbitalPrecisionStrike);
        TestEqual(TEXT("A4: slot 2 -1"), HDArmory::CycleSlot(DefaultLoadout, 1, -1, Empty, Catalog), OrbitalPrecisionStrike);

        const FHDProfileData Gatling = MakeGatlingProfile();
        TestEqual(TEXT("A4: gatling unlocked slot 2 +1"), HDArmory::CycleSlot(DefaultLoadout, 1, 1, Gatling, Catalog), OrbitalPrecisionStrike);
        TestEqual(TEXT("A4: gatling unlocked slot 2 -1"), HDArmory::CycleSlot(DefaultLoadout, 1, -1, Gatling, Catalog), OrbitalGatlingBarrage);
    }

    // A5 이름 글자
    {
        TestEqual(TEXT("A5: EagleAirstrike"), HDArmory::MakeDisplayName(EagleAirstrike), FString(TEXT("EAGLE AIRSTRIKE")));
        TestEqual(TEXT("A5: Orbital120mmBarrage"), HDArmory::MakeDisplayName(Orbital120mmBarrage), FString(TEXT("ORBITAL 120MM BARRAGE")));
        TestEqual(TEXT("A5: AntiMaterielRifle"), HDArmory::MakeDisplayName(AntiMaterielRifle), FString(TEXT("ANTI MATERIEL RIFLE")));
        TestEqual(TEXT("A5: MachineGunSentry"), HDArmory::MakeDisplayName(MachineGunSentry), FString(TEXT("MACHINE GUN SENTRY")));
        TestEqual(TEXT("A5: command text"), HDArmory::FormatCommand({ EHDCommandInput::Up, EHDCommandInput::Right, EHDCommandInput::Down, EHDCommandInput::Right }), FString(TEXT("UP RIGHT DOWN RIGHT")));
    }

    // A6 열 읽기
    {
        const FHDStratagemCatalog Parsed = FHDStratagemCatalog::FromJsonString(TEXT(R"([ { "Name": "X", "CommandSequence": ["Up"], "UnlockCost": 5 } ])"));
        for (const FString& Problem : Parsed.GetProblems()) { AddInfo(FString::Printf(TEXT("A6 problem: %s"), *Problem)); }
        TestEqual(TEXT("A6: no problems"), Parsed.GetProblems().Num(), 0);
        const FHDStratagemDefinition* X = Parsed.Find(TEXT("X"));
        if (TestNotNull(TEXT("A6: row X"), X))
        {
            TestEqual(TEXT("A6: unlock cost 5"), X->UnlockCost, 5);
        }
    }

    // A7 메뉴
    {
        FHDShipMenuContext Context;
        Context.Difficulties    = GetDefault<UHDProgressionSettings>()->DifficultyLevels;
        Context.Stratagems      = &Catalog;
        Context.DefaultLoadout  = DefaultLoadout;

        const TArray<FHDShipMenuItem> BridgeItems = HDShipMenu::BuildItems(EHDShipScreen::Bridge, Context);
        TArray<FName> BridgeIds;
        for (const FHDShipMenuItem& Item : BridgeItems)
        {
            BridgeIds.Add(Item.Id);
        }
        TestNames(*this, TEXT("A7: Bridge ids"), BridgeIds,
            { TEXT("Deploy"), TEXT("Difficulty"), TEXT("Loadout"), TEXT("Upgrades"), TEXT("Armory"), TEXT("Quit") });
        if (BridgeItems.Num() == 6)
        {
            TestTrue(TEXT("A7: Loadout row opens Loadout"), BridgeItems[2].ConfirmAction == EHDShipActionType::OpenScreen && BridgeItems[2].TargetScreen == EHDShipScreen::Loadout);
            TestTrue(TEXT("A7: Armory row opens Armory"), BridgeItems[4].ConfirmAction == EHDShipActionType::OpenScreen && BridgeItems[4].TargetScreen == EHDShipScreen::Armory);
        }

        const TArray<FHDShipMenuItem> LoadoutItems = HDShipMenu::BuildItems(EHDShipScreen::Loadout, Context);
        if (TestTrue(TEXT("A7: Loadout has at least 4 rows"), LoadoutItems.Num() >= 4))
        {
            bool bSlotIds = true;
            bool bAllAdjustable = true;
            for (int32 Index = 0; Index < 4; ++Index)
            {
                bSlotIds &= LoadoutItems[Index].Id == FName(*FString::Printf(TEXT("Slot%d"), Index + 1));
                bAllAdjustable &= LoadoutItems[Index].bAdjustable;
            }
            TestTrue(TEXT("A7: first 4 ids are Slot1..Slot4"), bSlotIds);
            TestTrue(TEXT("A7: slot rows are adjustable"), bAllAdjustable);
            TestEqual(TEXT("A7: first slot label"), LoadoutItems[0].Label, FString(TEXT("SLOT 1  < EAGLE AIRSTRIKE >")));
        }

        const TArray<FHDShipMenuItem> PoorArmory = HDShipMenu::BuildItems(EHDShipScreen::Armory, Context);
        if (TestTrue(TEXT("A7: Armory has at least 2 rows"), PoorArmory.Num() >= 2))
        {
            TestEqual(TEXT("A7: armory row 1 id"), PoorArmory[0].Id, OrbitalGatlingBarrage);
            TestEqual(TEXT("A7: armory row 2 id"), PoorArmory[1].Id, Orbital120mmBarrage);
            TestTrue(TEXT("A7: 0 requisition disables both"), PoorArmory[0].bEnabled == false && PoorArmory[1].bEnabled == false);
            TestTrue(TEXT("A7: armory rows confirm Select"), PoorArmory[0].ConfirmAction == EHDShipActionType::Select);
        }

        FHDShipMenuContext RichContext = Context;
        RichContext.Profile = MakeGatlingProfile();
        RichContext.Profile.Requisition = 1000;
        const TArray<FHDShipMenuItem> RichArmory = HDShipMenu::BuildItems(EHDShipScreen::Armory, RichContext);
        if (TestTrue(TEXT("A7: rich Armory has at least 2 rows"), RichArmory.Num() >= 2))
        {
            TestTrue(TEXT("A7: unlocked row label ends with UNLOCKED"), RichArmory[0].Label.EndsWith(TEXT("UNLOCKED")));
            TestTrue(TEXT("A7: unlocked row disabled"), RichArmory[0].bEnabled == false);
            TestTrue(TEXT("A7: affordable locked row enabled"), RichArmory[1].bEnabled);
        }
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
