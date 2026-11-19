// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_StratagemOrbital.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Stratagem/HDStratagemPattern.h"
#include "Stratagem/BattleShip/HDBattleShip.h"
#include "Weapon/Projectile/HDProjectileBase.h"

UHDGA_StratagemOrbital::UHDGA_StratagemOrbital(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetActivationEventTag(Tag_Event_Stratagem_Orbital);
}

void UHDGA_StratagemOrbital::ExecuteStratagem(const FHDStratagemEffectData& EffectData)
{
    OrbitalData = EffectData;
    ShotIndex = 0;
    ShotCount = HDStratagemPattern::GetProjectileCount(OrbitalData);
    ShotInterval = HDStratagemPattern::GetShotInterval(OrbitalData);
    RandomStream.Initialize(FMath::Rand());

    FireShots();
}

void UHDGA_StratagemOrbital::FireShots()
{
    while (ShotIndex < ShotCount)
    {
        SpawnProjectile(ShotIndex);
        ++ShotIndex;

        if (ShotInterval > 0.f && ShotIndex < ShotCount)
        {
            UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, ShotInterval);
            WaitTask->OnFinish.AddDynamic(this, &UHDGA_StratagemOrbital::FireShots);
            WaitTask->ReadyForActivation();
            return;
        }
    }

    // PIE 판정용(D12): 이 인스턴스가 끝까지 자기 표적에 쐈는지 본다(계획 「현황 7-③」 덮어쓰기)
    const FVector TargetLocation = GetTargetTransform().GetLocation();
    UE_LOG(LogTemp, Display, TEXT("[BattleShip] %s done shots=%d at=(%.0f, %.0f)"), *GetStratagemName().ToString(), ShotIndex, TargetLocation.X, TargetLocation.Y);
    FinishStratagem(false);
}

void UHDGA_StratagemOrbital::SpawnProjectile(const int32 Index)
{
    AHDBattleShip* BattleShip = GetBattleShip();
    UWorld* World = GetWorld();
    const TSubclassOf<AHDProjectileBase> ProjectileClass = BattleShip ? BattleShip->GetProjectileClass(OrbitalData.StratagemProjectileType) : nullptr;
    NULL_CHECK(World);
    NULL_CHECK(ProjectileClass);

    const FVector2D DropOffset = HDStratagemPattern::GetDropOffset(OrbitalData, Index, RandomStream);
    const FVector TargetLocation = GetTargetTransform().GetLocation() + FVector(DropOffset.X, DropOffset.Y, 0.f);
    const FVector SpawnLocation = BattleShip->GetActorLocation();
    const FTransform SpawnTransform((TargetLocation - SpawnLocation).Rotation(), SpawnLocation);

    // 소유자 = 전함. AHDProjectileBase 가 소유자의 ASC 로 피해를 준다
    AHDProjectileBase* Projectile = World->SpawnActorDeferred<AHDProjectileBase>(
        ProjectileClass,
        SpawnTransform,
        BattleShip,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
    );

    UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
}
