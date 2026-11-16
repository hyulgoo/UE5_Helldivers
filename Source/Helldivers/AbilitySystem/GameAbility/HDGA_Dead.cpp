// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_Dead.h"
#include "Interface/HDDeadInterface.h"

UHDGA_Dead::UHDGA_Dead(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 체력 0 에서 UHDHealthAttributeSet 이 붙이는 Trigger.Dead 로 발동한다. BPGA_Dead 는 자기 직렬화 값을 쓴다
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = Tag_Trigger_Dead;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagAdded;
    AbilityTriggers.Add(TriggerData);
}

void UHDGA_Dead::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    TScriptInterface<IHDDeadInterface> DeadInterface = ActorInfo->AvatarActor.Get();
    NULL_CHECK(DeadInterface);

    DeadInterface->SetDead();
}
