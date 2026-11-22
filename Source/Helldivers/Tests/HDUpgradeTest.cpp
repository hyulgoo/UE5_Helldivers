// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Progression/HDUpgradeCatalog.h"
#include "Progression/HDUpgradeRules.h"
#include "GameData/HDStratagemDefinition.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "UI/HDShipMenu.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDUpgradeTest, "Helldivers.Progression.Upgrades", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDUpgradeTest
{
    // A: 이글 재무장 배율 2단계 / B: 증원 + 1단계. 1단계의 희귀·최상급은 적지 않는다(0)
    const TCHAR* const TestCatalogJson = TEXT(R"([
        { "Name": "A", "DisplayName": "ALPHA", "Description": "Alpha.", "Effect": "EagleRearmScale",
          "Levels": [ { "Requisition": 100, "Common": 5, "Value": 0.8 }, { "Requisition": 200, "Common": 10, "Rare": 3, "Super": 1, "Value": 0.6 } ] },
        { "Name": "B", "DisplayName": "BRAVO", "Description": "Bravo.", "Effect": "ExtraReinforcements",
          "Levels": [ { "Requisition": 150, "Common": 8, "Value": 1 } ] }
    ])");

    bool HasProblemContaining(const FHDUpgradeCatalog& Catalog, const TCHAR* Text)
    {
        return Catalog.GetProblems().ContainsByPredicate([Text](const FString& Problem) { return Problem.Contains(Text); });
    }

    bool IsSameLevel(const FHDUpgradeLevel& Level, int32 Requisition, int32 Common, int32 Rare, int32 Super, float Value)
    {
        return Level.Requisition == Requisition
            && Level.Common == Common
            && Level.Rare == Rare
            && Level.Super == Super
            && FMath::IsNearlyEqual(Level.Value, Value);
    }

    bool IsSameProfile(const FHDProfileData& A, const FHDProfileData& B)
    {
        return A.Requisition == B.Requisition
            && A.Samples == B.Samples
            && A.UpgradeLevels.OrderIndependentCompareEqual(B.UpgradeLevels);
    }

    // 배율 효과(값을 곱한다)인가. 아니면 더하기 효과
    bool IsScaleEffect(EHDUpgradeEffect Effect)
    {
        return Effect == EHDUpgradeEffect::EagleRearmScale
            || Effect == EHDUpgradeEffect::OrbitalCooldownScale
            || Effect == EHDUpgradeEffect::SupportCooldownScale;
    }

    FHDStratagemDefinition MakeDefinition(EHDStratagemKind Kind, bool bEagle, int32 MaxUses, float RearmTime, float ActivationDelay, float Cooldown)
    {
        FHDStratagemDefinition Definition;
        Definition.Kind             = Kind;
        Definition.bIsEagle         = bEagle ? 1 : 0;
        Definition.MaxUses          = MaxUses;
        Definition.RearmTime        = RearmTime;
        Definition.ActivationDelay  = ActivationDelay;
        Definition.Cooldown         = Cooldown;
        return Definition;
    }

    // UHDStratagemComponent::BuildCatalog 처럼 정의를 칸 값으로 옮긴다
    FHDStratagemEntry MakeEntry(const FHDStratagemDefinition& Definition)
    {
        FHDStratagemEntry Entry;
        Entry.ActivationDelay   = Definition.ActivationDelay;
        Entry.Cooldown          = Definition.Cooldown;
        Entry.MaxUses           = Definition.MaxUses;
        Entry.RearmTime         = Definition.RearmTime;
        return Entry;
    }

    FHDStratagemEntry ApplyTo(const FHDStratagemDefinition& Definition, const FHDUpgradeModifiers& Modifiers)
    {
        FHDStratagemEntry Entry = MakeEntry(Definition);
        HDUpgrade::ApplyToEntry(Entry, Definition, Modifiers);
        return Entry;
    }

    bool IsDefaultModifiers(const FHDUpgradeModifiers& Modifiers, bool bSkipEagleRearm, bool bSkipReinforcements)
    {
        const FHDUpgradeModifiers Defaults;
        return Modifiers.EagleExtraUses == Defaults.EagleExtraUses
            && (bSkipEagleRearm || FMath::IsNearlyEqual(Modifiers.EagleRearmScale, Defaults.EagleRearmScale))
            && FMath::IsNearlyEqual(Modifiers.OrbitalCooldownScale, Defaults.OrbitalCooldownScale)
            && FMath::IsNearlyEqual(Modifiers.SupportCooldownScale, Defaults.SupportCooldownScale)
            && FMath::IsNearlyEqual(Modifiers.ActivationDelayReduction, Defaults.ActivationDelayReduction)
            && Modifiers.ExtraStimsAndGrenades == Defaults.ExtraStimsAndGrenades
            && (bSkipReinforcements || Modifiers.ExtraReinforcements == Defaults.ExtraReinforcements);
    }
}

