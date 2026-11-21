// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HDArmorRules.h"

namespace HDArmor
{
    struct FZonePrefix
    {
        const TCHAR*    Prefix;
        EHDHitZone      Zone;
    };

    // 소문자 본 이름의 접두사(SKM_Manny 의 피직스 바디 이름 기준). 여기 없는 이름은 Body
    const FZonePrefix ZonePrefixes[] =
    {
        { TEXT("head"),     EHDHitZone::Head },
        { TEXT("neck"),     EHDHitZone::Head },
        { TEXT("upperarm"), EHDHitZone::Arm },
        { TEXT("lowerarm"), EHDHitZone::Arm },
        { TEXT("hand"),     EHDHitZone::Arm },
        { TEXT("clavicle"), EHDHitZone::Arm },
        { TEXT("thigh"),    EHDHitZone::Leg },
        { TEXT("calf"),     EHDHitZone::Leg },
        { TEXT("foot"),     EHDHitZone::Leg },
        { TEXT("ball"),     EHDHitZone::Leg },
    };

    int32 GetZoneArmor(const FHDArmorProfile& Profile, const EHDHitZone Zone)
    {
        switch (Zone)
        {
        case EHDHitZone::Head:  return Profile.HeadArmor;
        case EHDHitZone::Arm:   return Profile.ArmArmor;
        case EHDHitZone::Leg:   return Profile.LegArmor;
        default:                return Profile.BodyArmor;
        }
    }

    float GetZoneDamageMultiplier(const FHDArmorProfile& Profile, const EHDHitZone Zone)
    {
        switch (Zone)
        {
        case EHDHitZone::Head:  return Profile.HeadDamageMultiplier;
        case EHDHitZone::Arm:   return Profile.ArmDamageMultiplier;
        case EHDHitZone::Leg:   return Profile.LegDamageMultiplier;
        default:                return Profile.BodyDamageMultiplier;
        }
    }

    float GetOutcomeDamageRatio(const EHDHitOutcome Outcome)
    {
        switch (Outcome)
        {
        case EHDHitOutcome::Full:       return 1.f;
        case EHDHitOutcome::Reduced:    return ReducedDamageRatio;
        default:                        return 0.f;
        }
    }
}

EHDHitZone HDArmor::ZoneFromBone(FName BoneName)
{
    if (BoneName.IsNone())
    {
        return EHDHitZone::Body;
    }

    const FString LowerBoneName = BoneName.ToString().ToLower();
    for (const FZonePrefix& ZonePrefix : ZonePrefixes)
    {
        if (LowerBoneName.StartsWith(ZonePrefix.Prefix, ESearchCase::CaseSensitive))
        {
            return ZonePrefix.Zone;
        }
    }

    return EHDHitZone::Body;
}

EHDHitOutcome HDArmor::ResolveOutcome(int32 Penetration, int32 Armor)
{
    if (Penetration > Armor)
    {
        return EHDHitOutcome::Full;
    }

    return Penetration == Armor ? EHDHitOutcome::Reduced : EHDHitOutcome::Ricochet;
}

int32 HDArmor::ApplyIncidence(int32 Penetration, float CosIncidence)
{
    if (CosIncidence < GrazingCos)
    {
        return FMath::Max(Penetration - 1, 1);
    }

    return Penetration;
}

FHDHitResolution HDArmor::ResolveHit(const FHDArmorProfile& Profile, FName BoneName, int32 Penetration, float BaseDamage)
{
    FHDHitResolution Resolution;
    Resolution.Zone         = ZoneFromBone(BoneName);
    Resolution.Penetration  = Penetration;
    Resolution.Armor        = GetZoneArmor(Profile, Resolution.Zone);
    Resolution.Outcome      = ResolveOutcome(Resolution.Penetration, Resolution.Armor);
    Resolution.Damage       = FMath::Max(BaseDamage, 0.f) * GetZoneDamageMultiplier(Profile, Resolution.Zone) * GetOutcomeDamageRatio(Resolution.Outcome);
    return Resolution;
}

FHDHitResolution HDArmor::ResolveExplosion(const FHDArmorProfile& Profile, int32 Penetration, float BaseDamage)
{
    FHDHitResolution Resolution;
    Resolution.Zone         = EHDHitZone::Body;
    Resolution.Penetration  = Penetration;
    Resolution.Armor        = Profile.BodyArmor;
    Resolution.Outcome      = ResolveOutcome(Resolution.Penetration, Resolution.Armor);
    Resolution.Damage       = FMath::Max(BaseDamage, 0.f) * GetOutcomeDamageRatio(Resolution.Outcome);
    return Resolution;
}

const TCHAR* HDArmor::GetZoneName(EHDHitZone Zone)
{
    switch (Zone)
    {
    case EHDHitZone::Body:  return TEXT("Body");
    case EHDHitZone::Head:  return TEXT("Head");
    case EHDHitZone::Arm:   return TEXT("Arm");
    case EHDHitZone::Leg:   return TEXT("Leg");
    }

    return TEXT("Unknown");
}

const TCHAR* HDArmor::GetOutcomeName(EHDHitOutcome Outcome)
{
    switch (Outcome)
    {
    case EHDHitOutcome::Full:       return TEXT("Full");
    case EHDHitOutcome::Reduced:    return TEXT("Reduced");
    case EHDHitOutcome::Ricochet:   return TEXT("Ricochet");
    }

    return TEXT("Unknown");
}
