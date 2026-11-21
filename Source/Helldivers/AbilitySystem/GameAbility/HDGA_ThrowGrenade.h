// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_Base.h"
#include "HDGA_ThrowGrenade.generated.h"

class UAnimMontage;

// 4 로 수류탄을 든 상태(Character.State.GrenadeReady)에서 좌클릭(Trigger.Attack)으로 한 개 던진다.
// AM_Throw 의 DetachStratagem 이벤트에서 캐릭터가 투사체를 만들고, 몽타주가 끝나면 끝난다
UCLASS()
class HELLDIVERS_API UHDGA_ThrowGrenade : public UHDGA_Base
{
	GENERATED_BODY()

public:
    UHDGA_ThrowGrenade(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void                    ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override final;
    virtual void                    EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override final;
    // 버튼을 떼도 끝내지 않는다. UHDGA_Base 는 떼면 끝내므로 짧게 클릭하면 손을 떠나기 전에 끝난다
    virtual void                    InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override final;

private:
    UFUNCTION()
    void                            OnDetach(FGameplayEventData Payload);

    UFUNCTION()
    void                            OnMontageFinished(FGameplayEventData Payload);

private:
    UPROPERTY()
    TObjectPtr<UAnimMontage>        ThrowMontage;
};
