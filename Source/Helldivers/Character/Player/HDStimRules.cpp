// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Player/HDStimRules.h"

EHDStimRefusal HDStim::CheckUse(float StimCount, float Health, float MaxHealth, bool bDead, bool bActive)
{
    if (bDead)
    {
        return EHDStimRefusal::Dead;
    }

    if (bActive)
    {
        return EHDStimRefusal::Active;
    }

    if (StimCount < 1.f)
    {
        return EHDStimRefusal::NoStim;
    }

    if (Health >= MaxHealth)
    {
        return EHDStimRefusal::FullHealth;
    }

    return EHDStimRefusal::None;
}

float HDStim::HealStep(float Health, float MaxHealth, float HealPerSecond, float StepSeconds)
{
    const float HealAmount = FMath::Max(HealPerSecond, 0.f) * FMath::Max(StepSeconds, 0.f);

    return FMath::Clamp(Health + HealAmount, 0.f, FMath::Max(MaxHealth, 0.f));
}

const TCHAR* HDStim::GetRefusalName(EHDStimRefusal Refusal)
{
    switch (Refusal)
    {
    case EHDStimRefusal::None:          return TEXT("none");
    case EHDStimRefusal::Dead:          return TEXT("dead");
    case EHDStimRefusal::Active:        return TEXT("active");
    case EHDStimRefusal::NoStim:        return TEXT("no stims");
    case EHDStimRefusal::FullHealth:    return TEXT("full health");
    default:                            return TEXT("unknown");
    }
}
