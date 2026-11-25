// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_Base.h"
#include "HDGA_Fire.generated.h"

class IHDWeaponInterface;
class UHDAT_PlayMontageAndWaitForEvent;
class UHDCombatComponent;

UCLASS()
class HELLDIVERS_API UHDGA_Fire : public UHDGA_Base
{
	GENERATED_BODY()

public:
	 UHDGA_Fire(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual bool                            CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void							ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override final;
	virtual void							EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override final;

private:
    // 사격 몽타주를 틀고 탄 생성 이벤트를 기다린다. 뒤를 보고 있었으면 충분히 돈 뒤에 부른다
    void                                    StartFiring();

    // 뒤돌기 중 매 틱: 처음 각도의 TurnProgressToFire 이상 돌았으면 StartFiring. 그 전에 버튼을 떼면 InputReleased 가 어빌리티를 끝낸다
    void                                    CheckTurnBeforeFire();

    UFUNCTION()
    void 								    OnEventRecieved(FGameplayEventData Payload);

    UFUNCTION()
    void 								    OnFireDelayCompleted();

protected:
    UPROPERTY()
    TObjectPtr<UHDAT_PlayMontageAndWaitForEvent> PlayMontageAndWaitEventTask;

    UPROPERTY()
	TScriptInterface<IHDWeaponInterface>	WeaponInterface;

    UPROPERTY()
    TObjectPtr<UHDCombatComponent>          Combat;

    // 몸이 조준 방향에서 이 각도(도) 이상 벗어나 있으면 뒤를 보는 것으로 보고 먼저 돈다
    UPROPERTY(EditDefaultsOnly, Category = "Fire|Turn")
    float                                   BackFacingAngle     = 90.f;

    // 처음 각도 중 이 비율 이상 돌았을 때 쏜다
    UPROPERTY(EditDefaultsOnly, Category = "Fire|Turn")
    float                                   TurnProgressToFire  = 0.5f;

    float                                   AutoFireDelay       = 0.f;
    float                                   InitialYawToAim     = 0.f;
    FName                                   SectionName         = NAME_None;
    FTimerHandle                            TurnCheckTimer;
    uint8                                   bFaceAimBegun : 1   = false;
    uint8                                   bSprintBlocked : 1  = false;
};
