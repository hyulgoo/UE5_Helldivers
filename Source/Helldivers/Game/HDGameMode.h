// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/HDMissionFlow.h"
#include "HDGameMode.generated.h"

class UHDMissionSettings;
class ACameraActor;

// 한 판의 단계(FHDMissionFlow)를 소유하고 착지·목표·추출·사망을 받아 게임스테이트와 스폰 디렉터에 반영한다.
// UHDProgressionSettings::bStartInShip 이면 함선 단계(폰 없음, 고정 카메라 + 메뉴)로 시작하고 RequestDeploy 때 미션을 시작한다
UCLASS()
class HELLDIVERS_API AHDGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
    AHDGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 함선 단계를 정한다. 첫 헬포드는 로그인 때 스폰되므로 StartPlay 보다 먼저인 여기서 정한다
    virtual void                InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    // 함선 단계면 EnterShip, 아니면 BeginMission
	virtual void	StartPlay() override final;
    virtual void                PostLogin(APlayerController* NewPlayer) override;
    virtual void                Tick(float DeltaSeconds) override;

    // 함선 단계면 거짓(로그인 때 기본 폰을 스폰하지 않는다), 아니면 Super
    virtual bool                PlayerCanRestart_Implementation(APlayerController* Player) override;

    // 출격(메뉴 DEPLOY·치트 HDDeploy). 함선 단계가 아니면 거짓. 메뉴를 닫고 미션을 시작한 뒤 헬포드를 스폰하고 함선 카메라를 지운다
    bool                        RequestDeploy(APlayerController* PC);
    bool                        IsInShip() const;

    // 브리프 7~9 가 부른다
    void                        NotifyObjectiveCompleted(AActor* Objective);
    void                        NotifyPlayerInExtractionZone(bool bInZone);

    // 미션이 안 끝났고 남은 증원이 있으면 RespawnDelay 뒤 Reinforce 를 예약한다
    void                        NotifyPlayerDied(AController* DeadController);

    // 추출을 부른다(호출 커맨드·치트 HDCallExtraction). Called 면 스폰 강도를 ExtractionIntensity 로 올린다. 미션 로직이 없으면 NotExtraction
    EHDExtractionCallResult     RequestExtractionCall();

    const FHDMissionFlow*       GetMissionFlow() const;
    FHDMissionFlow*             GetMissionFlowForDebug();

    // 치트용. 로직의 같은 이름 함수 + 게임스테이트 반영
    void                        DebugSetReinforcementsLeft(int32 Count);
    void                        DebugSetMissionTimeLimit(float Seconds);

private:
    // 목표를 배치하고 FHDMissionFlow 를 만든다([HDMission] start 로그까지)
    void                        BeginMission();

    // 첫 PlayerStart(없으면 원점) + ShipCameraOffset 에 PlayerStart 를 바라보는 카메라를 두고, 모든 플레이어의 시점을 옮겨 함선 메뉴를 연다
    void                        EnterShip();

    // 맵에 놓인 목표가 RequiredObjectives 보다 적으면 모자란 만큼 시설을 첫 PlayerStart 둘레 지면에 스폰한다. 실제로 있는 목표 수를 돌려준다
    int32                       PlaceObjectives(const UHDMissionSettings& Settings);

    // Extraction: 플레이어 폰(없으면 첫 PlayerStart) 둘레 링 위 지면에 AHDExtractionZone 을 스폰한다
    void                        SpawnExtractionZone(const UHDMissionSettings& Settings);

    // Success·Fail: 증원 대기 예약을 지우고, 모든 적 AI 를 멈추고, 플레이어 폰 입력을 막는다
    void                        StopForMissionEnd();

    // 증원 하나를 써서 사망 태그·어빌리티를 정리하고, 시체 근처 하늘에 헬포드(기본 폰)를 스폰해 빙의한다
    void                        Reinforce(APlayerController* PC);

    // 처음 빙의는 헬포드, 착지 뒤 캐릭터. 캐릭터가 되면 착지(Deploy) 또는 재강하로 본다
    UFUNCTION()
    void                        HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    // 이전 단계 기억 -> Change 호출 -> 게임스테이트 반영 -> 단계가 바뀌었으면 HandlePhaseChanged
    void                        ApplyMissionChange(TFunctionRef<void(FHDMissionFlow&)> Change);
    void                        SyncGameState() const;
    void                        HandlePhaseChanged(EHDMissionPhase OldPhase, EHDMissionPhase NewPhase);

private:
    TUniquePtr<FHDMissionFlow>  MissionFlow;

    // 사망 -> Reinforce 대기. Tick 에서 남은 시간을 게임스테이트 RespawnRemaining 에 옮긴다
    FTimerHandle                ReinforceTimerHandle;

    // 함선 단계인가(InitGame 에서 정하고 RequestDeploy 가 내린다)
    bool                        bInShip = false;

    // 함선 단계의 고정 카메라. 출격하면 지운다
    UPROPERTY()
    TObjectPtr<ACameraActor>    ShipCamera;
};
