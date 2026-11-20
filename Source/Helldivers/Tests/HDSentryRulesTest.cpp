// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Stratagem/Deployable/HDSentryRules.h"

namespace HDSentryRulesTest
{
    // X 축 위의 후보 하나
    FHDSentryCandidate MakeCandidate(const float X, const bool bAlive = true, const bool bVisible = true)
    {
        FHDSentryCandidate Candidate;
        Candidate.Location  = FVector(X, 0.f, 0.f);
        Candidate.bAlive    = bAlive;
        Candidate.bVisible  = bVisible;
        return Candidate;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDSentryRulesTest, "Helldivers.Stratagem.Sentry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDSentryRulesTest::RunTest(const FString& Parameters)
{
    using namespace HDSentryRulesTest;

    // 원점 (0,0,0), 사거리 1000
    const FVector Origin = FVector::ZeroVector;
    constexpr float Range = 1000.f;

    TestEqual(TEXT("SelectTarget: nearest chosen"),
        HDSentryRules::SelectTarget(Origin, Range, { MakeCandidate(500.f), MakeCandidate(300.f), MakeCandidate(800.f) }), 1);

    TestEqual(TEXT("SelectTarget: out of range excluded"),
        HDSentryRules::SelectTarget(Origin, Range, { MakeCandidate(1200.f) }), (int32)INDEX_NONE);

    TestEqual(TEXT("SelectTarget: exactly at range included"),
        HDSentryRules::SelectTarget(Origin, Range, { MakeCandidate(1000.f) }), 0);

    // 가까운 쪽이 죽었거나 가려져 있으면 먼 쪽을 고른다
    TestEqual(TEXT("SelectTarget: dead excluded"),
        HDSentryRules::SelectTarget(Origin, Range, { MakeCandidate(200.f, false, true), MakeCandidate(600.f) }), 1);

    TestEqual(TEXT("SelectTarget: not visible excluded"),
        HDSentryRules::SelectTarget(Origin, Range, { MakeCandidate(200.f, true, false), MakeCandidate(600.f) }), 1);

    TestEqual(TEXT("SelectTarget: empty -> none"),
        HDSentryRules::SelectTarget(Origin, Range, TArray<FHDSentryCandidate>()), (int32)INDEX_NONE);

    TestEqual(TEXT("SelectTarget: tie -> first"),
        HDSentryRules::SelectTarget(Origin, Range, { MakeCandidate(700.f), MakeCandidate(400.f), MakeCandidate(-400.f) }), 1);

    TestEqual(TEXT("SelectTarget: range 0 -> none"),
        HDSentryRules::SelectTarget(Origin, 0.f, { MakeCandidate(0.f), MakeCandidate(100.f) }), (int32)INDEX_NONE);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
