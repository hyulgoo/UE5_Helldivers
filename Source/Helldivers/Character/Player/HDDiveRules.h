// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 급강하를 할 수 없는 사유. None 이면 할 수 있다
enum class EHDDiveRefusal : uint8
{
    None,
    Dead,
    Blocked,
    Ragdoll,
    Airborne,
    Prone,
    Cooldown
};

// 급강하 규칙. 월드·액터와 무관한 순수 함수라 자동 테스트(Helldivers.Movement.Dive)로 고정한다
namespace HDDive
{
    constexpr float HorizontalSpeed     = 650.f;    // 던지는 수평 속도(cm/s)
    constexpr float VerticalSpeed       = 300.f;    // 던지는 위쪽 속도(cm/s)
    constexpr float Cooldown            = 1.5f;     // 다시 던질 수 있을 때까지(초)
    constexpr float MinMoveSpeed        = 10.f;     // 수평 속도가 이 이하면 보는 방향으로 던진다

    // 던질 수 있으면 None. 위에서부터 먼저 맞는 사유: 죽음 → Block.All → 래그돌 → 공중 → 엎드림 → 대기(SecondsSinceLastDive < Cooldown)
    HELLDIVERS_API EHDDiveRefusal   CheckDive(bool bDead, bool bBlocked, bool bRagdoll, bool bAirborne, bool bProne, float SecondsSinceLastDive);

    // 수평 방향(움직이는 방향, 느리면 보는 방향) × HorizontalSpeed + 위로 VerticalSpeed. 수평 방향이 없으면 위로만
    HELLDIVERS_API FVector          ComputeLaunchVelocity(const FVector& Velocity, const FVector& Forward);

    // 로그용 사유 이름: "none"·"dead"·"blocked"·"ragdoll"·"airborne"·"prone"·"cooldown"
    HELLDIVERS_API const TCHAR*     GetRefusalName(EHDDiveRefusal Refusal);
}
