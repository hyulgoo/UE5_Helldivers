// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_Stim.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_Repeat.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Attribute/HDSpeedAttributeSet.h"
#include "Attribute/HDWeaponAttributeSet.h"
#include "Character/Player/HDStimRules.h"

UHDGA_Stim::UHDGA_Stim(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 에셋 태그는 주지 않는다. 입력 태그 경로(UHDAbilitySystemComponent::AbilityInputTagTriggered)로 켜지지 않게 한다
    ActivationBlockedTags.AddTag(Tag_Block_All);
    ActivationBlockedTags.AddTag(Tag_Character_Action_Dead);
    ActivationBlockedTags.AddTag(Tag_Character_State_Stimmed);

    ActivationOwnedTags.AddTag(Tag_Character_State_Stimmed);
}

bool UHDGA_Stim::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
    if (Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) == false)
    {
        return false;
    }

    const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    if (ASC == nullptr)
    {
        return false;
    }

    const EHDStimRefusal Refusal = HDStim::CheckUse(
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetStimAttribute()),
        ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()),
        ASC->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute()),
        ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead),
        ASC->HasMatchingGameplayTag(Tag_Character_State_Stimmed));

    return Refusal == EHDStimRefusal::None;
}

void UHDGA_Stim::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = true;

    // NULL_CHECK 는 return 만 해서 어빌리티가 끝나지 않으므로 직접 끝낸다
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false || ASC == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    const float StimCount = FMath::Max(ASC->GetNumericAttributeBase(UHDWeaponAttributeSet::GetStimAttribute()) - 1.f, 0.f);
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetStimAttribute(), StimCount);

    // 스태미나는 바로 최대로
    ASC->SetNumericAttributeBase(UHDSpeedAttributeSet::GetCurrentStaminaAttribute(), ASC->GetNumericAttribute(UHDSpeedAttributeSet::GetMaxStaminaAttribute()));

    UE_LOG(LogTemp, Display, TEXT("[HDStim] used left=%.0f health=%.0f/%.0f"),
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetStimAttribute()),
        ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()),
        ASC->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute()));

    // 태스크는 켜지는 순간 첫 회복을 하고 그 뒤 HealInterval 마다 한다. 마지막 회복 뒤 OnFinished 를 보낸다
    const int32 HealCount = FMath::Max(1, FMath::RoundToInt32(HealDuration / HealInterval));
    UAbilityTask_Repeat* RepeatTask = UAbilityTask_Repeat::RepeatAction(this, HealInterval, HealCount);
    if (RepeatTask == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    RepeatTask->OnPerformAction.AddDynamic(this, &UHDGA_Stim::OnHealTick);
    RepeatTask->OnFinished.AddDynamic(this, &UHDGA_Stim::OnHealFinished);
    RepeatTask->ReadyForActivation();
}

void UHDGA_Stim::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
}

void UHDGA_Stim::OnHealTick(int32 ActionNumber)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    // 보통은 AHDCharacterPlayer::SetDead 가 먼저 취소한다. 사망 GA 가 켜지지 않은 채 태그만 붙은 경우를 여기서 막는다
    if (ASC == nullptr || ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead))
    {
        UE_LOG(LogTemp, Display, TEXT("[HDStim] cancelled (dead)"));

        const bool bReplicateEndAbility = true;
        const bool bWasCancelled = true;
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    // 더하기 GE 는 기준값이 최대를 넘을 수 있어, 계산한 값을 기준값에 넣는다
    const float NewHealth = HDStim::HealStep(
        ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()),
        ASC->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute()),
        HealPerSecond,
        HealInterval);
    ASC->SetNumericAttributeBase(UHDHealthAttributeSet::GetCurrentHealthAttribute(), NewHealth);
}

void UHDGA_Stim::OnHealFinished(int32 ActionNumber)
{
    const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDStim] done health=%.0f/%.0f"),
            ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()),
            ASC->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute()));
    }

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = false;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}
