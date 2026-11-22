// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Progression/HDWeaponCatalog.h"
#include "Progression/HDProgressionRules.h"
#include "Progression/HDProfileSaveGame.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Stratagem/HDStratagemSettings.h"
#include "UI/HDShipMenu.h"
#include "Weapon/HDWeaponSlots.h"
#include "Weapon/HDShotgun.h"
#include "Weapon/HDSecondaryWeapon.h"
#include "Weapon/HDMachinePistol.h"
#include "Weapon/Projectile/HDProjectileGrenade.h"
#include "Weapon/Projectile/HDProjectileImpactGrenade.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDWeaponCatalogTest, "Helldivers.Progression.Weapons", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDWeaponCatalogTest
{
    const FName AssaultRifle(TEXT("AssaultRifle"));
    const FName Shotgun(TEXT("Shotgun"));
    const FName Pistol(TEXT("Pistol"));
    const FName MachinePistol(TEXT("MachinePistol"));
    const FName FragGrenade(TEXT("FragGrenade"));
    const FName ImpactGrenade(TEXT("ImpactGrenade"));
    const FName NoSuch(TEXT("NoSuch"));
    const FName Resupply(TEXT("Resupply"));

    FString JoinNames(const TArray<FName>& Names)
    {
        TArray<FString> Parts;
        for (const FName& Name : Names)
        {
            Parts.Add(Name.ToString());
        }
        return FString::Join(Parts, TEXT(","));
    }

    FString JoinIds(const TArray<FHDShipMenuItem>& Items)
    {
        TArray<FName> Ids;
        for (const FHDShipMenuItem& Item : Items)
        {
            Ids.Add(Item.Id);
        }
        return JoinNames(Ids);
    }

    const FHDShipMenuItem* FindItem(const TArray<FHDShipMenuItem>& Items, FName Id)
    {
        return Items.FindByPredicate([Id](const FHDShipMenuItem& Item) { return Item.Id == Id; });
    }

    const TCHAR* GetSlotText(EHDLoadoutWeapon Slot)
    {
        switch (Slot)
        {
        case EHDLoadoutWeapon::Primary:     return TEXT("Primary");
        case EHDLoadoutWeapon::Secondary:   return TEXT("Secondary");
        case EHDLoadoutWeapon::Grenade:     return TEXT("Grenade");
        default:                            return TEXT("?");
        }
    }

    // 세 칸과 해금 목록을 채운 프로필
    FHDProfileData MakeWeaponProfile()
    {
        FHDProfileData Data;
        Data.Requisition        = 7;
        Data.PrimaryWeapon      = Shotgun;
        Data.SecondaryWeapon    = MachinePistol;
        Data.Grenade            = ImpactGrenade;
        Data.UnlockedWeapons    = { Shotgun, MachinePistol, ImpactGrenade };
        return Data;
    }

    // 함선 메뉴 문맥: 빈 프로필 + 프로젝트 스트라타젬 카탈로그 + 설정 기본 로드아웃(컨트롤러가 만드는 것과 같은 모양)
    FHDShipMenuContext MakeMenuContext(const FHDProfileData& Profile)
    {
        FHDShipMenuContext Context;
        Context.Profile         = Profile;
        Context.Stratagems      = &FHDStratagemCatalog::GetProjectCatalog();
        Context.DefaultLoadout  = GetDefault<UHDStratagemSettings>()->Loadout;
        return Context;
    }
}

