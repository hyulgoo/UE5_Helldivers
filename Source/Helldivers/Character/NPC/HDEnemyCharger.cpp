// Fill out your copyright notice in the Description page of Project Settings.

#include "HDEnemyCharger.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/GameAbility/HDGA_Dead.h"
#include "AbilitySystem/GameAbility/HDGA_EnemyMeleeAttack.h"

AHDEnemyCharger::AHDEnemyCharger(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Stats 는 FHDEnemyStats 기본값 그대로 쓴다

    // Mesh
    USkeletalMeshComponent* SkeletalMeshComponent = GetMesh();
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/Asset/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny'"));
    if (CharacterMeshRef.Succeeded())
    {
        SkeletalMeshComponent->SetSkeletalMesh(CharacterMeshRef.Object);
    }
    SkeletalMeshComponent->SetRelativeScale3D(FVector(1.1f));

    // Weapon
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMeshRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/Knife_A.Knife_A"));
    if (WeaponMeshRef.Succeeded())
    {
        EnemyWeaponMesh->SetSkeletalMesh(WeaponMeshRef.Object);
    }

    // Abilities
    Abilities.Add(UHDGA_Dead::StaticClass());
    Abilities.Add(UHDGA_EnemyMeleeAttack::StaticClass());

    // 폭발 넉백 이벤트(Data.Knockback.Ragdoll)를 받는 GA
    static ConstructorHelpers::FClassFinder<UGameplayAbility> RagdollAbilityRef(TEXT("/Game/Helldivers/Blueprint/GameAbility/Ability/BPGA_Ragdoll"));
    if (RagdollAbilityRef.Succeeded())
    {
        Abilities.Add(RagdollAbilityRef.Class);
    }
}
