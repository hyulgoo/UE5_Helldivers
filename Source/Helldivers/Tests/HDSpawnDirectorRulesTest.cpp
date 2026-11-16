// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Game/HDSpawnDirectorRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDSpawnDirectorRulesTest, "Helldivers.Director.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDSpawnDirectorRulesTest::RunTest(const FString& Parameters)
{
    constexpr float LargeSeconds = 1000000.f;

    // ClampSpawnCount(Requested, Alive, MaxAlive)
    TestEqual(TEXT("ClampSpawnCount(3, 10, 30)"), HDSpawnRules::ClampSpawnCount(3, 10, 30), 3);
    TestEqual(TEXT("ClampSpawnCount(8, 26, 30)"), HDSpawnRules::ClampSpawnCount(8, 26, 30), 4);
    TestEqual(TEXT("ClampSpawnCount(5, 30, 30)"), HDSpawnRules::ClampSpawnCount(5, 30, 30), 0);
    TestEqual(TEXT("ClampSpawnCount(5, 35, 30) is not negative"), HDSpawnRules::ClampSpawnCount(5, 35, 30), 0);

    // ShooterCount(GroupSize, ShooterRatio)
    TestEqual(TEXT("ShooterCount(3, 0.3)"), HDSpawnRules::ShooterCount(3, 0.3f), 1);
    TestEqual(TEXT("ShooterCount(8, 0.3)"), HDSpawnRules::ShooterCount(8, 0.3f), 2);
    TestEqual(TEXT("ShooterCount(4, 1.5) ratio clamped to 1"), HDSpawnRules::ShooterCount(4, 1.5f), 4);
    TestEqual(TEXT("ShooterCount(4, -0.2) ratio clamped to 0"), HDSpawnRules::ShooterCount(4, -0.2f), 0);

    // ShouldBreach(AlertedSeconds, BreachDelay, SecondsSinceLastBreach, BreachCooldown)
    TestFalse(TEXT("ShouldBreach(5.9, 6, large, 40)"), HDSpawnRules::ShouldBreach(5.9f, 6.f, LargeSeconds, 40.f));
    TestTrue(TEXT("ShouldBreach(6.0, 6, large, 40)"), HDSpawnRules::ShouldBreach(6.0f, 6.f, LargeSeconds, 40.f));
    TestFalse(TEXT("ShouldBreach(10, 6, 39.9, 40)"), HDSpawnRules::ShouldBreach(10.f, 6.f, 39.9f, 40.f));
    TestTrue(TEXT("ShouldBreach(10, 6, 40, 40)"), HDSpawnRules::ShouldBreach(10.f, 6.f, 40.f, 40.f));

    // ScaledCount(Base, Intensity)
    TestEqual(TEXT("ScaledCount(3, 2.0)"), HDSpawnRules::ScaledCount(3, 2.0f), 6);
    TestEqual(TEXT("ScaledCount(3, 0.5) intensity below 1 is 1"), HDSpawnRules::ScaledCount(3, 0.5f), 3);

    // ScaledInterval(BaseSeconds, Intensity)
    TestEqual(TEXT("ScaledInterval(25, 2.0)"), HDSpawnRules::ScaledInterval(25.f, 2.0f), 12.5f);
    TestEqual(TEXT("ScaledInterval(25, 0.5) intensity below 1 is 1"), HDSpawnRules::ScaledInterval(25.f, 0.5f), 25.f);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
