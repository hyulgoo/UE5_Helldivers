// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_Base.h"
#include "HDGA_Stim.generated.h"

// 스팀(회복 주사) 하나를 써서 HealDuration 동안 HealInterval 마다 체력을 채우고 스태미나를 바로 최대로 만든다.
// 입력 태그로 켜지지 않는다. AHDCharacterPlayer::UseStim(Q 키·치트 HDUseStim)이 클래스로 켠다. 규칙은 HDStim(HDStimRules.h)
UCLASS()
class HELLDIVERS_API UHDGA_Stim : public UHDGA_Base
{
	GENERATED_BODY()

public:
    UHDGA_Stim(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual bool                    CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override final;
    virtual void                    ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override final;
    // 키를 떼도 끝내지 않는다. UHDGA_Base 는 떼면 끝낸다
    virtual void                    InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override final;

private:
    UFUNCTION()
    void                            OnHealTick(int32 ActionNumber);

    UFUNCTION()
    void                            OnHealFinished(int32 ActionNumber);

private:
    // 초당 회복량
    UPROPERTY(EditDefaultsOnly)
    float                           HealPerSecond = 60.f;

    // 회복이 이어지는 시간(초)
    UPROPERTY(EditDefaultsOnly)
    float                           HealDuration = 2.f;

    // 회복 한 번의 간격(초). 켜지는 순간 첫 회복을 하고 그 뒤 이 간격마다 한다
    UPROPERTY(EditDefaultsOnly)
    float                           HealInterval = 0.1f;
};
