// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/Player/HDStimRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDStimRulesTest, "Helldivers.Combat.Stim", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDStimRulesTest::RunTest(const FString& Parameters)
{
    // 쓸 수 있는가. 사유 우선순위: Dead → Active → NoStim → FullHealth
    {
        struct FUseCase
        {
            const TCHAR*        Message;
            float               StimCount;
            float               Health;
            float               MaxHealth;
            bool                bDead;
            bool                bActive;
            EHDStimRefusal      Expected;
        };

        const FUseCase UseCases[] =
        {
            { TEXT("Use: hurt with stims"),             4.f, 50.f,   125.f, false, false, EHDStimRefusal::None },
            { TEXT("Use: just below max"),              4.f, 124.5f, 125.f, false, false, EHDStimRefusal::None },
            { TEXT("Use: full health refused"),         4.f, 125.f,  125.f, false, false, EHDStimRefusal::FullHealth },
            { TEXT("Use: no stims refused"),            0.f, 50.f,   125.f, false, false, EHDStimRefusal::NoStim },
            { TEXT("Use: dead refused"),                4.f, 0.f,    125.f, true,  false, EHDStimRefusal::Dead },
            { TEXT("Use: dead before others"),          0.f, 125.f,  125.f, true,  true,  EHDStimRefusal::Dead },
            { TEXT("Use: active before no stims"),      0.f, 125.f,  125.f, false, true,  EHDStimRefusal::Active },
            { TEXT("Use: no stims before full health"), 0.f, 125.f,  125.f, false, false, EHDStimRefusal::NoStim },
        };

        for (const FUseCase& UseCase : UseCases)
        {
            const EHDStimRefusal Result = HDStim::CheckUse(UseCase.StimCount, UseCase.Health, UseCase.MaxHealth, UseCase.bDead, UseCase.bActive);
            TestEqual(UseCase.Message, FString(HDStim::GetRefusalName(Result)), FString(HDStim::GetRefusalName(UseCase.Expected)));
        }
    }

    // 한 번의 회복
    {
        TestEqual(TEXT("Heal: one step"), HDStim::HealStep(50.f, 125.f, 60.f, 0.1f), 56.f, KINDA_SMALL_NUMBER);
        TestEqual(TEXT("Heal: clamped to max"), HDStim::HealStep(122.f, 125.f, 60.f, 0.1f), 125.f, KINDA_SMALL_NUMBER);
        TestEqual(TEXT("Heal: already max"), HDStim::HealStep(125.f, 125.f, 60.f, 0.1f), 125.f, KINDA_SMALL_NUMBER);
        TestEqual(TEXT("Heal: negative rate is zero"), HDStim::HealStep(50.f, 125.f, -10.f, 0.1f), 50.f, KINDA_SMALL_NUMBER);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
