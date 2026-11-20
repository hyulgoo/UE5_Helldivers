
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
    // 다른 칸에 무기가 있으면 그 무기로 바꾸고 참. 쓰던 무기의 탄약은 ASC 의 지금 값으로 적어 둔다
    bool                                    SwapWeapon(UAbilitySystemComponent* ASC);
    // 두 칸의 무기(비어 있는 칸은 뺀다)
    void                                    GetAllWeapons(TArray<AHDWeapon*>& OutWeapons) const;
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

protected:
    virtual void                            BeginPlay() override final;
    virtual void                            TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void					                TraceUnderCrosshairs();
    void					                AimOffset(const float DeltaTime);
    void									TurnInPlace(const float DeltaTime);
    // 주인 앞 150 의 지면
    FVector                                 GetWeaponDropLocation() const;

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
    TObjectPtr<AHDWeapon>                   SupportWeapon;

    FHDWeaponSlots                          WeaponSlots;

    UPROPERTY()
	TObjectPtr<UGameplayEffect>	            ReloadGameplayEffect;

    UPROPERTY(EditAnywhere)
    TMap<EHDCombatMontage,TObjectPtr<UAnimMontage>> CombatMontage;
};
