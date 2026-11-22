// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDWeaponCatalog.h"
#include "Progression/HDProgressionRules.h"
#include "Weapon/HDShotgun.h"
#include "Weapon/HDSecondaryWeapon.h"
#include "Weapon/HDMachinePistol.h"
#include "Weapon/Projectile/HDProjectileGrenade.h"
#include "Weapon/Projectile/HDProjectileImpactGrenade.h"

namespace HDWeaponCatalog
{
    namespace
    {
        const FName AssaultRifleId(TEXT("AssaultRifle"));
        const FName ShotgunId(TEXT("Shotgun"));
        const FName PistolId(TEXT("Pistol"));
        const FName MachinePistolId(TEXT("MachinePistol"));
        const FName FragGrenadeId(TEXT("FragGrenade"));
        const FName ImpactGrenadeId(TEXT("ImpactGrenade"));

        FHDWeaponOption MakeOption(FName Id, EHDLoadoutWeapon Slot, const TCHAR* DisplayName, const TCHAR* Detail, int32 UnlockCost)
        {
            FHDWeaponOption Option;
            Option.Id           = Id;
            Option.Slot         = Slot;
            Option.DisplayName  = DisplayName;
            Option.Detail       = Detail;
            Option.UnlockCost   = UnlockCost;
            return Option;
        }

        FName GetProfileValue(const FHDProfileData& Data, EHDLoadoutWeapon Slot)
        {
            switch (Slot)
            {
            case EHDLoadoutWeapon::Primary:     return Data.PrimaryWeapon;
            case EHDLoadoutWeapon::Secondary:   return Data.SecondaryWeapon;
            case EHDLoadoutWeapon::Grenade:     return Data.Grenade;
            default:                            return NAME_None;
            }
        }
    }

    const TArray<FHDWeaponOption>& GetOptions()
    {
        // 수치는 각 클래스 생성자 값이다(돌격소총은 BP_Rifle)
        static const TArray<FHDWeaponOption> Options =
        {
            MakeOption(AssaultRifleId,  EHDLoadoutWeapon::Primary,      TEXT("ASSAULT RIFLE"),  TEXT("AUTOMATIC   45 ROUNDS   ARMOR PEN 2"),    0),
            MakeOption(ShotgunId,       EHDLoadoutWeapon::Primary,      TEXT("SHOTGUN"),        TEXT("8 PELLETS   6 SHELLS   ARMOR PEN 1"),     100),
            MakeOption(PistolId,        EHDLoadoutWeapon::Secondary,    TEXT("PISTOL"),         TEXT("SEMI-AUTO   15 ROUNDS   ARMOR PEN 1"),    0),
            MakeOption(MachinePistolId, EHDLoadoutWeapon::Secondary,    TEXT("MACHINE PISTOL"), TEXT("AUTOMATIC   24 ROUNDS   ARMOR PEN 1"),    60),
            MakeOption(FragGrenadeId,   EHDLoadoutWeapon::Grenade,      TEXT("FRAG GRENADE"),   TEXT("3 SECOND FUSE"),                          0),
            MakeOption(ImpactGrenadeId, EHDLoadoutWeapon::Grenade,      TEXT("IMPACT GRENADE"), TEXT("EXPLODES ON IMPACT"),                     80),
        };
        return Options;
    }

    const FHDWeaponOption* Find(FName Id)
    {
        return GetOptions().FindByPredicate([Id](const FHDWeaponOption& Option) { return Option.Id == Id; });
    }

    FName GetDefault(EHDLoadoutWeapon Slot)
    {
        switch (Slot)
        {
        case EHDLoadoutWeapon::Primary:     return AssaultRifleId;
        case EHDLoadoutWeapon::Secondary:   return PistolId;
        case EHDLoadoutWeapon::Grenade:     return FragGrenadeId;
        default:                            return NAME_None;
        }
    }

    bool IsUnlocked(const FHDProfileData& Data, FName Id)
    {
        const FHDWeaponOption* Option = Find(Id);
        return Option != nullptr && (Option->UnlockCost <= 0 || Data.UnlockedWeapons.Contains(Id));
    }

