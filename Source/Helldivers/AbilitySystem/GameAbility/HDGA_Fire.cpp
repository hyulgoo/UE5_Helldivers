// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_Fire.h"
#include "Interface/HDWeaponInterface.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "AbilitySystem/AbilityTask/HDAT_PlayMontageAndWaitForEvent.h"
#include "Define/HDMontageSectionNames.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Component/HDCombatComponent.h"
#include "Component/HDFaceAimRules.h"
#include "TimerManager.h"
#include "Component/HDMovementStateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UHDGA_Fire::UHDGA_Fire(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UHDGA_Fire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
    CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags), false);

    // 스트라타젬을 든 동안 같은 입력(Trigger.Attack)은 투척 GA 몫이다. 사격 몽타주가 투척 몽타주를 끊지 않게 막는다
    const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(ASC && ASC->HasMatchingGameplayTag(Tag_Character_Action_HoldStratagem) == false, false);
    // 수류탄을 든 동안의 좌클릭은 투척 GA(UHDGA_ThrowGrenade) 몫이다
    CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(ASC->HasMatchingGameplayTag(Tag_Character_State_GrenadeReady) == false, false);

    return CheckCost(Handle, ActorInfo);
}

void UHDGA_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    WeaponInterface = ActorInfo->AvatarActor.Get();
    NULL_CHECK(WeaponInterface);

    // 달리면서는 쏘지 않는다. 달리던 중이면 달리기를 끝내 걷기로 돌리고, 쏘는 동안 Shift 를 새로 눌러도 달리지 않게 막는다
    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (ASC)
    {
        // 사격으로 끊긴 달리기는 블렌딩 없이 바로 걷는다. 애니메이션은 0초로 바꾸고, 속도도 이번 프레임에 걷기 속도로 자른다
        AActor* SprintAvatar = ActorInfo->AvatarActor.Get();
        UHDMovementStateComponent* MovementStateComp = SprintAvatar ? SprintAvatar->FindComponentByClass<UHDMovementStateComponent>() : nullptr;
        const bool bWasSprinting = MovementStateComp && MovementStateComp->GetMovementState() == EHDCharacterMovementState::Sprint;
        if (bWasSprinting)
        {
            MovementStateComp->RequestInstantSprintExit();
        }

        const FGameplayTagContainer SprintTags(Tag_Trigger_Sprint);
        ASC->CancelAbilities(&SprintTags, nullptr, this);

        UCharacterMovementComponent* CharacterMovement = SprintAvatar ? SprintAvatar->FindComponentByClass<UCharacterMovementComponent>() : nullptr;
        if (bWasSprinting && CharacterMovement)
        {
            const FVector2D Horizontal(CharacterMovement->Velocity);
            const float WalkSpeed = CharacterMovement->GetMaxSpeed();
            if (Horizontal.Size() > WalkSpeed)
            {
                const FVector2D Clamped = Horizontal.GetSafeNormal() * WalkSpeed;
                CharacterMovement->Velocity.X = Clamped.X;
                CharacterMovement->Velocity.Y = Clamped.Y;
            }
            UE_LOG(LogTemp, Display, TEXT("[HDFire] sprint cut instant speed=%.0f"), WalkSpeed);
        }
        ASC->AddLooseGameplayTag(Tag_Block_Sprint);
        bSprintBlocked = true;
    }

    // 쏘는 동안 몸을 조준 방향에 둔다. 뒤를 보고 있었으면 먼저 돌고, 절반 이상 돌았을 때도 누르고 있으면 쏜다
    AActor* Avatar = ActorInfo->AvatarActor.Get();
    Combat = Avatar ? Avatar->FindComponentByClass<UHDCombatComponent>() : nullptr;
    if (Combat)
    {
        Combat->BeginFaceAim();
        bFaceAimBegun = true;

        InitialYawToAim = Combat->GetYawToAim();
        if (HDFaceAim::NeedsTurnBeforeFire(InitialYawToAim, BackFacingAngle))
        {
            UE_LOG(LogTemp, Display, TEXT("[HDFire] turn before fire yaw=%.0f"), InitialYawToAim);

            UWorld* World = GetWorld();
            NULL_CHECK(World);
            World->GetTimerManager().SetTimer(TurnCheckTimer, FTimerDelegate::CreateUObject(this, &UHDGA_Fire::CheckTurnBeforeFire), 1.f / 60.f, true);
            return;
        }
    }

    StartFiring();
}

void UHDGA_Fire::CheckTurnBeforeFire()
{
    if (IsActive() == false || Combat == nullptr)
    {
        GetWorld()->GetTimerManager().ClearTimer(TurnCheckTimer);
        return;
    }

    const float CurrentYawToAim = Combat->GetYawToAim();
    if (HDFaceAim::IsTurnedEnough(InitialYawToAim, CurrentYawToAim, TurnProgressToFire) == false)
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(TurnCheckTimer);
    UE_LOG(LogTemp, Display, TEXT("[HDFire] turned %.0f -> %.0f, fire"), InitialYawToAim, CurrentYawToAim);
    StartFiring();
}

void UHDGA_Fire::StartFiring()
{
    NULL_CHECK(WeaponInterface);

    WeaponInterface->PlayWeaponMontage(EHDCombatMontage::Fire);
    AutoFireDelay = WeaponInterface->GetWeaponFireDelay();
    SectionName = WeaponInterface->IsShoulder() ? HDMONTAGE_SECTIONNAME_RIFLE_AIM : HDMONTAGE_SECTIONNAME_RIFLE_HIP;

    UAnimMontage* CombatMontage = WeaponInterface->GetCombatMontage(EHDCombatMontage::Fire);
    NULL_CHECK(CombatMontage);

    PlayMontageAndWaitEventTask = UHDAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitEvent(
        this,
        CombatMontage,
        FGameplayTagContainer(Tag_Event_Spawn_Projectile),
        1.f,
        SectionName,
        false
    );

    PlayMontageAndWaitEventTask->OnEventReceived.AddDynamic(this, &UHDGA_Fire::OnEventRecieved);
    PlayMontageAndWaitEventTask->ReadyForActivation();
}

void UHDGA_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(TurnCheckTimer);
    }

    if (bFaceAimBegun && Combat)
    {
        Combat->EndFaceAim();
    }
    bFaceAimBegun = false;

    if (bSprintBlocked)
    {
        UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
        if (ASC)
        {
            ASC->RemoveLooseGameplayTag(Tag_Block_Sprint);
        }
    }
    bSprintBlocked = false;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHDGA_Fire::OnEventRecieved(FGameplayEventData Payload)
{
    NULL_CHECK(WeaponInterface);

    if(CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo) == false)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
        return;
    }
    
    WeaponInterface->SpawnProjectile();

    if (WeaponInterface->IsWeaponAutoFire())
    {
        UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, AutoFireDelay);
        Task->OnFinish.AddDynamic(this, &UHDGA_Fire::OnFireDelayCompleted);
        Task->ReadyForActivation();

        if(PlayMontageAndWaitEventTask->IsActive())
        {
            PlayMontageAndWaitEventTask->JumpToSection(SectionName);
        }
    }
    else
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
    }
}

void UHDGA_Fire::OnFireDelayCompleted()
{
    NULL_CHECK(WeaponInterface);
    WeaponInterface->PlayWeaponMontage(EHDCombatMontage::Fire);

    NULL_CHECK(PlayMontageAndWaitEventTask);
    PlayMontageAndWaitEventTask->ResetMontage();
}
