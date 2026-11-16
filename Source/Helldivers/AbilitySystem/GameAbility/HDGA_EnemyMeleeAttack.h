// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_Base.h"
#include "HDGA_EnemyMeleeAttack.generated.h"

class UGameplayEffect;
class AHDCharacterNonPlayer;

// 적 근접 공격: 예비 동작 -> 돌진 -> 구 겹침 판정 -> 피해
UCLASS()
class HELLDIVERS_API UHDGA_EnemyMeleeAttack : public UHDGA_Base
{
	GENERATED_BODY()

public:
    UHDGA_EnemyMeleeAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void                    ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override final;

private:
    UFUNCTION()
    void                            OnWindupFinished();

    void                            ApplyMeleeHit(AHDCharacterNonPlayer* Enemy);

private:
    UPROPERTY()
    TSubclassOf<UGameplayEffect>    DamageEffectClass;
};
