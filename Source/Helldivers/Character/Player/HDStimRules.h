// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 스팀(회복 주사)을 쓸 수 없는 사유. None 이면 쓸 수 있다
enum class EHDStimRefusal : uint8
{
    None,
    Dead,
    Active,
    NoStim,
    FullHealth
};

// 스팀 규칙. 월드·액터와 무관한 순수 함수라 자동 테스트(Helldivers.Combat.Stim)로 고정한다
namespace HDStim
{
    // 쓸 수 있으면 None. 위에서부터 먼저 맞는 사유: 죽음 → 이미 회복 중 → 스팀 없음(1 미만) → 체력이 최대 이상
    HELLDIVERS_API EHDStimRefusal   CheckUse(float StimCount, float Health, float MaxHealth, bool bDead, bool bActive);

    // 한 번의 회복 뒤 체력. HealPerSecond·StepSeconds 가 음수면 0 으로 보고, 결과는 [0, MaxHealth]
    HELLDIVERS_API float            HealStep(float Health, float MaxHealth, float HealPerSecond, float StepSeconds);

    // 로그용 사유 이름: "none"·"dead"·"active"·"no stims"·"full health"
    HELLDIVERS_API const TCHAR*     GetRefusalName(EHDStimRefusal Refusal);
}
