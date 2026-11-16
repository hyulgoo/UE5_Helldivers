// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/HDMissionTypes.h"

// 미션 단계 전이 규칙. 월드·액터와 무관한 순수 로직이라 자동 테스트(Helldivers.Mission.Flow)로 고정한다.
// 바꾸는 함수의 bool 반환값은 "단계가 바뀌었는가" 다. 끝난 뒤(Success·Fail)에는 모든 바꾸기가 아무것도 하지 않는다
class HELLDIVERS_API FHDMissionFlow
{
public:
    explicit FHDMissionFlow(const FHDMissionConfig& InConfig);

    EHDMissionPhase     GetPhase() const { return Phase; }
    int32               GetCompletedObjectives() const { return CompletedObjectives; }
    int32               GetRequiredObjectives() const { return Config.RequiredObjectives; }
    int32               GetReinforcementsLeft() const { return ReinforcementsLeft; }
    float               GetExtractionRemaining() const { return ExtractionRemaining; }
    float               GetElapsedSeconds() const { return ElapsedSeconds; }
    bool                IsPlayerAlive() const { return bPlayerAlive; }
    bool                IsPlayerInExtractionZone() const { return bPlayerInExtractionZone; }
    bool                IsFinished() const;

    // Deploy 에서만 -> InProgress
    bool                NotifyPlayerLanded();

    // InProgress 에서만 +1. 필요 수에 닿으면 -> Extraction
    bool                NotifyObjectiveCompleted();

    void                SetPlayerInExtractionZone(bool bInZone);

    // 살아 있을 때만. 남은 증원이 0 이면 -> Fail
    bool                NotifyPlayerDied();

    // 죽어 있고 남은 증원이 있으면 1 줄이고 true
    bool                TryConsumeReinforcement();

    void                NotifyPlayerRespawned();

    // InProgress·Extraction 에서만 시간이 흐른다. 제한 시간 초과 -> Fail, 추출 대기 완료 -> Success
    bool                Tick(float DeltaSeconds);

    // 치트용. 0 이상으로 자른다
    void                DebugSetReinforcementsLeft(int32 NewReinforcementsLeft);

private:
    FHDMissionConfig    Config;
    EHDMissionPhase     Phase = EHDMissionPhase::Deploy;
    int32               CompletedObjectives = 0;
    int32               ReinforcementsLeft = 0;
    float               ExtractionRemaining = 0.f;
    float               ElapsedSeconds = 0.f;
    bool                bPlayerAlive = true;
    bool                bPlayerInExtractionZone = false;
};
