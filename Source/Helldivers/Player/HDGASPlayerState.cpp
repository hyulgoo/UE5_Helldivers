// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/HDGASPlayerState.h"
#include "Component/HDAbilitySystemComponent.h"

AHDGASPlayerState::AHDGASPlayerState(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AbilitySystemComponent = CreateDefaultSubobject<UHDAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* AHDGASPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AHDGASPlayerState::AddCarriedSamples(const FHDSampleCounts& Samples)
{
    CarriedSamples.Common   = FMath::Max(0, CarriedSamples.Common + Samples.Common);
    CarriedSamples.Rare     = FMath::Max(0, CarriedSamples.Rare + Samples.Rare);
    CarriedSamples.Super    = FMath::Max(0, CarriedSamples.Super + Samples.Super);
}

FHDSampleCounts AHDGASPlayerState::TakeCarriedSamples()
{
    const FHDSampleCounts Taken = CarriedSamples;
    CarriedSamples = FHDSampleCounts();
    return Taken;
}
