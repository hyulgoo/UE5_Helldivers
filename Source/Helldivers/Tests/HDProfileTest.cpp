// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Progression/HDProfileTypes.h"
#include "Progression/HDProgressionRules.h"
#include "Progression/HDProfileSaveGame.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDProfileTest, "Helldivers.Progression.Profile", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDProfileTest
{
    FHDSampleCounts MakeSamples(const int32 Common, const int32 Rare, const int32 Super)
    {
        FHDSampleCounts Samples;
        Samples.Common = Common;
        Samples.Rare = Rare;
        Samples.Super = Super;
        return Samples;
    }

    FHDCost MakeCost(const int32 Requisition, const int32 Common, const int32 Rare, const int32 Super)
    {
        FHDCost Cost;
        Cost.Requisition = Requisition;
        Cost.Samples = MakeSamples(Common, Rare, Super);
        return Cost;
    }

    FString DescribeSamples(const FHDSampleCounts& Samples)
    {
        return FString::Printf(TEXT("%d/%d/%d"), Samples.Common, Samples.Rare, Samples.Super);
    }

    FString DescribeNames(const TArray<FName>& Names)
    {
        TArray<FString> Parts;
        for (const FName& Name : Names)
        {
            Parts.Add(Name.ToString());
        }
        return FString::Join(Parts, TEXT(","));
    }

    // P4·P6 의 값
    FHDProfileData MakeRoundTripProfile()
    {
        FHDProfileData Data;
        Data.Requisition = 123;
        Data.Samples = MakeSamples(1, 2, 3);
        Data.UpgradeLevels.Add(TEXT("EagleFastRearm"), 2);
        Data.UnlockedStratagems = { TEXT("OrbitalGatlingBarrage") };
        Data.Loadout = { TEXT("Resupply"), TEXT("EagleAirstrike") };
        Data.Difficulty = 4;
        Data.MissionsPlayed = 3;
        Data.MissionsWon = 2;
        return Data;
    }
}

