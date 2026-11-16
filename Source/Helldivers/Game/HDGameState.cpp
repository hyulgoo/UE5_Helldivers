// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGameState.h"
#include "Define/HDDefine.h"
#include "Stratagem/BattleShip/HDBattleShip.h"

AHDGameState::AHDGameState()
	: BattleShipClass(nullptr)
	, BattleShip(nullptr)
    , SpawnHeight(80000.f)
    , MissionPhase(EHDMissionPhase::Deploy)
    , CompletedObjectives(0)
    , RequiredObjectives(0)
    , ReinforcementsLeft(0)
    , ExtractionRemaining(0.f)
    , bPlayerInExtractionZone(false)
{
}

AHDBattleShip* AHDGameState::GetBattleShip() const
{
    return BattleShip;
}

void AHDGameState::SetBattleShip(AHDBattleShip* NewBattleShip)
{
    BattleShip = NewBattleShip;
}

void AHDGameState::SetMissionSnapshot(EHDMissionPhase NewPhase, int32 Completed, int32 Required, int32 Reinforcements, float NewExtractionRemaining, bool bInZone)
{
    const EHDMissionPhase OldPhase = MissionPhase;

    MissionPhase = NewPhase;
    CompletedObjectives = Completed;
    RequiredObjectives = Required;
    ReinforcementsLeft = Reinforcements;
    ExtractionRemaining = NewExtractionRemaining;
    bPlayerInExtractionZone = bInZone;

    if (OldPhase != NewPhase)
    {
        OnMissionPhaseChanged.Broadcast(OldPhase, NewPhase);
    }
}

void AHDGameState::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	VALID_CHECK(World);
	NULL_CHECK(BattleShipClass);

	const FVector SpawnLocation(0.f, 0.f, SpawnHeight);
	BattleShip = World->SpawnActor<AHDBattleShip>(BattleShipClass, SpawnLocation, FRotator::ZeroRotator);
	NULL_CHECK(BattleShip);
}
