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

    // 참이면 추출 지점 안에서 호출 커맨드를 넣어야 대기 시간이 흐른다. 거짓이면 지점 안에 있기만 하면 흐른다
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    bool                bExtractionRequiresCall = true;
};
