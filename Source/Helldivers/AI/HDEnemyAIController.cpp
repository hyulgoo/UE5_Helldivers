// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/HDEnemyAIController.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "AbilitySystem/GameAbility/HDGA_EnemyMeleeAttack.h"
#include "AbilitySystem/GameAbility/HDGA_EnemyRangedAttack.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "Character/NPC/HDEnemyShooter.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Define/HDGameplayTag.h"
#include "Define/HDDefine.h"

static TAutoConsoleVariable<int32> CVarHDAIDebug(
    TEXT("hd.AI.Debug"),
    0,
    TEXT("1: draw enemy AI state and health above head and melee hit sphere"),
    ECVF_Cheat);

namespace HDEnemyAIControllerPrivate
{
    // 시야각(반각)
    constexpr float SightPeripheralVisionAngleDegrees = 70.f;

    void SetDetectAllAffiliations(FAISenseAffiliationFilter& Filter)
    {
        Filter.bDetectEnemies = true;
        Filter.bDetectNeutrals = true;
        Filter.bDetectFriendlies = true;
    }
}

AHDEnemyAIController::AHDEnemyAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // 인지: 시야(우세) + 청각. 반경은 OnPossess 에서 폰의 Stats 로 다시 넣는다
    const FHDEnemyStats DefaultStats;

    Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = DefaultStats.SightRadius;
    SightConfig->LoseSightRadius = DefaultStats.SightRadius + LoseSightExtraRadius;
    SightConfig->PeripheralVisionAngleDegrees = HDEnemyAIControllerPrivate::SightPeripheralVisionAngleDegrees;
    HDEnemyAIControllerPrivate::SetDetectAllAffiliations(SightConfig->DetectionByAffiliation);
    SightConfig->SetMaxAge(DefaultStats.LoseTargetTime);

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = DefaultStats.HearingRange;
    HDEnemyAIControllerPrivate::SetDetectAllAffiliations(HearingConfig->DetectionByAffiliation);

    Perception->ConfigureSense(*SightConfig);
    Perception->ConfigureSense(*HearingConfig);
    Perception->SetDominantSense(SightConfig->GetSenseImplementation());
    SetPerceptionComponent(*Perception);
}

void AHDEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AHDEnemyAIController::OnTargetPerceptionUpdated);

    const AHDCharacterNonPlayer* Enemy = Cast<AHDCharacterNonPlayer>(InPawn);
    NULL_CHECK(Enemy);

    const FHDEnemyStats& Stats = Enemy->GetEnemyStats();
    SightConfig->SightRadius = Stats.SightRadius;
    SightConfig->LoseSightRadius = Stats.SightRadius + LoseSightExtraRadius;
    SightConfig->SetMaxAge(Stats.LoseTargetTime);
    HearingConfig->HearingRange = Stats.HearingRange;

    Perception->ConfigureSense(*SightConfig);
    Perception->ConfigureSense(*HearingConfig);
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
    UpdateLineOfSight(*Enemy, DeltaSeconds);

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

void AHDEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (bIsPawnDead || Stimulus.WasSuccessfullySensed() == false)
    {
        return;
    }

    AHDCharacterPlayer* Player = Cast<AHDCharacterPlayer>(Actor);
    if (Player == nullptr || IsTargetDead(Player))
    {
        return;
    }

    const bool bHeard = UAIPerceptionSystem::GetSenseClassForStimulus(this, Stimulus) == UAISense_Hearing::StaticClass();
    if (bHeard)
    {
        LastNoiseLocation = Stimulus.StimulusLocation;
        bHasNoiseLocation = true;
    }

    AcquireTarget(Player, bHeard ? TEXT("Hearing") : TEXT("Sight"));
}

void AHDEnemyAIController::AcquireTarget(APawn* NewTarget, const TCHAR* Reason)
{
    NULL_CHECK(NewTarget);

    if (Target.Get() != NewTarget)
    {
        Target = NewTarget;

        // 새 대상은 이번 Tick 에 바로 시야 선을 쏜다
        LineOfSightElapsed = LineOfSightInterval;
    }

    LastSensedTime = GetWorld()->GetTimeSeconds();

    // 처음 대상을 잡을 때 한 번만
    if (bAlerted == false)
    {
        bAlerted = true;
        UE_LOG(LogTemp, Display, TEXT("[HDEnemy] %s alerted (%s)"), *GetNameSafe(GetPawn()), Reason);
    }
}

