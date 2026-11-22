// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HDProfileTypes.generated.h"

// 샘플 종류. Count 는 개수 표시용이다
UENUM(BlueprintType)
enum class EHDSampleType : uint8
{
    Common,
    Rare,
    Super,
    Count
};

// 샘플 세 종류의 수
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDSampleCounts
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32               Common = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32               Rare = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32               Super = 0;

    // Count 면 0
    int32               Get(EHDSampleType Type) const;
    // 자르지 않는다. 자르기는 HDProgression 이 한다
    void                Add(EHDSampleType Type, int32 Amount);
    int32               Total() const;
    // 음수가 섞인 값에는 쓰지 않는다
    bool                IsEmpty() const { return Total() == 0; }
    bool                operator==(const FHDSampleCounts& Other) const;
};

// 업그레이드·해금 가격
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDCost
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32               Requisition = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FHDSampleCounts     Samples;
};

// 저장되는 진행 데이터 전부. 바꾸는 곳은 UHDProfileSubsystem::ModifyProfile 하나다
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDProfileData
{
    GENERATED_BODY()

    UPROPERTY()
    int32               Version = 1;

    UPROPERTY()
    int32               Requisition = 0;

    UPROPERTY()
    FHDSampleCounts     Samples;

    // 업그레이드 이름(Config/HDUpgrades.json, 브리프 6) → 산 단계(1 부터)
    UPROPERTY()
    TMap<FName, int32>  UpgradeLevels;

    // 값을 치르고 해금한 스트라타젬 이름(처음부터 열린 것은 넣지 않는다, 브리프 7)
    UPROPERTY()
    TArray<FName>       UnlockedStratagems;

    // 고른 로드아웃. 비면 UHDStratagemSettings::Loadout(브리프 7)
    UPROPERTY()
    TArray<FName>       Loadout;

    // 1 부터. 2 = 지금 게임 수치(브리프 3)
    UPROPERTY()
    int32               Difficulty = 2;

    UPROPERTY()
    int32               MissionsPlayed = 0;

    UPROPERTY()
    int32               MissionsWon = 0;
};
