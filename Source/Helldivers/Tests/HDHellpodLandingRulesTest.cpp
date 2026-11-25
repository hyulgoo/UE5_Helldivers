// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Hellpod/HDHellpodLandingRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDHellpodLandingRulesTest, "Helldivers.Hellpod.Landing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDHellpodLandingRulesTest::RunTest(const FString& Parameters)
{
    // 내려앉기 비율: ease-out cubic, [0, 1]
    TestEqual(TEXT("Reveal: start"),            HDHellpodLanding::RevealAlpha(0.f, 1.f),    0.f);
    TestEqual(TEXT("Reveal: half is past half"), HDHellpodLanding::RevealAlpha(0.5f, 1.f),  0.875f);
    TestEqual(TEXT("Reveal: end"),              HDHellpodLanding::RevealAlpha(1.f, 1.f),    1.f);
    TestEqual(TEXT("Reveal: after end clamps"), HDHellpodLanding::RevealAlpha(3.f, 1.f),    1.f);
    TestEqual(TEXT("Reveal: before start"),     HDHellpodLanding::RevealAlpha(-1.f, 1.f),   0.f);
    TestEqual(TEXT("Reveal: zero duration"),    HDHellpodLanding::RevealAlpha(0.f, 0.f),    1.f);

    // 충격 흔들림: 1 → 0, 제곱 감쇠
    TestEqual(TEXT("Shake: start"),             HDHellpodLanding::ImpactShakeWeight(0.f, 0.6f),  1.f);
    TestEqual(TEXT("Shake: half"),              HDHellpodLanding::ImpactShakeWeight(0.3f, 0.6f), 0.25f, KINDA_SMALL_NUMBER);
    TestEqual(TEXT("Shake: end"),               HDHellpodLanding::ImpactShakeWeight(0.6f, 0.6f), 0.f);
    TestEqual(TEXT("Shake: negative elapsed"),  HDHellpodLanding::ImpactShakeWeight(-0.1f, 0.6f), 0.f);
    TestEqual(TEXT("Shake: zero duration"),     HDHellpodLanding::ImpactShakeWeight(0.f, 0.f),   0.f);

    // 조작 반환 시각: 블렌드 끝과 상승 끝(시작 지연 포함) 중 늦은 쪽
    TestEqual(TEXT("Control: blend later"),     HDHellpodLanding::ControlReturnTime(0.7f, 1.4f, 0.4f, 1.5f), 2.1f, KINDA_SMALL_NUMBER);
    TestEqual(TEXT("Control: rise later"),      HDHellpodLanding::ControlReturnTime(0.7f, 1.0f, 0.4f, 1.5f), 1.9f, KINDA_SMALL_NUMBER);
    TestEqual(TEXT("Control: no rise delay"),   HDHellpodLanding::ControlReturnTime(0.2f, 0.5f, 0.f, 1.5f),  1.5f);
    TestEqual(TEXT("Control: negatives as 0"),  HDHellpodLanding::ControlReturnTime(-1.f, -1.f, -1.f, -1.f), 0.f);

    // 캡 사출 속도: 위 + BaseYaw 방향 수평, 퍼짐은 [-1, 1] 로 자름
    {
        const FVector Straight = HDHellpodLanding::CapEjectVelocity(0.f, 900.f, 400.f, 30.f, 0.f);
        TestTrue(TEXT("Cap: straight forward"), Straight.Equals(FVector(400.f, 0.f, 900.f), KINDA_SMALL_NUMBER));

        const FVector Right = HDHellpodLanding::CapEjectVelocity(60.f, 900.f, 400.f, 30.f, 1.f);
        TestTrue(TEXT("Cap: yaw + full spread = 90"), Right.Equals(FVector(0.f, 400.f, 900.f), 0.01f));

        const FVector Clamped = HDHellpodLanding::CapEjectVelocity(60.f, 900.f, 400.f, 30.f, 5.f);
        TestTrue(TEXT("Cap: spread unit clamps"), Clamped.Equals(Right, 0.01f));

        TestEqual(TEXT("Cap: side speed kept"), static_cast<float>(FVector2D(Clamped.X, Clamped.Y).Size()), 400.f, 0.01f);
    }

    // 카메라 거리: 막히면 당기고 [Min, Desired]
    TestEqual(TEXT("Distance: open"),           HDHellpodLanding::ClampCameraDistance(650.f, false, 0.f, 20.f, 150.f),   650.f);
    TestEqual(TEXT("Distance: blocked"),        HDHellpodLanding::ClampCameraDistance(650.f, true, 400.f, 20.f, 150.f),  380.f);
    TestEqual(TEXT("Distance: blocked close"),  HDHellpodLanding::ClampCameraDistance(650.f, true, 100.f, 20.f, 150.f),  150.f);
    TestEqual(TEXT("Distance: hit beyond"),     HDHellpodLanding::ClampCameraDistance(650.f, true, 900.f, 20.f, 150.f),  650.f);

    return true;
}

#endif
