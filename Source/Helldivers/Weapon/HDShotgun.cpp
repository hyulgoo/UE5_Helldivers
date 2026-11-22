// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HDShotgun.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Projectile/HDProjectileBase.h"
#include "Weapon/HDCasing.h"

AHDShotgun::AHDShotgun(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 메시: MuzzleFlash·AmmoEject 소켓이 있다. 애니메이션은 BP_Rifle 과 같은 기본 AnimInstance 로 몽타주만 돌린다
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMeshRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/Shotgun_A.Shotgun_A"));
    if (WeaponMeshRef.Succeeded())
    {
        WeaponMesh->SetSkeletalMesh(WeaponMeshRef.Object);
    }
    WeaponMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    WeaponMesh->SetAnimInstanceClass(UAnimInstance::StaticClass());

    // 섹션 Fire·Reload_Hip·Reload_Prone·Reload_Shoulder, 노티파이 없음
    static ConstructorHelpers::FObjectFinder<UAnimMontage> WeaponMontageRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/Animations/Montage/SK_AM_Shotgun.SK_AM_Shotgun"));
    if (WeaponMontageRef.Succeeded())
    {
        WeaponAnimMontage = WeaponMontageRef.Object;
    }

    static ConstructorHelpers::FClassFinder<AHDProjectileBase> ProjectileClassRef(TEXT("/Game/Helldivers/Blueprint/Projectile/BPP_Bullet_Light"));
    if (ProjectileClassRef.Succeeded())
    {
        ProjectileClass = ProjectileClassRef.Class;
    }

    static ConstructorHelpers::FClassFinder<AHDCasing> CasingClassRef(TEXT("/Game/Helldivers/Blueprint/Weapon/Casing/BP_Shotgun_Casing"));
    if (CasingClassRef.Succeeded())
    {
        CasingClass = CasingClassRef.Class;
    }

    // C++ 무기가 검증된 Projectile 경로로 쏜다(EHDFireType::Shotgun 은 C++ 이 가르지 않는다). 한 발씩 0.9초 간격, 탄창 6발, 예비 탄창 5개(BP_Rifle 은 45·45·7·7)
    FireType    = EHDFireType::Projectile;
    FireDelay   = 0.9f;
    bIsAutoFire = false;

    AmmoData.Ammo           = 6;
    AmmoData.MaxAmmo        = 6;
    AmmoData.Capacity       = 5;
    AmmoData.MaxCapacity    = 5;

    // BP_Rifle 은 50·10·5
    ErgoData.ZoomedFOV          = 55.f;
    ErgoData.ZoomInterpSpeed    = 10.f;
    ErgoData.ErgonomicFactor    = 4.f;

    // BP_Rifle 은 2
    ArmorPenetration = 1;

    // 한 번에 8발을 6도 원뿔 안에 흩뿌린다
    PelletCount         = 8;
    PelletSpreadDegrees = 6.f;
}
