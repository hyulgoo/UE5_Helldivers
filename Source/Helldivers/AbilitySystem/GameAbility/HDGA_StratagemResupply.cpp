// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_StratagemResupply.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameData/HDStratagemEffectData.h"
#include "Stratagem/BattleShip/HDBattleShip.h"
#include "Stratagem/Deployable/HDSupplyPod.h"

namespace HDGA_StratagemResupplyPrivate
{
    // 상자 높이(Cube 100cm x 0.9)의 절반만큼 띄워 바닥이 표적 지면에 닿게 한다
    const FVector PodSpawnOffset(0.f, 0.f, 45.f);
}

UHDGA_StratagemResupply::UHDGA_StratagemResupply(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetActivationEventTag(Tag_Event_Stratagem_Resupply);
}

void UHDGA_StratagemResupply::ExecuteStratagem(const FHDStratagemEffectData& EffectData)
{
    using namespace HDGA_StratagemResupplyPrivate;

    AHDBattleShip* BattleShip = GetBattleShip();
    UWorld* World = GetWorld();
    if (World == nullptr || BattleShip == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    // 표적 회전은 던진 방향(피치 포함)이라 쓰지 않는다. 상자는 똑바로 선다
    const FTransform SpawnTransform(GetTargetTransform().GetLocation() + PodSpawnOffset);
    AHDSupplyPod* SupplyPod = World->SpawnActorDeferred<AHDSupplyPod>(
        AHDSupplyPod::StaticClass(),
        SpawnTransform,
        BattleShip,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
    );

    if (SupplyPod == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    SupplyPod->InitializePod(EffectData.EffectLifetime, EffectData.EffectRange);
    UGameplayStatics::FinishSpawningActor(SupplyPod, SpawnTransform);

    FinishStratagem(false);
}
