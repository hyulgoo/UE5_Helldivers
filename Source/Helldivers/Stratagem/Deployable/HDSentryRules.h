// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 포탑 표적 후보 하나(AHDSentryTurret 가 적마다 만든다)
struct FHDSentryCandidate
{
    FVector     Location    = FVector::ZeroVector;
    bool        bAlive      = true;
    bool        bVisible    = true;
};

// 포탑 표적 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Stratagem.Sentry)로 고정한다
namespace HDSentryRules
{
    // 살아 있고 보이며 Origin 에서 Range 이하(경계 포함)인 후보 중 가장 가까운 것. 거리가 같으면 앞 번호. 없거나 Range <= 0 이면 INDEX_NONE
    HELLDIVERS_API int32    SelectTarget(const FVector& Origin, const float Range, const TArray<FHDSentryCandidate>& Candidates);
}
