// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Game/HDMissionPlacement.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDMissionPlacementTest, "Helldivers.Mission.Placement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDMissionPlacementTest
{
    TArray<FVector2D> Compute(int32 Count, float MinRadius, float MaxRadius, float MinSeparation, int32 Seed)
    {
        FRandomStream Stream(Seed);
        return HDMissionPlacement::ComputeRingOffsets(Count, MinRadius, MaxRadius, MinSeparation, Stream);
    }

    // 점이 하나 이상 있고 모든 점의 길이가 [MinRadius, MaxRadius] 안인가
    bool AreAllWithinRadius(const TArray<FVector2D>& Offsets, float MinRadius, float MaxRadius)
    {
        if (Offsets.IsEmpty())
        {
            return false;
        }

        for (const FVector2D& Offset : Offsets)
        {
            const double Length = Offset.Size();
            if (Length < MinRadius || Length > MaxRadius)
            {
                return false;
            }
        }
        return true;
    }

    // 모든 쌍 중 가장 가까운 두 점의 거리. 점이 2개 미만이면 0
    double MinPairDistance(const TArray<FVector2D>& Offsets)
    {
        if (Offsets.Num() < 2)
        {
            return 0.0;
        }

        double MinDistance = TNumericLimits<double>::Max();
        for (int32 First = 0; First < Offsets.Num(); ++First)
        {
            for (int32 Second = First + 1; Second < Offsets.Num(); ++Second)
            {
                MinDistance = FMath::Min(MinDistance, FVector2D::Distance(Offsets[First], Offsets[Second]));
            }
        }
        return MinDistance;
    }
}

bool FHDMissionPlacementTest::RunTest(const FString& Parameters)
{
    using namespace HDMissionPlacementTest;

    // 개수 · 반지름 범위 · 간격: 목표 2, 링 80~150m, 간격 60m, 시드 7
    const TArray<FVector2D> Offsets = Compute(2, 8000.f, 15000.f, 6000.f, 7);
    TestEqual(TEXT("Count: 2 points"), Offsets.Num(), 2);
    TestTrue(TEXT("Radius: every point is within [8000, 15000]"), AreAllWithinRadius(Offsets, 8000.f, 15000.f));
    TestTrue(TEXT("Separation: has 2 points"), Offsets.Num() == 2);
    if (Offsets.Num() == 2)
    {
        const double Separation = FVector2D::Distance(Offsets[0], Offsets[1]);
        TestTrue(FString::Printf(TEXT("Separation: distance %.1f >= 6000"), Separation), Separation >= 6000.0);
    }

    // 결정적: 같은 시드로 두 번
    {
        const TArray<FVector2D> First = Compute(2, 8000.f, 15000.f, 6000.f, 7);
        const TArray<FVector2D> Second = Compute(2, 8000.f, 15000.f, 6000.f, 7);
        TestEqual(TEXT("Deterministic: first run has 2 points"), First.Num(), 2);
        TestTrue(TEXT("Deterministic: same seed gives the same points"), First.Num() == Second.Num() && First == Second);
    }

    // 0개
    {
        const TArray<FVector2D> Empty = Compute(0, 8000.f, 15000.f, 6000.f, 7);
        TestEqual(TEXT("Zero: empty array"), Empty.Num(), 0);
    }

    // 1개
    {
        const TArray<FVector2D> Single = Compute(1, 10000.f, 16000.f, 0.f, 3);
        TestEqual(TEXT("Single: 1 point"), Single.Num(), 1);
        TestTrue(TEXT("Single: point is within [10000, 16000]"), AreAllWithinRadius(Single, 10000.f, 16000.f));
    }

    // 간격이 불가능할 때: 개수는 채운다
    {
        const TArray<FVector2D> Crowded = Compute(6, 1000.f, 1200.f, 5000.f, 1);
        TestEqual(TEXT("Impossible separation: still 6 points"), Crowded.Num(), 6);
        TestTrue(TEXT("Impossible separation: every point is within [1000, 1200]"), AreAllWithinRadius(Crowded, 1000.f, 1200.f));
    }

    // 간격 규칙(8개): 이웃 각도 차가 22.5° 까지 좁아져 간격 확인이 없으면 6000 을 못 맞추는 시드가 생긴다
    for (int32 Seed = 1; Seed <= 50; ++Seed)
    {
        const TArray<FVector2D> Ring = Compute(8, 8000.f, 15000.f, 6000.f, Seed);
        if (TestEqual(FString::Printf(TEXT("Separation (8 points): seed %d has 8 points"), Seed), Ring.Num(), 8) == false)
        {
            continue;
        }

        TestTrue(FString::Printf(TEXT("Separation (8 points): seed %d every point is within [8000, 15000]"), Seed), AreAllWithinRadius(Ring, 8000.f, 15000.f));

        const double NearestPair = MinPairDistance(Ring);
        TestTrue(FString::Printf(TEXT("Separation (8 points): seed %d nearest pair %.1f >= 6000"), Seed, NearestPair), NearestPair >= 6000.0);
    }

    // 대체 후보: 링 1000~1200 에서 간격 5000 은 늘 못 맞추므로 두 번째 점은 "가장 가까운 점과의 거리가 가장 큰" 후보다
    for (int32 Seed = 1; Seed <= 50; ++Seed)
    {
        const TArray<FVector2D> Pair = Compute(2, 1000.f, 1200.f, 5000.f, Seed);
        if (TestEqual(FString::Printf(TEXT("Fallback: seed %d has 2 points"), Seed), Pair.Num(), 2) == false)
        {
            continue;
        }

        const double Distance = MinPairDistance(Pair);
        TestTrue(FString::Printf(TEXT("Fallback: seed %d distance %.1f >= 2000"), Seed, Distance), Distance >= 2000.0);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
