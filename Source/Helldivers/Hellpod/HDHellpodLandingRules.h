// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 헬포드 착지 연출 규칙. 월드·액터와 무관한 순수 함수라 자동 테스트(Helldivers.Hellpod.Landing)로 고정한다
// 흐름: 착지 → 충격 흔들림(ImpactShakeWeight) + 강하 시점에서 포드 뒤 넓은 시점으로 내려앉기(RevealAlpha)
//      → CapEjectDelay 에 캡이 튕겨 나가고(CapEjectVelocity) 캐릭터가 올라온다
//      → ViewBlendDelay 뒤 캐릭터 카메라로 블렌드 → ControlReturnTime 에 조작 반환
namespace HDHellpodLanding
{
    // 강하 시점 → 착지 시점 이동 비율. ease-out cubic 이라 처음에 빠르고 끝에서 멎는다. Duration 이 0 이하면 1, 결과는 [0, 1]
    HELLDIVERS_API float    RevealAlpha(float Elapsed, float Duration);

    // 충격 흔들림 세기. 1 에서 시작해 Duration 동안 제곱으로 줄어 0. Duration 이 0 이하거나 음수 경과면 0
    HELLDIVERS_API float    ImpactShakeWeight(float Elapsed, float Duration);

    // 조작을 돌려줄 시각(착지 기준 초). 시점 블렌드가 끝나는 때(Delay + BlendTime)와 캐릭터 상승이 끝나는 때(RiseStart + RiseTime) 중 늦은 쪽. 음수 입력은 0 으로 본다
    HELLDIVERS_API float    ControlReturnTime(float ViewBlendDelay, float ViewBlendTime, float RiseStart, float RiseTime);

    // 카메라가 지형에 막힐 때 거리. 막히면 HitDistance - Margin, 아니면 DesiredDistance. 결과는 [MinDistance, DesiredDistance]
    HELLDIVERS_API float    ClampCameraDistance(float DesiredDistance, bool bBlocked, float HitDistance, float Margin, float MinDistance);

    // 캡 사출 속도(cm/s, 월드). 위로 UpSpeed, 수평으로 SideSpeed 를 BaseYaw + SpreadDeg * SpreadUnit 방향으로 준다
    // SpreadUnit 은 [-1, 1] 로 자른다(호출부가 난수를 넣는다)
    HELLDIVERS_API FVector  CapEjectVelocity(float BaseYawDeg, float UpSpeed, float SideSpeed, float SpreadDeg, float SpreadUnit);
}
