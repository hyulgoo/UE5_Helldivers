// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_Knockback.h"
#include "AbilitySystemComponent.h"
#include "Interface/HDCharacterRagdollInterface.h"

UHDGA_Knockback::UHDGA_Knockback(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UHDGA_Knockback::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (TriggerEventData == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }

    if(TriggerEventData->EventTag == Tag_Data_Knockback_Hit)
    {
        LOG(TEXT("Knockback GA called!"));
        EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
        return;
    }
    else if (TriggerEventData->EventTag == Tag_Data_Knockback_Ragdoll)
    {
        // Character Ragdoll
        RagdollInterface = ActorInfo->AvatarActor.Get();
        const AActor* Instigator = TriggerEventData->Instigator.Get();
        if (RagdollInterface == nullptr || Instigator == nullptr)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
            return;
        }

        // 인스턴스를 다시 쓰므로(InstancedPerActor) 지난 발동에서 남은 값을 지운다. 남아 있으면 아래 판정이 영영 참이 안 된다
        bRecoveryFromRagdoll = false;

        // 폭발(Instigator = 투사체) 바깥쪽으로 민다
        const FVector& Direction = (ActorInfo->AvatarActor->GetActorLocation() - Instigator->GetActorLocation()).GetSafeNormal();
        const FVector Impulse = Direction * TriggerEventData->EventMagnitude;

        RagdollInterface->SetRagdoll(true, Impulse);

        if(StateCheckTimerHandle.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(StateCheckTimerHandle);
        }

        GetWorld()->GetTimerManager().SetTimer(StateCheckTimerHandle, this, &UHDGA_Knockback::CheckCharacterRagdollState, 0.1f, true);
    }
}

void UHDGA_Knockback::CheckCharacterRagdollState()
{
    NULL_CHECK(RagdollInterface);

    const float Speed = RagdollInterface->GetRagdollPysicsLinearVelocity();
    if (Speed < 5.f && bRecoveryFromRagdoll == false)
    {
        bRecoveryFromRagdoll = true;

        GetWorld()->GetTimerManager().ClearTimer(StateCheckTimerHandle);
        const bool bIsDead = GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(Tag_Character_Action_Dead);
        if (bIsDead == false)
        {
            GetWorld()->GetTimerManager().SetTimer(RecoveryFromRagdollTimerHandle, this, &UHDGA_Knockback::RecoveryFromRagdoll, 0.5f, false);
        }
        else
        {
            // 죽었으면 일어나지 않고(시체는 래그돌 그대로) 능력만 끝낸다. 켜 둔 채 두면 증원 뒤 다음 넉백에서 일어나지 못한다
            EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        }
    }
}

void UHDGA_Knockback::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // 증원 때 CancelAllAbilities 로 끝나도 다음 발동이 깨끗하게 시작하도록 타이머와 플래그를 정리한다
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(StateCheckTimerHandle);
        World->GetTimerManager().ClearTimer(RecoveryFromRagdollTimerHandle);
    }
    bRecoveryFromRagdoll = false;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHDGA_Knockback::RecoveryFromRagdoll()
{
    NULL_CHECK(RagdollInterface);

    RagdollInterface->SetRagdoll(false);
    GetWorld()->GetTimerManager().ClearTimer(RecoveryFromRagdollTimerHandle);

    bRecoveryFromRagdoll = false;

    const bool bReplicatedEndAbility = true;
    const bool bWasCancelled = true;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicatedEndAbility, bWasCancelled);
}
