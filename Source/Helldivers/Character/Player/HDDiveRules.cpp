// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Player/HDDiveRules.h"

EHDDiveRefusal HDDive::CheckDive(bool bDead, bool bBlocked, bool bRagdoll, bool bAirborne, bool bProne, float SecondsSinceLastDive)
{
    if (bDead)
    {
        return EHDDiveRefusal::Dead;
    }

    if (bBlocked)
    {
        return EHDDiveRefusal::Blocked;
    }

    if (bRagdoll)
    {
        return EHDDiveRefusal::Ragdoll;
    }

    if (bAirborne)
    {
        return EHDDiveRefusal::Airborne;
    }

    if (bProne)
    {
        return EHDDiveRefusal::Prone;
    }

    if (SecondsSinceLastDive < Cooldown)
    {
        return EHDDiveRefusal::Cooldown;
    }

    return EHDDiveRefusal::None;
}

FVector HDDive::ComputeLaunchVelocity(const FVector& Velocity, const FVector& Forward)
{
    const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0);
    // 느리면 보는 방향. 보는 방향에도 수평 성분이 없으면 GetSafeNormal2D 가 0 벡터를 돌려줘 위로만 던진다
    const FVector Direction = HorizontalVelocity.Size() > MinMoveSpeed ? HorizontalVelocity.GetSafeNormal() : Forward.GetSafeNormal2D();

    return Direction * HorizontalSpeed + FVector(0.0, 0.0, VerticalSpeed);
}

const TCHAR* HDDive::GetRefusalName(EHDDiveRefusal Refusal)
{
    switch (Refusal)
    {
    case EHDDiveRefusal::None:      return TEXT("none");
    case EHDDiveRefusal::Dead:      return TEXT("dead");
    case EHDDiveRefusal::Blocked:   return TEXT("blocked");
    case EHDDiveRefusal::Ragdoll:   return TEXT("ragdoll");
    case EHDDiveRefusal::Airborne:  return TEXT("airborne");
    case EHDDiveRefusal::Prone:     return TEXT("prone");
    case EHDDiveRefusal::Cooldown:  return TEXT("cooldown");
    default:                        return TEXT("unknown");
    }
}
