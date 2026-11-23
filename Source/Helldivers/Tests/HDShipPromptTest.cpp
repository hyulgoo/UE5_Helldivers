// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Ship/HDShipHubRules.h"
#include "UI/HDShipUILayout.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDShipPromptTest, "Helldivers.FrontEnd.ShipPrompt", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDShipPromptTest
{
    const FBox2D        Safe(FVector2D(0.f, 0.f), FVector2D(1280.f, 720.f));
    constexpr float     Gap             = 6.f;
    constexpr float     ShowDistance    = 1800.f;
    const FVector2D     PassiveSize(160.f, 28.f);
    const FVector2D     ActiveSize(240.f, 40.f);

    FHDPromptTagInput MakeInput(const TCHAR* Id, const FVector2D& Anchor, bool bProjected, float Distance, bool bInRange)
    {
        FHDPromptTagInput Input;
        Input.Id            = FName(Id);
        Input.ScreenAnchor  = Anchor;
        Input.bProjected    = bProjected;
        Input.Distance2D    = Distance;
        Input.bInRange      = bInRange;
        Input.Size          = bInRange ? ActiveSize : PassiveSize;
        return Input;
    }

    bool IsInside(const FBox2D& Outer, const FBox2D& Inner, float Tolerance = 0.01f)
    {
        return Inner.Min.X >= Outer.Min.X - Tolerance && Inner.Min.Y >= Outer.Min.Y - Tolerance
            && Inner.Max.X <= Outer.Max.X + Tolerance && Inner.Max.Y <= Outer.Max.Y + Tolerance;
    }

    bool Overlaps(const FBox2D& A, const FBox2D& B, float Tolerance = 0.01f)
    {
        const float Width = FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X);
        const float Height = FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y);
        return Width > Tolerance && Height > Tolerance;
    }

    // 보이는 상자를 글자 상자로 넣어 겹침·잘림을 센다
    HDShipUILayout::FHDTextProblems CheckTags(const TArray<FHDPromptTag>& Tags, const FBox2D& View)
    {
        TArray<HDShipUILayout::FHDDrawnText> Texts;
        for (const FHDPromptTag& Tag : Tags)
        {
            if (Tag.bVisible == false)
            {
                continue;
            }
            HDShipUILayout::FHDDrawnText Drawn;
            Drawn.Rect  = Tag.Rect;
            Drawn.Text  = Tag.Id.ToString();
            Texts.Add(Drawn);
        }
        return HDShipUILayout::FindTextProblems(Texts, View);
    }
}

