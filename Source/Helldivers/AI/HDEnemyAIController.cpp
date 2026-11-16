// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/HDEnemyAIController.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "AbilitySystem/GameAbility/HDGA_EnemyMeleeAttack.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "Define/HDGameplayTag.h"
#include "Define/HDDefine.h"

static TAutoConsoleVariable<int32> CVarHDAIDebug(
    TEXT("hd.AI.Debug"),
    0,
    TEXT("1: draw enemy AI state above head and melee hit sphere"),
    ECVF_Cheat);

AHDEnemyAIController::AHDEnemyAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

bool AHDEnemyAIController::IsDebugEnabled()
{
    return CVarHDAIDebug.GetValueOnGameThread() == 1;
}

void AHDEnemyAIController::OnControlledPawnDead()
{
    bIsPawnDead = true;
    EnterState(EHDEnemyAIState::Dead);
}

void AHDEnemyAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    AHDCharacterNonPlayer* Enemy = Cast<AHDCharacterNonPlayer>(GetPawn());
    NULL_CHECK_WITHOUT_LOG(Enemy);

    UpdateTarget(*Enemy);

    const EHDEnemyAIState NewState = HDEnemyAI::DecideState(MakeInput(*Enemy));
    if (NewState != CurrentState)
    {
        EnterState(NewState);
    }

    TickState(*Enemy, DeltaSeconds);

    if (IsDebugEnabled() && CurrentState != EHDEnemyAIState::Dead)
    {
        DrawDebugState(*Enemy, 0.f);
    }
}

void AHDEnemyAIController::UpdateTarget(const AHDCharacterNonPlayer& Enemy)
{
    const FHDEnemyStats& Stats = Enemy.GetEnemyStats();
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const bool bPlayerAlive = IsValid(PlayerPawn) && IsTargetDead(PlayerPawn) == false;

    // 대상이 LoseTargetRadius 밖이거나 죽으면 놓는다
    if (Target.IsValid())
    {
        const bool bKeepTarget = bPlayerAlive && Target.Get() == PlayerPawn && FVector::Dist(Enemy.GetActorLocation(), PlayerPawn->GetActorLocation()) <= Stats.LoseTargetRadius;
        if (bKeepTarget == false)
        {
            Target.Reset();
        }
    }

    // 이 브리프의 대상 찾기: 거리 기준
    if (Target.IsValid() == false && bPlayerAlive && FVector::Dist(Enemy.GetActorLocation(), PlayerPawn->GetActorLocation()) <= Stats.AggroRadius)
    {
        Target = PlayerPawn;
    }
}

FHDEnemyAIInput AHDEnemyAIController::MakeInput(const AHDCharacterNonPlayer& Enemy) const
{
    const FHDEnemyStats& Stats = Enemy.GetEnemyStats();
    const double ElapsedSinceAttack = GetWorld()->GetTimeSeconds() - LastAttackTime;

    FHDEnemyAIInput Input;
    Input.bIsDead           = bIsPawnDead || Enemy.IsDead();
    Input.bHasTarget        = Target.IsValid();
    Input.DistanceToTarget  = Target.IsValid() ? FVector::Dist(Enemy.GetActorLocation(), Target->GetActorLocation()) : 0.f;
    Input.AttackRange       = Stats.AttackRange;
    Input.bAttackReady      = bHasAttacked == false || ElapsedSinceAttack >= Stats.AttackCooldown;
    Input.bIsAttacking      = bHasAttacked && ElapsedSinceAttack < Stats.AttackWindup + AttackRecoveryTime;
    return Input;
}

void AHDEnemyAIController::EnterState(const EHDEnemyAIState NewState)
{
    CONDITION_CHECK_WITHOUT_LOG(CurrentState != NewState);
    CurrentState = NewState;

    switch (NewState)
    {
    case EHDEnemyAIState::Idle:
        StopMovement();
        bDirectChase = false;
        break;

    case EHDEnemyAIState::Chase:
        // 들어가자마자 한 번 요청한다
        MoveRequestElapsed = MoveRequestInterval;
        break;

    case EHDEnemyAIState::Attack:
    {
        StopMovement();

        UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(GetPawn());
        if (ASC)
        {
            ASC->TryActivateAbilityByClass(UHDGA_EnemyMeleeAttack::StaticClass());
        }

        bHasAttacked = true;
        LastAttackTime = GetWorld()->GetTimeSeconds();
        break;
    }

    case EHDEnemyAIState::Dead:
    {
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        SetActorTickEnabled(false);

        // Tick 을 끄므로 Dead 표시는 사라질 때까지 남긴다(액터가 없어지면 HUD 가 지운다)
        AHDCharacterNonPlayer* Enemy = Cast<AHDCharacterNonPlayer>(GetPawn());
        if (Enemy && IsDebugEnabled())
        {
            DrawDebugState(*Enemy, 30.f);
        }
        break;
    }

    default:
        break;
    }
}

void AHDEnemyAIController::TickState(const AHDCharacterNonPlayer& Enemy, const float DeltaSeconds)
{
    switch (CurrentState)
    {
    case EHDEnemyAIState::Chase:
    {
        NULL_CHECK_WITHOUT_LOG(Target.Get());

        SetFocus(Target.Get());

        MoveRequestElapsed += DeltaSeconds;
        if (MoveRequestElapsed >= MoveRequestInterval)
        {
            MoveRequestElapsed = 0.f;

            const EPathFollowingRequestResult::Type Result = MoveToActor(Target.Get(), Enemy.GetEnemyStats().AttackRange * 0.8f);
            if (Result == EPathFollowingRequestResult::Failed)
            {
                bDirectChase = true;
            }
            else if (Result == EPathFollowingRequestResult::RequestSuccessful)
            {
                bDirectChase = false;
            }
        }

        if (bDirectChase)
        {
            const FVector Direction = (Target->GetActorLocation() - Enemy.GetActorLocation()).GetSafeNormal2D();
            GetPawn()->AddMovementInput(Direction, 1.f);
        }
        break;
    }

    case EHDEnemyAIState::Attack:
        if (Target.IsValid())
        {
            SetFocus(Target.Get());
        }
        break;

    default:
        break;
    }
}

void AHDEnemyAIController::DrawDebugState(AHDCharacterNonPlayer& Enemy, const float Duration) const
{
    FString StateText = StaticEnum<EHDEnemyAIState>()->GetNameStringByValue(static_cast<int64>(CurrentState));
    if (CurrentState == EHDEnemyAIState::Chase && bDirectChase)
    {
        StateText += TEXT("(direct)");
    }

    DrawDebugString(GetWorld(), FVector(0.f, 0.f, DebugTextHeight), StateText, &Enemy, FColor::Yellow, Duration);
}

bool AHDEnemyAIController::IsTargetDead(AActor* Actor)
{
    UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Actor);
    return ASC && ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead);
}
