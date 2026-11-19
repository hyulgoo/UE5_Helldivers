// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_StratagemBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameData/HDStratagemEffectData.h"
#include "Stratagem/HDStratagem.h"
#include "Stratagem/BattleShip/HDBattleShip.h"

UHDGA_StratagemBase::UHDGA_StratagemBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 발동마다 새 인스턴스. 앞 호출이 끝나기 전에 다시 던져도 서로의 이름·표적을 덮어쓰지 않는다
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UHDGA_StratagemBase::SetActivationEventTag(const FGameplayTag& EventTag)
{
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EventTag;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UHDGA_StratagemBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    const AHDStratagem* Beacon = TriggerEventData ? Cast<AHDStratagem>(TriggerEventData->OptionalObject.Get()) : nullptr;
    const FGameplayAbilityTargetData* TargetData = TriggerEventData ? TriggerEventData->TargetData.Get(0) : nullptr;

    // NULL_CHECK 는 return 만 해서 어빌리티가 끝나지 않으므로 직접 끝낸다
    if (Beacon == nullptr || TargetData == nullptr || TargetData->HasEndPoint() == false || CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
    {
        FinishStratagem(true);
        return;
    }

    StratagemName = Beacon->GetStratagemName();
    TargetTransform = TargetData->GetEndPointTransform();

    // 어빌리티 태스크라 EndAbility 때 함께 정리된다. 0 초면 다음 틱에 끝난다
    UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, Beacon->GetStratagemActiveDelay());
    WaitTask->OnFinish.AddDynamic(this, &UHDGA_StratagemBase::OnActivationDelayFinished);
    WaitTask->ReadyForActivation();
}

void UHDGA_StratagemBase::ExecuteStratagem(const FHDStratagemEffectData& EffectData)
{
    FinishStratagem(false);
}

void UHDGA_StratagemBase::FinishStratagem(const bool bWasCancelled)
{
    const bool bReplicateEndAbility = true;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AHDBattleShip* UHDGA_StratagemBase::GetBattleShip() const
{
    return Cast<AHDBattleShip>(GetAvatarActorFromActorInfo());
}

void UHDGA_StratagemBase::OnActivationDelayFinished()
{
    const AHDBattleShip* BattleShip = GetBattleShip();
    const FHDStratagemEffectData* EffectData = BattleShip ? BattleShip->FindStratagemEffectData(StratagemName) : nullptr;
    if (EffectData == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    ExecuteStratagem(*EffectData);
}
