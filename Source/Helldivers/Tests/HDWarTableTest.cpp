// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HDShipMenuView.h"
#include "Ship/HDWarTableMap.h"
#include "Progression/HDDifficulty.h"
#include "Progression/HDMissionReward.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDWarTableTest, "Helldivers.FrontEnd.WarTable", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDWarTableTest
{
    FHDShipMenuContext MakeContext(int32 Difficulty)
    {
        FHDShipMenuContext Context;
        Context.Profile.Difficulty  = Difficulty;
        Context.Difficulties        = HDDifficulty::GetDefaultTable();
        return Context;
    }

    bool NearlyEqual(const FVector2D& A, const FVector2D& B)
    {
        return A.Equals(B, 0.01);
    }
}

bool FHDWarTableTest::RunTest(const FString& Parameters)
{
    using namespace HDWarTableTest;

    const FHDRewardRules Rules;
    const FString Level(TEXT("DesertMountains"));

    // T1 이름
    TestEqual(TEXT("T1 DesertMountains"), HDShipMenuView::FormatMapName(TEXT("DesertMountains")), FString(TEXT("DESERT MOUNTAINS")));
    TestEqual(TEXT("T1 PIE prefix"), HDShipMenuView::FormatMapName(TEXT("UEDPIE_0_DesertMountains")), FString(TEXT("DESERT MOUNTAINS")));
    TestEqual(TEXT("T1 Default"), HDShipMenuView::FormatMapName(TEXT("Default")), FString(TEXT("DEFAULT")));
    TestEqual(TEXT("T1 empty"), HDShipMenuView::FormatMapName(FString()), FString(TEXT("UNKNOWN SECTOR")));

    // T2 난이도 2
    {
        const FHDWarTableView View = HDShipMenuView::BuildWarTable(MakeContext(2), Level, Rules);
        TestEqual(TEXT("T2 name"), View.OperationName, FString(TEXT("DESERT MOUNTAINS")));
        TestEqual(TEXT("T2 type"), View.OperationType, FString(TEXT("DESTROY FACILITIES")));
        const TArray<FString> Expected = { TEXT("DESTROY 2 FACILITIES"), TEXT("REACH EXTRACTION"), TEXT("OPTIONAL: RECOVER SAMPLES") };
        TestEqual(TEXT("T2 objectives"), View.Objectives, Expected);
        TestTrue(TEXT("T2 has difficulty"), View.bHasDifficulty);
        const TArray<FString> Labels = { TEXT("EASY"), TEXT("NORMAL"), TEXT("HARD"), TEXT("VETERAN"), TEXT("ELITE") };
        if (TestEqual(TEXT("T2 segments"), View.Segments.Num(), 5))
        {
            for (int32 Index = 0; Index < 5; ++Index)
            {
                TestEqual(FString::Printf(TEXT("T2 segment %d level"), Index), View.Segments[Index].Level, Index + 1);
                TestEqual(FString::Printf(TEXT("T2 segment %d label"), Index), View.Segments[Index].Label, Labels[Index]);
                TestEqual(FString::Printf(TEXT("T2 segment %d current"), Index), View.Segments[Index].bCurrent, Index == 1);
            }
        }
        TestEqual(TEXT("T2 label"), View.DifficultyLabel, FString(TEXT("2  NORMAL")));
        TestEqual(TEXT("T2 multiplier"), View.RewardMultiplier, FString(TEXT("X1.25")));
        TestEqual(TEXT("T2 requisition"), View.SuccessRequisition, 225);
        TestEqual(TEXT("T2 samples common"), View.MapSamples.Common, 6);
        TestEqual(TEXT("T2 samples rare"), View.MapSamples.Rare, 2);
        TestEqual(TEXT("T2 samples super"), View.MapSamples.Super, 0);
    }

    // T3 끝 단계
    {
        const FHDWarTableView Elite = HDShipMenuView::BuildWarTable(MakeContext(5), Level, Rules);
        TestTrue(TEXT("T3 elite objective"), Elite.Objectives.Num() > 0 && Elite.Objectives[0] == TEXT("DESTROY 3 FACILITIES"));
        TestEqual(TEXT("T3 elite multiplier"), Elite.RewardMultiplier, FString(TEXT("X2.5")));
        TestEqual(TEXT("T3 elite requisition"), Elite.SuccessRequisition, 550);
        TestTrue(TEXT("T3 elite samples"), Elite.MapSamples.Common == 8 && Elite.MapSamples.Rare == 5 && Elite.MapSamples.Super == 2);

        const FHDWarTableView Easy = HDShipMenuView::BuildWarTable(MakeContext(1), Level, Rules);
        TestTrue(TEXT("T3 easy objective (singular)"), Easy.Objectives.Num() > 0 && Easy.Objectives[0] == TEXT("DESTROY 1 FACILITY"));
        TestEqual(TEXT("T3 easy multiplier"), Easy.RewardMultiplier, FString(TEXT("X1.0")));
        TestEqual(TEXT("T3 easy requisition"), Easy.SuccessRequisition, 140);
        TestTrue(TEXT("T3 easy samples"), Easy.MapSamples.Common == 4 && Easy.MapSamples.Rare == 0 && Easy.MapSamples.Super == 0);

        const FHDWarTableView Over = HDShipMenuView::BuildWarTable(MakeContext(9), Level, Rules);
        TestEqual(TEXT("T3 clamp objectives"), Over.Objectives, Elite.Objectives);
        TestEqual(TEXT("T3 clamp label"), Over.DifficultyLabel, Elite.DifficultyLabel);
        TestEqual(TEXT("T3 clamp multiplier"), Over.RewardMultiplier, Elite.RewardMultiplier);
        TestEqual(TEXT("T3 clamp requisition"), Over.SuccessRequisition, Elite.SuccessRequisition);
        TestTrue(TEXT("T3 clamp current"), Over.Segments.Num() == 5 && Over.Segments[4].bCurrent && Over.Segments[3].bCurrent == false);
    }

    // T4 표 없음
    {
        FHDShipMenuContext Context = MakeContext(2);
        Context.Difficulties.Reset();
        const FHDWarTableView View = HDShipMenuView::BuildWarTable(Context, Level, Rules);
        TestFalse(TEXT("T4 no difficulty"), View.bHasDifficulty);
        TestEqual(TEXT("T4 segments"), View.Segments.Num(), 0);
        TestTrue(TEXT("T4 first objective"), View.Objectives.Num() > 0 && View.Objectives[0] == TEXT("DESTROY FACILITIES"));
        TestEqual(TEXT("T4 requisition"), View.SuccessRequisition, 0);
    }

    // T5 지도 수학
    {
        TestEqual(TEXT("T5 half extent"), HDWarTableMap::ComputeHalfExtent(15000.f), 18750.f);
        TestEqual(TEXT("T5 half extent min"), HDWarTableMap::ComputeHalfExtent(2000.f), 5000.f);

        const FBox2D Rect(FVector2D(100.0, 100.0), FVector2D(500.0, 500.0));
        const FVector2D Center(1000.0, 2000.0);
        TestTrue(TEXT("T5 center"), NearlyEqual(HDWarTableMap::WorldToMap(Center, Center, 1000.f, Rect), FVector2D(300.0, 300.0)));
        TestTrue(TEXT("T5 +X is up"), NearlyEqual(HDWarTableMap::WorldToMap(FVector2D(2000.0, 2000.0), Center, 1000.f, Rect), FVector2D(300.0, 100.0)));
        TestTrue(TEXT("T5 +Y is right"), NearlyEqual(HDWarTableMap::WorldToMap(FVector2D(1000.0, 3000.0), Center, 1000.f, Rect), FVector2D(500.0, 300.0)));
        TestTrue(TEXT("T5 bottom left"), NearlyEqual(HDWarTableMap::WorldToMap(FVector2D(0.0, 1000.0), Center, 1000.f, Rect), FVector2D(100.0, 500.0)));

        const TArray<FVector2D> Ring = HDWarTableMap::MakeRing(FVector2D::ZeroVector, 100.f, 4);
        if (TestEqual(TEXT("T5 ring points"), Ring.Num(), 4))
        {
            for (const FVector2D& Point : Ring)
            {
                TestTrue(TEXT("T5 ring radius"), FMath::IsNearlyEqual(Point.Size(), 100.0, 0.01));
            }
            TestTrue(TEXT("T5 ring first"), NearlyEqual(Ring[0], FVector2D(100.0, 0.0)));
        }

        TestTrue(TEXT("T5 capture location"), HDWarTableMap::GetCaptureLocation(FVector(10.0, 20.0, 15000.0), 35000.f).Equals(FVector(10.0, 20.0, 30000.0), 0.01));
    }

    return true;
}

#endif
