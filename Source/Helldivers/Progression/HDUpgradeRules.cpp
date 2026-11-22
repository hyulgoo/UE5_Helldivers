// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDUpgradeRules.h"
#include "Progression/HDProgressionRules.h"
#include "Progression/HDUpgradeCatalog.h"
#include "GameData/HDStratagemDefinition.h"
#include "Stratagem/HDStratagemLoadout.h"

FHDCost FHDUpgradeLevel::ToCost() const
{
    FHDCost Cost;
    Cost.Requisition    = Requisition;
    Cost.Samples.Common = Common;
    Cost.Samples.Rare   = Rare;
    Cost.Samples.Super  = Super;
    return Cost;
}

namespace HDUpgrade
{
    namespace
    {
        // 부호를 늘 붙인다: +1 / -1 / +0
        FString FormatSignedInt(const int32 Value)
        {
            return FString::Printf(TEXT("%s%d"), Value < 0 ? TEXT("-") : TEXT("+"), FMath::Abs(Value));
        }

        // 부호를 늘 붙인다: -0.5 / +0.0
        FString FormatSignedFloat(const float Value)
        {
            return FString::Printf(TEXT("%s%.1f"), Value < 0.f ? TEXT("-") : TEXT("+"), FMath::Abs(Value));
        }

        // 배율 → 백분율 차이: 0.6 → "-40%", 1.1 → "+10%"
        FString FormatPercent(const float Scale)
        {
            return FString::Printf(TEXT("%s%%"), *FormatSignedInt(FMath::RoundToInt((Scale - 1.f) * 100.f)));
        }
    }

    int32 GetLevel(const FHDProfileData& Data, FName UpgradeName)
    {
        return FMath::Max(0, Data.UpgradeLevels.FindRef(UpgradeName));
    }

    const FHDUpgradeLevel* GetNextLevel(const FHDUpgradeDefinition& Definition, int32 CurrentLevel)
    {
        const int32 NextIndex = FMath::Max(0, CurrentLevel);
        return Definition.Levels.IsValidIndex(NextIndex) ? &Definition.Levels[NextIndex] : nullptr;
    }

    EHDUpgradeBuyResult TryBuy(FHDProfileData& Data, FName UpgradeName, const FHDUpgradeCatalog& Catalog)
    {
        const FHDUpgradeDefinition* Definition = Catalog.Find(UpgradeName);
        if (Definition == nullptr)
        {
            return EHDUpgradeBuyResult::Unknown;
        }

        const int32 CurrentLevel = GetLevel(Data, UpgradeName);
        const FHDUpgradeLevel* NextLevel = GetNextLevel(*Definition, CurrentLevel);
        if (NextLevel == nullptr)
        {
            return EHDUpgradeBuyResult::MaxLevel;
        }

        // 모자라면 TrySpend 가 아무것도 바꾸지 않는다
        if (HDProgression::TrySpend(Data, NextLevel->ToCost()) == false)
        {
            return EHDUpgradeBuyResult::CannotAfford;
        }

        Data.UpgradeLevels.FindOrAdd(UpgradeName) = CurrentLevel + 1;
        return EHDUpgradeBuyResult::Bought;
    }

    FHDUpgradeModifiers ComputeModifiers(const FHDProfileData& Data, const FHDUpgradeCatalog& Catalog)
    {
        FHDUpgradeModifiers Modifiers;
        for (const TPair<FName, int32>& Pair : Data.UpgradeLevels)
        {
            const FHDUpgradeDefinition* Definition = Catalog.Find(Pair.Key);
            if (Definition == nullptr)
            {
                continue;
            }

            const int32 Level = FMath::Clamp(Pair.Value, 0, Definition->Levels.Num());
            if (Level == 0)
            {
                continue;
            }

            // 값은 그 단계까지의 누적 값이다
            const float Value = Definition->Levels[Level - 1].Value;
            switch (Definition->Effect)
            {
            case EHDUpgradeEffect::EagleExtraUses:              Modifiers.EagleExtraUses += FMath::RoundToInt(Value); break;
            case EHDUpgradeEffect::EagleRearmScale:             Modifiers.EagleRearmScale *= Value; break;
            case EHDUpgradeEffect::OrbitalCooldownScale:        Modifiers.OrbitalCooldownScale *= Value; break;
            case EHDUpgradeEffect::SupportCooldownScale:        Modifiers.SupportCooldownScale *= Value; break;
            case EHDUpgradeEffect::ActivationDelayReduction:    Modifiers.ActivationDelayReduction += Value; break;
            case EHDUpgradeEffect::ExtraStimsAndGrenades:       Modifiers.ExtraStimsAndGrenades += FMath::RoundToInt(Value); break;
            case EHDUpgradeEffect::ExtraReinforcements:         Modifiers.ExtraReinforcements += FMath::RoundToInt(Value); break;
            default:                                            break;
            }
        }

        return Modifiers;
    }

