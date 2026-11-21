// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/HDCharacterBase.h"
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
enum class EHDCommandInput : uint8;
enum class EHDWeaponSlot : uint8;

UCLASS()
class HELLDIVERS_API AHDCharacterPlayer : public AHDCharacterBase, public IHDCharacterMovementInterface, public IHDWeaponInterface
{
	GENERATED_BODY()

public:
    AHDCharacterPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    void									SetCharacterControlData(UHDCharacterControlData* CharacterControlData);

    // 탄창과 예비 탄약을 최대로 채운다(보급 상자). 죽었거나 ASC 가 없으면 거짓
    bool                                    Resupply();

    // Slot 의 무기를 든다(숫자 키 1~4, AHDPlayerController 가 원시 키로 넘긴다). 죽음·스트라타젬·사격/재장전 중·같은 칸·빈 칸이면 거부 로그만 남긴다
    void                                    SelectWeaponSlot(const EHDWeaponSlot Slot);

    // 스트라타젬 화살표 하나(AHDPlayerController 가 원시 키 순서대로 넘긴다). 입력 모드가 아니면 무시
    void                                    InputStratagemCommand(const EHDCommandInput Command);

protected:
	virtual void							SetDead() override final;
    virtual void							Tick(float DeltaTime) override final;
	virtual void							PossessedBy(AController* NewController) override final;
    // PossessedBy 가 PlayerState ASC 에 묶은 이벤트 콜백 4개를 푼다
    virtual void                            UnPossessed() override;
    virtual void                            SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override final;
    // 총 칸의 무기 액터도 같이 지운다
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

private:
    void                                    InitAbilitySystemComponent();
	void									InterpFOV(float DeltaSeconds);
    // 쓰는 칸 무기의 AmmoData 로 무기 Attribute GE 를 적용한다
    void                                    ApplyWeaponAttributes(const AHDWeapon* InWeapon);

    // 무기 몽타주에 노티파이가 없는 무기(SendsMontageEventsFromCode)의 발사·재장전 이벤트를 예약한다.
    // GA 가 대기 태스크보다 PlayWeaponMontage 를 먼저 불러, 같은 프레임에 보내면 받는 쪽이 없다
    void                                    ScheduleCodeMontageEvent(const EHDCombatMontage MontageType, const UAnimMontage* WeaponMontage, const FName SectionName);
    void                                    SendCodeMontageEvent(const FGameplayTag EventTag);

    // 사격·재장전 GA(UHDGA_Fire·UHDGA_Reload)가 활성이면 참. 무기 칸 선택을 거부하는 데 쓴다
    bool                                    IsWeaponActionActive() const;

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

    FTimerHandle                            CodeReloadEventTimerHandle;
    // 직전 코드 발사 이벤트 시각(월드 초). 아직 없으면 음수
    double                                  LastCodeFireEventTime = -1.0;
};
