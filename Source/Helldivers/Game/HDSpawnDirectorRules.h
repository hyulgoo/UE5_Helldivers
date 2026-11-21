// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 스폰 디렉터의 수 계산 규칙. 월드·액터와 무관한 순수 함수라 자동 테스트(Helldivers.Director.Rules)로 고정한다
namespace HDSpawnRules
{
    // 살아 있는 적이 MaxAlive 를 넘지 않게 자른 스폰 수. 음수는 0
    HELLDIVERS_API int32    ClampSpawnCount(int32 Requested, int32 Alive, int32 MaxAlive);

    // 그룹 중 원거리형 수. 비율은 0~1 로 자른다
    HELLDIVERS_API int32    ShooterCount(int32 GroupSize, float ShooterRatio);

    // 그룹 중 중장갑형 수. 비율은 0~1 로 자르고 반올림한다. 음수 그룹은 0
    HELLDIVERS_API int32    HeavyCount(int32 GroupSize, float HeavyRatio);

    // 경보가 BreachDelay 이상 이어졌고 마지막 웨이브에서 BreachCooldown 이상 지났는가
    HELLDIVERS_API bool     ShouldBreach(float AlertedSeconds, float BreachDelay, float SecondsSinceLastBreach, float BreachCooldown);

    // 강도(1 미만은 1)를 곱해 올림한 수
    HELLDIVERS_API int32    ScaledCount(int32 Base, float Intensity);

    // 강도(1 미만은 1)로 나눈 간격
    HELLDIVERS_API float    ScaledInterval(float BaseSeconds, float Intensity);
}
