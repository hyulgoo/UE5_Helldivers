// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Weapon/WeaponTypes.h"
#include "GameData/HDWeaponData.h"
#include "HDWeapon.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpreadChanged, const float);

class USphereComponent;
class UAnimMontage;
class AHDCasing;
class AHDProjectileBase;

UCLASS()
class HELLDIVERS_API AHDWeapon : public AActor
{
    GENERATED_BODY()

    friend class UHDCombatComponent;

public:
    AHDWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    void                                EquipWeapon(AActor* OwnerActor);
    const void                          TraceEndWithScatter(const FVector& HitTarget);
    void                                SpawnProjectile(const FVector& HitTarget);

    // 지원 무기 칸에 들어가는 무기면 참(AHDSupportWeapon)
    virtual bool                        IsSupportWeapon() const { return false; }

    // 무기 몽타주에 Event.Spawn.Projectile·Event.Reload 노티파이가 없어 캐릭터가 C++ 로 보내야 하면 참.
    // 칸 규칙(IsSupportWeapon)과 따로 둔다: 노티파이가 있는 지원 무기가 생기면 이벤트가 두 번 가기 때문이다
    virtual bool                        SendsMontageEventsFromCode() const { return false; }

    // 손에서 떼어 Location 에 놓는다. 줍기 구는 PickupRearmDelay 뒤에 되살린다(바로 켜면 발밑에 버린 무기를 그 자리에서 다시 줍는다)
    void                                DropAt(const FVector& Location);

private:
    UFUNCTION()
    void                                OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult);

    void                                RearmPickupSphere();

public:
    FOnSpreadChanged                    OnSpreadChanged;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    TObjectPtr<USkeletalMeshComponent>	WeaponMesh;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Properties")
    TObjectPtr<USphereComponent>		AreaSphere;
    
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    EHDFireType                         FireType = EHDFireType::Count;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    TSubclassOf<AHDProjectileBase>      ProjectileClass;
    
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    TSubclassOf<AHDCasing>				CasingClass;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimMontage>            WeaponAnimMontage;
    
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    FGameplayTag                        EquipSoundTag;
            
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    float                               FireDelay;
    
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    uint8                               bIsAutoFire : 1;
    
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    FHDWeaponAmmoData                   AmmoData;
    
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    FHDWeaponErgoData                   ErgoData;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Default")
    FHDWeaponUIData                     UIData;

    EWeaponState                        WeaponState = EWeaponState::Drop;

private:
    FTimerHandle                        PickupRearmTimerHandle;
};
