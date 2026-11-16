// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/HDEnemyAITypes.h"

namespace HDEnemyAIStateTest
{
    FHDEnemyAIInput MakeInput(const bool bHasTarget, const float DistanceToTarget, const float AttackRange, const bool bAttackReady)
    {
        FHDEnemyAIInput Input;
        Input.bHasTarget        = bHasTarget;
        Input.DistanceToTarget  = DistanceToTarget;
        Input.AttackRange       = AttackRange;
        Input.bAttackReady      = bAttackReady;
        return Input;
    }

    FString ToString(const EHDEnemyAIState State)
    {
        return UEnum::GetValueAsString(State);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDEnemyAIStateTest, "Helldivers.AI.DecideState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDEnemyAIStateTest::RunTest(const FString& Parameters)
{
    using namespace HDEnemyAIStateTest;

    // 죽음이 우선: 대상 있음, 사거리 안, 준비됨
    {
        FHDEnemyAIInput Input = MakeInput(true, 150.f, 180.f, true);
        Input.bIsDead = true;
        TestEqual(TEXT("Dead has priority"), ToString(HDEnemyAI::DecideState(Input)), ToString(EHDEnemyAIState::Dead));
    }

    // 공격 중 유지: 대상이 사거리 밖으로 감(공격 직후라 준비 안 됨)
    {
        FHDEnemyAIInput Input = MakeInput(true, 500.f, 180.f, false);
        Input.bIsAttacking = true;
        TestEqual(TEXT("Attacking keeps Attack"), ToString(HDEnemyAI::DecideState(Input)), ToString(EHDEnemyAIState::Attack));
    }

    // 대상 없음: 거리·준비 값이 Attack 조건이어도 Idle
    {
        const FHDEnemyAIInput Input = MakeInput(false, 0.f, 180.f, true);
        TestEqual(TEXT("No target"), ToString(HDEnemyAI::DecideState(Input)), ToString(EHDEnemyAIState::Idle));
    }

    // 사거리 안 + 준비
    {
        const FHDEnemyAIInput Input = MakeInput(true, 150.f, 180.f, true);
        TestEqual(TEXT("In range and ready"), ToString(HDEnemyAI::DecideState(Input)), ToString(EHDEnemyAIState::Attack));
    }

    // 사거리 안 + 쿨다운
    {
        const FHDEnemyAIInput Input = MakeInput(true, 150.f, 180.f, false);
        TestEqual(TEXT("In range and cooling down"), ToString(HDEnemyAI::DecideState(Input)), ToString(EHDEnemyAIState::Chase));
    }

    // 사거리 밖
    {
        const FHDEnemyAIInput Input = MakeInput(true, 500.f, 180.f, true);
        TestEqual(TEXT("Out of range"), ToString(HDEnemyAI::DecideState(Input)), ToString(EHDEnemyAIState::Chase));
    }

    // 경계값: 거리 = 사거리
    {
        const FHDEnemyAIInput Input = MakeInput(true, 180.f, 180.f, true);
        TestEqual(TEXT("Distance equals range"), ToString(HDEnemyAI::DecideState(Input)), ToString(EHDEnemyAIState::Attack));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
