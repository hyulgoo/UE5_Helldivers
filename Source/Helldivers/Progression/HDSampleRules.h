// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Progression/HDProfileTypes.h"

// 샘플 배치 한 줄: 이 종류를 Count 개, 강하 지점에서 MinRadius~MaxRadius 링에
struct FHDSamplePlacement
{
    EHDSampleType   Type        = EHDSampleType::Common;
    int32           Count       = 0;
    float           MinRadius   = 0.f;
    float           MaxRadius   = 0.f;
};

// 샘플 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Progression.Samples)로 고정한다
namespace HDSamples
{
    // "common"·"rare"·"super"(대소문자 무시)
    HELLDIVERS_API bool                         TryParseType(const FString& Text, EHDSampleType& OutType);
    // "Common"·"Rare"·"Super"
    HELLDIVERS_API const TCHAR*                 GetTypeName(EHDSampleType Type);
    // 일반 /Engine/BasicShapes/Sphere.Sphere, 희귀 Cone.Cone, 최상급 Cylinder.Cylinder
    HELLDIVERS_API const TCHAR*                 GetMeshPath(EHDSampleType Type);
    // 수가 0 인 종류는 뺀다. 순서 일반·희귀·최상급. 링은 (최소, 최대)
    HELLDIVERS_API TArray<FHDSamplePlacement>   MakePlacementPlan(const FHDSampleCounts& Counts, const FVector2D& CommonRing, const FVector2D& RareRing, const FVector2D& SuperRing);
    // HUD 줄 "SAMPLES  C 2  R 1  S 0"
    HELLDIVERS_API FString                      FormatCarried(const FHDSampleCounts& Counts);
    // 로그용 "2/1/0"
    HELLDIVERS_API FString                      Describe(const FHDSampleCounts& Counts);
}
