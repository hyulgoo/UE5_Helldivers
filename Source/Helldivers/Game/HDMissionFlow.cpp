// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/HDMissionFlow.h"

FHDMissionFlow::FHDMissionFlow(const FHDMissionConfig& InConfig)
    : Config(InConfig)
    , Phase(EHDMissionPhase::Deploy)
    , CompletedObjectives(0)
    , ReinforcementsLeft(FMath::Max(0, InConfig.MaxReinforcements))
    , ExtractionRemaining(InConfig.ExtractionHoldTime)
    , ElapsedSeconds(0.f)
    , bPlayerAlive(true)
    , bPlayerInExtractionZone(false)
{
}

bool FHDMissionFlow::IsFinished() const
{
    return Phase == EHDMissionPhase::Success || Phase == EHDMissionPhase::Fail;
}

bool FHDMissionFlow::NotifyPlayerLanded()
{
    if (Phase != EHDMissionPhase::Deploy)
    {
        return false;
    }

    Phase = EHDMissionPhase::InProgress;
    return true;
}

bool FHDMissionFlow::NotifyObjectiveCompleted()
{
    if (Phase != EHDMissionPhase::InProgress)
    {
        return false;
    }

    ++CompletedObjectives;
    if (CompletedObjectives < Config.RequiredObjectives)
    {
        return false;
    }

    Phase = EHDMissionPhase::Extraction;
    ExtractionRemaining = Config.ExtractionHoldTime;
    return true;
}

void FHDMissionFlow::SetPlayerInExtractionZone(bool bInZone)
{
    if (IsFinished())
    {
        return;
    }

    bPlayerInExtractionZone = bInZone;
}

bool FHDMissionFlow::NotifyPlayerDied()
{
    if (IsFinished() || bPlayerAlive == false)
    {
        return false;
    }

    bPlayerAlive = false;
    if (ReinforcementsLeft > 0)
    {
        return false;
    }

    Phase = EHDMissionPhase::Fail;
    return true;
}

bool FHDMissionFlow::TryConsumeReinforcement()
{
    if (IsFinished() || bPlayerAlive || ReinforcementsLeft <= 0)
    {
        return false;
    }

    --ReinforcementsLeft;
    return true;
}

void FHDMissionFlow::NotifyPlayerRespawned()
{
    if (IsFinished())
    {
        return;
    }

    bPlayerAlive = true;
}

bool FHDMissionFlow::Tick(float DeltaSeconds)
{
    if (Phase != EHDMissionPhase::InProgress && Phase != EHDMissionPhase::Extraction)
    {
        return false;
    }

    ElapsedSeconds += DeltaSeconds;
    if (Config.MissionTimeLimit > 0.f && ElapsedSeconds >= Config.MissionTimeLimit)
    {
        Phase = EHDMissionPhase::Fail;
        return true;
    }

    if (Phase != EHDMissionPhase::Extraction || bPlayerInExtractionZone == false || bPlayerAlive == false)
    {
        return false;
    }

    ExtractionRemaining -= DeltaSeconds;
    if (ExtractionRemaining > 0.f)
    {
        return false;
    }

    ExtractionRemaining = 0.f;
    Phase = EHDMissionPhase::Success;
    return true;
}

void FHDMissionFlow::DebugSetReinforcementsLeft(int32 NewReinforcementsLeft)
{
    if (IsFinished())
    {
        return;
    }

    ReinforcementsLeft = FMath::Max(0, NewReinforcementsLeft);
}
