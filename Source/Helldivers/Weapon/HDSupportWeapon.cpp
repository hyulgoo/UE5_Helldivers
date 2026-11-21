// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HDSupportWeapon.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Projectile/HDProjectileBase.h"
#include "Weapon/HDCasing.h"

AHDSupportWeapon::AHDSupportWeapon(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 메시: MuzzleFlash·AmmoEject 소켓이 있다. 애니메이션은 BP_Rifle 과 같은 기본 AnimInstance 로 몽타주만 돌린다
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMeshRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/Sniper_Rifle_A.Sniper_Rifle_A"));
    if (WeaponMeshRef.Succeeded())
    {
        WeaponMesh->SetSkeletalMesh(WeaponMeshRef.Object);
    }
    WeaponMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    WeaponMesh->SetAnimInstanceClass(UAnimInstance::StaticClass());

    // 섹션 Fire·Reload_Hip·Reload_Shoulder·Reload_Prone 이 SK_AM_Rifle 과 같다
    static ConstructorHelpers::FObjectFinder<UAnimMontage> WeaponMontageRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/Animations/Montage/SK_AM_SniperRifle.SK_AM_SniperRifle"));
    if (WeaponMontageRef.Succeeded())
    {
        WeaponAnimMontage = WeaponMontageRef.Object;
    }

    static ConstructorHelpers::FClassFinder<AHDProjectileBase> ProjectileClassRef(TEXT("/Game/Helldivers/Blueprint/Projectile/BPP_Bullet_Heavy"));
    if (ProjectileClassRef.Succeeded())
    {
        ProjectileClass = ProjectileClassRef.Class;
    }

    static ConstructorHelpers::FClassFinder<AHDCasing> CasingClassRef(TEXT("/Game/Helldivers/Blueprint/Weapon/Casing/BP_Casing"));
    if (CasingClassRef.Succeeded())
    {
        CasingClass = CasingClassRef.Class;
    }

    // 한 발씩 1초 간격. 탄창 6발, 예비 탄창 4개(BP_Rifle 은 45·45·7·7)
    FireType    = EHDFireType::Projectile;
    FireDelay   = 1.f;
    bIsAutoFire = false;

    AmmoData.Ammo           = 6;
    AmmoData.MaxAmmo        = 6;
    AmmoData.Capacity       = 4;
    AmmoData.MaxCapacity    = 4;

    // BP_Rifle 은 50·10·5
    ErgoData.ZoomedFOV          = 25.f;
    ErgoData.ZoomInterpSpeed    = 10.f;
    ErgoData.ErgonomicFactor    = 5.f;

    // BP_Rifle 은 2
    ArmorPenetration = 4;
}
