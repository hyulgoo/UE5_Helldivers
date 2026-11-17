// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Game/HDMissionFlow.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDMissionFlowTest, "Helldivers.Mission.Flow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDMissionFlowTest
{
    // 목표 2, 증원 2, 추출 45초, 제한 없음
    FHDMissionConfig MakeTestConfig(float MissionTimeLimit = 0.f)
    {
        FHDMissionConfig Config;
        Config.RequiredObjectives = 2;
        Config.MaxReinforcements = 2;
        Config.ExtractionHoldTime = 45.f;
        Config.MissionTimeLimit = MissionTimeLimit;
        return Config;
    }

    FString PhaseName(EHDMissionPhase Phase)
    {
        return UEnum::GetValueAsString(Phase);
    }

    // 착지 -> 목표 2 로 Extraction 까지 보낸다
    void AdvanceToExtraction(FHDMissionFlow& Flow)
    {
        Flow.NotifyPlayerLanded();
        Flow.NotifyObjectiveCompleted();
        Flow.NotifyObjectiveCompleted();
    }
}

bool FHDMissionFlowTest::RunTest(const FString& Parameters)
{
    using namespace HDMissionFlowTest;

    // 정상 한 판
    {
        FHDMissionFlow Flow(MakeTestConfig());
        TestEqual(TEXT("Normal: starts in Deploy"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Deploy));
        TestTrue(TEXT("Normal: landing changes phase"), Flow.NotifyPlayerLanded());
        TestEqual(TEXT("Normal: InProgress after landing"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::InProgress));
        TestFalse(TEXT("Normal: first objective keeps phase"), Flow.NotifyObjectiveCompleted());
        TestEqual(TEXT("Normal: 1 objective completed"), Flow.GetCompletedObjectives(), 1);
        TestTrue(TEXT("Normal: second objective changes phase"), Flow.NotifyObjectiveCompleted());
        TestEqual(TEXT("Normal: Extraction after 2 objectives"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Extraction));
        TestEqual(TEXT("Normal: extraction remaining 45"), Flow.GetExtractionRemaining(), 45.f);
        Flow.SetPlayerInExtractionZone(true);
        TestTrue(TEXT("Normal: in zone"), Flow.IsPlayerInExtractionZone());
        TestTrue(TEXT("Normal: Tick(45) changes phase"), Flow.Tick(45.f));
        TestEqual(TEXT("Normal: Success at the end"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Success));
        TestTrue(TEXT("Normal: finished"), Flow.IsFinished());
    }

    // 착지 전 목표
    {
        FHDMissionFlow Flow(MakeTestConfig());
        TestFalse(TEXT("BeforeLanding: objective ignored"), Flow.NotifyObjectiveCompleted());
        TestEqual(TEXT("BeforeLanding: still Deploy"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Deploy));
        TestEqual(TEXT("BeforeLanding: 0 objectives"), Flow.GetCompletedObjectives(), 0);
    }

    // 존 밖 정지
    {
        FHDMissionFlow Flow(MakeTestConfig());
        AdvanceToExtraction(Flow);
        TestEqual(TEXT("OutOfZone: in Extraction"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Extraction));
        TestFalse(TEXT("OutOfZone: Tick(10) keeps phase"), Flow.Tick(10.f));
        TestEqual(TEXT("OutOfZone: remaining stays 45"), Flow.GetExtractionRemaining(), 45.f);
    }

    // 죽어 있으면 정지
    {
        FHDMissionFlow Flow(MakeTestConfig());
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        Flow.NotifyPlayerDied();
        TestFalse(TEXT("DeadInZone: player is dead"), Flow.IsPlayerAlive());
        TestFalse(TEXT("DeadInZone: Tick(10) keeps phase"), Flow.Tick(10.f));
        TestEqual(TEXT("DeadInZone: remaining stays 45"), Flow.GetExtractionRemaining(), 45.f);
        TestEqual(TEXT("DeadInZone: still Extraction"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Extraction));
    }

    // 증원 소모
    {
        FHDMissionFlow Flow(MakeTestConfig());
        Flow.NotifyPlayerLanded();
        TestFalse(TEXT("Reinforce: death with reinforcements keeps phase"), Flow.NotifyPlayerDied());
        TestTrue(TEXT("Reinforce: consume while dead"), Flow.TryConsumeReinforcement());
        TestEqual(TEXT("Reinforce: 1 left"), Flow.GetReinforcementsLeft(), 1);
        TestEqual(TEXT("Reinforce: still InProgress"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::InProgress));
    }

    // 살아 있을 땐 소모 안 함
    {
        FHDMissionFlow Flow(MakeTestConfig());
        Flow.NotifyPlayerLanded();
        TestFalse(TEXT("AliveNoConsume: consume refused"), Flow.TryConsumeReinforcement());
        TestEqual(TEXT("AliveNoConsume: 2 left"), Flow.GetReinforcementsLeft(), 2);
    }

    // 증원 0 에서 사망
    {
        FHDMissionFlow Flow(MakeTestConfig());
        Flow.NotifyPlayerLanded();
        for (int32 Index = 0; Index < 2; ++Index)
        {
            Flow.NotifyPlayerDied();
            Flow.TryConsumeReinforcement();
            Flow.NotifyPlayerRespawned();
        }
        TestEqual(TEXT("OutOfReinforcements: 0 left"), Flow.GetReinforcementsLeft(), 0);
        TestTrue(TEXT("OutOfReinforcements: alive after respawn"), Flow.IsPlayerAlive());
        TestTrue(TEXT("OutOfReinforcements: third death changes phase"), Flow.NotifyPlayerDied());
        TestEqual(TEXT("OutOfReinforcements: Fail"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Fail));
    }

    // 끝난 뒤 무시
    {
        FHDMissionFlow Flow(MakeTestConfig());
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        Flow.Tick(45.f);
        TestEqual(TEXT("Finished: Success"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Success));
        TestFalse(TEXT("Finished: objective ignored"), Flow.NotifyObjectiveCompleted());
        TestFalse(TEXT("Finished: death ignored"), Flow.NotifyPlayerDied());
        TestFalse(TEXT("Finished: Tick ignored"), Flow.Tick(10.f));
        TestTrue(TEXT("Finished: still alive"), Flow.IsPlayerAlive());
        TestEqual(TEXT("Finished: objectives stay 2"), Flow.GetCompletedObjectives(), 2);
        TestEqual(TEXT("Finished: still Success"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Success));
    }

    // 제한 시간
    {
        FHDMissionFlow Flow(MakeTestConfig(100.f));
        Flow.NotifyPlayerLanded();
        TestTrue(TEXT("TimeLimit: Tick(100) changes phase"), Flow.Tick(100.f));
        TestEqual(TEXT("TimeLimit: Fail"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Fail));
    }

    // 죽어 있는 동안 제한 시간 초과
    {
        FHDMissionFlow Flow(MakeTestConfig(100.f));
        Flow.NotifyPlayerLanded();
        Flow.NotifyPlayerDied();
        TestTrue(TEXT("DeadTimeout: Tick(100) changes phase"), Flow.Tick(100.f));
        TestEqual(TEXT("DeadTimeout: Fail"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Fail));
        TestFalse(TEXT("DeadTimeout: consume refused after Fail"), Flow.TryConsumeReinforcement());
        TestEqual(TEXT("DeadTimeout: 2 left"), Flow.GetReinforcementsLeft(), 2);
    }

    // 치트 제한 시간
    {
        FHDMissionFlow Flow(MakeTestConfig());
        Flow.NotifyPlayerLanded();
        TestFalse(TEXT("DebugTimeLimit: no limit, Tick(10) keeps phase"), Flow.Tick(10.f));
        Flow.DebugSetMissionTimeLimit(5.f);
        TestEqual(TEXT("DebugTimeLimit: limit 5"), Flow.GetMissionTimeLimit(), 5.f);
        TestTrue(TEXT("DebugTimeLimit: elapsed 10 past limit, next Tick changes phase"), Flow.Tick(0.1f));
        TestEqual(TEXT("DebugTimeLimit: Fail"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Fail));
        Flow.DebugSetMissionTimeLimit(0.f);
        TestEqual(TEXT("DebugTimeLimit: ignored after Fail"), Flow.GetMissionTimeLimit(), 5.f);
    }

    // 남은 시간 하한
    {
        FHDMissionFlow Flow(MakeTestConfig());
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        TestTrue(TEXT("RemainingFloor: Tick(60) changes phase"), Flow.Tick(60.f));
        TestEqual(TEXT("RemainingFloor: Success"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Success));
        TestEqual(TEXT("RemainingFloor: remaining is 0, not negative"), Flow.GetExtractionRemaining(), 0.f);
    }

    // 목표 초과
    {
        FHDMissionFlow Flow(MakeTestConfig());
        AdvanceToExtraction(Flow);
        TestFalse(TEXT("ExtraObjective: ignored in Extraction"), Flow.NotifyObjectiveCompleted());
        TestEqual(TEXT("ExtraObjective: objectives stay 2"), Flow.GetCompletedObjectives(), 2);
        TestEqual(TEXT("ExtraObjective: still Extraction"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Extraction));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
