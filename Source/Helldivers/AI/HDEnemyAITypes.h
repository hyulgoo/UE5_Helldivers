// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HDEnemyAITypes.generated.h"

UENUM(BlueprintType)
enum class EHDEnemyAIState : uint8
{
    Idle,
    Chase,
    Attack,
    Dead,
    Hold,
};

// 상태 판정 입력. 컨트롤러가 매 Tick 채운다
struct FHDEnemyAIInput
{
    bool    bIsDead             = false;
    bool    bHasTarget          = false;
    float   DistanceToTarget    = 0.f;
    float   AttackRange         = 0.f;
    bool    bAttackReady        = false;
    bool    bIsAttacking        = false;
    bool    bHasLineOfSight     = true;
    float   PreferredRange      = 0.f;
};

namespace HDEnemyAI
{
    // 위에서부터 먼저 맞는 규칙: 죽음 -> Dead, 공격 중 -> Attack, 대상 없음 -> Idle, 사거리 안 + 준비 + 시야 -> Attack,
    // 선호 거리(PreferredRange > 0) 안 + 시야 -> Hold, 그 밖 -> Chase
    HELLDIVERS_API EHDEnemyAIState DecideState(const FHDEnemyAIInput& Input);
}
