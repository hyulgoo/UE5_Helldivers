// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/HDEnemyAITypes.h"
#include "HDEnemyAIController.generated.h"

class AHDCharacterNonPlayer;

// 적 AI: 대상 갱신 -> FHDEnemyAIInput -> HDEnemyAI::DecideState -> 상태별 동작
UCLASS()
class HELLDIVERS_API AHDEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
    AHDEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    void                        OnControlledPawnDead();
    EHDEnemyAIState             GetCurrentState() const { return CurrentState; }

    // 콘솔 변수 hd.AI.Debug 가 1 인가
    static bool                 IsDebugEnabled();

protected:
    virtual void                Tick(float DeltaSeconds) override final;

private:
    void                        UpdateTarget(const AHDCharacterNonPlayer& Enemy);
    FHDEnemyAIInput             MakeInput(const AHDCharacterNonPlayer& Enemy) const;
    void                        EnterState(const EHDEnemyAIState NewState);
    void                        TickState(const AHDCharacterNonPlayer& Enemy, const float DeltaSeconds);
    void                        DrawDebugState(AHDCharacterNonPlayer& Enemy, const float Duration) const;

    static bool                 IsTargetDead(AActor* Actor);

private:
    TWeakObjectPtr<APawn>       Target;

    EHDEnemyAIState             CurrentState = EHDEnemyAIState::Idle;

    double                      LastAttackTime = 0.0;
    bool                        bHasAttacked = false;
    bool                        bIsPawnDead = false;

    // Chase: MoveToActor 가 Failed 면 직선 추적으로 바꾸고, 길찾기 요청이 성공하면 되돌린다
    bool                        bDirectChase = false;
    float                       MoveRequestElapsed = 0.f;

    static constexpr float      MoveRequestInterval = 0.5f;
    static constexpr float      AttackRecoveryTime = 0.35f;
    static constexpr float      DebugTextHeight = 130.f;
};
