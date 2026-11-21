// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_Base.h"
#include "HDGA_Reload.generated.h"

class UGameplayEffect;

UCLASS()
class HELLDIVERS_API UHDGA_Reload : public UHDGA_Base
{
	GENERATED_BODY()
	
public:
    virtual bool                    CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void                    ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override final;
    virtual void                    EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override final;
    
private:
    UFUNCTION()
    void 	                        OnEventRecieved(FGameplayEventData Payload);

    // 몸 몽타주가 Event.Reload 보다 먼저 끝났다(엎드려 재장전). 이벤트를 잠깐 더 기다린다
    UFUNCTION()
    void                            OnBodyMontageCompleted(FGameplayEventData Payload);

    // 몸 몽타주가 끊겼다. 탄약을 채우지 않고 끝낸다
    UFUNCTION()
    void                            OnBodyMontageInterrupted(FGameplayEventData Payload);

    // 몸 몽타주가 끝난 뒤에도 Event.Reload 가 오지 않았다. 탄약을 채우지 않고 끝낸다
    UFUNCTION()
    void                            OnReloadEventTimeout();

private:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UGameplayEffect>    ReloadEffectClass; 
};
