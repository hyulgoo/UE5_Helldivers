// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Progression/HDMissionReward.h"
#include "Progression/HDProfileTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDMissionRewardTest, "Helldivers.Progression.Reward", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDMissionRewardTest
{
    FHDSampleCounts MakeSamples(const int32 Common, const int32 Rare, const int32 Super)
    {
        FHDSampleCounts Samples;
        Samples.Common  = Common;
        Samples.Rare    = Rare;
        Samples.Super   = Super;
        return Samples;
    }

    FHDMissionOutcome MakeOutcome(const bool bSuccess, const bool bAbandoned, const int32 Completed, const int32 Required, const float RewardMultiplier, const FHDSampleCounts& CarriedSamples = FHDSampleCounts())
    {
        FHDMissionOutcome Outcome;
        Outcome.bSuccess            = bSuccess;
        Outcome.bAbandoned          = bAbandoned;
        Outcome.CompletedObjectives = Completed;
        Outcome.RequiredObjectives  = Required;
        Outcome.RewardMultiplier    = RewardMultiplier;
        Outcome.CarriedSamples      = CarriedSamples;
        return Outcome;
    }

    // "c/r/s". 실패 메시지에 값이 보이게 글자로 견준다
    FString SamplesText(const FHDSampleCounts& Samples)
    {
        return FString::Printf(TEXT("%d/%d/%d"), Samples.Common, Samples.Rare, Samples.Super);
    }
}

bool FHDMissionRewardTest::RunTest(const FString& Parameters)
{
    using namespace HDMissionRewardTest;

    FHDRewardRules Rules;
    Rules.RequisitionPerObjective = 40;
    Rules.SuccessRequisitionBonus = 100;

    // R1 성공: (2 × 40 + 100) × 1.25 = 225, 소지 샘플이 들어간다
    {
        const FHDMissionReward Reward = HDProgression::ComputeReward(MakeOutcome(true, false, 2, 2, 1.25f, MakeSamples(3, 1, 0)), Rules);
        TestEqual(TEXT("R1 Success: requisition 225"), Reward.Requisition, 225);
        TestEqual(TEXT("R1 Success: samples 3/1/0"), SamplesText(Reward.Samples), FString(TEXT("3/1/0")));
        TestEqual(TEXT("R1 Success: lost samples 0/0/0"), SamplesText(Reward.LostSamples), FString(TEXT("0/0/0")));
    }

    // R2 실패: 목표 몫만(1 × 40 × 1.25 = 50), 소지 샘플은 잃는다
    {
        const FHDMissionReward Reward = HDProgression::ComputeReward(MakeOutcome(false, false, 1, 2, 1.25f, MakeSamples(3, 1, 0)), Rules);
        TestEqual(TEXT("R2 Fail: requisition 50"), Reward.Requisition, 50);
        TestEqual(TEXT("R2 Fail: samples 0/0/0"), SamplesText(Reward.Samples), FString(TEXT("0/0/0")));
        TestEqual(TEXT("R2 Fail: lost samples 3/1/0"), SamplesText(Reward.LostSamples), FString(TEXT("3/1/0")));
    }

    // R3 포기(실패): 목표 0 이면 0
    {
        const FHDMissionReward Reward = HDProgression::ComputeReward(MakeOutcome(false, true, 0, 2, 1.25f), Rules);
        TestEqual(TEXT("R3 Abandon: requisition 0"), Reward.Requisition, 0);
        TestEqual(TEXT("R3 Abandon: samples 0/0/0"), SamplesText(Reward.Samples), FString(TEXT("0/0/0")));
    }

    // R4 반올림
    {
        const FHDMissionReward FailReward = HDProgression::ComputeReward(MakeOutcome(false, false, 1, 2, 1.3f), Rules);
        TestEqual(TEXT("R4 Round: fail 1/2 x1.3 = 52"), FailReward.Requisition, 52);

        const FHDMissionReward SuccessReward = HDProgression::ComputeReward(MakeOutcome(true, false, 3, 3, 2.5f), Rules);
        TestEqual(TEXT("R4 Round: success 3/3 x2.5 = 550"), SuccessReward.Requisition, 550);
    }

    // R5 배율 음수는 0 으로 본다
    {
        const FHDMissionReward Reward = HDProgression::ComputeReward(MakeOutcome(true, false, 2, 2, -1.f), Rules);
        TestEqual(TEXT("R5 NegativeMultiplier: requisition 0"), Reward.Requisition, 0);
    }

    // R6 프로필 반영: 상한 자르기와 전적
    {
        FHDProfileData Data;
        Data.Requisition    = 49900;
        Data.Samples        = MakeSamples(0, 0, 0);
        Data.MissionsPlayed = 0;
        Data.MissionsWon    = 0;

        // R1 의 결과와 보상
        FHDMissionReward SuccessReward;
        SuccessReward.Requisition   = 225;
        SuccessReward.Samples       = MakeSamples(3, 1, 0);
        HDProgression::ApplyMissionResult(Data, MakeOutcome(true, false, 2, 2, 1.25f, MakeSamples(3, 1, 0)), SuccessReward);
        TestEqual(TEXT("R6 Apply: requisition capped at 50000"), Data.Requisition, 50000);
        TestEqual(TEXT("R6 Apply: samples 3/1/0"), SamplesText(Data.Samples), FString(TEXT("3/1/0")));
        TestEqual(TEXT("R6 Apply: played 1"), Data.MissionsPlayed, 1);
        TestEqual(TEXT("R6 Apply: won 1"), Data.MissionsWon, 1);

        // 이어서 R2 의 결과와 보상
        FHDMissionReward FailReward;
        FailReward.Requisition  = 50;
        FailReward.LostSamples  = MakeSamples(3, 1, 0);
        HDProgression::ApplyMissionResult(Data, MakeOutcome(false, false, 1, 2, 1.25f, MakeSamples(3, 1, 0)), FailReward);
        TestEqual(TEXT("R6 Apply: requisition stays 50000"), Data.Requisition, 50000);
        TestEqual(TEXT("R6 Apply: samples stay 3/1/0 after fail"), SamplesText(Data.Samples), FString(TEXT("3/1/0")));
        TestEqual(TEXT("R6 Apply: played 2"), Data.MissionsPlayed, 2);
        TestEqual(TEXT("R6 Apply: won stays 1"), Data.MissionsWon, 1);
    }

    // R7 결과 화면 입력 지연
    {
        TestFalse(TEXT("R7 Delay: 0.5s after result is too early"), HDProgression::CanLeaveResult(10.5, 10.0, 1.f));
        TestTrue(TEXT("R7 Delay: 1.0s after result can leave"), HDProgression::CanLeaveResult(11.0, 10.0, 1.f));
        TestFalse(TEXT("R7 Delay: before result time is too early"), HDProgression::CanLeaveResult(9.0, 10.0, 1.f));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
