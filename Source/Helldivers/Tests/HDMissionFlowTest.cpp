// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Game/HDMissionFlow.h"
#include "Stratagem/HDStratagemCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDMissionFlowTest, "Helldivers.Mission.Flow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDMissionFlowTest
{
    // 목표 2, 증원 2, 추출 45초, 제한 없음. bRequiresCall 기본값 거짓 = 호출 없이 지점 안에 있기만 하면 준다(호출 도입 전 동작)
    FHDMissionConfig MakeTestConfig(float MissionTimeLimit = 0.f, bool bRequiresCall = false)
    {
        FHDMissionConfig Config;
        Config.RequiredObjectives = 2;
        Config.MaxReinforcements = 2;
        Config.ExtractionHoldTime = 45.f;
        Config.MissionTimeLimit = MissionTimeLimit;
        Config.bExtractionRequiresCall = bRequiresCall;
        return Config;
    }

    FString PhaseName(EHDMissionPhase Phase)
    {
        return UEnum::GetValueAsString(Phase);
    }

    FString CallName(EHDExtractionCallResult Result)
    {
        return HDMissionRules::GetCallResultName(Result);
    }

    // Prefix 가 Whole 의 앞부분인가(같은 것 포함)
    bool IsCommandPrefix(const TArray<EHDCommandInput>& Prefix, const TArray<EHDCommandInput>& Whole)
    {
        if (Prefix.Num() > Whole.Num())
        {
            return false;
        }

        for (int32 Index = 0; Index < Prefix.Num(); ++Index)
        {
            if (Prefix[Index] != Whole[Index])
            {
                return false;
            }
        }
        return true;
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

    // C1 추출 호출 정상: 부르기 전에는 지점 안이어도 대기 시간이 흐르지 않는다
    {
        FHDMissionFlow Flow(MakeTestConfig(0.f, true));
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        TestFalse(TEXT("C1 Call: not called before the call"), Flow.IsExtractionCalled());
        TestFalse(TEXT("C1 Call: Tick(10) before the call keeps phase"), Flow.Tick(10.f));
        TestEqual(TEXT("C1 Call: remaining stays 45 before the call"), Flow.GetExtractionRemaining(), 45.f);
        TestEqual(TEXT("C1 Call: call accepted"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::Called));
        TestTrue(TEXT("C1 Call: called after the call"), Flow.IsExtractionCalled());
        TestTrue(TEXT("C1 Call: Tick(45) changes phase"), Flow.Tick(45.f));
        TestEqual(TEXT("C1 Call: Success"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Success));
    }

    // C2 지점 밖에서 호출
    {
        FHDMissionFlow Flow(MakeTestConfig(0.f, true));
        AdvanceToExtraction(Flow);
        TestEqual(TEXT("C2 OutOfZoneCall: refused not in zone"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::NotInZone));
        TestFalse(TEXT("C2 OutOfZoneCall: not called"), Flow.IsExtractionCalled());
    }

    // C3 추출 단계가 아닐 때 호출
    {
        FHDMissionFlow Flow(MakeTestConfig(0.f, true));
        Flow.NotifyPlayerLanded();
        TestEqual(TEXT("C3 NotExtractionCall: in InProgress"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::InProgress));
        TestEqual(TEXT("C3 NotExtractionCall: refused not extraction"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::NotExtraction));
    }

    // C4 죽어 있을 때 호출
    {
        FHDMissionFlow Flow(MakeTestConfig(0.f, true));
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        Flow.NotifyPlayerDied();
        TestEqual(TEXT("C4 DeadCall: refused dead"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::Dead));
        TestFalse(TEXT("C4 DeadCall: not called"), Flow.IsExtractionCalled());
    }

    // C5 두 번 호출
    {
        FHDMissionFlow Flow(MakeTestConfig(0.f, true));
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        TestEqual(TEXT("C5 TwiceCall: first accepted"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::Called));
        TestEqual(TEXT("C5 TwiceCall: second refused already called"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::AlreadyCalled));
    }

    // C6 부른 뒤 지점을 나갔다 돌아오면 다시 부르지 않아도 이어서 준다
    {
        FHDMissionFlow Flow(MakeTestConfig(0.f, true));
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        TestEqual(TEXT("C6 LeaveAfterCall: call accepted"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::Called));
        TestFalse(TEXT("C6 LeaveAfterCall: Tick(10) in zone keeps phase"), Flow.Tick(10.f));
        TestEqual(TEXT("C6 LeaveAfterCall: remaining 35 after Tick(10)"), Flow.GetExtractionRemaining(), 35.f);
        Flow.SetPlayerInExtractionZone(false);
        TestFalse(TEXT("C6 LeaveAfterCall: Tick(10) out of zone keeps phase"), Flow.Tick(10.f));
        TestEqual(TEXT("C6 LeaveAfterCall: remaining stays 35 out of zone"), Flow.GetExtractionRemaining(), 35.f);
        Flow.SetPlayerInExtractionZone(true);
        TestTrue(TEXT("C6 LeaveAfterCall: Tick(35) back in zone changes phase without a new call"), Flow.Tick(35.f));
        TestEqual(TEXT("C6 LeaveAfterCall: Success"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Success));
    }

    // C7 호출이 필요 없는 설정
    {
        FHDMissionFlow Flow(MakeTestConfig(0.f, false));
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        TestTrue(TEXT("C7 NoCallNeeded: called without a call"), Flow.IsExtractionCalled());
        TestEqual(TEXT("C7 NoCallNeeded: call refused not required"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::NotRequired));
        TestTrue(TEXT("C7 NoCallNeeded: Tick(45) changes phase"), Flow.Tick(45.f));
        TestEqual(TEXT("C7 NoCallNeeded: Success"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Success));
    }

    // C8 끝난 뒤 호출
    {
        FHDMissionFlow Flow(MakeTestConfig(0.f, true));
        AdvanceToExtraction(Flow);
        Flow.SetPlayerInExtractionZone(true);
        Flow.CallExtraction();
        Flow.Tick(45.f);
        TestEqual(TEXT("C8 FinishedCall: Success"), PhaseName(Flow.GetPhase()), PhaseName(EHDMissionPhase::Success));
        TestEqual(TEXT("C8 FinishedCall: refused not extraction"), CallName(Flow.CallExtraction()), CallName(EHDExtractionCallResult::NotExtraction));
    }

    // C9 호출 커맨드 판정: 길이·순서가 모두 같을 때만
    {
        using EIn = EHDCommandInput;
        TestTrue(TEXT("C9 CallSequence: Left Up Right Down"), HDMissionRules::IsExtractionCallSequence({ EIn::Left, EIn::Up, EIn::Right, EIn::Down }));
        TestFalse(TEXT("C9 CallSequence: Left Up Right (short)"), HDMissionRules::IsExtractionCallSequence({ EIn::Left, EIn::Up, EIn::Right }));
        TestFalse(TEXT("C9 CallSequence: Left Up Right Down Down (long)"), HDMissionRules::IsExtractionCallSequence({ EIn::Left, EIn::Up, EIn::Right, EIn::Down, EIn::Down }));
        TestFalse(TEXT("C9 CallSequence: empty"), HDMissionRules::IsExtractionCallSequence({}));
        TestFalse(TEXT("C9 CallSequence: Up Right Down Right"), HDMissionRules::IsExtractionCallSequence({ EIn::Up, EIn::Right, EIn::Down, EIn::Right }));
    }

    // C10 호출 커맨드와 스트라타젬 카탈로그 커맨드가 앞부분으로 겹치지 않는다(같은 것 포함)
    {
        const FHDStratagemCatalog& Catalog = FHDStratagemCatalog::GetProjectCatalog();
        const TArray<EHDCommandInput>& CallSequence = HDMissionRules::GetExtractionCallSequence();
        TestTrue(TEXT("C10 NoOverlap: catalog has entries"), Catalog.Num() > 0);

        FString OverlapNames;
        for (const FName& Name : Catalog.GetNames())
        {
            const FHDStratagemDefinition* Definition = Catalog.Find(Name);
            if (Definition == nullptr)
            {
                continue;
            }

            if (IsCommandPrefix(Definition->CommandSequence, CallSequence) || IsCommandPrefix(CallSequence, Definition->CommandSequence))
            {
                OverlapNames += Name.ToString() + TEXT(" ");
            }
        }
        TestTrue(FString::Printf(TEXT("C10 NoOverlap: call command overlaps [ %s]"), *OverlapNames), OverlapNames.IsEmpty());
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
