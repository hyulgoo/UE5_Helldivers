// Fill out your copyright notice in the Description page of Project Settings.

#include "Stratagem/HDStratagemSettings.h"

UHDStratagemSettings::UHDStratagemSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Loadout = { TEXT("OrbitalPrecisionStrike"), TEXT("EagleAirstrike"), TEXT("Resupply"), TEXT("MachineGunSentry") };
}

FName UHDStratagemSettings::GetCategoryName() const
{
    return TEXT("Game");
}
