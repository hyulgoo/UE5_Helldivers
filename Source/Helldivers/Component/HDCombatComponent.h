
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "Components/TimelineComponent.h"
#include "Weapon/HDWeaponSlots.h"
#include "HDCombatComponent.generated.h"

class UAnimMontage;
class UAnimationAsset;
class UCharacterMovementComponent;
class USpringArmComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class AHDWeapon;
class AHDStratagem;
class AHDProjectileBase;
enum class EHDFireType :uint8;
enum class EHDWeaponAnimationType : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpreadChanged, float);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HELLDIVERS_API UHDCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
    UHDCombatComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    AHDWeapon*                              SpawnDefaultWeapon();
    // NewWeapon 을 그 종류의 칸에 넣고 쓴다. 쓰던 무기는 지금 탄약 Attribute 를 적어 두고 숨기며, 같은 칸에 있던 무기는 앞에 버린다
    void                                    EquipWeapon(AHDWeapon* NewWeapon, UAbilitySystemComponent* ASC);
    // NewWeapon 을 그 종류의 칸에 넣되 쓰지 않는다(손에 붙인 채 숨김). 같은 칸에 있던 무기는 앞에 버린다. 쓰는 무기와 조준 값은 그대로
    void                                    StowWeapon(AHDWeapon* NewWeapon);
    // Slot 의 무기로 바꾼다. Selected 가 아니면 아무것도 바꾸지 않는다. 쓰던 무기의 탄약은 ASC 의 지금 값으로 적어 둔다.
    // Grenade 면 쓰던 총을 숨기기만 하고 Weapon 은 마지막 총으로 남긴다
    EHDWeaponSelectResult                   SelectWeaponSlot(const EHDWeaponSlot Slot, UAbilitySystemComponent* ASC);
    // 수류탄 칸을 고를 수 있는지(FHDWeaponSlots 에 넘긴다)
    void                                    SetGrenadeAvailable(const bool bAvailable);
    bool                                    IsGrenadeSlotActive() const;
    EHDWeaponSlot                           GetActiveWeaponSlot() const;
    EHDWeaponSlot                           GetLastGunSlot() const;
    // 총 칸의 무기(비어 있는 칸은 뺀다)
    void                                    GetAllWeapons(TArray<AHDWeapon*>& OutWeapons) const;
    // 보급: 들고 있지 않은 총의 AmmoData 를 최대로 채운다. 들고 있는 총(Weapon)은 Attribute 가 값이라 보급 GE 가 채운다
    void                                    RefillStowedWeaponAmmo();
    void                                    SpawnProjectile();

    AHDWeapon*                              GetWeapon() const;
    USkeletalMeshComponent*                 GetWeaponMesh() const;
    void                                    SetWeaponActive(const bool bActive);
    UAnimMontage*                           GetWeaponMontage() const;
    const EHDFireType                       GetWeaponFireType() const;
    const float                             GetWeaponFireDelay() const;
    const int32                             GetWeaponAmmoCount() const;
    const int32                             GetWeaponMaxAmmoCount() const;
    const int32                             GetWeaponCapacityCount() const;
    const int32                             GetWeaponMaxCapacityCount() const;
    const float                             GetWeaponZoomedFOV() const;
    const float                             GetWeaponZoomInterpSpeed() const;
    const bool                              IsWeaponAutoFire() const;
    const bool                              IsEquippedWeapon() const;
    const bool                              IsShoulder() const;
    void                                    SetShoulder(const bool bShoudler);

    const float                             GetAimOffset_Yaw() const;
    const float                             GetAimOffset_Pitch() const;

    const FVector&                          GetHitTarget() const;

    const float                             GetCurrentFOV() const;
    void                                    SetCurrentFOV(const float NewFOV);
    const float                             GetDefaultFOV() const;
    const float                             GetZoomInterpSpeed() const;

    const bool                              IsUseRotateBone() const;
    const bool                              IsCharacterLookingViewport() const;
    void                                    SetSpringArmTargetLength(const float TargetArmLength);

    const EHDTurningInPlace                 GetTurnInPlace() const;
    UAnimMontage*                           GetCombatMontage(const EHDCombatMontage MontageType);

    // 조준 방향 바라보기(UHDGA_Fire 가 켜고 끈다). 켜져 있는 동안 비견착이어도 몸을 컨트롤 요로 FaceAimTurnRate 만큼씩 돌린다.
    // 끈 뒤에도 FaceAimLingerTime 동안 유지해서 단발 사이에 이동 방향으로 되돌아가지 않게 한다
    void                                    BeginFaceAim();
    void                                    EndFaceAim();
    // 몸 요 → 조준(컨트롤) 요 차이, [-180, 180]
    float                                   GetYawToAim() const;

