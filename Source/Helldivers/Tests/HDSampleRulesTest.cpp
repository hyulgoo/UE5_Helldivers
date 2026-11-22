// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Game/HDMissionPlacement.h"
#include "Progression/HDDifficulty.h"
#include "Progression/HDProgressionSettings.h"
#include "Progression/HDSampleRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDSampleRulesTest, "Helldivers.Progression.Samples", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDSampleRulesTest
{
    // UHDProgressionSettings 의 기본 링과 같은 값
    const FVector2D CommonRing(3000.f, 12000.f);
    const FVector2D RareRing(6000.f, 15000.f);
    const FVector2D SuperRing(10000.f, 18000.f);

    FHDSampleCounts MakeCounts(const int32 Common, const int32 Rare, const int32 Super)
    {
        FHDSampleCounts Counts;
        Counts.Common = Common;
        Counts.Rare = Rare;
        Counts.Super = Super;
        return Counts;
    }

    // 표 밖이면 기본값(표가 틀려도 테스트가 멈추지 않고 실패로 남게)
    FHDDifficultyLevel GetRow(const TArray<FHDDifficultyLevel>& Table, const int32 Index)
    {
        return Table.IsValidIndex(Index) ? Table[Index] : FHDDifficultyLevel();
    }

    bool IsPlacement(const TArray<FHDSamplePlacement>& Plan, const int32 Index, const EHDSampleType Type, const int32 Count, const FVector2D& Ring)
    {
        if (Plan.IsValidIndex(Index) == false)
        {
            return false;
        }

        const FHDSamplePlacement& Placement = Plan[Index];
        return Placement.Type == Type && Placement.Count == Count && Placement.MinRadius == Ring.X && Placement.MaxRadius == Ring.Y;
    }

    bool HasType(const TArray<FHDSamplePlacement>& Plan, const EHDSampleType Type)
    {
        return Plan.ContainsByPredicate([Type](const FHDSamplePlacement& Placement) { return Placement.Type == Type; });
    }

    bool IsValidRing(const FVector2D& Ring)
    {
        return Ring.X >= 0.f && Ring.X < Ring.Y;
    }
}

