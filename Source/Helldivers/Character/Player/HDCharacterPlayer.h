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
class AHDProjectileGrenade;
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

    // Slot 의 무기를 든다(숫자 키 1~4, AHDPlayerController 가 원시 키로 넘긴다). 죽음·스트라타젬 입력 모드·사격/재장전/투척 중·같은 칸·빈 칸이면 거부 로그만 남긴다.
    // 스트라타젬을 들고 있으면 던지지 않고 집어넣은 뒤(PutAwayStratagem) 그 칸을 든다
    void                                    SelectWeaponSlot(const EHDWeaponSlot Slot);

    // 스트라타젬 화살표 하나(AHDPlayerController 가 원시 키 순서대로 넘긴다). 입력 모드가 아니면 무시
    void                                    InputStratagemCommand(const EHDCommandInput Command);

    // 투척 GA(UHDGA_ThrowGrenade)가 AM_Throw 의 DetachStratagem 에서 부른다. 오른손에서 조준점 쪽으로 수류탄을 던지고 개수를 1 줄인다
    void                                    ThrowGrenade();
    // 투척 GA 가 끝날 때 부른다. 다 던졌고 수류탄 칸을 쓰는 중이면 마지막 총으로 돌아간다
    void                                    OnGrenadeThrowFinished();

    // 스팀 하나를 쓴다(Q 키, AHDPlayerController 가 원시 키로 넘긴다. 치트 HDUseStim 도 같은 경로). 회복 GA(UHDGA_Stim)가 켜지지 않으면 거부 사유를 로그로 남긴다
    void                                    UseStim();

    // 급강하: 움직이던 방향(느리면 보는 방향)으로 몸을 던지고 엎드린다. 왼쪽 Alt(AHDPlayerController 가 원시 키로 넘긴다)·치트 HDDive 가 같은 경로로 부른다.
    // 죽음·Block.All·래그돌·공중·엎드림·대기 중이면 거부 사유를 로그로 남긴다
    void                                    Dive();
    // 치트 HDDive status 용: "stance=<Idle|Crouch|Prone> diving=<0|1> airborne=<0|1>"
    FString                                 GetDiveStatusText() const;

    // 급강하로 뜬 뒤의 착지면 Diving 태그를 내린다. 점프·헬포드 뒤 스폰의 착지에는 아무것도 하지 않는다
    virtual void                            Landed(const FHitResult& Hit) override;

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
    virtual bool                            CanFireNow() const override final;
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

    // 사격·재장전·수류탄 투척 GA(UHDGA_Fire·UHDGA_Reload·UHDGA_ThrowGrenade)가 활성이면 참. 무기 칸 선택을 거부하는 데 쓴다
    bool                                    IsWeaponActionActive() const;

    // 수류탄 칸에서 마지막 총 칸으로 돌아간다(거부 검사 없음). Reason 은 로그용
    void                                    LeaveGrenadeSlot(const TCHAR* Reason);

    // 든 스트라타젬을 던지지 않고 집어넣는다(횟수·대기를 쓰지 않는다). 든 상태를 풀고 총을 다시 보인다
    void                                    PutAwayStratagem();

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

    // 던질 수류탄 클래스. SpawnDefaultWeapon 이 프로필에서 고른 것으로 채운다(비면 파편 수류탄)
    UPROPERTY()
    TSubclassOf<AHDProjectileGrenade>       GrenadeClass;

    FTimerHandle                            CodeReloadEventTimerHandle;
    // 직전 코드 발사 이벤트 시각(월드 초). 아직 없으면 음수
    double                                  LastCodeFireEventTime = -1.0;
    // 직전 급강하 시각(월드 초). 첫 급강하가 대기에 걸리지 않게 먼 과거로 둔다
    double                                  LastDiveTime = -1000.0;
};
