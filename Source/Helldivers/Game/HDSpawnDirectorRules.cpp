// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/HDSpawnDirectorRules.h"

namespace HDSpawnRules
{
    int32 ClampSpawnCount(int32 Requested, int32 Alive, int32 MaxAlive)
    {
        return FMath::Max(0, FMath::Min(Requested, MaxAlive - Alive));
    }

    int32 ShooterCount(int32 GroupSize, float ShooterRatio)
    {
        return FMath::RoundToInt(GroupSize * FMath::Clamp(ShooterRatio, 0.f, 1.f));
    }

    bool ShouldBreach(float AlertedSeconds, float BreachDelay, float SecondsSinceLastBreach, float BreachCooldown)
    {
        return AlertedSeconds >= BreachDelay && SecondsSinceLastBreach >= BreachCooldown;
    }

    int32 ScaledCount(int32 Base, float Intensity)
    {
        return FMath::CeilToInt(Base * FMath::Max(Intensity, 1.f));
    }

    float ScaledInterval(float BaseSeconds, float Intensity)
    {
        return BaseSeconds / FMath::Max(Intensity, 1.f);
    }
}
