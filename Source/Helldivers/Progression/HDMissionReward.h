// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Progression/HDProfileTypes.h"
#include "HDMissionReward.generated.h"

// 끝난 미션 한 판. AHDGameMode 가 성공·실패 때 채운다
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDMissionOutcome
{
    GENERATED_BODY()

    UPROPERTY()
    bool                bSuccess = false;

    // 작전 포기로 끝났는가(포기도 실패다)
    UPROPERTY()
    bool                bAbandoned = false;

    UPROPERTY()
    int32               CompletedObjectives = 0;

    UPROPERTY()
    int32               RequiredObjectives = 0;

    // 난이도 단계(1 부터)와 이름
    UPROPERTY()
    int32               Difficulty = 2;

    UPROPERTY()
    FString             DifficultyLabel;

    // 난이도 행의 징발 보상 배율
    UPROPERTY()
    float               RewardMultiplier = 1.f;

    // FHDMissionFlow::GetElapsedSeconds(InProgress·Extraction 동안 흐른 시간)
    UPROPERTY()
    float               ElapsedSeconds = 0.f;

    // 끝날 때 플레이어가 지닌 샘플(브리프 5)
    UPROPERTY()
    FHDSampleCounts     CarriedSamples;
};

// 한 판의 보상
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDMissionReward
{
    GENERATED_BODY()

    UPROPERTY()
    int32               Requisition = 0;

    UPROPERTY()
    FHDSampleCounts     Samples;

    // 실패·포기로 잃은 소지 샘플(화면 표시용)
    UPROPERTY()
    FHDSampleCounts     LostSamples;
};

// 결과 패널과 함선의 LAST MISSION 줄이 쓰는 값
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDMissionResultSummary
{
    GENERATED_BODY()

    UPROPERTY()
    FHDMissionOutcome   Outcome;

    UPROPERTY()
    FHDMissionReward    Reward;

    // 반영한 뒤 프로필의 징발
    UPROPERTY()
    int32               TotalRequisition = 0;

    // 결과가 나온 월드 시각(초)
    UPROPERTY()
    double              ResultWorldTime = 0.0;
};

// 징발 보상 규칙. UHDProgressionSettings::MakeRewardRules 가 설정 값으로 만든다
struct FHDRewardRules
{
    int32               RequisitionPerObjective = 40;
    int32               SuccessRequisitionBonus = 100;
};

// 미션 보상 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Progression.Reward)로 고정한다
namespace HDProgression
{
    // 징발 = round((완료 × 목표당 + 성공 보너스) × max(0, 배율)). 샘플은 성공일 때만 소지분, 아니면 소지분이 LostSamples
    HELLDIVERS_API FHDMissionReward     ComputeReward(const FHDMissionOutcome& Outcome, const FHDRewardRules& Rules);

    // AddResources(상한 자르기) + MissionsPlayed + 1, 성공이면 MissionsWon + 1
    HELLDIVERS_API void                 ApplyMissionResult(FHDProfileData& Data, const FHDMissionOutcome& Outcome, const FHDMissionReward& Reward);

    // 결과 화면에서 나갈 수 있는가: Now − ResultTime ≥ Delay
    HELLDIVERS_API bool                 CanLeaveResult(double Now, double ResultTime, float Delay);

    // "[HDMission] result" 뒤에 붙는 글자: success=1 abandoned=0 objectives=2/2 requisition=+225 samples=+0/0/0 total=225
    HELLDIVERS_API FString              DescribeResult(const FHDMissionResultSummary& Summary);
}
