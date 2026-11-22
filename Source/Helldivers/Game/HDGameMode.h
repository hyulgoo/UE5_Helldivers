// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/HDMissionFlow.h"
#include "Progression/HDDifficulty.h"
#include "Ship/HDShipHubRules.h"
#include "HDGameMode.generated.h"

class UHDMissionSettings;
class ACameraActor;
class AHDShipInterior;

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

    // 출격(메뉴 DEPLOY·출격 대기 끝·치트 HDDeploy). 함선 단계가 아니면 거짓. 출격 대기를 끄고, 함선 속 헬다이버·함선 방을 지우고,
    // 플레이어스테이트 로드아웃을 비운 뒤(착지한 캐릭터가 프로필로 다시 묶는다) 메뉴를 닫고 미션을 시작해 헬포드를 스폰하고 함선 카메라를 지운다
    bool                        RequestDeploy(APlayerController* PC);
    bool                        IsInShip() const;

    // 함선 속 헬다이버를 함선 방 스폰 지점에 스폰·빙의한다(타이틀 뒤·복귀 뒤). 함선 단계 아님·bShipInterior 꺼짐·방 없음·이미 폰 있음이면 거짓(interior refused 로그)
    bool                        EnterShipInterior(APlayerController* PC);
    AHDShipInterior*            GetShipInterior() const;

    // READY(메뉴)·치트 HDLaunch: 출격 대기를 LaunchCountdownSeconds 로 시작한다. 함선 단계가 아니거나 이미 돌고 있으면 거짓(launch refused 로그)
    bool                        RequestLaunch(APlayerController* PC);
    // 출격 대기가 돌고 있었으면 끄고 참(launch cancelled 로그)
    bool                        CancelLaunch();
    bool                        IsLaunching() const;

    // 브리프 7~9 가 부른다
    void                        NotifyObjectiveCompleted(AActor* Objective);
    void                        NotifyPlayerInExtractionZone(bool bInZone);

    // 미션이 안 끝났고 남은 증원이 있으면 RespawnDelay 뒤 Reinforce 를 예약한다
    void                        NotifyPlayerDied(AController* DeadController);

    // 추출을 부른다(호출 커맨드·치트 HDCallExtraction). Called 면 스폰 강도를 ExtractionIntensity × 난이도 강도로 올린다. 미션 로직이 없으면 NotExtraction
    EHDExtractionCallResult     RequestExtractionCall();

    const FHDMissionFlow*       GetMissionFlow() const;
    FHDMissionFlow*             GetMissionFlowForDebug();

    // 치트용. 로직의 같은 이름 함수 + 게임스테이트 반영
    void                        DebugSetReinforcementsLeft(int32 Count);
    void                        DebugSetMissionTimeLimit(float Seconds);

    // 작전 포기(확인 창 Enter·치트 HDAbandon). 함선 단계·미션 로직 없음·이미 끝났으면 거짓(abandon refused 로그).
    // 받아들이면 [HDMission] abandon 뒤 보통의 Fail 경로(HandlePhaseChanged → FinishMission)로 끝난다
    bool                        RequestAbandon();

    // 결과 화면에서 함선으로(Enter·클릭·치트 HDReturnToShip). 미션이 끝났을 때만 같은 맵을 다시 연다(새 월드는 함선 단계로 시작한다)
    bool                        ReturnToShip();

private:
    // 프로필의 난이도 행을 정하고, 목표를 배치하고 FHDMissionFlow 를 만든다([HDMission] start 로그까지)
    void                        BeginMission();

    // 첫 PlayerStart(없으면 원점) + ShipCameraOffset 에 PlayerStart 를 바라보는 카메라를 두고, bShipInterior 면 + ShipInteriorOffset 에 함선 방을 스폰한다.
    // 모든 플레이어의 시점을 옮긴 뒤, 함선 방이 있고 타이틀을 이미 봤으면(복귀) EnterShipInterior, 아니면 함선 메뉴(타이틀 또는 Bridge)를 연다
    void                        EnterShip();

    // 맵에 놓인 목표가 RequiredCount 보다 적으면 모자란 만큼 시설을 첫 PlayerStart 둘레 지면에 스폰한다. 실제로 있는 목표 수를 돌려준다
    int32                       PlaceObjectives(const UHDMissionSettings& Settings, int32 RequiredCount);

    // 난이도 행의 샘플 수만큼 첫 PlayerStart(없으면 원점) 둘레 링 위 지면 + 50 에 AHDSamplePickup 을 놓는다([HDSample] placed 로그)
    void                        PlaceSamples(const FHDSampleCounts& Counts);

    // 증원을 기다리는 사망일 때만: 플레이어스테이트의 소지 샘플을 꺼내 폰 아래 지면 + 50 에 묶음으로 떨군다([HDSample] dropped 로그)
    void                        DropCarriedSamples(APlayerController* DeadPlayerController);

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

    // Success·Fail 뒤 한 미션에 한 번만: 보상 계산 → 프로필 반영·저장 → 게임스테이트·서브시스템에 요약 → 결과 화면 → [HDMission] result 로그
    void                        FinishMission(EHDMissionPhase NewPhase);

private:
    TUniquePtr<FHDMissionFlow>  MissionFlow;

    // 사망 -> Reinforce 대기. Tick 에서 남은 시간을 게임스테이트 RespawnRemaining 에 옮긴다
    FTimerHandle                ReinforceTimerHandle;

    // 함선 단계인가(InitGame 에서 정하고 RequestDeploy 가 내린다)
    bool                        bInShip = false;

    // 함선 단계의 고정 카메라. 출격하면 지운다
    UPROPERTY()
    TObjectPtr<ACameraActor>    ShipCamera;

    // 걸어 다니는 함선 방과 그 안의 헬다이버. 출격하면 지운다
    UPROPERTY()
    TObjectPtr<AHDShipInterior> ShipInterior;

    UPROPERTY()
    TObjectPtr<APawn>           ShipCrew;

    // READY 뒤 출격 대기. Tick 이 돌리고 끝나면 LaunchPlayerController 로 RequestDeploy 를 부른다
    FHDLaunchCountdown                  LaunchCountdown;
    TWeakObjectPtr<APlayerController>   LaunchPlayerController;

    // 이번 미션의 난이도 단계(1 부터, BeginMission 전에는 0)와 그 행. BeginMission 이 프로필에서 정한다
    int32                       MissionDifficultyLevel = 0;
    FHDDifficultyLevel          MissionDifficulty;

    // 이번 미션의 보상을 이미 넣었는가(FinishMission)
    bool                        bResultApplied = false;
};
