// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AI/HDEnemyAITypes.h"
#include "HDEnemyAIController.generated.h"

class AHDCharacterNonPlayer;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

// 적 AI: 인지(시야·청각·근접) -> 대상 갱신 -> FHDEnemyAIInput -> HDEnemyAI::DecideState -> 상태별 동작
UCLASS()
class HELLDIVERS_API AHDEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
    AHDEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    void                        OnControlledPawnDead();
    EHDEnemyAIState             GetCurrentState() const { return CurrentState; }
    bool                        IsAlerted() const { return bAlerted; }
    AActor*                     GetTargetActor() const { return Target.Get(); }

    // 콘솔 변수 hd.AI.Debug 가 1 인가
    static bool                 IsDebugEnabled();

protected:
    virtual void                Tick(float DeltaSeconds) override final;
    virtual void                OnPossess(APawn* InPawn) override;

private:
    UFUNCTION()
    void                        OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    void                        AcquireTarget(APawn* NewTarget, const TCHAR* Reason);
    void                        UpdateTarget(const AHDCharacterNonPlayer& Enemy);
    void                        UpdateLineOfSight(const AHDCharacterNonPlayer& Enemy, const float DeltaSeconds);
    FHDEnemyAIInput             MakeInput(const AHDCharacterNonPlayer& Enemy) const;
    void                        EnterState(const EHDEnemyAIState NewState);
    void                        TickState(const AHDCharacterNonPlayer& Enemy, const float DeltaSeconds);
    void                        DrawDebugState(AHDCharacterNonPlayer& Enemy, const float Duration) const;

    static bool                 IsTargetDead(AActor* Actor);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UAIPerceptionComponent>  Perception;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight>    SightConfig;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Hearing>  HearingConfig;

    TWeakObjectPtr<APawn>       Target;

    // 마지막으로 대상을 감지한 시각(시야·청각·근접)
    double                      LastSensedTime = 0.0;
    bool                        bAlerted = false;

    // 마지막으로 들은 소음 위치
    FVector                     LastNoiseLocation = FVector::ZeroVector;
    bool                        bHasNoiseLocation = false;

    // 시야(LOS): LineOfSightInterval 마다 눈 -> 대상 가슴 선
    bool                        bHasLineOfSight = false;
    float                       LineOfSightElapsed = 0.f;

    EHDEnemyAIState             CurrentState = EHDEnemyAIState::Idle;

    double                      LastAttackTime = 0.0;
    bool                        bHasAttacked = false;
    bool                        bIsPawnDead = false;

    // Chase: MoveToActor 가 Failed 면 직선 추적으로 바꾸고, 길찾기 요청이 성공하면 되돌린다
    bool                        bDirectChase = false;
    float                       MoveRequestElapsed = 0.f;

    static constexpr float      MoveRequestInterval = 0.5f;
    static constexpr float      AttackEndPadding = 0.2f;
    static constexpr float      LineOfSightInterval = 0.2f;
    static constexpr float      TargetChestHeight = 40.f;
    static constexpr float      LoseSightExtraRadius = 1000.f;
    static constexpr float      DebugTextHeight = 130.f;
};