void AHDEnemyAIController::UpdateTarget(const AHDCharacterNonPlayer& Enemy)
{
    const FHDEnemyStats& Stats = Enemy.GetEnemyStats();
    const double Now = GetWorld()->GetTimeSeconds();

    // AutoAwareRadius 안의 살아 있는 플레이어는 시야 밖이어도 안다
    AHDCharacterPlayer* Player = Cast<AHDCharacterPlayer>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (Player && IsTargetDead(Player) == false && FVector::Dist(Enemy.GetActorLocation(), Player->GetActorLocation()) <= Stats.AutoAwareRadius)
    {
        AcquireTarget(Player, TEXT("Proximity"));
    }

    NULL_CHECK_WITHOUT_LOG(Target.Get());

    // 시야 자극이 살아 있는 동안은 감지 시각을 이어서 갱신한다
    if (Perception->HasActiveStimulus(*Target.Get(), UAISense::GetSenseID<UAISense_Sight>()))
    {
        LastSensedTime = Now;
    }

    // 감지가 LoseTargetTime 넘게 없거나, LoseTargetRadius 밖이거나, 대상이 죽으면 놓는다
    const bool bSenseTimedOut = Now - LastSensedTime > Stats.LoseTargetTime;
    const bool bTooFar = FVector::Dist(Enemy.GetActorLocation(), Target->GetActorLocation()) > Stats.LoseTargetRadius;
    if (bSenseTimedOut || bTooFar || IsTargetDead(Target.Get()))
    {
        Target.Reset();
        bHasLineOfSight = false;
    }
}

void AHDEnemyAIController::UpdateLineOfSight(const AHDCharacterNonPlayer& Enemy, const float DeltaSeconds)
{
    if (Target.IsValid() == false)
    {
        bHasLineOfSight = false;
        return;
    }

    LineOfSightElapsed += DeltaSeconds;
    CONDITION_CHECK_WITHOUT_LOG(LineOfSightElapsed >= LineOfSightInterval);
    LineOfSightElapsed = 0.f;

    const FVector EyeLocation = Enemy.GetPawnViewLocation();
    const FVector ChestLocation = Target->GetActorLocation() + FVector(0.f, 0.f, TargetChestHeight);

    // 적 자신과 대상(대상에 붙은 무기 액터 포함)은 무시한다
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDEnemyLineOfSight), false, &Enemy);
    QueryParams.AddIgnoredActor(Target.Get());
    TArray<AActor*> TargetAttachedActors;
    Target->GetAttachedActors(TargetAttachedActors);
    QueryParams.AddIgnoredActors(TargetAttachedActors);

    bHasLineOfSight = GetWorld()->LineTraceTestByChannel(EyeLocation, ChestLocation, ECC_Visibility, QueryParams) == false;
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
    Input.bIsAttacking      = bHasAttacked && ElapsedSinceAttack < Stats.AttackWindup + Stats.BurstCount * Stats.BurstInterval + AttackEndPadding;
    Input.bHasLineOfSight   = bHasLineOfSight;
    Input.PreferredRange    = Stats.PreferredRange;
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

    case EHDEnemyAIState::Hold:
        StopMovement();
        bDirectChase = false;
        if (Target.IsValid())
        {
            SetFocus(Target.Get());
        }
        break;

    case EHDEnemyAIState::Attack:
    {
        StopMovement();

        // 원거리형은 사격, 그 밖은 근접
        const TSubclassOf<UGameplayAbility> AttackAbilityClass = Cast<AHDEnemyShooter>(GetPawn()) ? UHDGA_EnemyRangedAttack::StaticClass() : UHDGA_EnemyMeleeAttack::StaticClass();

        UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(GetPawn());
        if (ASC)
        {
            ASC->TryActivateAbilityByClass(AttackAbilityClass);
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
    case EHDEnemyAIState::Hold:
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

    // 예: "Hold HP 80"
    UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(&Enemy);
    const float CurrentHealth = ASC ? ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()) : 0.f;
    StateText += FString::Printf(TEXT(" HP %.0f"), CurrentHealth);

    DrawDebugString(GetWorld(), FVector(0.f, 0.f, DebugTextHeight), StateText, &Enemy, FColor::Yellow, Duration);
}

bool AHDEnemyAIController::IsTargetDead(AActor* Actor)
{
    UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Actor);
    return ASC && ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead);
}
