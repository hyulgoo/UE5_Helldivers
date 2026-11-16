// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Game/HDMissionTypes.h"
#include "HDMissionSettings.generated.h"

// Project Settings > Game > HD Mission. 바꾸면 Config/DefaultGame.ini 에 저장된다
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "HD Mission"))
class HELLDIVERS_API UHDMissionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
    virtual FName       GetCategoryName() const override;

    FHDMissionConfig    MakeConfig() const;

public:
    // 추출이 열리기까지 파괴할 목표 수
    UPROPERTY(Config, EditAnywhere, Category = "Mission")
    int32               RequiredObjectives = 2;

    UPROPERTY(Config, EditAnywhere, Category = "Mission")
    int32               MaxReinforcements = 5;

    // 추출 지점 안에서 버텨야 하는 시간(초)
    UPROPERTY(Config, EditAnywhere, Category = "Mission")
    float               ExtractionHoldTime = 45.f;

    // 미션 제한 시간(초). 0 = 제한 없음
    UPROPERTY(Config, EditAnywhere, Category = "Mission")
    float               MissionTimeLimit = 0.f;
};
