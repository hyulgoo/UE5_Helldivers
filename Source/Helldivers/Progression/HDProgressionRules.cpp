// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDProgressionRules.h"

int32 FHDSampleCounts::Get(EHDSampleType Type) const
{
    switch (Type)
    {
    case EHDSampleType::Common: return Common;
    case EHDSampleType::Rare:   return Rare;
    case EHDSampleType::Super:  return Super;
    default:                    return 0;
    }
}

void FHDSampleCounts::Add(EHDSampleType Type, int32 Amount)
{
    switch (Type)
    {
    case EHDSampleType::Common: Common += Amount; break;
    case EHDSampleType::Rare:   Rare += Amount; break;
    case EHDSampleType::Super:  Super += Amount; break;
    default:                    break;
    }
}

int32 FHDSampleCounts::Total() const
{
    return Common + Rare + Super;
}

bool FHDSampleCounts::operator==(const FHDSampleCounts& Other) const
{
    return Common == Other.Common && Rare == Other.Rare && Super == Other.Super;
}

namespace HDProgression
{
    namespace
    {
        // 더해도 넘치지 않게 64비트로 더한 뒤 [0, Max] 로 자른다
        int32 AddClamped(const int32 Value, const int32 Amount, const int32 Max)
        {
            return static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Value) + Amount, 0, Max));
        }

        // 처음 나온 것만 남긴다(순서 유지)
        void RemoveDuplicateNames(TArray<FName>& Names)
        {
            TArray<FName> UniqueNames;
            UniqueNames.Reserve(Names.Num());
            for (const FName& Name : Names)
            {
                UniqueNames.AddUnique(Name);
            }
            Names = MoveTemp(UniqueNames);
        }
    }

    bool CanAfford(const FHDProfileData& Data, const FHDCost& Cost)
    {
        return Data.Requisition >= Cost.Requisition
            && Data.Samples.Common >= Cost.Samples.Common
            && Data.Samples.Rare >= Cost.Samples.Rare
            && Data.Samples.Super >= Cost.Samples.Super;
    }

    bool TrySpend(FHDProfileData& Data, const FHDCost& Cost)
    {
        if (CanAfford(Data, Cost) == false)
        {
            return false;
        }

        Data.Requisition -= Cost.Requisition;
        Data.Samples.Common -= Cost.Samples.Common;
        Data.Samples.Rare -= Cost.Samples.Rare;
        Data.Samples.Super -= Cost.Samples.Super;
        return true;
    }

    void AddResources(FHDProfileData& Data, int32 Requisition, const FHDSampleCounts& Samples)
    {
        Data.Requisition = AddClamped(Data.Requisition, Requisition, MaxRequisition);
        Data.Samples.Common = AddClamped(Data.Samples.Common, Samples.Common, MaxCommonSamples);
        Data.Samples.Rare = AddClamped(Data.Samples.Rare, Samples.Rare, MaxRareSamples);
        Data.Samples.Super = AddClamped(Data.Samples.Super, Samples.Super, MaxSuperSamples);
    }

    void Sanitize(FHDProfileData& Data, int32 MaxDifficulty)
    {
        Data.Version = 1;

        // 0 을 더한다 = 자원을 [0, 상한] 으로 자르기만 한다
        AddResources(Data, 0, FHDSampleCounts());

        Data.Difficulty = FMath::Clamp(Data.Difficulty, 1, FMath::Max(1, MaxDifficulty));

        for (auto It = Data.UpgradeLevels.CreateIterator(); It; ++It)
        {
            if (It.Value() < 1)
            {
                It.RemoveCurrent();
            }
        }

        RemoveDuplicateNames(Data.UnlockedStratagems);
        RemoveDuplicateNames(Data.Loadout);
    }

    FString Describe(const FHDProfileData& Data)
    {
        TArray<FName> UpgradeNames;
        Data.UpgradeLevels.GetKeys(UpgradeNames);
        UpgradeNames.Sort(FNameLexicalLess());

        TArray<FString> UpgradeParts;
        for (const FName& UpgradeName : UpgradeNames)
        {
            UpgradeParts.Add(FString::Printf(TEXT("%s:%d"), *UpgradeName.ToString(), Data.UpgradeLevels.FindRef(UpgradeName)));
        }

        TArray<FString> UnlockedParts;
        for (const FName& StratagemName : Data.UnlockedStratagems)
        {
            UnlockedParts.Add(StratagemName.ToString());
        }

        TArray<FString> LoadoutParts;
        for (const FName& StratagemName : Data.Loadout)
        {
            LoadoutParts.Add(StratagemName.ToString());
        }

        return FString::Printf(TEXT("Requisition=%d Samples=%d/%d/%d Difficulty=%d Played=%d Won=%d Upgrades=%s Unlocked=%s Loadout=%s"),
            Data.Requisition,
            Data.Samples.Common,
            Data.Samples.Rare,
            Data.Samples.Super,
            Data.Difficulty,
            Data.MissionsPlayed,
            Data.MissionsWon,
            *FString::Join(UpgradeParts, TEXT(",")),
            *FString::Join(UnlockedParts, TEXT(",")),
            *FString::Join(LoadoutParts, TEXT(",")));
    }
}
