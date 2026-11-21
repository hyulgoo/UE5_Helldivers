// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HDSecondaryWeapon.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Projectile/HDProjectileBase.h"
#include "Weapon/HDCasing.h"

AHDSecondaryWeapon::AHDSecondaryWeapon(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 메시: MuzzleFlash·AmmoEject 소켓이 있다. 애니메이션은 BP_Rifle 과 같은 기본 AnimInstance 로 몽타주만 돌린다
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMeshRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/Pistols_A.Pistols_A"));
    if (WeaponMeshRef.Succeeded())
    {
        WeaponMesh->SetSkeletalMesh(WeaponMeshRef.Object);
    }
    WeaponMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    WeaponMesh->SetAnimInstanceClass(UAnimInstance::StaticClass());

    // 섹션은 Fire·Reload_Hip 뿐이다. 다른 재장전 섹션은 캐릭터가 Reload_Hip 으로 바꾼다(AHDCharacterPlayer::PlayWeaponMontage)
    static ConstructorHelpers::FObjectFinder<UAnimMontage> WeaponMontageRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/Animations/Montage/SK_AM_Pistol.SK_AM_Pistol"));
    if (WeaponMontageRef.Succeeded())
    {
        WeaponAnimMontage = WeaponMontageRef.Object;
    }

    static ConstructorHelpers::FClassFinder<AHDProjectileBase> ProjectileClassRef(TEXT("/Game/Helldivers/Blueprint/Projectile/BPP_Bullet_Light"));
    if (ProjectileClassRef.Succeeded())
    {
        ProjectileClass = ProjectileClassRef.Class;
    }

    static ConstructorHelpers::FClassFinder<AHDCasing> CasingClassRef(TEXT("/Game/Helldivers/Blueprint/Weapon/Casing/BP_Casing"));
    if (CasingClassRef.Succeeded())
    {
        CasingClass = CasingClassRef.Class;
    }

    // 한 발씩 0.25초 간격. 탄창 15발, 예비 탄창 4개(BP_Rifle 은 45·45·7·7)
    FireType    = EHDFireType::Projectile;
    FireDelay   = 0.25f;
    bIsAutoFire = false;

    AmmoData.Ammo           = 15;
    AmmoData.MaxAmmo        = 15;
    AmmoData.Capacity       = 4;
    AmmoData.MaxCapacity    = 4;

    // BP_Rifle 은 50·10·5
    ErgoData.ZoomedFOV          = 40.f;
    ErgoData.ZoomInterpSpeed    = 12.f;
    ErgoData.ErgonomicFactor    = 3.f;
}