bool FHDSampleRulesTest::RunTest(const FString& Parameters)
{
    using namespace HDSampleRulesTest;

    // S1 수
    {
        FHDSampleCounts Counts;
        TestTrue(TEXT("S1: default counts are empty"), Counts.IsEmpty());

        Counts.Add(EHDSampleType::Common, 2);
        Counts.Add(EHDSampleType::Rare, 1);
        TestEqual(TEXT("S1: Get(Common) = 2"), Counts.Get(EHDSampleType::Common), 2);
        TestEqual(TEXT("S1: Get(Rare) = 1"), Counts.Get(EHDSampleType::Rare), 1);
        TestEqual(TEXT("S1: Get(Super) = 0"), Counts.Get(EHDSampleType::Super), 0);
        TestEqual(TEXT("S1: Total = 3"), Counts.Total(), 3);
        TestFalse(TEXT("S1: 2/1/0 is not empty"), Counts.IsEmpty());
        TestEqual(TEXT("S1: Get(Count) = 0"), Counts.Get(EHDSampleType::Count), 0);
    }

    // S2 종류 이름
    {
        EHDSampleType Type = EHDSampleType::Count;
        TestTrue(TEXT("S2: \"common\" parses"), HDSamples::TryParseType(TEXT("common"), Type) && Type == EHDSampleType::Common);

        Type = EHDSampleType::Count;
        TestTrue(TEXT("S2: \"Rare\" parses"), HDSamples::TryParseType(TEXT("Rare"), Type) && Type == EHDSampleType::Rare);

        Type = EHDSampleType::Count;
        TestTrue(TEXT("S2: \"SUPER\" parses"), HDSamples::TryParseType(TEXT("SUPER"), Type) && Type == EHDSampleType::Super);

        TestFalse(TEXT("S2: empty text does not parse"), HDSamples::TryParseType(TEXT(""), Type));
        TestFalse(TEXT("S2: \"epic\" does not parse"), HDSamples::TryParseType(TEXT("epic"), Type));

        for (const EHDSampleType Each : { EHDSampleType::Common, EHDSampleType::Rare, EHDSampleType::Super })
        {
            EHDSampleType Parsed = EHDSampleType::Count;
            const bool bParsed = HDSamples::TryParseType(HDSamples::GetTypeName(Each), Parsed);
            TestTrue(*FString::Printf(TEXT("S2: TryParseType(GetTypeName(%d)) round trip"), static_cast<int32>(Each)), bParsed && Parsed == Each);
        }
    }

    // S3 배치 계획
    {
        const TArray<FHDDifficultyLevel>& Table = HDDifficulty::GetDefaultTable();

        const TArray<FHDSamplePlacement> Normal = HDSamples::MakePlacementPlan(GetRow(Table, 1).SampleCounts, CommonRing, RareRing, SuperRing);
        TestEqual(TEXT("S3: row 2 has 2 lines"), Normal.Num(), 2);
        TestTrue(TEXT("S3: row 2 line 1 is common 6 (3000-12000)"), IsPlacement(Normal, 0, EHDSampleType::Common, 6, CommonRing));
        TestTrue(TEXT("S3: row 2 line 2 is rare 2 (6000-15000)"), IsPlacement(Normal, 1, EHDSampleType::Rare, 2, RareRing));
        TestFalse(TEXT("S3: row 2 has no super line"), HasType(Normal, EHDSampleType::Super));

        const TArray<FHDSamplePlacement> Elite = HDSamples::MakePlacementPlan(GetRow(Table, 4).SampleCounts, CommonRing, RareRing, SuperRing);
        TestEqual(TEXT("S3: row 5 has 3 lines"), Elite.Num(), 3);
        TestTrue(TEXT("S3: row 5 line 1 is common 8"), IsPlacement(Elite, 0, EHDSampleType::Common, 8, CommonRing));
        TestTrue(TEXT("S3: row 5 line 2 is rare 5"), IsPlacement(Elite, 1, EHDSampleType::Rare, 5, RareRing));
        TestTrue(TEXT("S3: row 5 line 3 is super 2 (10000-18000)"), IsPlacement(Elite, 2, EHDSampleType::Super, 2, SuperRing));

        const TArray<FHDSamplePlacement> Easy = HDSamples::MakePlacementPlan(GetRow(Table, 0).SampleCounts, CommonRing, RareRing, SuperRing);
        TestEqual(TEXT("S3: row 1 has 1 line"), Easy.Num(), 1);
        TestTrue(TEXT("S3: row 1 line 1 is common 4"), IsPlacement(Easy, 0, EHDSampleType::Common, 4, CommonRing));
    }

    // S4 모양
    {
        const FString CommonPath = HDSamples::GetMeshPath(EHDSampleType::Common);
        const FString RarePath = HDSamples::GetMeshPath(EHDSampleType::Rare);
        const FString SuperPath = HDSamples::GetMeshPath(EHDSampleType::Super);

        TestTrue(TEXT("S4: common and rare shapes differ"), CommonPath != RarePath);
        TestTrue(TEXT("S4: common and super shapes differ"), CommonPath != SuperPath);
        TestTrue(TEXT("S4: rare and super shapes differ"), RarePath != SuperPath);

        for (const FString& Path : { CommonPath, RarePath, SuperPath })
        {
            TestTrue(*FString::Printf(TEXT("S4: %s is an engine basic shape"), *Path), Path.StartsWith(TEXT("/Engine/BasicShapes/")));
        }
    }

    // S5 링
    {
        FRandomStream Stream(20261122);
        const TArray<FVector2D> Offsets = HDMissionPlacement::ComputeRingOffsets(6, 3000.f, 12000.f, 1500.f, Stream);
        TestEqual(TEXT("S5: 6 offsets"), Offsets.Num(), 6);

        for (int32 Index = 0; Index < Offsets.Num(); ++Index)
        {
            const double Length = Offsets[Index].Size();
            TestTrue(*FString::Printf(TEXT("S5: offset %d length %.0f within [3000, 12000]"), Index, Length), Length >= 3000.0 && Length <= 12000.0);
        }
    }

    // S6 글자
    {
        TestEqual(TEXT("S6: FormatCarried(2/1/0)"), HDSamples::FormatCarried(MakeCounts(2, 1, 0)), FString(TEXT("SAMPLES  C 2  R 1  S 0")));
        TestEqual(TEXT("S6: Describe(2/1/0)"), HDSamples::Describe(MakeCounts(2, 1, 0)), FString(TEXT("2/1/0")));
    }

    // S7 설정
    {
        const UHDProgressionSettings* Settings = GetDefault<UHDProgressionSettings>();
        if (TestNotNull(TEXT("S7: progression settings"), Settings))
        {
            TestTrue(TEXT("S7: common ring 0 <= min < max"), IsValidRing(Settings->CommonSampleRing));
            TestTrue(TEXT("S7: rare ring 0 <= min < max"), IsValidRing(Settings->RareSampleRing));
            TestTrue(TEXT("S7: super ring 0 <= min < max"), IsValidRing(Settings->SuperSampleRing));
            TestTrue(TEXT("S7: pickup radius > 0"), Settings->SamplePickupRadius > 0.f);
        }
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
