// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/HDEnemyAITypes.h"

EHDEnemyAIState HDEnemyAI::DecideState(const FHDEnemyAIInput& Input)
{
    if (Input.bIsDead)
    {
        return EHDEnemyAIState::Dead;
    }

    if (Input.bIsAttacking)
    {
        return EHDEnemyAIState::Attack;
    }

    if (Input.bHasTarget == false)
    {
        return EHDEnemyAIState::Idle;
    }

    if (Input.DistanceToTarget <= Input.AttackRange && Input.bAttackReady && Input.bHasLineOfSight)
    {
        return EHDEnemyAIState::Attack;
    }

    if (Input.PreferredRange > 0.f && Input.DistanceToTarget <= Input.PreferredRange && Input.bHasLineOfSight)
    {
        return EHDEnemyAIState::Hold;
    }

    return EHDEnemyAIState::Chase;
}
