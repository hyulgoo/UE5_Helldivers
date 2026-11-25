// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Component/HDFaceAimRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDFaceAimRulesTest, "Helldivers.Combat.FaceAim", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDFaceAimRulesTest::RunTest(const FString& Parameters)
{
    // 몸 → 조준 요 차이: [-180, 180] 로 감는다
    TestEqual(TEXT("Yaw: same"),                HDFaceAim::YawToAim(30.f, 30.f),    0.f);
    TestEqual(TEXT("Yaw: right 90"),            HDFaceAim::YawToAim(0.f, 90.f),     90.f);
    TestEqual(TEXT("Yaw: wraps left"),          HDFaceAim::YawToAim(170.f, -170.f), 20.f);
    TestEqual(TEXT("Yaw: wraps right"),         HDFaceAim::YawToAim(-170.f, 170.f), -20.f);
    TestEqual(TEXT("Yaw: behind"),              FMath::Abs(HDFaceAim::YawToAim(0.f, 180.f)), 180.f);

    // 뒤를 보는가: 절댓값이 기준 이상
    TestTrue(TEXT("Back: 180 needs turn"),      HDFaceAim::NeedsTurnBeforeFire(180.f, 90.f));
    TestTrue(TEXT("Back: -120 needs turn"),     HDFaceAim::NeedsTurnBeforeFire(-120.f, 90.f));
    TestTrue(TEXT("Back: exactly threshold"),   HDFaceAim::NeedsTurnBeforeFire(90.f, 90.f));
    TestFalse(TEXT("Back: 60 fires at once"),   HDFaceAim::NeedsTurnBeforeFire(60.f, 90.f));

    // 돈 비율
    TestEqual(TEXT("Progress: not turned"),     HDFaceAim::TurnProgress(180.f, 180.f),  0.f);
    TestEqual(TEXT("Progress: half"),           HDFaceAim::TurnProgress(180.f, 90.f),   0.5f);
    TestEqual(TEXT("Progress: sign ignored"),   HDFaceAim::TurnProgress(-160.f, 40.f),  0.75f);
    TestEqual(TEXT("Progress: aim moved away"), HDFaceAim::TurnProgress(120.f, 150.f),  0.f);
    TestEqual(TEXT("Progress: zero initial"),   HDFaceAim::TurnProgress(0.f, 10.f),     1.f);

    // 절반 이상 돌았으면 쏜다
    TestFalse(TEXT("Enough: 40% not yet"),      HDFaceAim::IsTurnedEnough(180.f, 108.f, 0.5f));
    TestTrue(TEXT("Enough: exactly half"),      HDFaceAim::IsTurnedEnough(180.f, 90.f, 0.5f));
    TestTrue(TEXT("Enough: facing"),            HDFaceAim::IsTurnedEnough(180.f, 0.f, 0.5f));

    return true;
}

#endif
