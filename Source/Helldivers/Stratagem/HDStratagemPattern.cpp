// Fill out your copyright notice in the Description page of Project Settings.

#include "Stratagem/HDStratagemPattern.h"
#include "GameData/HDStratagemEffectData.h"
#include "Define/HDGameplayTag.h"

namespace HDStratagemPattern
{
    FGameplayTag GetActivationEventTag(const FHDStratagemEffectData& Data)
    {
        if (Data.Kind == EHDStratagemKind::Resupply)
        {
            return Tag_Event_Stratagem_Resupply.GetTag();
        }

        if (Data.Kind == EHDStratagemKind::Sentry)
        {
            return Tag_Event_Stratagem_Sentry.GetTag();
        }

        if (Data.Kind == EHDStratagemKind::SupportWeapon)
        {
            return Tag_Event_Stratagem_SupportWeapon.GetTag();
        }

        // Projectile: bIsEagle 로 이글·궤도를 나눈다
        return Data.bIsEagle ? Tag_Event_Stratagem_Eagle.GetTag() : Tag_Event_Stratagem_Orbital.GetTag();
    }

    int32 GetProjectileCount(const FHDStratagemEffectData& Data)
    {
        return Data.ProjectileDropLocation.IsEmpty() ? Data.SpecifyProjectileSpawnCount : Data.ProjectileDropLocation.Num();
    }

    FVector2D GetDropOffset(const FHDStratagemEffectData& Data, int32 Index, FRandomStream& RandomStream)
    {
        if (Data.bUseRandomRange)
        {
            // 반지름에 Sqrt 를 씌워야 원 안에 고르게 퍼진다(바꾸기 전 AHDBattleShip::OrbitalStrikeWithDelay 와 같은 식)
            const float Angle    = RandomStream.FRandRange(0.f, 2.f * PI);
            const float Distance = FMath::Sqrt(RandomStream.FRand()) * Data.RandomPositionRange;
            return FVector2D(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance);
        }

        return Data.ProjectileDropLocation.IsValidIndex(Index) ? Data.ProjectileDropLocation[Index] : FVector2D::ZeroVector;
    }

    float GetShotInterval(const FHDStratagemEffectData& Data)
    {
        const int32 Count = GetProjectileCount(Data);
        return Count > 0 ? Data.OrbitalDuration / Count : 0.f;
    }
}
