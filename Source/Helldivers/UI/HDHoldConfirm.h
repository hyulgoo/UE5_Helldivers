// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 길게 눌러 확정하기(함선 관리 구매). 시작·진행·취소·한 번만 완료.
// 월드와 무관한 순수 로직이라 자동 테스트(Helldivers.FrontEnd.UpgradeScreen)로 고정한다
class HELLDIVERS_API FHDHoldConfirm
{
public:
    // Seconds ≤ 0 이거나 이미 돌고 있으면 거짓
    bool                Start(FName InItemId, int32 InSubIndex, float Seconds);
    void                Cancel();
    // 다 찬 그 한 번만 참. 그 뒤 꺼진다
    bool                Tick(float DeltaSeconds);
    bool                IsActive() const        { return bActive; }
    // 0~1
    float               GetProgress() const;
    FName               GetItemId() const       { return ItemId; }
    int32               GetSubIndex() const     { return SubIndex; }

private:
    FName               ItemId;
    int32               SubIndex    = 0;
    float               Duration    = 0.f;
    float               Elapsed     = 0.f;
    bool                bActive     = false;
};
