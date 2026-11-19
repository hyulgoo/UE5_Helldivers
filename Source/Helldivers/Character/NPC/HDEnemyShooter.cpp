// Fill out your copyright notice in the Description page of Project Settings.

#include "HDEnemyShooter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/GameAbility/HDGA_Dead.h"
#include "AbilitySystem/GameAbility/HDGA_EnemyRangedAttack.h"
#include "Define/HDSocketNames.h"

AHDEnemyShooter::AHDEnemyShooter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Stats: 나머지는 FHDEnemyStats 기본값
    Stats.MaxHealth         = 80.f;
    Stats.WalkSpeed         = 320.f;
    Stats.AttackRange       = 2500.f;
    Stats.PreferredRange    = 1800.f;
    Stats.AttackWindup      = 0.35f;
    Stats.AttackCooldown    = 2.4f;
    Stats.BurstCount        = 3;
    Stats.BurstInterval     = 0.15f;
    Stats.AimSpreadDegrees  = 3.f;

    // Mesh
    USkeletalMeshComponent* SkeletalMeshComponent = GetMesh();
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/Asset/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny'"));
    if (CharacterMeshRef.Succeeded())
    {
        SkeletalMeshComponent->SetSkeletalMesh(CharacterMeshRef.Object);
    }
    SkeletalMeshComponent->SetRelativeScale3D(FVector(0.95f));

    // Weapon
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMeshRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/Assault_Rifle_A.Assault_Rifle_A"));
    if (WeaponMeshRef.Succeeded())
    {
        EnemyWeaponMesh->SetSkeletalMesh(WeaponMeshRef.Object);
    }

    // Abilities
    Abilities.Add(UHDGA_Dead::StaticClass());
    Abilities.Add(UHDGA_EnemyRangedAttack::StaticClass());

    // 폭발 넉백 이벤트(Data.Knockback.Ragdoll)를 받는 GA
    static ConstructorHelpers::FClassFinder<UGameplayAbility> RagdollAbilityRef(TEXT("/Game/Helldivers/Blueprint/GameAbility/Ability/BPGA_Ragdoll"));
    if (RagdollAbilityRef.Succeeded())
    {
        Abilities.Add(RagdollAbilityRef.Class);
    }
}

FVector AHDEnemyShooter::GetMuzzleLocation() const
{
    // 조준 자세 애니메이션이 없어 소총이 아래를 향하므로 MuzzleFlash 소켓은 지면 바로 위(발목 높이)다.
    // 거기서 쏘면 비탈에서 총알이 곧바로 지형에 박힌다. AI 시선 판정(GetPawnViewLocation → 가슴)과 같은 눈 높이에서 쏜다
    constexpr float MuzzleForwardOffset = 40.f;
    return GetPawnViewLocation() + GetActorForwardVector() * MuzzleForwardOffset;
}