    EHDUnlockResult TryUnlock(FHDProfileData& Data, FName Id)
    {
        const FHDWeaponOption* Option = Find(Id);
        if (Option == nullptr)
        {
            return EHDUnlockResult::Unknown;
        }

        // 처음부터 열린 무기(가격 ≤ 0)도 여기서 돌아가므로 해금 목록에 들어가지 않는다
        if (IsUnlocked(Data, Id))
        {
            return EHDUnlockResult::AlreadyUnlocked;
        }

        FHDCost Cost;
        Cost.Requisition = Option->UnlockCost;

        // 모자라면 TrySpend 가 아무것도 바꾸지 않는다
        if (HDProgression::TrySpend(Data, Cost) == false)
        {
            return EHDUnlockResult::CannotAfford;
        }

        Data.UnlockedWeapons.AddUnique(Id);
        return EHDUnlockResult::Unlocked;
    }

    FName GetChosen(const FHDProfileData& Data, EHDLoadoutWeapon Slot)
    {
        const FName Value = GetProfileValue(Data, Slot);
        const FHDWeaponOption* Option = Find(Value);
        if (Option == nullptr || Option->Slot != Slot || IsUnlocked(Data, Value) == false)
        {
            return GetDefault(Slot);
        }

        return Value;
    }

    FName Cycle(const FHDProfileData& Data, EHDLoadoutWeapon Slot, int32 Delta)
    {
        TArray<FName> Candidates;
        for (const FHDWeaponOption& Option : GetOptions())
        {
            if (Option.Slot == Slot && IsUnlocked(Data, Option.Id))
            {
                Candidates.Add(Option.Id);
            }
        }

        const FName Current = GetChosen(Data, Slot);
        const int32 CurrentIndex = Candidates.IndexOfByKey(Current);
        if (CurrentIndex == INDEX_NONE)
        {
            // 기본 무기는 늘 열려 있어 오지 않는다
            return Current;
        }

        const int32 CandidateCount = Candidates.Num();
        const int32 NextIndex = ((CurrentIndex + Delta) % CandidateCount + CandidateCount) % CandidateCount;
        return Candidates[NextIndex];
    }

    void SetChosen(FHDProfileData& Data, EHDLoadoutWeapon Slot, FName Id)
    {
        switch (Slot)
        {
        case EHDLoadoutWeapon::Primary:     Data.PrimaryWeapon = Id; break;
        case EHDLoadoutWeapon::Secondary:   Data.SecondaryWeapon = Id; break;
        case EHDLoadoutWeapon::Grenade:     Data.Grenade = Id; break;
        default:                            break;
        }
    }

    UClass* GetWeaponClass(FName Id)
    {
        // 돌격소총은 캐릭터 기본(UHDCombatComponent::DefaultWeaponClass = BP_Rifle)이라 nullptr
        if (Id == ShotgunId)
        {
            return AHDShotgun::StaticClass();
        }

        if (Id == PistolId)
        {
            return AHDSecondaryWeapon::StaticClass();
        }

        if (Id == MachinePistolId)
        {
            return AHDMachinePistol::StaticClass();
        }

        if (Id == FragGrenadeId)
        {
            return AHDProjectileGrenade::StaticClass();
        }

        if (Id == ImpactGrenadeId)
        {
            return AHDProjectileImpactGrenade::StaticClass();
        }

        return nullptr;
    }

    const TCHAR* GetSlotName(EHDLoadoutWeapon Slot)
    {
        switch (Slot)
        {
        case EHDLoadoutWeapon::Primary:     return TEXT("Primary");
        case EHDLoadoutWeapon::Secondary:   return TEXT("Secondary");
        case EHDLoadoutWeapon::Grenade:     return TEXT("Grenade");
        default:                            return TEXT("?");
        }
    }

    FString Describe(const FHDProfileData& Data)
    {
        TArray<FString> UnlockedParts;
        for (const FName& Id : Data.UnlockedWeapons)
        {
            UnlockedParts.Add(Id.ToString());
        }

        return FString::Printf(TEXT("primary=%s secondary=%s grenade=%s unlocked=%s"),
            *GetChosen(Data, EHDLoadoutWeapon::Primary).ToString(),
            *GetChosen(Data, EHDLoadoutWeapon::Secondary).ToString(),
            *GetChosen(Data, EHDLoadoutWeapon::Grenade).ToString(),
            *FString::Join(UnlockedParts, TEXT(",")));
    }
}
