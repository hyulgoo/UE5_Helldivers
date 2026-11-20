// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FHDStratagemEffectData;

// 스트라타젬 효과 데이터(FHDStratagemEffectData)에서 발동 이벤트·투사체 수·낙하 위치·간격을 계산한다.
// 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Stratagem.Activation)로 고정한다
namespace HDStratagemPattern
{
    // 전함 GA 를 깨우는 이벤트. Kind 가 Resupply 면 Event.Stratagem.Resupply.
    // Projectile 이면 bIsEagle 로 Event.Stratagem.Eagle, 아니면 Event.Stratagem.Orbital
    HELLDIVERS_API FGameplayTag GetActivationEventTag(const FHDStratagemEffectData& Data);

    // ProjectileDropLocation 이 있으면 그 개수, 없으면 SpecifyProjectileSpawnCount
    HELLDIVERS_API int32        GetProjectileCount(const FHDStratagemEffectData& Data);

    // Index 번째 투사체의 표적 기준 XY 오프셋.
    // bUseRandomRange 면 반지름 RandomPositionRange 원 안 균등 분포, 아니면 ProjectileDropLocation[Index](범위 밖이면 0)
    HELLDIVERS_API FVector2D    GetDropOffset(const FHDStratagemEffectData& Data, int32 Index, FRandomStream& RandomStream);

    // 궤도 포격의 투사체 사이 간격(초) = OrbitalDuration / 개수. 개수가 0 이면 0
    HELLDIVERS_API float        GetShotInterval(const FHDStratagemEffectData& Data);
}