bool FHDProfileTest::RunTest(const FString& Parameters)
{
    using namespace HDProfileTest;

    // P1 지불: 다 내거나 하나도 안 낸다
    {
        FHDProfileData Data;
        Data.Requisition = 100;
        Data.Samples = MakeSamples(5, 0, 0);

        const FHDCost Exact = MakeCost(100, 5, 0, 0);
        TestTrue(TEXT("P1: exact cost affordable"), HDProgression::CanAfford(Data, Exact));
        TestTrue(TEXT("P1: exact cost spent"), HDProgression::TrySpend(Data, Exact));
        TestEqual(TEXT("P1: requisition 0 after exact spend"), Data.Requisition, 0);
        TestEqual(TEXT("P1: samples 0/0/0 after exact spend"), DescribeSamples(Data.Samples), FString(TEXT("0/0/0")));

        Data.Requisition = 100;
        Data.Samples = MakeSamples(5, 0, 0);

        const FHDCost TooMuchRequisition = MakeCost(101, 0, 0, 0);
        TestFalse(TEXT("P1: requisition 101 not affordable"), HDProgression::CanAfford(Data, TooMuchRequisition));
        TestFalse(TEXT("P1: requisition 101 refused"), HDProgression::TrySpend(Data, TooMuchRequisition));
        TestEqual(TEXT("P1: requisition stays 100 after refused spend"), Data.Requisition, 100);
        TestEqual(TEXT("P1: samples stay 5/0/0 after refused spend"), DescribeSamples(Data.Samples), FString(TEXT("5/0/0")));

        const FHDCost TooManySamples = MakeCost(50, 6, 0, 0);
        TestFalse(TEXT("P1: samples 6 not affordable"), HDProgression::CanAfford(Data, TooManySamples));
        TestFalse(TEXT("P1: samples 6 refused"), HDProgression::TrySpend(Data, TooManySamples));
        TestEqual(TEXT("P1: requisition stays 100 when samples short"), Data.Requisition, 100);
        TestEqual(TEXT("P1: samples stay 5/0/0 when samples short"), DescribeSamples(Data.Samples), FString(TEXT("5/0/0")));
    }

    // P2 더하기와 상한
    {
        FHDProfileData Data;
        Data.Requisition = 0;

        HDProgression::AddResources(Data, 60000, MakeSamples(600, 300, 200));
        TestEqual(TEXT("P2: requisition clamped to 50000"), Data.Requisition, 50000);
        TestEqual(TEXT("P2: samples clamped to 500/250/100"), DescribeSamples(Data.Samples), FString(TEXT("500/250/100")));

        HDProgression::AddResources(Data, -70000, MakeSamples(-1, 0, 0));
        TestEqual(TEXT("P2: requisition clamped to 0"), Data.Requisition, 0);
        TestEqual(TEXT("P2: samples 499/250/100 after -1 common"), DescribeSamples(Data.Samples), FString(TEXT("499/250/100")));
    }

    // P3 정리
    {
        const FName A(TEXT("A"));
        const FName B(TEXT("B"));
        const FName C(TEXT("C"));
        const FName X(TEXT("X"));
        const FName Y(TEXT("Y"));

        FHDProfileData Data;
        Data.Version = 7;
        Data.Requisition = -5;
        Data.Samples = MakeSamples(-1, 3, -2);
        Data.Difficulty = 0;
        Data.UpgradeLevels.Add(A, 2);
        Data.UpgradeLevels.Add(B, 0);
        Data.UpgradeLevels.Add(C, -1);
        Data.UnlockedStratagems = { X, X, Y };
        Data.Loadout = { X, X };

        HDProgression::Sanitize(Data, 5);
        TestEqual(TEXT("P3: requisition 0"), Data.Requisition, 0);
        TestEqual(TEXT("P3: samples 0/3/0"), DescribeSamples(Data.Samples), FString(TEXT("0/3/0")));
        TestEqual(TEXT("P3: difficulty raised to 1"), Data.Difficulty, 1);
        TestEqual(TEXT("P3: one upgrade left"), Data.UpgradeLevels.Num(), 1);
        TestEqual(TEXT("P3: upgrade A stays 2"), Data.UpgradeLevels.FindRef(A), 2);
        TestEqual(TEXT("P3: unlocked deduplicated in order"), DescribeNames(Data.UnlockedStratagems), FString(TEXT("X,Y")));
        TestEqual(TEXT("P3: loadout deduplicated"), DescribeNames(Data.Loadout), FString(TEXT("X")));
        TestEqual(TEXT("P3: version 1"), Data.Version, 1);

        Data.Difficulty = 9;
        HDProgression::Sanitize(Data, 5);
        TestEqual(TEXT("P3: difficulty lowered to 5"), Data.Difficulty, 5);
    }

    // P4 저장 왕복(디스크 대신 메모리)
    {
        const FHDProfileData Expected = MakeRoundTripProfile();

        UHDProfileSaveGame* SaveGame = NewObject<UHDProfileSaveGame>();
        SaveGame->Data = Expected;

        TArray<uint8> Bytes;
        TestTrue(TEXT("P4: SaveGameToMemory"), UGameplayStatics::SaveGameToMemory(SaveGame, Bytes));
        TestTrue(TEXT("P4: bytes written"), Bytes.Num() > 0);

        const UHDProfileSaveGame* Loaded = Cast<UHDProfileSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
        if (TestNotNull(TEXT("P4: loaded as UHDProfileSaveGame"), Loaded))
        {
            const FHDProfileData& Actual = Loaded->Data;
            TestEqual(TEXT("P4: version"), Actual.Version, Expected.Version);
            TestEqual(TEXT("P4: requisition"), Actual.Requisition, Expected.Requisition);
            TestTrue(TEXT("P4: samples"), Actual.Samples == Expected.Samples);
            TestTrue(TEXT("P4: upgrade levels"), Actual.UpgradeLevels.OrderIndependentCompareEqual(Expected.UpgradeLevels));
            TestEqual(TEXT("P4: upgrade EagleFastRearm"), Actual.UpgradeLevels.FindRef(TEXT("EagleFastRearm")), 2);
            TestTrue(TEXT("P4: unlocked"), Actual.UnlockedStratagems == Expected.UnlockedStratagems);
            TestTrue(TEXT("P4: loadout"), Actual.Loadout == Expected.Loadout);
            TestEqual(TEXT("P4: difficulty"), Actual.Difficulty, Expected.Difficulty);
            TestEqual(TEXT("P4: missions played"), Actual.MissionsPlayed, Expected.MissionsPlayed);
            TestEqual(TEXT("P4: missions won"), Actual.MissionsWon, Expected.MissionsWon);
        }
    }

    // P5 빈 버퍼
    {
        const TArray<uint8> Empty;
        TestNull(TEXT("P5: empty buffer loads nothing"), UGameplayStatics::LoadGameFromMemory(Empty));
    }

    // P6 설명 줄
    {
        TestEqual(TEXT("P6: describe"), HDProgression::Describe(MakeRoundTripProfile()),
            FString(TEXT("Requisition=123 Samples=1/2/3 Difficulty=4 Played=3 Won=2 Upgrades=EagleFastRearm:2 Unlocked=OrbitalGatlingBarrage Loadout=Resupply,EagleAirstrike")));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
