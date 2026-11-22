// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDMissionReward.h"
#include "Progression/HDProgressionRules.h"

namespace HDProgression
{
    FHDMissionReward ComputeReward(const FHDMissionOutcome& Outcome, const FHDRewardRules& Rules)
    {
        FHDMissionReward Reward;

        // 실패·포기도 목표 몫은 받는다
        const int32 BaseRequisition = Outcome.CompletedObjectives * Rules.RequisitionPerObjective + (Outcome.bSuccess ? Rules.SuccessRequisitionBonus : 0);
        const double Multiplier = FMath::Max(0.0, static_cast<double>(Outcome.RewardMultiplier));
        Reward.Requisition = FMath::RoundToInt(static_cast<double>(BaseRequisition) * Multiplier);

        // 샘플은 추출 성공 때만 소지분이 들어간다
        if (Outcome.bSuccess)
        {
            Reward.Samples = Outcome.CarriedSamples;
        }
        else
        {
            Reward.LostSamples = Outcome.CarriedSamples;
        }

        return Reward;
    }

    void ApplyMissionResult(FHDProfileData& Data, const FHDMissionOutcome& Outcome, const FHDMissionReward& Reward)
    {
        AddResources(Data, Reward.Requisition, Reward.Samples);

        ++Data.MissionsPlayed;
        if (Outcome.bSuccess)
        {
            ++Data.MissionsWon;
        }
    }

    bool CanLeaveResult(double Now, double ResultTime, float Delay)
    {
        return Now - ResultTime >= Delay;
    }

    FString DescribeResult(const FHDMissionResultSummary& Summary)
    {
        const FHDMissionOutcome& Outcome = Summary.Outcome;
        const FHDMissionReward& Reward = Summary.Reward;
        return FString::Printf(TEXT("success=%d abandoned=%d objectives=%d/%d requisition=+%d samples=+%d/%d/%d total=%d"),
            Outcome.bSuccess ? 1 : 0,
            Outcome.bAbandoned ? 1 : 0,
            Outcome.CompletedObjectives,
            Outcome.RequiredObjectives,
            Reward.Requisition,
            Reward.Samples.Common,
            Reward.Samples.Rare,
            Reward.Samples.Super,
            Summary.TotalRequisition);
    }
}
