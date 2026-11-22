// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/HDMissionTypes.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"

// 추출 호출(FHDMissionFlow::CallExtraction) 결과. Called 일 때만 호출이 받아들여진 것이다
enum class EHDExtractionCallResult : uint8
{
    Called,
    NotRequired,
    NotExtraction,
    AlreadyCalled,
    Dead,
    NotInZone
};

// 추출 호출 규칙. 월드·액터와 무관한 순수 함수라 자동 테스트(Helldivers.Mission.Flow)로 고정한다
namespace HDMissionRules
{
    // 추출 호출 커맨드: Left, Up, Right, Down. 스트라타젬 카탈로그의 어느 커맨드와도 앞부분이 겹치지 않아야 한다(테스트 C10)
    HELLDIVERS_API const TArray<EHDCommandInput>&   GetExtractionCallSequence();
    // 입력이 호출 커맨드와 길이·순서가 모두 같을 때만 참
    HELLDIVERS_API bool                             IsExtractionCallSequence(const TArray<EHDCommandInput>& Input);
    // 로그용 결과 이름: "called"·"not required"·"not extraction"·"already called"·"dead"·"not in zone"
    HELLDIVERS_API const TCHAR*                     GetCallResultName(EHDExtractionCallResult Result);
}

// 미션 단계 전이 규칙. 월드·액터와 무관한 순수 로직이라 자동 테스트(Helldivers.Mission.Flow)로 고정한다.
// 바꾸는 함수의 bool 반환값은 "단계가 바뀌었는가" 다. 끝난 뒤(Success·Fail)에는 모든 바꾸기가 아무것도 하지 않는다
class HELLDIVERS_API FHDMissionFlow
{
public:
    explicit FHDMissionFlow(const FHDMissionConfig& InConfig);

    EHDMissionPhase             GetPhase() const { return Phase; }
    int32                       GetCompletedObjectives() const { return CompletedObjectives; }
    int32                       GetRequiredObjectives() const { return Config.RequiredObjectives; }
    int32                       GetReinforcementsLeft() const { return ReinforcementsLeft; }
    float                       GetExtractionRemaining() const { return ExtractionRemaining; }
    float                       GetElapsedSeconds() const { return ElapsedSeconds; }
    float                       GetMissionTimeLimit() const { return Config.MissionTimeLimit; }
    bool                        IsPlayerAlive() const { return bPlayerAlive; }
    bool                        IsPlayerInExtractionZone() const { return bPlayerInExtractionZone; }
    bool                        IsFinished() const;

    // 추출 대기 시간이 흐를 수 있는 호출 상태인가: Extraction 이고 (불렀거나 설정이 호출 불필요)
    bool                        IsExtractionCalled() const;

    // Deploy 에서만 -> InProgress
    bool                        NotifyPlayerLanded();

    // InProgress 에서만 +1. 필요 수에 닿으면 -> Extraction
    bool                        NotifyObjectiveCompleted();

    void                        SetPlayerInExtractionZone(bool bInZone);

    // 추출을 부른다. 위에서부터 먼저 맞는 결과: Extraction 아님(끝난 뒤 포함) → 호출 불필요 → 이미 부름 → 죽음 → 지점 밖 → Called(이때만 상태를 바꾼다)
    EHDExtractionCallResult     CallExtraction();

    // 살아 있을 때만. 남은 증원이 0 이면 -> Fail
    bool                        NotifyPlayerDied();

    // 죽어 있고 남은 증원이 있으면 1 줄이고 true
    bool                        TryConsumeReinforcement();

    void                        NotifyPlayerRespawned();

    // InProgress·Extraction 에서만 시간이 흐른다. 제한 시간 초과 -> Fail.
    // 추출 대기는 지점 안·살아 있음·IsExtractionCalled 일 때만 줄고, 다 줄면 -> Success
    bool                        Tick(float DeltaSeconds);

    // 치트용. 0 이상으로 자른다
    void                        DebugSetReinforcementsLeft(int32 NewReinforcementsLeft);

    // 치트용. 제한 시간(초, 0 = 제한 없음)을 바꾼다. 지난 시간이 이미 넘으면 다음 Tick 에서 Fail
    void                        DebugSetMissionTimeLimit(float NewMissionTimeLimit);

private:
    FHDMissionConfig            Config;
    EHDMissionPhase             Phase = EHDMissionPhase::Deploy;
    int32                       CompletedObjectives = 0;
    int32                       ReinforcementsLeft = 0;
    float                       ExtractionRemaining = 0.f;
    float                       ElapsedSeconds = 0.f;
    bool                        bPlayerAlive = true;
    bool                        bPlayerInExtractionZone = false;
    // 한 번 부르면 사망·증원으로 지워지지 않는다
    bool                        bExtractionCalled = false;
};