bool FHDUpgradeTest::RunTest(const FString& Parameters)
{
    using namespace HDUpgradeTest;

    const FHDUpgradeCatalog Catalog = FHDUpgradeCatalog::FromJsonString(TestCatalogJson);
    const FName NameA(TEXT("A"));
    const FName NameB(TEXT("B"));

    // U1 읽기
    {
        for (const FString& Problem : Catalog.GetProblems()) { AddInfo(FString::Printf(TEXT("problem: %s"), *Problem)); }
        TestEqual(TEXT("U1: no problems"), Catalog.GetProblems().Num(), 0);
        TestEqual(TEXT("U1: 2 rows"), Catalog.Num(), 2);
        TestTrue(TEXT("U1: file order kept"), Catalog.GetNames() == TArray<FName>({ NameA, NameB }));

        const FHDUpgradeDefinition* A = Catalog.Find(NameA);
        const FHDUpgradeDefinition* B = Catalog.Find(NameB);
        if (TestNotNull(TEXT("U1: A found"), A) && TestNotNull(TEXT("U1: B found"), B))
        {
            TestEqual(TEXT("U1: A display name"), A->DisplayName, FString(TEXT("ALPHA")));
            TestTrue(TEXT("U1: A effect EagleRearmScale"), A->Effect == EHDUpgradeEffect::EagleRearmScale);
            TestEqual(TEXT("U1: A has 2 levels"), A->Levels.Num(), 2);
            if (A->Levels.Num() == 2)
            {
                TestTrue(TEXT("U1: A level 1 is 100/5/0/0 -> 0.8"), IsSameLevel(A->Levels[0], 100, 5, 0, 0, 0.8f));
                TestTrue(TEXT("U1: A level 2 is 200/10/3/1 -> 0.6"), IsSameLevel(A->Levels[1], 200, 10, 3, 1, 0.6f));
            }

            TestTrue(TEXT("U1: B effect ExtraReinforcements"), B->Effect == EHDUpgradeEffect::ExtraReinforcements);
            TestEqual(TEXT("U1: B has 1 level"), B->Levels.Num(), 1);
            if (B->Levels.Num() == 1)
            {
                TestTrue(TEXT("U1: B level 1 is 150/8/0/0 -> 1"), IsSameLevel(B->Levels[0], 150, 8, 0, 0, 1.f));
            }
        }
        TestNull(TEXT("U1: missing is null"), Catalog.Find(TEXT("Missing")));

        const FHDUpgradeCatalog Typo = FHDUpgradeCatalog::FromJsonString(TEXT(R"([ { "Name": "A", "Effect": "EagleRearmScale", "Valu": 1, "Levels": [ { "Requisition": 1, "Value": 0.5 } ] } ])"));
        for (const FString& Problem : Typo.GetProblems()) { AddInfo(FString::Printf(TEXT("typo problem: %s"), *Problem)); }
        TestTrue(TEXT("U1: unknown column reported"), HasProblemContaining(Typo, TEXT("Valu")));
    }

    // U2 프로젝트 파일
    {
        const FHDUpgradeCatalog& Project = FHDUpgradeCatalog::GetProjectCatalog();
        for (const FString& Problem : Project.GetProblems()) { AddInfo(FString::Printf(TEXT("project problem: %s"), *Problem)); }
        TestTrue(TEXT("U2: file exists"), FPaths::FileExists(FHDUpgradeCatalog::GetProjectFilePath()));
        TestEqual(TEXT("U2: no problems"), Project.GetProblems().Num(), 0);
        TestEqual(TEXT("U2: 7 rows"), Project.Num(), 7);
        TestEqual(TEXT("U2: names are distinct"), TSet<FName>(Project.GetNames()).Num(), Project.Num());

        for (const FName& UpgradeName : Project.GetNames())
        {
            const FHDUpgradeDefinition* Definition = Project.Find(UpgradeName);
            if (TestNotNull(*FString::Printf(TEXT("U2: %s found"), *UpgradeName.ToString()), Definition) == false)
            {
                continue;
            }

            const TArray<FHDUpgradeLevel>& Levels = Definition->Levels;
            TestTrue(*FString::Printf(TEXT("U2: %s has a level"), *UpgradeName.ToString()), Levels.Num() >= 1);

            bool bNoNegativePrice = true;
            bool bEveryLevelCosts = true;
            bool bValuesInRange = true;
            bool bValuesOrdered = true;
            for (int32 Index = 0; Index < Levels.Num(); ++Index)
            {
                const FHDUpgradeLevel& Level = Levels[Index];
                bNoNegativePrice &= Level.Requisition >= 0 && Level.Common >= 0 && Level.Rare >= 0 && Level.Super >= 0;
                bEveryLevelCosts &= (Level.Requisition + Level.Common + Level.Rare + Level.Super) > 0;

                if (IsScaleEffect(Definition->Effect))
                {
                    // 배율은 0 < 값 ≤ 1, 단계가 오를수록 커지지 않는다
                    bValuesInRange &= Level.Value > 0.f && Level.Value <= 1.f;
                    bValuesOrdered &= Index == 0 || Level.Value <= Levels[Index - 1].Value;
                }
                else
                {
                    // 더하기는 값 > 0, 단계가 오를수록 작아지지 않는다
                    bValuesInRange &= Level.Value > 0.f;
                    bValuesOrdered &= Index == 0 || Level.Value >= Levels[Index - 1].Value;
                }
            }

            TestTrue(*FString::Printf(TEXT("U2: %s has no negative price"), *UpgradeName.ToString()), bNoNegativePrice);
            TestTrue(*FString::Printf(TEXT("U2: %s every level costs something"), *UpgradeName.ToString()), bEveryLevelCosts);
            TestTrue(*FString::Printf(TEXT("U2: %s values in range"), *UpgradeName.ToString()), bValuesInRange);
            TestTrue(*FString::Printf(TEXT("U2: %s values ordered by level"), *UpgradeName.ToString()), bValuesOrdered);
        }
    }

    // U3 구매
    {
        FHDProfileData Poor;
        Poor.Requisition = 50;
        const FHDProfileData PoorBefore = Poor;
        TestTrue(TEXT("U3: 50 requisition cannot afford A"), HDUpgrade::TryBuy(Poor, NameA, Catalog) == EHDUpgradeBuyResult::CannotAfford);
        TestTrue(TEXT("U3: cannot afford leaves profile"), IsSameProfile(Poor, PoorBefore));

        FHDProfileData Rich;
        Rich.Requisition = 1000;
        Rich.Samples.Common = 50;
        Rich.Samples.Rare = 10;
        Rich.Samples.Super = 5;

        TestTrue(TEXT("U3: first buy is Bought"), HDUpgrade::TryBuy(Rich, NameA, Catalog) == EHDUpgradeBuyResult::Bought);
        TestEqual(TEXT("U3: level 1 after first buy"), HDUpgrade::GetLevel(Rich, NameA), 1);
        TestEqual(TEXT("U3: paid level 1 requisition"), Rich.Requisition, 900);
        TestTrue(TEXT("U3: paid level 1 samples 5/0/0"), Rich.Samples.Common == 45 && Rich.Samples.Rare == 10 && Rich.Samples.Super == 5);

        TestTrue(TEXT("U3: second buy is Bought"), HDUpgrade::TryBuy(Rich, NameA, Catalog) == EHDUpgradeBuyResult::Bought);
        TestEqual(TEXT("U3: level 2 after second buy"), HDUpgrade::GetLevel(Rich, NameA), 2);
        TestEqual(TEXT("U3: paid level 2 requisition"), Rich.Requisition, 700);
        TestTrue(TEXT("U3: paid level 2 samples 10/3/1"), Rich.Samples.Common == 35 && Rich.Samples.Rare == 7 && Rich.Samples.Super == 4);

        const FHDProfileData MaxBefore = Rich;
        TestTrue(TEXT("U3: third buy is MaxLevel"), HDUpgrade::TryBuy(Rich, NameA, Catalog) == EHDUpgradeBuyResult::MaxLevel);
        TestTrue(TEXT("U3: max level leaves profile"), IsSameProfile(Rich, MaxBefore));

        const FHDProfileData UnknownBefore = Rich;
        TestTrue(TEXT("U3: missing name is Unknown"), HDUpgrade::TryBuy(Rich, TEXT("Missing"), Catalog) == EHDUpgradeBuyResult::Unknown);
        TestTrue(TEXT("U3: unknown leaves profile"), IsSameProfile(Rich, UnknownBefore));
    }

    // U4 효과 합
    {
        FHDProfileData Data;
        Data.UpgradeLevels.Add(NameA, 2);
        Data.UpgradeLevels.Add(NameB, 1);
        Data.UpgradeLevels.Add(TEXT("Missing"), 1);
        const FHDUpgradeModifiers Modifiers = HDUpgrade::ComputeModifiers(Data, Catalog);
        AddInfo(FString::Printf(TEXT("U4 modifiers: %s"), *HDUpgrade::DescribeModifiers(Modifiers)));
        TestEqual(TEXT("U4: eagle rearm is A level 2 value"), Modifiers.EagleRearmScale, 0.6f);
        TestEqual(TEXT("U4: reinforcements + B value"), Modifiers.ExtraReinforcements, 1);
        TestTrue(TEXT("U4: others default"), IsDefaultModifiers(Modifiers, true, true));

        FHDProfileData Over;
        Over.UpgradeLevels.Add(NameA, 9);
        const FHDUpgradeModifiers OverModifiers = HDUpgrade::ComputeModifiers(Over, Catalog);
        TestEqual(TEXT("U4: level 9 clamps to last level"), OverModifiers.EagleRearmScale, 0.6f);
        TestTrue(TEXT("U4: level 9 others default"), IsDefaultModifiers(OverModifiers, true, false));
    }

    // U5 칸에 적용
    {
        const FHDStratagemDefinition Eagle = MakeDefinition(EHDStratagemKind::Projectile, true, 2, 120.f, 3.f, 10.f);
        FHDUpgradeModifiers EagleModifiers;
        EagleModifiers.EagleExtraUses = 1;
        EagleModifiers.EagleRearmScale = 0.6f;
        EagleModifiers.ActivationDelayReduction = 1.f;
        const FHDStratagemEntry EagleEntry = ApplyTo(Eagle, EagleModifiers);
        TestEqual(TEXT("U5: eagle uses 3"), EagleEntry.MaxUses, 3);
        TestEqual(TEXT("U5: eagle rearm 72"), EagleEntry.RearmTime, 72.f);
        TestEqual(TEXT("U5: eagle delay 2"), EagleEntry.ActivationDelay, 2.f);
        TestEqual(TEXT("U5: eagle cooldown 10"), EagleEntry.Cooldown, 10.f);

        const FHDStratagemDefinition Orbital = MakeDefinition(EHDStratagemKind::Projectile, false, 0, 0.f, 2.f, 30.f);
        FHDUpgradeModifiers OrbitalModifiers;
        OrbitalModifiers.OrbitalCooldownScale = 0.8f;
        TestEqual(TEXT("U5: orbital cooldown 24"), ApplyTo(Orbital, OrbitalModifiers).Cooldown, 24.f);

        const FHDStratagemDefinition Resupply = MakeDefinition(EHDStratagemKind::Resupply, false, 0, 0.f, 2.f, 60.f);
        FHDUpgradeModifiers SupportModifiers;
        SupportModifiers.SupportCooldownScale = 0.9f;
        SupportModifiers.OrbitalCooldownScale = 0.5f;
        TestEqual(TEXT("U5: resupply cooldown 54 (orbital scale not applied)"), ApplyTo(Resupply, SupportModifiers).Cooldown, 54.f);

        FHDUpgradeModifiers DelayModifiers;
        DelayModifiers.ActivationDelayReduction = 1.f;
        const FHDStratagemDefinition OneSecond = MakeDefinition(EHDStratagemKind::Projectile, false, 0, 0.f, 1.f, 30.f);
        TestEqual(TEXT("U5: delay 1.0 - 1.0 floors at 0.5"), ApplyTo(OneSecond, DelayModifiers).ActivationDelay, 0.5f);
        const FHDStratagemDefinition Short = MakeDefinition(EHDStratagemKind::Projectile, false, 0, 0.f, 0.3f, 30.f);
        TestEqual(TEXT("U5: delay 0.3 - 1.0 stays 0.3"), ApplyTo(Short, DelayModifiers).ActivationDelay, 0.3f);
    }

    // U6 업그레이드 없음
    {
        const FHDUpgradeModifiers NoUpgrades;
        const FHDStratagemDefinition Definitions[] =
        {
            MakeDefinition(EHDStratagemKind::Projectile, true, 2, 120.f, 3.f, 10.f),
            MakeDefinition(EHDStratagemKind::Projectile, false, 0, 0.f, 2.f, 30.f),
            MakeDefinition(EHDStratagemKind::Resupply, false, 0, 0.f, 2.f, 60.f),
            MakeDefinition(EHDStratagemKind::Sentry, false, 0, 0.f, 3.f, 90.f),
            MakeDefinition(EHDStratagemKind::SupportWeapon, false, 0, 0.f, 3.f, 120.f),
        };

        bool bAllSame = true;
        for (const FHDStratagemDefinition& Definition : Definitions)
        {
            const FHDStratagemEntry Entry = ApplyTo(Definition, NoUpgrades);
            bAllSame &= Entry.MaxUses == Definition.MaxUses
                && FMath::IsNearlyEqual(Entry.RearmTime, Definition.RearmTime)
                && FMath::IsNearlyEqual(Entry.ActivationDelay, Definition.ActivationDelay)
                && FMath::IsNearlyEqual(Entry.Cooldown, Definition.Cooldown);
        }
        TestTrue(TEXT("U6: default modifiers keep definition values"), bAllSame);
    }

    // U7 메뉴
    {
        const FName UpgradesId(TEXT("Upgrades"));

        FHDShipMenuContext Context;
        Context.Upgrades = &Catalog;

        const TArray<FHDShipMenuItem> BridgeItems = HDShipMenu::BuildItems(EHDShipScreen::Bridge, Context);
        const int32 UpgradesIndex = BridgeItems.IndexOfByPredicate([&UpgradesId](const FHDShipMenuItem& Item) { return Item.Id == UpgradesId; });
        TestTrue(TEXT("U7: Bridge has Upgrades row"), BridgeItems.IsValidIndex(UpgradesIndex));
        if (BridgeItems.IsValidIndex(UpgradesIndex))
        {
            const FHDShipMenuItem& UpgradesItem = BridgeItems[UpgradesIndex];
            TestTrue(TEXT("U7: Upgrades row opens screen"), UpgradesItem.ConfirmAction == EHDShipActionType::OpenScreen);
            TestTrue(TEXT("U7: Upgrades row targets Upgrades"), UpgradesItem.TargetScreen == EHDShipScreen::Upgrades);

            FHDShipMenu Menu(EHDShipScreen::Bridge);
            Menu.SetCursor(UpgradesIndex, BridgeItems.Num());
            const FHDShipMenuAction OpenAction = Menu.HandleKey(EHDShipMenuKey::Confirm, BridgeItems);
            TestTrue(TEXT("U7: Confirm opens Upgrades"), OpenAction.Type == EHDShipActionType::OpenScreen && OpenAction.Id == UpgradesId);
            TestTrue(TEXT("U7: screen is Upgrades"), Menu.GetScreen() == EHDShipScreen::Upgrades);
            TestEqual(TEXT("U7: cursor 0 on Upgrades"), Menu.GetCursor(), 0);

            const FHDShipMenuAction BackAction = Menu.HandleKey(EHDShipMenuKey::Back, HDShipMenu::BuildItems(EHDShipScreen::Upgrades, Context));
            TestTrue(TEXT("U7: Back opens Bridge"), BackAction.Type == EHDShipActionType::OpenScreen);
            TestTrue(TEXT("U7: screen is Bridge after Back"), Menu.GetScreen() == EHDShipScreen::Bridge);
            TestEqual(TEXT("U7: cursor back on Upgrades row"), Menu.GetCursor(), UpgradesIndex);
        }

        const TArray<FHDShipMenuItem> PoorItems = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, Context);
        TestEqual(TEXT("U7: one row per catalog row"), PoorItems.Num(), Catalog.Num());
        bool bAllDisabled = true;
        for (const FHDShipMenuItem& Item : PoorItems)
        {
            bAllDisabled &= Item.bEnabled == false;
        }
        TestTrue(TEXT("U7: 0 requisition disables all rows"), bAllDisabled);
        if (PoorItems.Num() == 2)
        {
            TestEqual(TEXT("U7: row 0 id is A"), PoorItems[0].Id, NameA);
            TestTrue(TEXT("U7: row 0 label has 0/2"), PoorItems[0].Label.Contains(TEXT("0/2")));
            TestTrue(TEXT("U7: row 0 confirms Select"), PoorItems[0].ConfirmAction == EHDShipActionType::Select);
            TestTrue(TEXT("U7: row 0 detail starts with NEXT"), PoorItems[0].Detail.StartsWith(TEXT("NEXT")));
        }

        FHDShipMenuContext BoughtContext = Context;
        BoughtContext.Profile.Requisition = 1000;
        BoughtContext.Profile.Samples.Common = 50;
        BoughtContext.Profile.UpgradeLevels.Add(NameB, 1);
        const TArray<FHDShipMenuItem> BoughtItems = HDShipMenu::BuildItems(EHDShipScreen::Upgrades, BoughtContext);
        if (TestEqual(TEXT("U7: bought rows count"), BoughtItems.Num(), 2))
        {
            TestTrue(TEXT("U7: affordable row enabled"), BoughtItems[0].bEnabled);
            TestTrue(TEXT("U7: max level row disabled"), BoughtItems[1].bEnabled == false);
            TestTrue(TEXT("U7: max level row label has 1/1"), BoughtItems[1].Label.Contains(TEXT("1/1")));
            TestTrue(TEXT("U7: max level detail starts with MAX LEVEL"), BoughtItems[1].Detail.StartsWith(TEXT("MAX LEVEL")));
        }

        TestTrue(TEXT("U7: no catalog, no rows"), HDShipMenu::BuildItems(EHDShipScreen::Upgrades, FHDShipMenuContext()).IsEmpty());
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
