// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/HDMissionFlow.h"

const TArray<EHDCommandInput>& HDMissionRules::GetExtractionCallSequence()
{
    static const TArray<EHDCommandInput> Sequence = { EHDCommandInput::Left, EHDCommandInput::Up, EHDCommandInput::Right, EHDCommandInput::Down };
    return Sequence;
}

bool HDMissionRules::IsExtractionCallSequence(const TArray<EHDCommandInput>& Input)
{
    // TArray 의 == 는 길이와 원소 순서를 모두 본다
    return Input == GetExtractionCallSequence();
}

const TCHAR* HDMissionRules::GetCallResultName(EHDExtractionCallResult Result)
{
    switch (Result)
    {
    case EHDExtractionCallResult::Called:           return TEXT("called");
    case EHDExtractionCallResult::NotRequired:      return TEXT("not required");
    case EHDExtractionCallResult::NotExtraction:    return TEXT("not extraction");
    case EHDExtractionCallResult::AlreadyCalled:    return TEXT("already called");
    case EHDExtractionCallResult::Dead:             return TEXT("dead");
    case EHDExtractionCallResult::NotInZone:        return TEXT("not in zone");
    default:                                        return TEXT("unknown");
    }
}

FHDMissionFlow::FHDMissionFlow(const FHDMissionConfig& InConfig)
    : Config(InConfig)
    , Phase(EHDMissionPhase::Deploy)
    , CompletedObjectives(0)
    , ReinforcementsLeft(FMath::Max(0, InConfig.MaxReinforcements))
    , ExtractionRemaining(InConfig.ExtractionHoldTime)
    , ElapsedSeconds(0.f)
    , bPlayerAlive(true)
    , bPlayerInExtractionZone(false)
    , bExtractionCalled(false)
    , bAbandoned(false)
{
}

bool FHDMissionFlow::IsFinished() const
{
    return Phase == EHDMissionPhase::Success || Phase == EHDMissionPhase::Fail;
}

bool FHDMissionFlow::IsExtractionCalled() const
{
    return Phase == EHDMissionPhase::Extraction && (bExtractionCalled || Config.bExtractionRequiresCall == false);
}

EHDExtractionCallResult FHDMissionFlow::CallExtraction()
{
    // 끝난 뒤(Success·Fail)도 여기서 걸린다
    if (Phase != EHDMissionPhase::Extraction)
    {
        return EHDExtractionCallResult::NotExtraction;
    }

    if (Config.bExtractionRequiresCall == false)
    {
        return EHDExtractionCallResult::NotRequired;
    }

    if (bExtractionCalled)
    {
        return EHDExtractionCallResult::AlreadyCalled;
    }

    if (bPlayerAlive == false)
    {
        return EHDExtractionCallResult::Dead;
    }

    if (bPlayerInExtractionZone == false)
    {
        return EHDExtractionCallResult::NotInZone;
    }

    bExtractionCalled = true;
    return EHDExtractionCallResult::Called;
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

    // 부른 뒤 지점을 나가면 멈추고, 돌아오면 다시 부르지 않아도 이어 준다
    if (Phase != EHDMissionPhase::Extraction || bPlayerInExtractionZone == false || bPlayerAlive == false || IsExtractionCalled() == false)
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

void FHDMissionFlow::DebugSetMissionTimeLimit(float NewMissionTimeLimit)
{
    if (IsFinished())
    {
        return;
    }

    Config.MissionTimeLimit = NewMissionTimeLimit;
}

bool FHDMissionFlow::Abandon()
{
    // 끝난 뒤(Success·Fail)에는 바꾸지 않는다. 남은 단계는 Deploy·InProgress·Extraction 뿐이다
    if (IsFinished())
    {
        return false;
    }

    Phase = EHDMissionPhase::Fail;
    bAbandoned = true;
    return true;
}

bool FHDMissionFlow::IsAbandoned() const
{
    return bAbandoned;
}
