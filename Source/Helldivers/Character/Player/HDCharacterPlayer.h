// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/HDCharacterBase.h"
#include "InputActionValue.h"
#include "Interface/HDCharacterMovementInterface.h"
#include "Interface/HDWeaponInterface.h"
#include "HDCharacterPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UHDCombatComponent;
class UHDMovementStateComponent;
class UHDStratagemComponent;
class UHDCharacterControlData;
struct FGameplayEventData;
enum class EHDCharacterMovementState : uint8;
enum class EHDCharacterStanceState : uint8;

UCLASS()
class HELLDIVERS_API AHDCharacterPlayer : public AHDCharacterBase, public IHDCharacterMovementInterface, public IHDWeaponInterface
{
	GENERATED_BODY()

public:
    AHDCharacterPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    void									SetCharacterControlData(UHDCharacterControlData* CharacterControlData);

    // 탄창과 예비 탄약을 최대로 채운다(보급 상자). 죽었거나 ASC 가 없으면 거짓
    bool                                    Resupply();

protected:
	virtual void							SetDead() override final;
    virtual void							Tick(float DeltaTime) override final;
	virtual void							PossessedBy(AController* NewController) override final;
    // PossessedBy 가 PlayerState ASC 에 묶은 이벤트 콜백 4개를 푼다
    virtual void                            UnPossessed() override;
    virtual void                            SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override final;
    // 장착 무기 액터도 같이 지운다
    virtual void                            EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// RagdollInterface
	virtual void							SetRagdoll(const bool bRagdoll, const FVector& Impulse = FVector::ZeroVector) override final;

	// WeaponInterface
    virtual void                            EquipWeapon(AHDWeapon* NewWeapon) override final;
    virtual void                            SpawnProjectile() override final;
    virtual UAnimMontage*                   GetCombatMontage(const EHDCombatMontage MontageType) const override final;
    virtual void                            PlayWeaponMontage(const EHDCombatMontage MontageType) override final;
    virtual bool                            IsEquippedWeapon() const override final;
    virtual bool                            IsShoulder() const override final;
    virtual bool                            IsWeaponAutoFire() const override final;
	virtual float                           GetWeaponFireDelay() const override final;
    virtual void							SetWeaponActive(const bool bActive) override final;

    void                                    SpawnDefaultWeapon(const FGameplayEventData* Payload);

	// CharacterMovementInterface
	virtual void							SetShouldering(const bool bSetAiming) override final;
	virtual void							SetMovementState(const EHDCharacterMovementState NewState) override;
	virtual void							SetCharacterStanceState(const EHDCharacterStanceState NewState, const bool bForced = false);
	virtual void							RestoreStanceState() override;

    // EventTag 는 바인딩할 때 묶는다. 애님 노티파이(Send Gameplay Event to Actor)는 Payload->EventTag 를 채우지 않는다
    void                                    OnStratagemEventReceived(const FGameplayEventData* Payload, const FGameplayTag EventTag);
	void							        TryHoldStratagem();

    void    								UpdateSpeed(const EHDCharacterStanceState StanceState, const EHDCharacterMovementState MoveState);
    
	UFUNCTION()
	void									InputStratagemCommand(const FInputActionValue& Value);
	
private:
    void                                    InitAbilitySystemComponent();
	void									InterpFOV(float DeltaSeconds);

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USpringArmComponent>			SpringArm;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCameraComponent>			FollowCamera;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UHDCombatComponent>			Combat;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UHDMovementStateComponent>	MovementStateComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UHDStratagemComponent>		Stratagem;
    
    UPROPERTY(EditAnywhere)
    TSubclassOf<UGameplayEffect>            WeaponAttributeSetEffect;
};