bool FHDShipPromptTest::RunTest(const FString& Parameters)
{
    using namespace HDShipPromptTest;

    // P1 기준점
    {
        const TArray<FHDShipTerminalInfo>& Terminals = HDShipHub::GetTerminals();
        if (TestEqual(TEXT("P1: three terminals"), Terminals.Num(), 3))
        {
            TestEqual(TEXT("P1: war table anchor"), HDShipHub::GetPromptAnchor(Terminals[0]), FVector(0.f, 0.f, 300.f));
            TestEqual(TEXT("P1: ship management anchor"), HDShipHub::GetPromptAnchor(Terminals[1]), FVector(-600.f, -1050.f, 230.f));
            TestEqual(TEXT("P1: armory anchor"), HDShipHub::GetPromptAnchor(Terminals[2]), FVector(-600.f, 1050.f, 230.f));
        }
    }

    // P2 하나
    {
        const TArray<FHDPromptTag> Tags = HDShipHub::PlacePromptTags({ MakeInput(TEXT("A"), FVector2D(640.f, 400.f), true, 1000.f, false) }, Safe, Gap, ShowDistance);
        if (TestEqual(TEXT("P2: one result"), Tags.Num(), 1))
        {
            TestTrue(TEXT("P2: visible"), Tags[0].bVisible);
            TestFalse(TEXT("P2: passive"), Tags[0].bActive);
            TestEqual(TEXT("P2: center X 640"), static_cast<float>(Tags[0].Rect.GetCenter().X), 640.f, 0.01f);
            TestEqual(TEXT("P2: bottom 394"), static_cast<float>(Tags[0].Rect.Max.Y), 394.f, 0.01f);
            TestEqual(TEXT("P2: size"), Tags[0].Rect.GetSize(), PassiveSize);
        }
    }

    // P3 겹침 밀기
    TArray<FHDPromptTag> P3Tags;
    {
        P3Tags = HDShipHub::PlacePromptTags({
            MakeInput(TEXT("A"), FVector2D(640.f, 400.f), true, 1000.f, false),
            MakeInput(TEXT("B"), FVector2D(640.f, 400.f), true, 1000.f, false) }, Safe, Gap, ShowDistance);
        if (TestEqual(TEXT("P3: two results"), P3Tags.Num(), 2))
        {
            TestTrue(TEXT("P3: both visible"), P3Tags[0].bVisible && P3Tags[1].bVisible);
            TestFalse(TEXT("P3: no overlap"), Overlaps(P3Tags[0].Rect, P3Tags[1].Rect));
            TestEqual(TEXT("P3: first bottom 394"), static_cast<float>(P3Tags[0].Rect.Max.Y), 394.f, 0.01f);
            TestEqual(TEXT("P3: second bottom 360"), static_cast<float>(P3Tags[1].Rect.Max.Y), 360.f, 0.01f);
        }
    }

    // P4 숨김
    {
        const TArray<FHDPromptTag> Tags = HDShipHub::PlacePromptTags({
            MakeInput(TEXT("Far"), FVector2D(640.f, 400.f), true, 1801.f, false),
            MakeInput(TEXT("Behind"), FVector2D(640.f, 400.f), false, 1000.f, false),
            MakeInput(TEXT("Outside"), FVector2D(-50.f, 400.f), true, 1000.f, false) }, Safe, Gap, ShowDistance);
        if (TestEqual(TEXT("P4: three results"), Tags.Num(), 3))
        {
            TestFalse(TEXT("P4: distance 1801 hidden"), Tags[0].bVisible);
            TestFalse(TEXT("P4: not projected hidden"), Tags[1].bVisible);
            TestFalse(TEXT("P4: outside safe hidden"), Tags[2].bVisible);
        }
    }

    // P5 활성
    {
        const TArray<FHDPromptTag> Behind = HDShipHub::PlacePromptTags({ MakeInput(TEXT("A"), FVector2D(640.f, 400.f), false, 200.f, true) }, Safe, Gap, ShowDistance);
        if (TestEqual(TEXT("P5: behind one result"), Behind.Num(), 1))
        {
            TestTrue(TEXT("P5: behind visible"), Behind[0].bVisible);
            TestTrue(TEXT("P5: behind active"), Behind[0].bActive);
            TestTrue(TEXT("P5: behind edge clamped"), Behind[0].bEdgeClamped);
            TestEqual(TEXT("P5: behind center X 640"), static_cast<float>(Behind[0].Rect.GetCenter().X), 640.f, 0.01f);
            TestEqual(TEXT("P5: behind bottom 672"), static_cast<float>(Behind[0].Rect.Max.Y), 672.f, 0.01f);
        }

        const TArray<FHDPromptTag> Outside = HDShipHub::PlacePromptTags({ MakeInput(TEXT("A"), FVector2D(-50.f, 400.f), true, 200.f, true) }, Safe, Gap, ShowDistance);
        if (TestEqual(TEXT("P5: outside one result"), Outside.Num(), 1))
        {
            TestTrue(TEXT("P5: outside visible"), Outside[0].bVisible);
            TestTrue(TEXT("P5: outside inside safe"), IsInside(Safe, Outside[0].Rect));
            TestEqual(TEXT("P5: outside left 0"), static_cast<float>(Outside[0].Rect.Min.X), 0.f, 0.01f);
            TestTrue(TEXT("P5: outside edge clamped"), Outside[0].bEdgeClamped);
        }

        const TArray<FHDPromptTag> Far = HDShipHub::PlacePromptTags({ MakeInput(TEXT("A"), FVector2D(640.f, 400.f), true, 2500.f, true) }, Safe, Gap, ShowDistance);
        if (TestEqual(TEXT("P5: far one result"), Far.Num(), 1))
        {
            TestTrue(TEXT("P5: far active visible"), Far[0].bVisible);
        }
    }

    // P6 순서
    TArray<FHDPromptTag> P6Tags;
    {
        P6Tags = HDShipHub::PlacePromptTags({
            MakeInput(TEXT("A"), FVector2D(640.f, 400.f), true, 1000.f, false),
            MakeInput(TEXT("B"), FVector2D(640.f, 400.f), true, 200.f, true) }, Safe, Gap, ShowDistance);
        if (TestEqual(TEXT("P6: two results"), P6Tags.Num(), 2))
        {
            TestEqual(TEXT("P6: result order A"), P6Tags[0].Id, FName(TEXT("A")));
            TestEqual(TEXT("P6: result order B"), P6Tags[1].Id, FName(TEXT("B")));
            TestTrue(TEXT("P6: B active"), P6Tags[1].bActive);
            TestEqual(TEXT("P6: B in place (bottom 394)"), static_cast<float>(P6Tags[1].Rect.Max.Y), 394.f, 0.01f);
            TestEqual(TEXT("P6: B center X 640"), static_cast<float>(P6Tags[1].Rect.GetCenter().X), 640.f, 0.01f);
            TestTrue(TEXT("P6: A visible"), P6Tags[0].bVisible);
            TestTrue(TEXT("P6: A pushed"), P6Tags[0].Rect.Max.Y < 394.f - 0.01f);
            TestFalse(TEXT("P6: no overlap"), Overlaps(P6Tags[0].Rect, P6Tags[1].Rect));
        }
    }

    // P7 붐빔
    const FBox2D CrowdSafe(FVector2D(0.f, 0.f), FVector2D(400.f, 100.f));
    TArray<FHDPromptTag> P7Tags;
    {
        TArray<FHDPromptTagInput> Inputs;
        for (int32 Index = 0; Index < 6; ++Index)
        {
            Inputs.Add(MakeInput(*FString::Printf(TEXT("T%d"), Index), FVector2D(200.f, 50.f), true, 1000.f, false));
        }
        P7Tags = HDShipHub::PlacePromptTags(Inputs, CrowdSafe, Gap, ShowDistance);
        if (TestEqual(TEXT("P7: six results"), P7Tags.Num(), 6))
        {
            int32 Hidden = 0;
            for (int32 Index = 0; Index < P7Tags.Num(); ++Index)
            {
                if (P7Tags[Index].bVisible == false)
                {
                    ++Hidden;
                    continue;
                }
                TestTrue(*FString::Printf(TEXT("P7: T%d inside safe"), Index), IsInside(CrowdSafe, P7Tags[Index].Rect));
                for (int32 Other = 0; Other < Index; ++Other)
                {
                    if (P7Tags[Other].bVisible)
                    {
                        TestFalse(*FString::Printf(TEXT("P7: T%d and T%d do not overlap"), Index, Other), Overlaps(P7Tags[Index].Rect, P7Tags[Other].Rect));
                    }
                }
            }
            TestTrue(TEXT("P7: at least one hidden"), Hidden >= 1);
        }
    }

    // P8 겹침 없음 확인
    {
        const HDShipUILayout::FHDTextProblems P3Problems = CheckTags(P3Tags, Safe);
        TestEqual(TEXT("P8: P3 overlaps 0"), P3Problems.Overlaps, 0);
        TestEqual(TEXT("P8: P3 clipped 0"), P3Problems.Clipped, 0);
        const HDShipUILayout::FHDTextProblems P6Problems = CheckTags(P6Tags, Safe);
        TestEqual(TEXT("P8: P6 overlaps 0"), P6Problems.Overlaps, 0);
        TestEqual(TEXT("P8: P6 clipped 0"), P6Problems.Clipped, 0);
        const HDShipUILayout::FHDTextProblems P7Problems = CheckTags(P7Tags, CrowdSafe);
        TestEqual(TEXT("P8: P7 overlaps 0"), P7Problems.Overlaps, 0);
        TestEqual(TEXT("P8: P7 clipped 0"), P7Problems.Clipped, 0);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
