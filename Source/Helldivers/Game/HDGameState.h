// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Game/HDMissionTypes.h"
#include "Progression/HDMissionReward.h"
#include "HDGameState.generated.h"

class AHDBattleShip;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHDMissionPhaseChanged, EHDMissionPhase /*Old*/, EHDMissionPhase /*New*/);

UCLASS()
class HELLDIVERS_API AHDGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
                        AHDGameState();

    AHDBattleShip*              GetBattleShip() const;
    void                        SetBattleShip(AHDBattleShip* NewBattleShip);

    // 미션 표시용 값. AHDGameMode 가 FHDMissionFlow 를 바꿀 때마다 반영한다
    void                        SetMissionSnapshot(EHDMissionPhase NewPhase, int32 Completed, int32 Required, int32 Reinforcements, float NewExtractionRemaining, bool bInZone, bool bCalled);

    EHDMissionPhase             GetMissionPhase() const { return MissionPhase; }
    int32                       GetCompletedObjectives() const { return CompletedObjectives; }
    int32                       GetRequiredObjectives() const { return RequiredObjectives; }
    int32                       GetReinforcementsLeft() const { return ReinforcementsLeft; }
    float                       GetExtractionRemaining() const { return ExtractionRemaining; }
    bool                        IsPlayerInExtractionZone() const { return bPlayerInExtractionZone; }
    // FHDMissionFlow::IsExtractionCalled. 호출이 필요 없는 설정이면 추출 단계 내내 참
    bool                        IsExtractionCalled() const { return bExtractionCalled; }

    // 증원까지 남은 시간(초). 기다리는 중이 아니면 -1
    float                       GetRespawnRemaining() const { return RespawnRemaining; }
    void                        SetRespawnRemaining(float NewRespawnRemaining);

    // 출격 대기(READY 뒤 카운트다운) 남은 시간(초). 돌고 있지 않으면 -1. AHDGameMode 가 Tick 에서 옮긴다
    float                       GetLaunchRemaining() const { return LaunchRemaining; }
    void                        SetLaunchRemaining(float NewLaunchRemaining);

    // 함선 단계(폰 없음, 메뉴)인가. AHDGameMode 가 함선에 들어올 때 참, 출격할 때 거짓으로 둔다
    void                        SetInShip(bool bNewInShip);
    bool                        IsInShip() const { return bInShip; }

    // 미션 결과(정산을 마친 요약). AHDGameMode 가 미션이 끝날 때 한 번 넣는다. 결과 패널과 결과 화면 입력이 쓴다
    void                        SetMissionResult(const FHDMissionResultSummary& Summary);
    bool                        HasMissionResult() const { return bHasMissionResult; }
    const FHDMissionResultSummary& GetMissionResult() const { return MissionResult; }

public:
    // SetMissionSnapshot 안에서 단계가 바뀌면 방송한다
    FOnHDMissionPhaseChanged    OnMissionPhaseChanged;

protected:
    virtual void                BeginPlay() override;

private:
    UPROPERTY(EditAnywhere)
    TSubclassOf<AHDBattleShip>  BattleShipClass;

    UPROPERTY()
    TObjectPtr<AHDBattleShip>   BattleShip;
    
    UPROPERTY(EditDefaultsOnly)
    float                       SpawnHeight;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    EHDMissionPhase             MissionPhase;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    int32                       CompletedObjectives;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    int32                       RequiredObjectives;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    int32                       ReinforcementsLeft;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    float                       ExtractionRemaining;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    bool                        bPlayerInExtractionZone;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    bool                        bExtractionCalled;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    float                       RespawnRemaining;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    float                       LaunchRemaining;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    bool                        bInShip;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    FHDMissionResultSummary     MissionResult;

    UPROPERTY(VisibleInstanceOnly, Category = "Mission")
    bool                        bHasMissionResult;
};