bool FHDWeaponCatalogTest::RunTest(const FString& Parameters)
{
    using namespace HDWeaponCatalogTest;

    // W1 표
    {
        const TArray<FHDWeaponOption>& Options = HDWeaponCatalog::GetOptions();
        TestEqual(TEXT("W1: 6 rows"), Options.Num(), 6);

        TArray<FName> Ids;
        TArray<FString> SlotParts;
        TArray<FString> CostParts;
        TArray<FString> DisplayParts;
        for (const FHDWeaponOption& Option : Options)
        {
            Ids.Add(Option.Id);
            SlotParts.Add(GetSlotText(Option.Slot));
            CostParts.Add(FString::FromInt(Option.UnlockCost));
            DisplayParts.Add(Option.DisplayName);
        }

        TestEqual(TEXT("W1: ids in order"), JoinNames(Ids), FString(TEXT("AssaultRifle,Shotgun,Pistol,MachinePistol,FragGrenade,ImpactGrenade")));
        TestEqual(TEXT("W1: slots"), FString::Join(SlotParts, TEXT(",")), FString(TEXT("Primary,Primary,Secondary,Secondary,Grenade,Grenade")));
        TestEqual(TEXT("W1: costs"), FString::Join(CostParts, TEXT(",")), FString(TEXT("0,100,0,60,0,80")));
        TestEqual(TEXT("W1: display names"), FString::Join(DisplayParts, TEXT(",")), FString(TEXT("ASSAULT RIFLE,SHOTGUN,PISTOL,MACHINE PISTOL,FRAG GRENADE,IMPACT GRENADE")));

        TestEqual(TEXT("W1: default primary"), HDWeaponCatalog::GetDefault(EHDLoadoutWeapon::Primary), AssaultRifle);
        TestEqual(TEXT("W1: default secondary"), HDWeaponCatalog::GetDefault(EHDLoadoutWeapon::Secondary), Pistol);
        TestEqual(TEXT("W1: default grenade"), HDWeaponCatalog::GetDefault(EHDLoadoutWeapon::Grenade), FragGrenade);

        for (const EHDLoadoutWeapon Slot : { EHDLoadoutWeapon::Primary, EHDLoadoutWeapon::Secondary, EHDLoadoutWeapon::Grenade })
        {
            const FHDWeaponOption* DefaultOption = HDWeaponCatalog::Find(HDWeaponCatalog::GetDefault(Slot));
            if (TestNotNull(*FString::Printf(TEXT("W1: default %s in table"), GetSlotText(Slot)), DefaultOption))
            {
                TestEqual(*FString::Printf(TEXT("W1: default %s costs 0"), GetSlotText(Slot)), DefaultOption->UnlockCost, 0);
                TestTrue(*FString::Printf(TEXT("W1: default %s slot"), GetSlotText(Slot)), DefaultOption->Slot == Slot);
            }
        }

        TSet<FName> UniqueIds;
        UniqueIds.Append(Ids);
        TestEqual(TEXT("W1: ids are distinct"), UniqueIds.Num(), Ids.Num());

        const FHDStratagemCatalog& Stratagems = FHDStratagemCatalog::GetProjectCatalog();
        TestTrue(TEXT("W1: stratagem catalog not empty"), Stratagems.GetNames().Num() > 0);
        bool bNoStratagemName = true;
        for (const FName& Id : Ids)
        {
            if (Stratagems.GetNames().Contains(Id))
            {
                AddError(FString::Printf(TEXT("W1: weapon id %s is also a stratagem name"), *Id.ToString()));
                bNoStratagemName = false;
            }
        }
        TestTrue(TEXT("W1: no weapon id is a stratagem name"), bNoStratagemName);
        TestNull(TEXT("W1: Find NoSuch"), HDWeaponCatalog::Find(NoSuch));
    }

    // W2 클래스
    {
        TestNull(TEXT("W2: AssaultRifle -> character default (nullptr)"), HDWeaponCatalog::GetWeaponClass(AssaultRifle));
        TestTrue(TEXT("W2: Shotgun -> AHDShotgun"), HDWeaponCatalog::GetWeaponClass(Shotgun) == AHDShotgun::StaticClass());
        TestTrue(TEXT("W2: Pistol -> exactly AHDSecondaryWeapon"), HDWeaponCatalog::GetWeaponClass(Pistol) == AHDSecondaryWeapon::StaticClass());
        TestTrue(TEXT("W2: MachinePistol -> AHDMachinePistol"), HDWeaponCatalog::GetWeaponClass(MachinePistol) == AHDMachinePistol::StaticClass());
        TestTrue(TEXT("W2: FragGrenade -> exactly AHDProjectileGrenade"), HDWeaponCatalog::GetWeaponClass(FragGrenade) == AHDProjectileGrenade::StaticClass());
        TestTrue(TEXT("W2: ImpactGrenade -> AHDProjectileImpactGrenade"), HDWeaponCatalog::GetWeaponClass(ImpactGrenade) == AHDProjectileImpactGrenade::StaticClass());
        TestNull(TEXT("W2: NoSuch -> nullptr"), HDWeaponCatalog::GetWeaponClass(NoSuch));

        TestTrue(TEXT("W2: AHDMachinePistol is a child of AHDSecondaryWeapon"),
            AHDMachinePistol::StaticClass()->IsChildOf(AHDSecondaryWeapon::StaticClass()) && AHDMachinePistol::StaticClass() != AHDSecondaryWeapon::StaticClass());
        TestTrue(TEXT("W2: AHDProjectileImpactGrenade is a child of AHDProjectileGrenade"),
            AHDProjectileImpactGrenade::StaticClass()->IsChildOf(AHDProjectileGrenade::StaticClass()) && AHDProjectileImpactGrenade::StaticClass() != AHDProjectileGrenade::StaticClass());

        const AHDShotgun* ShotgunCDO = GetDefault<AHDShotgun>();
        if (TestNotNull(TEXT("W2: shotgun CDO"), ShotgunCDO))
        {
            TestTrue(TEXT("W2: shotgun slot Primary"), ShotgunCDO->GetWeaponSlot() == EHDWeaponSlot::Primary);
            TestTrue(TEXT("W2: shotgun sends montage events from code"), ShotgunCDO->SendsMontageEventsFromCode());
            TestEqual(TEXT("W2: shotgun 8 pellets"), ShotgunCDO->GetPelletCount(), 8);
            TestFalse(TEXT("W2: shotgun not auto fire"), ShotgunCDO->IsAutoFireWeapon());
        }

        const AHDMachinePistol* MachinePistolCDO = GetDefault<AHDMachinePistol>();
        if (TestNotNull(TEXT("W2: machine pistol CDO"), MachinePistolCDO))
        {
            TestTrue(TEXT("W2: machine pistol slot Secondary"), MachinePistolCDO->GetWeaponSlot() == EHDWeaponSlot::Secondary);
            TestTrue(TEXT("W2: machine pistol auto fire"), MachinePistolCDO->IsAutoFireWeapon());
        }
    }

    // W3 해금
    {
        FHDProfileData Poor;
        Poor.Requisition = 50;
        TestTrue(TEXT("W3: 50 req shotgun cannot afford"), HDWeaponCatalog::TryUnlock(Poor, Shotgun) == EHDUnlockResult::CannotAfford);
        TestEqual(TEXT("W3: 50 req unchanged"), Poor.Requisition, 50);
        TestTrue(TEXT("W3: nothing unlocked when poor"), Poor.UnlockedWeapons.IsEmpty());

        FHDProfileData Rich;
        Rich.Requisition = 200;
        TestTrue(TEXT("W3: 200 req shotgun unlocked"), HDWeaponCatalog::TryUnlock(Rich, Shotgun) == EHDUnlockResult::Unlocked);
        TestEqual(TEXT("W3: 200 -> 100 req"), Rich.Requisition, 100);
        TestEqual(TEXT("W3: unlocked list [Shotgun]"), JoinNames(Rich.UnlockedWeapons), FString(TEXT("Shotgun")));
        TestTrue(TEXT("W3: shotgun again already unlocked"), HDWeaponCatalog::TryUnlock(Rich, Shotgun) == EHDUnlockResult::AlreadyUnlocked);
        TestEqual(TEXT("W3: again keeps 100 req"), Rich.Requisition, 100);

        TestTrue(TEXT("W3: pistol already unlocked"), HDWeaponCatalog::TryUnlock(Rich, Pistol) == EHDUnlockResult::AlreadyUnlocked);
        TestEqual(TEXT("W3: pistol not added to list"), JoinNames(Rich.UnlockedWeapons), FString(TEXT("Shotgun")));

        TestTrue(TEXT("W3: NoSuch unknown"), HDWeaponCatalog::TryUnlock(Rich, NoSuch) == EHDUnlockResult::Unknown);
        TestTrue(TEXT("W3: stratagem name unknown"), HDWeaponCatalog::TryUnlock(Rich, Resupply) == EHDUnlockResult::Unknown);
        TestEqual(TEXT("W3: unknown keeps 100 req"), Rich.Requisition, 100);
        TestEqual(TEXT("W3: unknown keeps list"), JoinNames(Rich.UnlockedWeapons), FString(TEXT("Shotgun")));
    }

    // W4 고른 무기
    {
        const FHDProfileData Empty;
        TestEqual(TEXT("W4: empty primary"), HDWeaponCatalog::GetChosen(Empty, EHDLoadoutWeapon::Primary), AssaultRifle);
        TestEqual(TEXT("W4: empty secondary"), HDWeaponCatalog::GetChosen(Empty, EHDLoadoutWeapon::Secondary), Pistol);
        TestEqual(TEXT("W4: empty grenade"), HDWeaponCatalog::GetChosen(Empty, EHDLoadoutWeapon::Grenade), FragGrenade);

        FHDProfileData Locked;
        Locked.PrimaryWeapon = Shotgun;
        TestEqual(TEXT("W4: locked shotgun -> AssaultRifle"), HDWeaponCatalog::GetChosen(Locked, EHDLoadoutWeapon::Primary), AssaultRifle);

        FHDProfileData Unlocked = Locked;
        Unlocked.UnlockedWeapons.Add(Shotgun);
        TestEqual(TEXT("W4: unlocked shotgun -> Shotgun"), HDWeaponCatalog::GetChosen(Unlocked, EHDLoadoutWeapon::Primary), Shotgun);

        FHDProfileData WrongSlot;
        WrongSlot.PrimaryWeapon = Pistol;
        TestEqual(TEXT("W4: pistol in primary -> AssaultRifle"), HDWeaponCatalog::GetChosen(WrongSlot, EHDLoadoutWeapon::Primary), AssaultRifle);

        FHDProfileData BadGrenade;
        BadGrenade.Grenade = NoSuch;
        TestEqual(TEXT("W4: NoSuch grenade -> FragGrenade"), HDWeaponCatalog::GetChosen(BadGrenade, EHDLoadoutWeapon::Grenade), FragGrenade);

        TestEqual(TEXT("W4: describe empty"), HDWeaponCatalog::Describe(Empty), FString(TEXT("primary=AssaultRifle secondary=Pistol grenade=FragGrenade unlocked=")));
        TestEqual(TEXT("W4: describe full"), HDWeaponCatalog::Describe(MakeWeaponProfile()), FString(TEXT("primary=Shotgun secondary=MachinePistol grenade=ImpactGrenade unlocked=Shotgun,MachinePistol,ImpactGrenade")));
    }

    // W5 돌리기
    {
        const FHDProfileData Empty;
        TestEqual(TEXT("W5: empty primary +1 -> AssaultRifle"), HDWeaponCatalog::Cycle(Empty, EHDLoadoutWeapon::Primary, 1), AssaultRifle);

        FHDProfileData ShotgunOpen;
        ShotgunOpen.UnlockedWeapons.Add(Shotgun);
        TestEqual(TEXT("W5: shotgun open +1 -> Shotgun"), HDWeaponCatalog::Cycle(ShotgunOpen, EHDLoadoutWeapon::Primary, 1), Shotgun);
        TestEqual(TEXT("W5: shotgun open -1 -> Shotgun (wraps)"), HDWeaponCatalog::Cycle(ShotgunOpen, EHDLoadoutWeapon::Primary, -1), Shotgun);

        FHDProfileData ShotgunChosen = ShotgunOpen;
        ShotgunChosen.PrimaryWeapon = Shotgun;
        TestEqual(TEXT("W5: shotgun chosen +1 -> AssaultRifle"), HDWeaponCatalog::Cycle(ShotgunChosen, EHDLoadoutWeapon::Primary, 1), AssaultRifle);

        FHDProfileData AllOpen;
        AllOpen.UnlockedWeapons = { Shotgun, MachinePistol, ImpactGrenade };
        TestEqual(TEXT("W5: all open secondary +1 -> MachinePistol"), HDWeaponCatalog::Cycle(AllOpen, EHDLoadoutWeapon::Secondary, 1), MachinePistol);
        TestEqual(TEXT("W5: all open grenade -1 -> ImpactGrenade"), HDWeaponCatalog::Cycle(AllOpen, EHDLoadoutWeapon::Grenade, -1), ImpactGrenade);

        FHDProfileData Written;
        HDWeaponCatalog::SetChosen(Written, EHDLoadoutWeapon::Primary, Shotgun);
        HDWeaponCatalog::SetChosen(Written, EHDLoadoutWeapon::Secondary, MachinePistol);
        HDWeaponCatalog::SetChosen(Written, EHDLoadoutWeapon::Grenade, ImpactGrenade);
        TestEqual(TEXT("W5: SetChosen primary field"), Written.PrimaryWeapon, Shotgun);
        TestEqual(TEXT("W5: SetChosen secondary field"), Written.SecondaryWeapon, MachinePistol);
        TestEqual(TEXT("W5: SetChosen grenade field"), Written.Grenade, ImpactGrenade);
    }

    // W6 저장
    {
        const FHDProfileData Expected = MakeWeaponProfile();

        UHDProfileSaveGame* SaveGame = NewObject<UHDProfileSaveGame>();
        SaveGame->Data = Expected;

        TArray<uint8> Bytes;
        TestTrue(TEXT("W6: SaveGameToMemory"), UGameplayStatics::SaveGameToMemory(SaveGame, Bytes));
        TestTrue(TEXT("W6: bytes written"), Bytes.Num() > 0);

        const UHDProfileSaveGame* Loaded = Cast<UHDProfileSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
        if (TestNotNull(TEXT("W6: loaded as UHDProfileSaveGame"), Loaded))
        {
            const FHDProfileData& Actual = Loaded->Data;
            TestEqual(TEXT("W6: primary"), Actual.PrimaryWeapon, Expected.PrimaryWeapon);
            TestEqual(TEXT("W6: secondary"), Actual.SecondaryWeapon, Expected.SecondaryWeapon);
            TestEqual(TEXT("W6: grenade"), Actual.Grenade, Expected.Grenade);
            TestEqual(TEXT("W6: unlocked weapons"), JoinNames(Actual.UnlockedWeapons), JoinNames(Expected.UnlockedWeapons));
            TestEqual(TEXT("W6: requisition"), Actual.Requisition, Expected.Requisition);
        }

        FHDProfileData Duplicated;
        Duplicated.UnlockedWeapons = { Shotgun, Shotgun };
        HDProgression::Sanitize(Duplicated);
        TestEqual(TEXT("W6: sanitize removes duplicate weapons"), JoinNames(Duplicated.UnlockedWeapons), FString(TEXT("Shotgun")));

        // 무기 필드는 HDProgression::Describe 에 나오지 않는다(P6 의 줄 그대로)
        FHDProfileData WithoutWeapons = Expected;
        WithoutWeapons.PrimaryWeapon    = NAME_None;
        WithoutWeapons.SecondaryWeapon  = NAME_None;
        WithoutWeapons.Grenade          = NAME_None;
        WithoutWeapons.UnlockedWeapons.Reset();
        TestEqual(TEXT("W6: progression describe ignores weapons"), HDProgression::Describe(Expected), HDProgression::Describe(WithoutWeapons));
    }

    // W7 메뉴
    {
        const FHDShipMenuContext Context = MakeMenuContext(FHDProfileData());

        const TArray<FHDShipMenuItem> LoadoutItems = HDShipMenu::BuildItems(EHDShipScreen::Loadout, Context);
        TestEqual(TEXT("W7: Loadout ids"), JoinIds(LoadoutItems), FString(TEXT("Slot1,Slot2,Slot3,Slot4,Primary,Secondary,Grenade,Ready")));

        const FHDShipMenuItem* PrimaryItem = FindItem(LoadoutItems, TEXT("Primary"));
        const FHDShipMenuItem* SecondaryItem = FindItem(LoadoutItems, TEXT("Secondary"));
        const FHDShipMenuItem* GrenadeItem = FindItem(LoadoutItems, TEXT("Grenade"));
        if (TestNotNull(TEXT("W7: Primary row"), PrimaryItem) && TestNotNull(TEXT("W7: Secondary row"), SecondaryItem) && TestNotNull(TEXT("W7: Grenade row"), GrenadeItem))
        {
            TestEqual(TEXT("W7: Primary label"), PrimaryItem->Label, FString(TEXT("PRIMARY  < ASSAULT RIFLE >")));
            TestEqual(TEXT("W7: Secondary label"), SecondaryItem->Label, FString(TEXT("SECONDARY  < PISTOL >")));
            TestEqual(TEXT("W7: Grenade label"), GrenadeItem->Label, FString(TEXT("GRENADE  < FRAG GRENADE >")));
            TestTrue(TEXT("W7: weapon rows adjustable"), PrimaryItem->bAdjustable && SecondaryItem->bAdjustable && GrenadeItem->bAdjustable);
            TestTrue(TEXT("W7: weapon rows confirm None"),
                PrimaryItem->ConfirmAction == EHDShipActionType::None && SecondaryItem->ConfirmAction == EHDShipActionType::None && GrenadeItem->ConfirmAction == EHDShipActionType::None);
            TestEqual(TEXT("W7: Primary detail"), PrimaryItem->Detail, FString(TEXT("AUTOMATIC   45 ROUNDS   ARMOR PEN 2")));
        }

        const TArray<FHDShipMenuItem> PoorArmory = HDShipMenu::BuildItems(EHDShipScreen::Armory, Context);
        TestEqual(TEXT("W7: Armory ids"), JoinIds(PoorArmory), FString(TEXT("OrbitalGatlingBarrage,Orbital120mmBarrage,Shotgun,MachinePistol,ImpactGrenade")));
        bool bAllDisabled = PoorArmory.Num() > 0;
        for (const FHDShipMenuItem& Item : PoorArmory)
        {
            bAllDisabled &= Item.bEnabled == false;
        }
        TestTrue(TEXT("W7: 0 requisition disables all armory rows"), bAllDisabled);

        const FHDShipMenuItem* PoorShotgun = FindItem(PoorArmory, Shotgun);
        if (TestNotNull(TEXT("W7: armory Shotgun row"), PoorShotgun))
        {
            TestEqual(TEXT("W7: shotgun label"), PoorShotgun->Label, FString(TEXT("SHOTGUN  100 REQ")));
            TestEqual(TEXT("W7: shotgun detail"), PoorShotgun->Detail, FString(TEXT("8 PELLETS   6 SHELLS   ARMOR PEN 1")));
            TestTrue(TEXT("W7: shotgun row confirms Select"), PoorShotgun->ConfirmAction == EHDShipActionType::Select);
        }

        FHDProfileData RichProfile;
        RichProfile.Requisition = 1000;
        RichProfile.UnlockedWeapons.Add(Shotgun);
        const TArray<FHDShipMenuItem> RichArmory = HDShipMenu::BuildItems(EHDShipScreen::Armory, MakeMenuContext(RichProfile));
        const FHDShipMenuItem* RichShotgun = FindItem(RichArmory, Shotgun);
        const FHDShipMenuItem* RichMachinePistol = FindItem(RichArmory, MachinePistol);
        if (TestNotNull(TEXT("W7: rich Shotgun row"), RichShotgun) && TestNotNull(TEXT("W7: rich MachinePistol row"), RichMachinePistol))
        {
            TestEqual(TEXT("W7: unlocked shotgun label"), RichShotgun->Label, FString(TEXT("SHOTGUN  UNLOCKED")));
            TestFalse(TEXT("W7: unlocked shotgun disabled"), RichShotgun->bEnabled);
            TestEqual(TEXT("W7: machine pistol label"), RichMachinePistol->Label, FString(TEXT("MACHINE PISTOL  60 REQ")));
            TestTrue(TEXT("W7: affordable machine pistol enabled"), RichMachinePistol->bEnabled);
        }
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
