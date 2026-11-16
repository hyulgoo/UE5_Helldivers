// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HDMissionTypes.generated.h"

// 한 판의 단계. 강하 -> 진행 -> 추출 -> 성공/실패
UENUM(BlueprintType)
enum class EHDMissionPhase : uint8
{
    Deploy,
    InProgress,
    Extraction,
    Success,
    Fail
};

// FHDMissionFlow 가 쓰는 수치. UHDMissionSettings::MakeConfig 로 만든다
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDMissionConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    int32               RequiredObjectives = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    int32               MaxReinforcements = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    float               ExtractionHoldTime = 45.f;

    // 0 = 제한 없음
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    float               MissionTimeLimit = 0.f;
};
