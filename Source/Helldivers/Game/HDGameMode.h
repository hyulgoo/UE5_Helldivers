// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/HDMissionFlow.h"
#include "HDGameMode.generated.h"

class UHDMissionSettings;

// 한 판의 단계(FHDMissionFlow)를 소유하고 착지·목표·추출·사망을 받아 게임스테이트와 스폰 디렉터에 반영한다
UCLASS()
class HELLDIVERS_API AHDGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
    AHDGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void	StartPlay() override final;
    virtual void                PostLogin(APlayerController* NewPlayer) override;
    virtual void                Tick(float DeltaSeconds) override;

    // 브리프 7~9 가 부른다
    void                        NotifyObjectiveCompleted(AActor* Objective);
    void                        NotifyPlayerInExtractionZone(bool bInZone);
    void                        NotifyPlayerDied(AController* DeadController);

    const FHDMissionFlow*       GetMissionFlow() const;
    FHDMissionFlow*             GetMissionFlowForDebug();

    // 치트용. 로직의 같은 이름 함수 + 게임스테이트 반영
    void                        DebugSetReinforcementsLeft(int32 Count);

private:
    // 맵에 놓인 목표가 RequiredObjectives 보다 적으면 모자란 만큼 시설을 첫 PlayerStart 둘레 지면에 스폰한다. 실제로 있는 목표 수를 돌려준다
    int32                       PlaceObjectives(const UHDMissionSettings& Settings);

    // Extraction: 플레이어 폰(없으면 첫 PlayerStart) 둘레 링 위 지면에 AHDExtractionZone 을 스폰한다
    void                        SpawnExtractionZone(const UHDMissionSettings& Settings);

    // Success·Fail: 모든 적 AI 를 멈추고 플레이어 폰 입력을 막는다
    void                        StopForMissionEnd();

    // 처음 빙의는 헬포드, 착지 뒤 캐릭터. 캐릭터가 되면 착지(Deploy) 또는 재강하로 본다
    UFUNCTION()
    void                        HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    // 이전 단계 기억 -> Change 호출 -> 게임스테이트 반영 -> 단계가 바뀌었으면 HandlePhaseChanged
    void                        ApplyMissionChange(TFunctionRef<void(FHDMissionFlow&)> Change);
    void                        SyncGameState() const;
    void                        HandlePhaseChanged(EHDMissionPhase OldPhase, EHDMissionPhase NewPhase);

private:
    TUniquePtr<FHDMissionFlow>  MissionFlow;
};
