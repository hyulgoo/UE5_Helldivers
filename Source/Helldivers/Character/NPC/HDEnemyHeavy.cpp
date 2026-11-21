// Fill out your copyright notice in the Description page of Project Settings.

#include "HDEnemyHeavy.h"
#include "Components/SkeletalMeshComponent.h"

AHDEnemyHeavy::AHDEnemyHeavy(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Stats: 나머지는 원거리형(부모) 값. 메시·무기·어빌리티·총구 위치도 부모 것을 그대로 쓴다
    Stats.MaxHealth         = 400.f;
    Stats.WalkSpeed         = 220.f;
    Stats.PreferredRange    = 1200.f;
    Stats.AttackCooldown    = 3.f;
    Stats.BurstCount        = 5;

    // Armor: 몸통 3(주무기 도탄), 팔다리 2(주무기 65%), 머리 1(약점. 2배는 AHDCharacterNonPlayer 기본값)
    ArmorProfile.BodyArmor  = 3;
    ArmorProfile.ArmArmor   = 2;
    ArmorProfile.LegArmor   = 2;
    ArmorProfile.HeadArmor  = 1;

    // Mesh: 다른 적보다 크게
    GetMesh()->SetRelativeScale3D(FVector(1.3f));
}
