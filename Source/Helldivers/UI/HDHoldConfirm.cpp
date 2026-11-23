// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDHoldConfirm.h"

bool FHDHoldConfirm::Start(FName InItemId, int32 InSubIndex, float Seconds)
{
    // 0 이하 = 곧바로 실행하라는 뜻, 돌고 있으면 진행을 그대로 둔다
    if (Seconds <= 0.f || bActive)
    {
        return false;
    }

    ItemId      = InItemId;
    SubIndex    = InSubIndex;
    Duration    = Seconds;
    Elapsed     = 0.f;
    bActive     = true;
    return true;
}

void FHDHoldConfirm::Cancel()
{
    bActive = false;
    Elapsed = 0.f;
}

bool FHDHoldConfirm::Tick(float DeltaSeconds)
{
    if (bActive == false)
    {
        return false;
    }

    Elapsed += FMath::Max(0.f, DeltaSeconds);
    if (Elapsed < Duration)
    {
        return false;
    }

    // 다 찼다: 한 번만 참을 주고 끈다
    bActive = false;
    return true;
}

float FHDHoldConfirm::GetProgress() const
{
    return (bActive && Duration > 0.f) ? FMath::Clamp(Elapsed / Duration, 0.f, 1.f) : 0.f;
}
