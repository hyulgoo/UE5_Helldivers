// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/HDFaceAimRules.h"

namespace HDFaceAim
{
    float YawToAim(const float FacingYaw, const float AimYaw)
    {
        return FRotator::NormalizeAxis(AimYaw - FacingYaw);
    }

    bool NeedsTurnBeforeFire(const float YawToAimDeg, const float BackFacingAngle)
    {
        return FMath::Abs(YawToAimDeg) >= BackFacingAngle;
    }

    float TurnProgress(const float InitialYawToAimDeg, const float CurrentYawToAimDeg)
    {
        const float Initial = FMath::Abs(InitialYawToAimDeg);
        if (Initial <= 0.f)
        {
            return 1.f;
        }

        return FMath::Clamp(1.f - FMath::Abs(CurrentYawToAimDeg) / Initial, 0.f, 1.f);
    }

    bool IsTurnedEnough(const float InitialYawToAimDeg, const float CurrentYawToAimDeg, const float RequiredProgress)
    {
        return TurnProgress(InitialYawToAimDeg, CurrentYawToAimDeg) >= RequiredProgress;
    }
}
