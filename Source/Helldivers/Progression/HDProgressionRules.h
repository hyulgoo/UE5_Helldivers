// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Progression/HDProfileTypes.h"

// 진행 데이터 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Progression.Profile)로 고정한다
namespace HDProgression
{
    // 상한. 원작의 상한 규모를 따른 값이다
    constexpr int32                 MaxRequisition = 50000;
    constexpr int32                 MaxCommonSamples = 500;
    constexpr int32                 MaxRareSamples = 250;
    constexpr int32                 MaxSuperSamples = 100;
    constexpr int32                 DefaultMaxDifficulty = 5;

    HELLDIVERS_API bool             CanAfford(const FHDProfileData& Data, const FHDCost& Cost);

    // 모자라면 아무것도 바꾸지 않고 거짓(다 내거나 하나도 안 낸다)
    HELLDIVERS_API bool             TrySpend(FHDProfileData& Data, const FHDCost& Cost);

    // 더한 뒤 각각 [0, 상한] 으로 자른다. 음수도 받는다(치트)
    HELLDIVERS_API void             AddResources(FHDProfileData& Data, int32 Requisition, const FHDSampleCounts& Samples);

    // 읽은 뒤·바꾼 뒤마다 부른다: 자원 [0, 상한], 난이도 [1, MaxDifficulty], 단계 1 미만 업그레이드 삭제, 해금·로드아웃 중복 삭제(순서 유지), Version = 1
    HELLDIVERS_API void             Sanitize(FHDProfileData& Data, int32 MaxDifficulty = DefaultMaxDifficulty);

    // 로그·치트 한 줄. 업그레이드는 이름 순, 비면 빈 글자
    HELLDIVERS_API FString          Describe(const FHDProfileData& Data);
}
