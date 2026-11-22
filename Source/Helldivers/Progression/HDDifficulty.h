// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/HDMissionTypes.h"
#include "Progression/HDProfileTypes.h"
#include "HDDifficulty.generated.h"

// 난이도 한 행. 함선 메뉴에서 고르고 출격할 때 미션에 들어간다
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDDifficultyLevel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Difficulty")
    FString             Label;

    // 추출이 열리기까지 파괴할 목표 수. 0 이하면 UHDMissionSettings::RequiredObjectives
    UPROPERTY(EditAnywhere, Category = "Difficulty")
    int32               RequiredObjectives = 2;

    // 스폰 디렉터 강도(순찰 간격·웨이브 크기). 추출 호출 뒤에는 ExtractionIntensity 를 곱한다
    UPROPERTY(EditAnywhere, Category = "Difficulty")
    float               SpawnIntensity = 1.f;

    // 무리 중 중장갑형 비율(UHDSpawnDirectorSettings::HeavyRatio 대신)
    UPROPERTY(EditAnywhere, Category = "Difficulty")
    float               HeavyRatio = 0.1f;

    // 징발 보상에 곱한다(브리프 4)
    UPROPERTY(EditAnywhere, Category = "Difficulty")
    float               RewardMultiplier = 1.f;

    // 맵에 놓는 샘플 수(브리프 5)
    UPROPERTY(EditAnywhere, Category = "Difficulty")
    FHDSampleCounts     SampleCounts;
};

// 난이도 표 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Progression.Difficulty)로 고정한다
namespace HDDifficulty
{
    // EASY·NORMAL·HARD·VETERAN·ELITE. 2(NORMAL) = 지금 게임 수치
    HELLDIVERS_API const TArray<FHDDifficultyLevel>&    GetDefaultTable();
    // 1 부터. NumLevels 가 1 미만이면 1
    HELLDIVERS_API int32                                ClampLevel(int32 Level, int32 NumLevels);
    // 감기지 않는다
    HELLDIVERS_API int32                                StepLevel(int32 Level, int32 Delta, int32 NumLevels);
    // Level 을 자른 행. 표가 비면 기본값
    HELLDIVERS_API FHDDifficultyLevel                   GetLevel(const TArray<FHDDifficultyLevel>& Table, int32 Level);
    // 목표 수만 바꾼다(행의 값이 0 보다 클 때)
    HELLDIVERS_API FHDMissionConfig                     ApplyToConfig(const FHDMissionConfig& Base, const FHDDifficultyLevel& Level);
    // 추출 호출 뒤 강도 = 기본 추출 강도 × 행의 스폰 강도
    HELLDIVERS_API float                                GetExtractionIntensity(float BaseExtractionIntensity, const FHDDifficultyLevel& Level);
    // "DIFFICULTY  < 2  NORMAL >"
    HELLDIVERS_API FString                              FormatMenuLabel(int32 Level, const FHDDifficultyLevel& Row);
    // "OBJECTIVES 2   ENEMY INTENSITY 1.0   HEAVY 10%   REWARD x1.25"
    HELLDIVERS_API FString                              FormatDetail(const FHDDifficultyLevel& Row);
}
