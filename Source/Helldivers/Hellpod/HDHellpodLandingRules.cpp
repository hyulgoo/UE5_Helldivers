// Fill out your copyright notice in the Description page of Project Settings.

#include "Hellpod/HDHellpodLandingRules.h"

namespace HDHellpodLanding
{
    float RevealAlpha(const float Elapsed, const float Duration)
    {
        if (Duration <= 0.f)
        {
            return 1.f;
        }

        const float T       = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
        const float Inverse = 1.f - T;
        return 1.f - Inverse * Inverse * Inverse;
    }

    float ImpactShakeWeight(const float Elapsed, const float Duration)
    {
        if (Duration <= 0.f || Elapsed < 0.f || Elapsed >= Duration)
        {
            return 0.f;
        }

        const float Remain = 1.f - Elapsed / Duration;
        return Remain * Remain;
    }

    float ControlReturnTime(const float ViewBlendDelay, const float ViewBlendTime, const float RiseStart, const float RiseTime)
    {
        const float BlendEnd    = FMath::Max(ViewBlendDelay, 0.f) + FMath::Max(ViewBlendTime, 0.f);
        const float RiseEnd     = FMath::Max(RiseStart, 0.f) + FMath::Max(RiseTime, 0.f);
        return FMath::Max(BlendEnd, RiseEnd);
    }

    float ClampCameraDistance(const float DesiredDistance, const bool bBlocked, const float HitDistance, const float Margin, const float MinDistance)
    {
        const float Upper   = FMath::Max(DesiredDistance, MinDistance);
        const float Wanted  = bBlocked ? HitDistance - Margin : DesiredDistance;
        return FMath::Clamp(Wanted, MinDistance, Upper);
    }

    FVector CapEjectVelocity(const float BaseYawDeg, const float UpSpeed, const float SideSpeed, const float SpreadDeg, const float SpreadUnit)
    {
        const float Yaw         = BaseYawDeg + SpreadDeg * FMath::Clamp(SpreadUnit, -1.f, 1.f);
        const float YawRad      = FMath::DegreesToRadians(Yaw);
        return FVector(FMath::Cos(YawRad) * SideSpeed, FMath::Sin(YawRad) * SideSpeed, UpSpeed);
    }
}