    void ApplyToEntry(FHDStratagemEntry& Entry, const FHDStratagemDefinition& Definition, const FHDUpgradeModifiers& Modifiers)
    {
        if (Definition.bIsEagle)
        {
            // 무제한(0)은 그대로 둔다
            if (Entry.MaxUses > 0)
            {
                Entry.MaxUses = FMath::Max(1, Entry.MaxUses + Modifiers.EagleExtraUses);
            }
            Entry.RearmTime *= Modifiers.EagleRearmScale;
        }
        else if (Definition.Kind == EHDStratagemKind::Projectile)
        {
            Entry.Cooldown *= Modifiers.OrbitalCooldownScale;
        }
        else if (Definition.Kind == EHDStratagemKind::Resupply
            || Definition.Kind == EHDStratagemKind::Sentry
            || Definition.Kind == EHDStratagemKind::SupportWeapon)
        {
            Entry.Cooldown *= Modifiers.SupportCooldownScale;
        }

        // 하한보다 짧던 값은 늘리지 않는다
        Entry.ActivationDelay = FMath::Max(FMath::Min(Entry.ActivationDelay, MinActivationDelay), Entry.ActivationDelay - Modifiers.ActivationDelayReduction);
    }

    FString FormatEffect(EHDUpgradeEffect Effect, float Value)
    {
        switch (Effect)
        {
        case EHDUpgradeEffect::EagleExtraUses:              return FString::Printf(TEXT("%s EAGLE USES"), *FormatSignedInt(FMath::RoundToInt(Value)));
        case EHDUpgradeEffect::EagleRearmScale:             return FString::Printf(TEXT("EAGLE REARM %s"), *FormatPercent(Value));
        case EHDUpgradeEffect::OrbitalCooldownScale:        return FString::Printf(TEXT("ORBITAL COOLDOWN %s"), *FormatPercent(Value));
        case EHDUpgradeEffect::SupportCooldownScale:        return FString::Printf(TEXT("SUPPORT COOLDOWN %s"), *FormatPercent(Value));
        case EHDUpgradeEffect::ActivationDelayReduction:    return FString::Printf(TEXT("CALL-IN TIME %sS"), *FormatSignedFloat(-Value));
        case EHDUpgradeEffect::ExtraStimsAndGrenades:       return FString::Printf(TEXT("%s STIMS AND GRENADES"), *FormatSignedInt(FMath::RoundToInt(Value)));
        case EHDUpgradeEffect::ExtraReinforcements:         return FString::Printf(TEXT("%s REINFORCEMENTS"), *FormatSignedInt(FMath::RoundToInt(Value)));
        default:                                            return FString();
        }
    }

    FString FormatCost(const FHDCost& Cost)
    {
        TArray<FString> Parts;
        if (Cost.Requisition != 0)
        {
            Parts.Add(FString::Printf(TEXT("%d REQ"), Cost.Requisition));
        }
        if (Cost.Samples.Common != 0)
        {
            Parts.Add(FString::Printf(TEXT("%d C"), Cost.Samples.Common));
        }
        if (Cost.Samples.Rare != 0)
        {
            Parts.Add(FString::Printf(TEXT("%d R"), Cost.Samples.Rare));
        }
        if (Cost.Samples.Super != 0)
        {
            Parts.Add(FString::Printf(TEXT("%d S"), Cost.Samples.Super));
        }

        return Parts.IsEmpty() ? FString(TEXT("FREE")) : FString::Join(Parts, TEXT("  "));
    }

    FString DescribeModifiers(const FHDUpgradeModifiers& Modifiers)
    {
        return FString::Printf(TEXT("eagleUses=%s eagleRearm=%.2f orbital=%.2f support=%.2f delay=%s consumables=%s reinforcements=%s"),
            *FormatSignedInt(Modifiers.EagleExtraUses),
            Modifiers.EagleRearmScale,
            Modifiers.OrbitalCooldownScale,
            Modifiers.SupportCooldownScale,
            *FormatSignedFloat(-Modifiers.ActivationDelayReduction),
            *FormatSignedInt(Modifiers.ExtraStimsAndGrenades),
            *FormatSignedInt(Modifiers.ExtraReinforcements));
    }

    const TCHAR* GetBuyResultName(EHDUpgradeBuyResult Result)
    {
        switch (Result)
        {
        case EHDUpgradeBuyResult::Bought:       return TEXT("bought");
        case EHDUpgradeBuyResult::Unknown:      return TEXT("unknown");
        case EHDUpgradeBuyResult::MaxLevel:     return TEXT("max level");
        case EHDUpgradeBuyResult::CannotAfford: return TEXT("cannot afford");
        default:                                return TEXT("?");
        }
    }
}
