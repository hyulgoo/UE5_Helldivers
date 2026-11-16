// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGameMode.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Game/HDGameState.h"
#include "Game/HDMissionSettings.h"
#include "Game/HDSpawnDirectorSubsystem.h"
#include "GameFramework/PlayerController.h"

AHDGameMode::AHDGameMode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerClassRef(TEXT("/Script/Helldivers.HDPlayerController"));
    if (PlayerControllerClassRef.Class)
    {
        PlayerControllerClass = PlayerControllerClassRef.Class;
    }

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AHDGameMode::StartPlay()
{
	Super::StartPlay();

    const UHDMissionSettings* Settings = GetDefault<UHDMissionSettings>();
    MissionFlow = MakeUnique<FHDMissionFlow>(Settings->MakeConfig());
    SyncGameState();

    UE_LOG(LogTemp, Display, TEXT("[HDMission] start objectives=%d reinforcements=%d"), MissionFlow->GetRequiredObjectives(), MissionFlow->GetReinforcementsLeft());
}

void AHDGameMode::PostLogin(APlayerController* NewPlayer)
{
    // Super 안의 RestartPlayer 가 첫 빙의를 하므로 그 전에 묶는다
    if (NewPlayer)
    {
        NewPlayer->OnPossessedPawnChanged.AddUniqueDynamic(this, &AHDGameMode::HandlePossessedPawnChanged);
    }

    Super::PostLogin(NewPlayer);
}

void AHDGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ApplyMissionChange([DeltaSeconds](FHDMissionFlow& Flow) { Flow.Tick(DeltaSeconds); });
}

void AHDGameMode::NotifyObjectiveCompleted(AActor* Objective)
{
    ApplyMissionChange([](FHDMissionFlow& Flow) { Flow.NotifyObjectiveCompleted(); });
}

void AHDGameMode::NotifyPlayerInExtractionZone(bool bInZone)
{
    ApplyMissionChange([bInZone](FHDMissionFlow& Flow) { Flow.SetPlayerInExtractionZone(bInZone); });
}

void AHDGameMode::NotifyPlayerDied(AController* DeadController)
{
    ApplyMissionChange([](FHDMissionFlow& Flow) { Flow.NotifyPlayerDied(); });
}

const FHDMissionFlow* AHDGameMode::GetMissionFlow() const
{
    return MissionFlow.Get();
}

FHDMissionFlow* AHDGameMode::GetMissionFlowForDebug()
{
    return MissionFlow.Get();
}

void AHDGameMode::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    if (Cast<AHDCharacterPlayer>(NewPawn) == nullptr || MissionFlow.IsValid() == false)
    {
        return;
    }

    if (MissionFlow->GetPhase() == EHDMissionPhase::Deploy)
    {
        ApplyMissionChange([](FHDMissionFlow& Flow) { Flow.NotifyPlayerLanded(); });
    }
    else
    {
        ApplyMissionChange([](FHDMissionFlow& Flow) { Flow.NotifyPlayerRespawned(); });
    }
}

void AHDGameMode::ApplyMissionChange(TFunctionRef<void(FHDMissionFlow&)> Change)
{
    if (MissionFlow.IsValid() == false)
    {
        return;
    }

    const EHDMissionPhase OldPhase = MissionFlow->GetPhase();
    Change(*MissionFlow);
    SyncGameState();

    const EHDMissionPhase NewPhase = MissionFlow->GetPhase();
    if (OldPhase != NewPhase)
    {
        HandlePhaseChanged(OldPhase, NewPhase);
    }
}

void AHDGameMode::SyncGameState() const
{
    AHDGameState* HDGameState = GetGameState<AHDGameState>();
    if (HDGameState == nullptr || MissionFlow.IsValid() == false)
    {
        return;
    }

    HDGameState->SetMissionSnapshot(
        MissionFlow->GetPhase(),
        MissionFlow->GetCompletedObjectives(),
        MissionFlow->GetRequiredObjectives(),
        MissionFlow->GetReinforcementsLeft(),
        MissionFlow->GetExtractionRemaining(),
        MissionFlow->IsPlayerInExtractionZone());
}

void AHDGameMode::HandlePhaseChanged(EHDMissionPhase OldPhase, EHDMissionPhase NewPhase)
{
    UE_LOG(LogTemp, Display, TEXT("[HDMission] phase %s -> %s"), *UEnum::GetValueAsString(OldPhase), *UEnum::GetValueAsString(NewPhase));

    UWorld* World = GetWorld();
    UHDSpawnDirectorSubsystem* Director = World ? World->GetSubsystem<UHDSpawnDirectorSubsystem>() : nullptr;
    if (Director == nullptr)
    {
        return;
    }

    if (NewPhase == EHDMissionPhase::InProgress)
    {
        Director->StartDirector();
    }
    else if (NewPhase == EHDMissionPhase::Success || NewPhase == EHDMissionPhase::Fail)
    {
        Director->StopDirector();
    }
}
