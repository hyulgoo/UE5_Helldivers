// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HDWeaponSlots.h"
#include "AbilitySystemComponent.h"
#include "Attribute/HDWeaponAttributeSet.h"

const TCHAR* GetWeaponSlotName(const EHDWeaponSlot Slot)
{
    switch (Slot)
    {
    case EHDWeaponSlot::Primary:    return TEXT("Primary");
    case EHDWeaponSlot::Secondary:  return TEXT("Secondary");
    case EHDWeaponSlot::Support:    return TEXT("Support");
    case EHDWeaponSlot::Grenade:    return TEXT("Grenade");
    default:                        return TEXT("None");
    }
}

FHDWeaponPickupResult FHDWeaponSlots::Pickup(const EHDWeaponSlot Slot, const bool bMakeActive)
{
    FHDWeaponPickupResult Result;
    Result.Slot = Slot;

    bool* bHasSlotWeapon = FindGunSlotFlag(Slot);
    if (bHasSlotWeapon == nullptr)
    {
        return Result;
    }

    Result.bReplaced    = *bHasSlotWeapon;
    *bHasSlotWeapon     = true;

    if (bMakeActive)
    {
        SetActiveSlot(Slot);
    }

    return Result;
}

EHDWeaponSelectResult FHDWeaponSlots::Select(const EHDWeaponSlot Slot)
{
    if (Slot == ActiveSlot)
    {
        return EHDWeaponSelectResult::AlreadyActive;
    }

    if (HasWeapon(Slot) == false)
    {
        return EHDWeaponSelectResult::Empty;
    }

    SetActiveSlot(Slot);
    return EHDWeaponSelectResult::Selected;
}

bool FHDWeaponSlots::HasWeapon(const EHDWeaponSlot Slot) const
{
    switch (Slot)
    {
    case EHDWeaponSlot::Primary:    return bHasPrimary;
    case EHDWeaponSlot::Secondary:  return bHasSecondary;
    case EHDWeaponSlot::Support:    return bHasSupport;
    case EHDWeaponSlot::Grenade:    return bGrenadeAvailable;
    default:                        return false;
    }
}

void FHDWeaponSlots::SetGrenadeAvailable(const bool bAvailable)
{
    bGrenadeAvailable = bAvailable;
}

void FHDWeaponSlots::Reset()
{
    bHasPrimary         = false;
    bHasSecondary       = false;
    bHasSupport         = false;
    bGrenadeAvailable   = false;
    ActiveSlot          = EHDWeaponSlot::Primary;
    LastGunSlot         = EHDWeaponSlot::Primary;
}

bool* FHDWeaponSlots::FindGunSlotFlag(const EHDWeaponSlot Slot)
{
    switch (Slot)
    {
    case EHDWeaponSlot::Primary:    return &bHasPrimary;
    case EHDWeaponSlot::Secondary:  return &bHasSecondary;
    case EHDWeaponSlot::Support:    return &bHasSupport;
    default:                        return nullptr;
    }
}

void FHDWeaponSlots::SetActiveSlot(const EHDWeaponSlot Slot)
{
    ActiveSlot = Slot;
    if (Slot != EHDWeaponSlot::Grenade)
    {
        LastGunSlot = Slot;
    }
}

namespace HDWeaponAmmo
{
    FHDWeaponAmmoData ReadFromAttributes(const UAbilitySystemComponent* ASC)
    {
        FHDWeaponAmmoData AmmoData;
        if (ASC == nullptr)
        {
            return AmmoData;
        }

        AmmoData.Ammo           = FMath::RoundToInt32(ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetAmmoAttribute()));
        AmmoData.MaxAmmo        = FMath::RoundToInt32(ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxAmmoAttribute()));
        AmmoData.Capacity       = FMath::RoundToInt32(ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetCapacityAttribute()));
        AmmoData.MaxCapacity    = FMath::RoundToInt32(ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxCapacityAttribute()));
        return AmmoData;
    }
}

namespace HDWeaponMontageEvent
{
    float GetReloadEventDelay(const float WeaponSectionLength, const float BodyMontageLength)
    {
        if (WeaponSectionLength <= 0.f)
        {
            return 0.f;
        }

        if (BodyMontageLength <= 0.f || WeaponSectionLength < BodyMontageLength)
        {
            return WeaponSectionLength;
        }

        return FMath::Max(BodyMontageLength - ReloadEventMargin, 0.f);
    }

    bool IsFireReady(const double LastFireTime, const double Now, const float FireDelay)
    {
        if (LastFireTime < 0.0)
        {
            return true;
        }

        return Now - LastFireTime >= static_cast<double>(FireDelay);
    }
}
