// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 작전 테이블 지도 수학. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.FrontEnd.WarTable)로 고정한다
namespace HDWarTableMap
{
    // 지도 반 범위 = max(목표 최대 반경 × 1.25, 5000)
    constexpr float                 HalfExtentScale     = 1.25f;
    constexpr float                 MinHalfExtent       = 5000.f;
    // 캡처 높이 = 함선 Z − 5000
    constexpr float                 CaptureBelowShip    = 5000.f;
    constexpr int32                 TextureSize         = 1024;

    HELLDIVERS_API float            ComputeHalfExtent(float ObjectiveRadiusMax);
    // 지도 위 = 월드 +X, 오른쪽 = 월드 +Y. MapRect 는 정사각형
    HELLDIVERS_API FVector2D        WorldToMap(const FVector2D& WorldXY, const FVector2D& CenterXY, float HalfExtent, const FBox2D& MapRect);
    // 첫 점은 Center + (Radius, 0), 반시계로 같은 간격
    HELLDIVERS_API TArray<FVector2D> MakeRing(const FVector2D& Center, float Radius, int32 Segments);
    // (중심 X, 중심 Y, ShipZ − CaptureBelowShip)
    HELLDIVERS_API FVector          GetCaptureLocation(const FVector& CenterWorld, float ShipZ);
}
