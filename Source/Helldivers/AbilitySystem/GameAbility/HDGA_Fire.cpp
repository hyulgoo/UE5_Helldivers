// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_Fire.h"
#include "Interface/HDWeaponInterface.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "AbilitySystem/AbilityTask/HDAT_PlayMontageAndWaitForEvent.h"
#include "Define/HDMontageSectionNames.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"

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
