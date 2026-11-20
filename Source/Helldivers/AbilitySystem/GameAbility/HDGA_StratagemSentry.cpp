// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_StratagemSentry.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameData/HDStratagemEffectData.h"
#include "Stratagem/BattleShip/HDBattleShip.h"
#include "Stratagem/Deployable/HDSentryTurret.h"

namespace HDGA_StratagemSentryPrivate
{
    // 받침 높이(Cube 100cm x 0.5)의 절반만큼 띄워 바닥이 표적 지면에 닿게 한다
    const FVector TurretSpawnOffset(0.f, 0.f, 25.f);
}

UHDGA_StratagemSentry::UHDGA_StratagemSentry(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetActivationEventTag(Tag_Event_Stratagem_Sentry);
}

void UHDGA_StratagemSentry::ExecuteStratagem(const FHDStratagemEffectData& EffectData)
{
    using namespace HDGA_StratagemSentryPrivate;

    AHDBattleShip* BattleShip = GetBattleShip();
    UWorld* World = GetWorld();
    if (World == nullptr || BattleShip == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    // 표적 회전은 던진 방향의 요(AHDStratagem::OnHit 가 Z 를 0 으로 만든다). 포탑은 그쪽을 보고 선다
    const FTransform SpawnTransform(GetTargetTransform().GetRotation(), GetTargetTransform().GetLocation() + TurretSpawnOffset);

    // 던진 플레이어(이벤트 Instigator = 비컨의 Instigator)를 포탑 Instigator 로. 포탑 총성의 소음 주체가 된다(계획 15행 사용자 결정 (A))
    APawn* Thrower = const_cast<APawn*>(Cast<APawn>(CurrentEventData.Instigator.Get()));
    AHDSentryTurret* Turret = World->SpawnActorDeferred<AHDSentryTurret>(
        AHDSentryTurret::StaticClass(),
        SpawnTransform,
        BattleShip,
        Thrower,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
    );

    if (Turret == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    Turret->InitializeTurret(EffectData.EffectLifetime, EffectData.EffectRange, EffectData.EffectInterval, EffectData.EffectMaxCount,
        BattleShip->GetProjectileClass(EffectData.StratagemProjectileType));
    UGameplayStatics::FinishSpawningActor(Turret, SpawnTransform);

    FinishStratagem(false);
}
