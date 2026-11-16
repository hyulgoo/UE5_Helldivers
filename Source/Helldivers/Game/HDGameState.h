// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Game/HDMissionTypes.h"
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
    void                        SetMissionSnapshot(EHDMissionPhase NewPhase, int32 Completed, int32 Required, int32 Reinforcements, float NewExtractionRemaining, bool bInZone);

    EHDMissionPhase             GetMissionPhase() const { return MissionPhase; }
    int32                       GetCompletedObjectives() const { return CompletedObjectives; }
    int32                       GetRequiredObjectives() const { return RequiredObjectives; }
    int32                       GetReinforcementsLeft() const { return ReinforcementsLeft; }
    float                       GetExtractionRemaining() const { return ExtractionRemaining; }
    bool                        IsPlayerInExtractionZone() const { return bPlayerInExtractionZone; }

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
};
