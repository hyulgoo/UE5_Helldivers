// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_Base.h"
#include "HDGA_MovementTrigger.generated.h"

struct FTimerHandle;

UCLASS()
class HELLDIVERS_API UHDGA_MovementTrigger : public UHDGA_Base
{
	GENERATED_BODY()

public:
    UHDGA_MovementTrigger(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 달리기(Trigger.Sprint)는 Block.Sprint 가 있으면 켜지지 않는다. BP 기본값의 차단 태그와 상관없이 코드에서 막는다
    virtual bool    CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void    ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override final;
	virtual void    EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override final;
    
private:
	UFUNCTION()
	void		    OnCurrentStaminaChanged(bool bMatchesComparison, float CurrentValue);

private:
    float           MaxStamina = 0.f;
    FTimerHandle    RegenStaminaHandle = FTimerHandle();
};
