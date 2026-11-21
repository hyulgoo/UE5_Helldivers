// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_StratagemInputMode.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "AbilitySystemComponent.h"

UHDGA_StratagemInputMode::UHDGA_StratagemInputMode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UHDGA_StratagemInputMode::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
    CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags), false);

    const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(ASC != nullptr, false);

    // Block.Stratagem 이 있는 동안에는 입력 모드를 켜지 않는다. 이 태그를 갖는 GA 는 셋이다: BPGA_Fire(사격 버튼을 누르고 있는 동안),
    // BPGA_ThrowStratagem(스트라타젬 투척 중), UHDGA_ThrowGrenade(수류탄 투척 중). 투척 중에 스트라타젬을 들면 둘이 같이 나가거나 손에 액터가 남는다.
    // BP 의 ActivationBlockedTags 에는 이 태그가 없고 에셋은 고치지 않으므로 여기서 막는다
    CONDITION_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(ASC->HasMatchingGameplayTag(Tag_Block_Stratagem) == false, false);

    return true;
}

void UHDGA_StratagemInputMode::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    FGameplayAbilityHelper::SendGameplayEventToTarget(Tag_Event_StratagemHUD_Active, CurrentActorInfo, ActorInfo);
}

void UHDGA_StratagemInputMode::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    FGameplayAbilityHelper::SendGameplayEventToTarget(Tag_Character_Action_HoldStratagem, CurrentActorInfo, ActorInfo);
    FGameplayAbilityHelper::SendGameplayEventToTarget(Tag_Event_StratagemHUD_Deactive, CurrentActorInfo, ActorInfo);
}
