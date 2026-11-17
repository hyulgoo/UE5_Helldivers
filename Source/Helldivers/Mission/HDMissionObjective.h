// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HDMissionObjective.generated.h"

// 미션 목표 공통. 완료되면 한 번만 AHDGameMode::NotifyObjectiveCompleted 를 부른다. HUD 는 GetMarker* 로 표식을 그린다
UCLASS(Abstract)
class HELLDIVERS_API AHDMissionObjective : public AActor
{
	GENERATED_BODY()

public:
    AHDMissionObjective(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    bool                IsCompleted() const { return bCompleted; }

    virtual FString     GetMarkerLabel() const;

    // 기본: 액터 위치 + Z 400
    virtual FVector     GetMarkerLocation() const;

protected:
    // 한 번만 실행한다
    void                CompleteObjective();

private:
    bool                bCompleted = false;
};
