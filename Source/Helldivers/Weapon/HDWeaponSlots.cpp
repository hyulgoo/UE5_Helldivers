// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HDWeaponSlots.h"
#include "AbilitySystemComponent.h"
#include "Attribute/HDWeaponAttributeSet.h"

FHDWeaponPickupResult FHDWeaponSlots::Pickup(const bool bIsSupportWeapon)
{
    FHDWeaponPickupResult Result;
    Result.Slot = bIsSupportWeapon ? EHDWeaponSlot::Support : EHDWeaponSlot::Primary;

    bool& bHasSlotWeapon = bIsSupportWeapon ? bHasSupport : bHasPrimary;
    Result.bReplaced    = bHasSlotWeapon;
    bHasSlotWeapon      = true;

    ActiveSlot = Result.Slot;
    return Result;
}

bool FHDWeaponSlots::Swap()
{
    const EHDWeaponSlot OtherSlot = ActiveSlot == EHDWeaponSlot::Primary ? EHDWeaponSlot::Support : EHDWeaponSlot::Primary;
    if (HasWeapon(OtherSlot) == false)
    {
        return false;
    }

    ActiveSlot = OtherSlot;
    return true;
}

bool FHDWeaponSlots::HasWeapon(const EHDWeaponSlot Slot) const
{
    return Slot == EHDWeaponSlot::Primary ? bHasPrimary : bHasSupport;
}

void FHDWeaponSlots::Reset()
{
    bHasPrimary = false;
    bHasSupport = false;
    ActiveSlot  = EHDWeaponSlot::Primary;
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
