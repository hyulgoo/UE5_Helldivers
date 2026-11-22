// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Game/HDMissionTypes.h"
#include "Progression/HDDifficulty.h"
#include "Progression/HDProgressionSettings.h"
#include "UI/HDShipMenu.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDDifficultyTest, "Helldivers.Progression.Difficulty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDDifficultyTest
{
    // 표 밖이면 기본값(표가 틀려도 테스트가 멈추지 않고 실패로 남게)
    FHDDifficultyLevel GetRow(const TArray<FHDDifficultyLevel>& Table, const int32 Index)
    {
        return Table.IsValidIndex(Index) ? Table[Index] : FHDDifficultyLevel();
    }

    bool IsSameLevel(const FHDDifficultyLevel& A, const FHDDifficultyLevel& B)
    {
        return A.Label == B.Label
            && A.RequiredObjectives == B.RequiredObjectives
            && A.SpawnIntensity == B.SpawnIntensity
            && A.HeavyRatio == B.HeavyRatio
            && A.RewardMultiplier == B.RewardMultiplier
            && A.SampleCounts == B.SampleCounts;
    }

    bool IsSameTable(const TArray<FHDDifficultyLevel>& A, const TArray<FHDDifficultyLevel>& B)
    {
        if (A.Num() != B.Num())
        {
            return false;
        }

        for (int32 Index = 0; Index < A.Num(); ++Index)
        {
            if (IsSameLevel(A[Index], B[Index]) == false)
            {
                return false;
            }
        }
        return true;
    }

    bool IsAction(const FHDShipMenuAction& Action, EHDShipActionType Type, const TCHAR* Id, int32 Value)
    {
        return Action.Type == Type && Action.Id == FName(Id) && Action.Value == Value;
    }
}

