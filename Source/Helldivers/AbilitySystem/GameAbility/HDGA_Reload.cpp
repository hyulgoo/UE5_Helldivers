// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_Reload.h"
#include "AbilitySystemComponent.h"
#include "Interface/HDWeaponInterface.h"
#include "AbilitySystem/AbilityTask/HDAT_PlayMontageAndWaitForEvent.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "Attribute/HDWeaponAttributeSet.h"
#include "Define/HDMontageSectionNames.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

namespace
{
    // 몸 몽타주가 끝난 뒤 Event.Reload 를 더 기다리는 시간(초). 소총 엎드려 재장전은 약 0.5초 뒤에 온다
    constexpr float ReloadEventGraceTime = 2.f;
}

bool UHDGA_Reload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
    CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags), false);

    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    NULL_CHECK_WITH_RETURNTYPE(ASC, false);

    // 수류탄을 든 동안에는 재장전하지 않는다
    CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(ASC->HasMatchingGameplayTag(Tag_Character_State_GrenadeReady) == false, false);

    const UHDWeaponAttributeSet* WeaponAttributeSet = ASC->GetSet<UHDWeaponAttributeSet>();
    NULL_CHECK_WITH_RETURNTYPE(WeaponAttributeSet, false);

    return (WeaponAttributeSet->GetAmmo() < WeaponAttributeSet->GetMaxAmmo() && WeaponAttributeSet->GetCapacity() > 0.f);
}

void UHDGA_Reload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = true;

    // NULL_CHECK 는 return 만 해서 어빌리티가 끝나지 않으므로 직접 끝낸다
    TScriptInterface<IHDWeaponInterface> WeaponInterface = ActorInfo->AvatarActor.Get();
    if (WeaponInterface == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    WeaponInterface->PlayWeaponMontage(EHDCombatMontage::Reload);

    UAnimMontage* CombatMontage = WeaponInterface->GetCombatMontage(EHDCombatMontage::Reload);
    if (CombatMontage == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    UHDAT_PlayMontageAndWaitForEvent* PlayMontageAndWaitEventTask = UHDAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitEvent(
        this,
        CombatMontage,
        FGameplayTagContainer(Tag_Event_Reload)
    );

    // Event.Reload 가 오면 탄약을 채우고 끝낸다. 몸 몽타주가 먼저 끝나면 이벤트를 따로 더 기다리고, 끊기면 채우지 않고 끝낸다
    PlayMontageAndWaitEventTask->OnEventReceived.AddDynamic(this, &UHDGA_Reload::OnEventRecieved);
    PlayMontageAndWaitEventTask->OnCompleted.AddDynamic(this, &UHDGA_Reload::OnBodyMontageCompleted);
    PlayMontageAndWaitEventTask->OnInterrupted.AddDynamic(this, &UHDGA_Reload::OnBodyMontageInterrupted);
    PlayMontageAndWaitEventTask->ReadyForActivation();
}               

void UHDGA_Reload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHDGA_Reload::OnEventRecieved(FGameplayEventData Payload)
{
    NULL_CHECK(ReloadEffectClass);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    NULL_CHECK(ASC);

    const FGameplayEffectSpecHandle& SpecHandle = ASC->MakeOutgoingSpec(ReloadEffectClass, 1.f, ASC->MakeEffectContext());
    CONDITION_CHECK(SpecHandle.IsValid());

    ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

    UE_LOG(LogTemp, Display, TEXT("[HDWeapon] reload done ammo=%.0f capacity=%.0f"),
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetAmmoAttribute()), ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetCapacityAttribute()));

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
}

void UHDGA_Reload::OnBodyMontageCompleted(FGameplayEventData Payload)
{
    // 몸 몽타주(첫 섹션)가 끝나면 태스크가 끝나면서 이벤트 듣기도 풀린다.
    // 무기 몽타주의 Event.Reload 가 그 뒤에 오는 자세(엎드림)가 있어 따로 더 기다린다
    UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, Tag_Event_Reload, nullptr, true, true);
    WaitEventTask->EventReceived.AddDynamic(this, &UHDGA_Reload::OnEventRecieved);
    WaitEventTask->ReadyForActivation();

    UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ReloadEventGraceTime);
    WaitDelayTask->OnFinish.AddDynamic(this, &UHDGA_Reload::OnReloadEventTimeout);
    WaitDelayTask->ReadyForActivation();
}

void UHDGA_Reload::OnBodyMontageInterrupted(FGameplayEventData Payload)
{
    UE_LOG(LogTemp, Display, TEXT("[HDWeapon] reload interrupted"));

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = true;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHDGA_Reload::OnReloadEventTimeout()
{
    UE_LOG(LogTemp, Warning, TEXT("[HDWeapon] reload ended without Event.Reload"));

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = true;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}
