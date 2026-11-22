// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/Player/HDDiveRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDDiveRulesTest, "Helldivers.Movement.Dive", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDDiveRulesTest::RunTest(const FString& Parameters)
{
    // 던질 수 있는가. 사유 우선순위: dead → blocked → ragdoll → airborne → prone → cooldown
    {
        struct FDiveCase
        {
            const TCHAR*        Message;
            bool                bDead;
            bool                bBlocked;
            bool                bRagdoll;
            bool                bAirborne;
            bool                bProne;
            float               SecondsSinceLastDive;
            EHDDiveRefusal      Expected;
        };

        const FDiveCase DiveCases[] =
        {
            { TEXT("Check: standing, long since last dive"),    false, false, false, false, false, 10.f,  EHDDiveRefusal::None },
            { TEXT("Check: dead before others"),                true,  true,  true,  true,  true,  0.f,   EHDDiveRefusal::Dead },
            { TEXT("Check: blocked before ragdoll"),            false, true,  true,  true,  true,  0.f,   EHDDiveRefusal::Blocked },
            { TEXT("Check: ragdoll before airborne"),           false, false, true,  true,  true,  0.f,   EHDDiveRefusal::Ragdoll },
            { TEXT("Check: airborne before prone"),             false, false, false, true,  true,  0.f,   EHDDiveRefusal::Airborne },
            { TEXT("Check: prone before cooldown"),             false, false, false, false, true,  0.f,   EHDDiveRefusal::Prone },
            { TEXT("Check: just inside cooldown"),              false, false, false, false, false, 1.49f, EHDDiveRefusal::Cooldown },
            { TEXT("Check: cooldown boundary"),                 false, false, false, false, false, 1.5f,  EHDDiveRefusal::None },
        };

        for (const FDiveCase& DiveCase : DiveCases)
        {
            const EHDDiveRefusal Result = HDDive::CheckDive(DiveCase.bDead, DiveCase.bBlocked, DiveCase.bRagdoll, DiveCase.bAirborne, DiveCase.bProne, DiveCase.SecondsSinceLastDive);
            TestEqual(DiveCase.Message, FString(HDDive::GetRefusalName(Result)), FString(HDDive::GetRefusalName(DiveCase.Expected)));
        }
    }

    // 던지는 속도
    {
        struct FLaunchCase
        {
            const TCHAR*        Message;
            FVector             Velocity;
            FVector             Forward;
            FVector             Expected;
        };

        const FLaunchCase LaunchCases[] =
        {
            { TEXT("Launch: moving direction"),                 FVector(300.0, 0.0, 0.0),    FVector(0.0, 1.0, 0.0), FVector(650.0, 0.0, 300.0) },
            { TEXT("Launch: velocity Z dropped"),               FVector(0.0, -200.0, 500.0), FVector(1.0, 0.0, 0.0), FVector(0.0, -650.0, 300.0) },
            { TEXT("Launch: slow uses facing"),                 FVector(5.0, 0.0, 0.0),      FVector(0.0, 1.0, 0.0), FVector(0.0, 650.0, 300.0) },
            { TEXT("Launch: facing horizontal normalized"),     FVector(0.0, 0.0, 0.0),      FVector(3.0, 4.0, 2.0), FVector(390.0, 520.0, 300.0) },
            { TEXT("Launch: no horizontal direction"),          FVector(0.0, 0.0, 0.0),      FVector(0.0, 0.0, 1.0), FVector(0.0, 0.0, 300.0) },
        };

        for (const FLaunchCase& LaunchCase : LaunchCases)
        {
            const FVector Result = HDDive::ComputeLaunchVelocity(LaunchCase.Velocity, LaunchCase.Forward);
            TestEqual(LaunchCase.Message, Result, LaunchCase.Expected, 0.01f);
        }
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
