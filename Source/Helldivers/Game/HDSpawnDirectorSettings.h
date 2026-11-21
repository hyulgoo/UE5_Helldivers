// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "HDSpawnDirectorSettings.generated.h"

// Project Settings > Game > HD Spawn Director. 바꾸면 Config/DefaultGame.ini 에 저장된다
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "HD Spawn Director"))
class HELLDIVERS_API UHDSpawnDirectorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
    virtual FName       GetCategoryName() const override;

public:
    // 살아 있는 적(배치·치트 포함) 상한
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    int32               MaxAliveEnemies = 30;

    // 순찰조 스폰 간격(초)
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               PatrolInterval = 25.f;

    UPROPERTY(Config, EditAnywhere, Category = "Director")
    int32               PatrolGroupSize = 3;

    // 플레이어 주변 순찰조 스폰 링(cm)
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               PatrolRingMin = 4000.f;

    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               PatrolRingMax = 7000.f;

    // 경보가 이만큼(초) 이어지면 증원 웨이브
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               BreachDelayAfterAlert = 6.f;

    UPROPERTY(Config, EditAnywhere, Category = "Director")
    int32               BreachWaveSize = 8;

    // 웨이브 사이 최소 간격(초)
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               BreachCooldown = 40.f;

    // 대상 주변 웨이브 스폰 링(cm)
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               BreachRingMin = 2500.f;

    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               BreachRingMax = 4000.f;

    // 그룹 중 원거리형 비율(0~1)
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               ShooterRatio = 0.3f;

    // 그룹 중 중장갑형 비율(0~1). 순찰조 3명은 0, 웨이브 8명은 1
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               HeavyRatio = 0.1f;

    // 추출 중 강도(브리프 8 이 SetIntensity 로 쓴다)
    UPROPERTY(Config, EditAnywhere, Category = "Director")
    float               ExtractionIntensity = 2.f;

    // 원거리형 총알 한 발 피해. 총알 BP(BPP_Bullet_Light, 50)는 플레이어 소총과 같아 그대로 쓰면 3점사로 즉사한다
    UPROPERTY(Config, EditAnywhere, Category = "Enemy", meta = (ClampMin = "0"))
    float               ShooterShotDamage = 10.f;
};