bool FHDDifficultyTest::RunTest(const FString& Parameters)
{
    using namespace HDDifficultyTest;

    const TArray<FHDDifficultyLevel>& Table = HDDifficulty::GetDefaultTable();

    // D1 기본 표
    {
        TestEqual(TEXT("D1: default table has 5 rows"), Table.Num(), 5);

        const TArray<FString> ExpectedLabels = { TEXT("EASY"), TEXT("NORMAL"), TEXT("HARD"), TEXT("VETERAN"), TEXT("ELITE") };
        for (int32 Index = 0; Index < ExpectedLabels.Num(); ++Index)
        {
            TestEqual(*FString::Printf(TEXT("D1: row %d label"), Index + 1), GetRow(Table, Index).Label, ExpectedLabels[Index]);
        }

        // 2행(NORMAL) = 지금 게임 수치
        const FHDDifficultyLevel Normal = GetRow(Table, 1);
        TestEqual(TEXT("D1: NORMAL objectives equal FHDMissionConfig default"), Normal.RequiredObjectives, FHDMissionConfig().RequiredObjectives);
        TestEqual(TEXT("D1: NORMAL objectives 2"), Normal.RequiredObjectives, 2);
        TestEqual(TEXT("D1: NORMAL intensity 1.0"), Normal.SpawnIntensity, 1.f);
        TestEqual(TEXT("D1: NORMAL heavy 0.1"), Normal.HeavyRatio, 0.1f);
    }

    // D2 오름차순
    {
        TestEqual(TEXT("D2: default table has 5 rows"), Table.Num(), 5);

        for (int32 Index = 1; Index < Table.Num(); ++Index)
        {
            const FHDDifficultyLevel& Prev = Table[Index - 1];
            const FHDDifficultyLevel& Next = Table[Index];
            const FString Rows = FString::Printf(TEXT("D2: row %d -> %d"), Index, Index + 1);

            TestTrue(*(Rows + TEXT(" objectives not lower")), Next.RequiredObjectives >= Prev.RequiredObjectives);
            TestTrue(*(Rows + TEXT(" intensity not lower")), Next.SpawnIntensity >= Prev.SpawnIntensity);
            TestTrue(*(Rows + TEXT(" heavy not lower")), Next.HeavyRatio >= Prev.HeavyRatio);
            TestTrue(*(Rows + TEXT(" reward not lower")), Next.RewardMultiplier >= Prev.RewardMultiplier);
            TestTrue(*(Rows + TEXT(" common samples not lower")), Next.SampleCounts.Common >= Prev.SampleCounts.Common);
            TestTrue(*(Rows + TEXT(" rare samples not lower")), Next.SampleCounts.Rare >= Prev.SampleCounts.Rare);
            TestTrue(*(Rows + TEXT(" super samples not lower")), Next.SampleCounts.Super >= Prev.SampleCounts.Super);
        }

        TestEqual(TEXT("D2: row 1 reward 1.0"), GetRow(Table, 0).RewardMultiplier, 1.f);
    }

    // D3 자르기
    {
        TestEqual(TEXT("D3: ClampLevel(0, 5) = 1"), HDDifficulty::ClampLevel(0, 5), 1);
        TestEqual(TEXT("D3: ClampLevel(6, 5) = 5"), HDDifficulty::ClampLevel(6, 5), 5);
        TestEqual(TEXT("D3: ClampLevel(3, 5) = 3"), HDDifficulty::ClampLevel(3, 5), 3);
        TestEqual(TEXT("D3: ClampLevel(3, 0) = 1"), HDDifficulty::ClampLevel(3, 0), 1);

        TestEqual(TEXT("D3: StepLevel(1, -1, 5) = 1"), HDDifficulty::StepLevel(1, -1, 5), 1);
        TestEqual(TEXT("D3: StepLevel(5, +1, 5) = 5 (no wrap)"), HDDifficulty::StepLevel(5, 1, 5), 5);
        TestEqual(TEXT("D3: StepLevel(2, +1, 5) = 3"), HDDifficulty::StepLevel(2, 1, 5), 3);
        TestEqual(TEXT("D3: StepLevel(4, -1, 5) = 3"), HDDifficulty::StepLevel(4, -1, 5), 3);
    }

    // D4 미션 설정
    {
        const FHDMissionConfig Base;
        TestTrue(TEXT("D4: base config is 2/5/45/0/call"),
            Base.RequiredObjectives == 2 && Base.MaxReinforcements == 5 && Base.ExtractionHoldTime == 45.f && Base.MissionTimeLimit == 0.f && Base.bExtractionRequiresCall);

        const FHDMissionConfig Veteran = HDDifficulty::ApplyToConfig(Base, GetRow(Table, 3));
        TestEqual(TEXT("D4: VETERAN objectives 3"), Veteran.RequiredObjectives, 3);
        TestEqual(TEXT("D4: VETERAN keeps reinforcements"), Veteran.MaxReinforcements, Base.MaxReinforcements);
        TestEqual(TEXT("D4: VETERAN keeps extraction hold time"), Veteran.ExtractionHoldTime, Base.ExtractionHoldTime);
        TestEqual(TEXT("D4: VETERAN keeps time limit"), Veteran.MissionTimeLimit, Base.MissionTimeLimit);
        TestTrue(TEXT("D4: VETERAN keeps extraction call"), Veteran.bExtractionRequiresCall == Base.bExtractionRequiresCall);

        FHDDifficultyLevel ZeroObjectives = GetRow(Table, 3);
        ZeroObjectives.RequiredObjectives = 0;
        TestEqual(TEXT("D4: row with 0 objectives keeps base objectives"), HDDifficulty::ApplyToConfig(Base, ZeroObjectives).RequiredObjectives, Base.RequiredObjectives);
    }

    // D5 추출 강도
    {
        TestEqual(TEXT("D5: extraction intensity NORMAL = 2.0"), HDDifficulty::GetExtractionIntensity(2.f, GetRow(Table, 1)), 2.f);
        TestEqual(TEXT("D5: extraction intensity ELITE = 4.0"), HDDifficulty::GetExtractionIntensity(2.f, GetRow(Table, 4)), 4.f);
    }

    // D6 행 찾기
    {
        TestEqual(TEXT("D6: level 9 is ELITE"), HDDifficulty::GetLevel(Table, 9).Label, FString(TEXT("ELITE")));
        TestTrue(TEXT("D6: level 9 is row 5"), IsSameLevel(HDDifficulty::GetLevel(Table, 9), GetRow(Table, 4)));
        TestEqual(TEXT("D6: level 0 is EASY"), HDDifficulty::GetLevel(Table, 0).Label, FString(TEXT("EASY")));
        TestTrue(TEXT("D6: level 0 is row 1"), IsSameLevel(HDDifficulty::GetLevel(Table, 0), GetRow(Table, 0)));
        TestTrue(TEXT("D6: empty table gives default row"), IsSameLevel(HDDifficulty::GetLevel(TArray<FHDDifficultyLevel>(), 2), FHDDifficultyLevel()));
    }

    // D7 메뉴 줄
    {
        FHDShipMenuContext Context;
        Context.Profile.Difficulty = 3;
        Context.Difficulties = Table;

        const TArray<FHDShipMenuItem> Items = HDShipMenu::BuildItems(EHDShipScreen::Bridge, Context);
        TestTrue(TEXT("D7: Bridge has Deploy, Difficulty ... Quit"), Items.Num() >= 3);

        if (Items.Num() >= 2)
        {
            TestEqual(TEXT("D7: first id is Deploy"), Items[0].Id, FName(TEXT("Deploy")));
            TestEqual(TEXT("D7: last id is Quit"), Items.Last().Id, FName(TEXT("Quit")));

            const FHDShipMenuItem& DifficultyItem = Items[1];
            TestEqual(TEXT("D7: row 1 id is Difficulty"), DifficultyItem.Id, FName(TEXT("Difficulty")));
            TestTrue(TEXT("D7: row 1 is adjustable"), DifficultyItem.bAdjustable);
            TestEqual(TEXT("D7: row 1 label"), DifficultyItem.Label, FString(TEXT("DIFFICULTY  < 3  HARD >")));

            FHDShipMenu Menu(EHDShipScreen::Bridge);
            Menu.SetCursor(1, Items.Num());
            TestTrue(TEXT("D7: Right on Difficulty is Adjust +1"), IsAction(Menu.HandleKey(EHDShipMenuKey::Right, Items), EHDShipActionType::Adjust, TEXT("Difficulty"), 1));
            TestTrue(TEXT("D7: Left on Difficulty is Adjust -1"), IsAction(Menu.HandleKey(EHDShipMenuKey::Left, Items), EHDShipActionType::Adjust, TEXT("Difficulty"), -1));
            TestTrue(TEXT("D7: Confirm on Difficulty is None"), Menu.HandleKey(EHDShipMenuKey::Confirm, Items).Type == EHDShipActionType::None);
        }

        FHDShipMenuContext NoTableContext;
        NoTableContext.Profile.Difficulty = 3;
        const bool bHasDifficultyRow = HDShipMenu::BuildItems(EHDShipScreen::Bridge, NoTableContext).ContainsByPredicate([](const FHDShipMenuItem& Item)
        {
            return Item.Id == FName(TEXT("Difficulty"));
        });
        TestFalse(TEXT("D7: no Difficulty row without a table"), bHasDifficultyRow);
    }

    // D8 설정
    {
        const UHDProgressionSettings* Settings = GetDefault<UHDProgressionSettings>();
        if (TestNotNull(TEXT("D8: progression settings"), Settings))
        {
            TestEqual(TEXT("D8: settings table has 5 rows"), Settings->DifficultyLevels.Num(), 5);
            TestTrue(TEXT("D8: settings table equals default table"), IsSameTable(Settings->DifficultyLevels, Table));
        }
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
