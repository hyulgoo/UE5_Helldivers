// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_StratagemEagle.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Stratagem/BattleShip/HDBattleShip.h"
#include "Stratagem/Eagle/HDEagleFighter.h"

UHDGA_StratagemEagle::UHDGA_StratagemEagle(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetActivationEventTag(Tag_Event_Stratagem_Eagle);
}

void UHDGA_StratagemEagle::ExecuteStratagem(const FHDStratagemEffectData& EffectData)
{
    AHDBattleShip* BattleShip = GetBattleShip();
    UWorld* World = GetWorld();
    const TSubclassOf<AHDEagleFighter> EagleFighterClass = BattleShip ? BattleShip->GetEagleFighterClass() : nullptr;
    if (World == nullptr || EagleFighterClass == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    // 회전 = 던진 방향(AHDStratagem::OnHit). 전투기는 이 방향을 기준으로 진입 방향을 정한다
    const FTransform& SpawnTransform = GetTargetTransform();
    AHDEagleFighter* EagleFighter = World->SpawnActorDeferred<AHDEagleFighter>(
        EagleFighterClass,
        SpawnTransform,
        BattleShip,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
    );

    if (EagleFighter == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    EagleFighter->InitializeStrike(BattleShip->GetActorLocation(), EffectData, SpawnTransform.GetLocation(),
        BattleShip->GetProjectileClass(EHDStratagemProjectile::Bomb), BattleShip->GetProjectileClass(EHDStratagemProjectile::Bullet));
    UGameplayStatics::FinishSpawningActor(EagleFighter, SpawnTransform);

    FinishStratagem(false);
}