protected:
    virtual void                            BeginPlay() override final;
    virtual void                            TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void					                TraceUnderCrosshairs();
    void					                AimOffset(const float DeltaTime);
    void									TurnInPlace(const float DeltaTime);
    bool                                    IsFacingAim() const;
    // 비견착 조준 방향 바라보기 한 틱. AimOffset 이 IsFacingAim 일 때 부른다
    void                                    FaceAim(APawn* Owner, const float DeltaTime);
    // 주인 앞 150 의 지면
    FVector                                 GetWeaponDropLocation() const;
    // 칸 → 무기 포인터 멤버. Grenade 는 nullptr
    TObjectPtr<AHDWeapon>*                  GetSlotWeapon(const EHDWeaponSlot Slot);
    // NewWeapon 을 칸에 넣고(같은 칸의 옛 무기는 버린다) 손 소켓에 붙인다. 보이기·쓰는 무기는 부르는 쪽이 정한다
    bool                                    PutWeaponInSlot(AHDWeapon* NewWeapon, const bool bMakeActive);
    // 쓰는 칸의 무기를 보이고 쓰는 무기로 둔다. 쓰던 무기는 탄약을 적어 두고 숨긴다(수류탄에서 같은 총으로 돌아와도 적는다. 든 동안 받은 보급이 남는다)
    void                                    ActivateSlotWeapon(UAbilitySystemComponent* ASC);

    UFUNCTION()
    void                                    OnSpringArmLengthUpdate(const float Value);

public:
    FOnSpreadChanged                        OnSpreadChanged;

private:
    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> CharacterMovement;

    UPROPERTY()
    TObjectPtr<USpringArmComponent>         SpringArm;
    
    FRotator				                StartingAimRotation             = FRotator::ZeroRotator;
    float					                AimOffset_Yaw                   = 0.f;
    float					                AimOffset_Pitch                 = 0.f;
    float					                InterpAimOffset_Yaw             = 0.f;

    uint8					                bIsCharacterLookingViewport : 1 = false;
    uint8					                bUseRotateRootBone : 1          = false;
    
	UPROPERTY(EditAnywhere)
	float					                TurnThreshold;

	EHDTurningInPlace		                TurningInPlace                  = EHDTurningInPlace::NotTurning;

    // 조준 방향으로 도는 속도(도/초). 180도 뒤를 보다 쏘면 절반(90도)을 도는 데 90/이 값 초가 걸린다
    UPROPERTY(EditAnywhere, Category = "Combat|FaceAim")
    float                                   FaceAimTurnRate                 = 450.f;

    UPROPERTY(EditAnywhere, Category = "Combat|FaceAim")
    float                                   FaceAimLingerTime               = 0.6f;

    int32                                   FaceAimRequestCount             = 0;
    double                                  FaceAimHoldUntil                = 0.0;
    uint8					                bIsShoulder : 1                 = false;
    uint8					                bIsEquipped : 1                 = false;
    FVector					                HitTarget                       = FVector::ZeroVector;
    float                                   DefaultFOV                      = 50.f;
    float					                CurrentFOV                      = 0.f;
	float					                ZoomedFOV                       = 0.f;
	float					                ZoomInterpSpeed                 = 0.f;
    float					                ErgonomicFactor                 = 0.f;
    float 				                    CurrentSpread                   = 0.f;
    
    UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat>	                DefaultCurve;

	FTimeline				                SpringArmArmLengthTimeline;
	float					                SpringArmTargetArmLength        = 0.f;
    
	UPROPERTY(EditAnywhere)
	TSubclassOf<AHDWeapon>					DefaultWeaponClass;
    
    // 쓰는 칸의 무기
    UPROPERTY()
	TObjectPtr<AHDWeapon>	                Weapon;

    UPROPERTY()
    TObjectPtr<AHDWeapon>                   PrimaryWeapon;

    UPROPERTY()
    TObjectPtr<AHDWeapon>                   SecondaryWeapon;

    UPROPERTY()
    TObjectPtr<AHDWeapon>                   SupportWeapon;

    FHDWeaponSlots                          WeaponSlots;

    UPROPERTY()
	TObjectPtr<UGameplayEffect>	            ReloadGameplayEffect;

    UPROPERTY(EditAnywhere)
    TMap<EHDCombatMontage,TObjectPtr<UAnimMontage>> CombatMontage;
};
