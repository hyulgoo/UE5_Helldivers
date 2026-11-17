// Fill out your copyright notice in the Description page of Project Settings.

#include "Mission/HDMissionObjective.h"
#include "Engine/World.h"
#include "Game/HDGameMode.h"

AHDMissionObjective::AHDMissionObjective(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;
}

FString AHDMissionObjective::GetMarkerLabel() const
{
    return TEXT("OBJECTIVE");
}

FVector AHDMissionObjective::GetMarkerLocation() const
{
    return GetActorLocation() + FVector(0.f, 0.f, 400.f);
}

void AHDMissionObjective::CompleteObjective()
{
    if (bCompleted)
    {
        return;
    }

    bCompleted = true;

    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    if (HDGameMode == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDMission] %s completed but there is no HDGameMode"), *GetName());
        return;
    }

    HDGameMode->NotifyObjectiveCompleted(this);
}
