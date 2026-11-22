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

    // 시설 목표의 최대·시작 체력
    UPROPERTY(Config, EditAnywhere, Category = "Objective")
    float               FacilityHealth = 600.f;

    // 맵에 놓인 목표가 모자랄 때 첫 PlayerStart 둘레에 스폰하는 링의 반지름(cm)
    UPROPERTY(Config, EditAnywhere, Category = "Objective")
    float               ObjectiveRadiusMin = 8000.f;

    UPROPERTY(Config, EditAnywhere, Category = "Objective")
    float               ObjectiveRadiusMax = 15000.f;

    // 스폰하는 목표끼리 떨어뜨리려는 거리(cm). 못 맞춰도 개수는 채운다
    UPROPERTY(Config, EditAnywhere, Category = "Objective")
    float               ObjectiveMinSeparation = 6000.f;

    // 추출 단계가 되면 플레이어(없으면 첫 PlayerStart)에서 이 거리 링 위에 추출 지점을 스폰한다(cm)
    UPROPERTY(Config, EditAnywhere, Category = "Extraction")
    float               ExtractionDistanceMin = 10000.f;

    UPROPERTY(Config, EditAnywhere, Category = "Extraction")
    float               ExtractionDistanceMax = 16000.f;

    // 추출 지점 구의 반지름(cm). 플레이어 캡슐이 이 안에 있어야 대기 시간이 준다
    UPROPERTY(Config, EditAnywhere, Category = "Extraction")
    float               ExtractionZoneRadius = 1000.f;

    // 참이면 추출 지점 안에서 호출 커맨드를 넣어야 대기 시간이 흐른다. 거짓이면 지점 안에 있기만 하면 흐른다
    UPROPERTY(Config, EditAnywhere, Category = "Extraction")
    bool                bExtractionRequiresCall = true;

    // 죽은 뒤 증원 헬포드가 떨어지기까지 기다리는 시간(초)
    UPROPERTY(Config, EditAnywhere, Category = "Reinforcement")
    float               RespawnDelay = 5.f;

    // 증원 헬포드를 지면에서 이 높이(cm)에 스폰한다
    UPROPERTY(Config, EditAnywhere, Category = "Reinforcement")
    float               ReinforceDropHeight = 15000.f;

    // 시체에서 이 거리(cm) 사이 무작위 방향으로 떨어뜨린다
    UPROPERTY(Config, EditAnywhere, Category = "Reinforcement")
    float               ReinforceOffsetMin = 500.f;

    UPROPERTY(Config, EditAnywhere, Category = "Reinforcement")
    float               ReinforceOffsetMax = 1000.f;
};
