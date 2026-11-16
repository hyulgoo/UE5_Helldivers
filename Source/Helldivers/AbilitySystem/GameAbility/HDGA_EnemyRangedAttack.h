// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_Base.h"
#include "HDGA_EnemyRangedAttack.generated.h"

class AHDProjectileBase;
class AHDEnemyShooter;

// 적 원거리 공격: 예비 동작 -> BurstCount 발을 BurstInterval 간격으로 발사
UCLASS()
class HELLDIVERS_API UHDGA_EnemyRangedAttack : public UHDGA_Base
{
	GENERATED_BODY()

public:
    UHDGA_EnemyRangedAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void                        ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override final;

private:
    void                                WaitNextShot(const float DelaySeconds);

    UFUNCTION()
    void                                OnShotDelayFinished();

    void                                FireOneShot(AHDEnemyShooter& Shooter);

private:
    UPROPERTY()
    TSubclassOf<AHDProjectileBase>      ProjectileClass;

    int32                               ShotsFired = 0;

    static constexpr float              TargetChestHeight = 40.f;
};
