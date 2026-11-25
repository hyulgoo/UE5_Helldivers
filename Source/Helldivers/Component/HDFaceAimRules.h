// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 사격 전 몸 돌리기 규칙. 월드·액터와 무관한 순수 함수라 자동 테스트(Helldivers.Combat.FaceAim)로 고정한다
// 뒤를 보다 사격하면(UHDGA_Fire) 먼저 조준 방향으로 돌고, 처음 각도의 RequiredProgress 이상 돌았을 때도 누르고 있으면 쏜다
namespace HDFaceAim
{
    // 몸 요 → 조준 요 차이. [-180, 180]
    HELLDIVERS_API float    YawToAim(float FacingYaw, float AimYaw);

    // 몸이 조준 방향에서 BackFacingAngle 이상 벗어나 있으면(뒤를 보고 있으면) 먼저 돌아야 한다. 각도는 절댓값으로 본다
    HELLDIVERS_API bool     NeedsTurnBeforeFire(float YawToAimDeg, float BackFacingAngle);

    // 돈 비율. 처음 남은 각도 대비 줄어든 만큼, [0, 1]. 처음 각도가 0 이하면 1
    HELLDIVERS_API float    TurnProgress(float InitialYawToAimDeg, float CurrentYawToAimDeg);

    // RequiredProgress 이상 돌았으면 참
    HELLDIVERS_API bool     IsTurnedEnough(float InitialYawToAimDeg, float CurrentYawToAimDeg, float RequiredProgress);
}
