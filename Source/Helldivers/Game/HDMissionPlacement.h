// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 목표 배치 좌표 계산. 월드·액터와 무관한 순수 함수라 자동 테스트(Helldivers.Mission.Placement)로 고정한다
namespace HDMissionPlacement
{
    // 중심에서 반지름 [MinRadius, MaxRadius] 링 위에 Count 개 점(XY 오프셋)을 고르게 흩어 놓는다.
    // 점끼리 MinSeparation 이상 떨어뜨리려 하고, 못 맞춰도 개수는 채운다. 무작위는 모두 Stream 으로
    HELLDIVERS_API TArray<FVector2D>    ComputeRingOffsets(int32 Count, float MinRadius, float MaxRadius, float MinSeparation, FRandomStream& Stream);
}
