// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Stratagem/HDStratagemCatalog.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDStratagemCatalogTest, "Helldivers.Stratagem.Catalog", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDStratagemCatalogTest
{
    bool HasProblemContaining(const FHDStratagemCatalog& Catalog, const TCHAR* Text)
    {
        return Catalog.GetProblems().ContainsByPredicate([Text](const FString& Problem) { return Problem.Contains(Text); });
    }

    using FConflict = TPair<FName, FName>;

    FString DescribeConflicts(const TArray<FConflict>& Conflicts)
    {
        FString Text;
        for (const FConflict& Conflict : Conflicts)
        {
            Text += FString::Printf(TEXT("(%s, %s) "), *Conflict.Key.ToString(), *Conflict.Value.ToString());
        }
        return Text;
    }
}

bool FHDStratagemCatalogTest::RunTest(const FString& Parameters)
{
    using namespace HDStratagemCatalogTest;

    // 1. 문자열 읽기
    {
        const FString Json = TEXT(R"([
            { "Name": "A", "CommandSequence": ["Up", "Down"], "ActivationDelay": 2, "Cooldown": 5, "MaxUses": 3,
              "bIsEagle": true, "EagleAirStrikeDirection": "Right", "StratagemProjectileType": "Bomb",
              "ProjectileDropLocation": [ { "X": 1, "Y": 2 } ], "SpecifyProjectileSpawnCount": 150 },
            { "Name": "B", "CommandSequence": ["Left"] }
        ])");
        const FHDStratagemCatalog Catalog = FHDStratagemCatalog::FromJsonString(Json);
        for (const FString& Problem : Catalog.GetProblems()) { AddInfo(FString::Printf(TEXT("problem: %s"), *Problem)); }

        TestEqual(TEXT("Parse: no problems"), Catalog.GetProblems().Num(), 0);
        TestEqual(TEXT("Parse: 2 entries"), Catalog.Num(), 2);
        TestTrue(TEXT("Parse: file order kept"), Catalog.GetNames() == TArray<FName>({ TEXT("A"), TEXT("B") }));

        const FHDStratagemDefinition* A = Catalog.Find(TEXT("A"));
        const FHDStratagemDefinition* B = Catalog.Find(TEXT("B"));
        if (TestNotNull(TEXT("Parse: A found"), A) && TestNotNull(TEXT("Parse: B found"), B))
        {
            TestTrue(TEXT("Parse: A command Up Down"), A->CommandSequence == TArray<EHDCommandInput>({ EHDCommandInput::Up, EHDCommandInput::Down }));
            TestEqual(TEXT("Parse: A delay 2"), A->ActivationDelay, 2.f);
            TestEqual(TEXT("Parse: A cooldown 5"), A->Cooldown, 5.f);
            TestEqual(TEXT("Parse: A uses 3"), A->MaxUses, 3);
            TestTrue(TEXT("Parse: A eagle"), A->bIsEagle == 1);
            TestTrue(TEXT("Parse: A direction Right"), A->EagleAirStrikeDirection == EHDEagleAirStrikeDirection::Right);
            TestTrue(TEXT("Parse: A projectile Bomb"), A->StratagemProjectileType == EHDStratagemProjectile::Bomb);
            TestTrue(TEXT("Parse: A drop (1,2)"), A->ProjectileDropLocation.Num() == 1 && A->ProjectileDropLocation[0].Equals(FVector2D(1.f, 2.f)));
            TestEqual(TEXT("Parse: A spawn count 150"), (int32)A->SpecifyProjectileSpawnCount, 150);

            TestTrue(TEXT("Parse: B command Left"), B->CommandSequence == TArray<EHDCommandInput>({ EHDCommandInput::Left }));
            TestEqual(TEXT("Parse: B missing delay is 0"), B->ActivationDelay, 0.f);
            TestEqual(TEXT("Parse: B missing cooldown is 0"), B->Cooldown, 0.f);
            TestEqual(TEXT("Parse: B missing uses is 0"), B->MaxUses, 0);
            TestTrue(TEXT("Parse: B missing eagle is false"), B->bIsEagle == 0);
            TestTrue(TEXT("Parse: B missing drops empty"), B->ProjectileDropLocation.IsEmpty());
        }
        TestNull(TEXT("Find: missing is null"), Catalog.Find(TEXT("Missing")));
    }

    // 2. 문제 보고
    {
        const FHDStratagemCatalog Typo = FHDStratagemCatalog::FromJsonString(TEXT(R"([ { "Name": "A", "CommandSequence": ["Up"], "Cooldwn": 3 } ])"));
        for (const FString& Problem : Typo.GetProblems()) { AddInfo(FString::Printf(TEXT("typo problem: %s"), *Problem)); }
        TestTrue(TEXT("Problems: unknown column reported"), HasProblemContaining(Typo, TEXT("Cooldwn")));
        TestEqual(TEXT("Problems: row with unknown column still read"), Typo.Num(), 1);

        const FHDStratagemCatalog BadEnum = FHDStratagemCatalog::FromJsonString(TEXT(R"([ { "Name": "A", "CommandSequence": ["Up", "Sideways"] } ])"));
        for (const FString& Problem : BadEnum.GetProblems()) { AddInfo(FString::Printf(TEXT("enum problem: %s"), *Problem)); }
        TestTrue(TEXT("Problems: bad enum reported"), HasProblemContaining(BadEnum, TEXT("Sideways")));

        const FHDStratagemCatalog Duplicate = FHDStratagemCatalog::FromJsonString(TEXT(R"([ { "Name": "A", "CommandSequence": ["Up"] }, { "Name": "A", "CommandSequence": ["Down"] } ])"));
        for (const FString& Problem : Duplicate.GetProblems()) { AddInfo(FString::Printf(TEXT("dup problem: %s"), *Problem)); }
        TestTrue(TEXT("Problems: duplicate reported"), HasProblemContaining(Duplicate, TEXT("Duplicate")));
        TestEqual(TEXT("Problems: duplicate keeps one"), Duplicate.Num(), 1);
        const FHDStratagemDefinition* DupA = Duplicate.Find(TEXT("A"));
        TestTrue(TEXT("Problems: duplicate keeps first"), DupA && DupA->CommandSequence == TArray<EHDCommandInput>({ EHDCommandInput::Up }));

        const FHDStratagemCatalog Broken = FHDStratagemCatalog::FromJsonString(TEXT(R"([ { "Name": "A", )"));
        for (const FString& Problem : Broken.GetProblems()) { AddInfo(FString::Printf(TEXT("broken problem: %s"), *Problem)); }
        TestTrue(TEXT("Problems: broken JSON reported"), HasProblemContaining(Broken, TEXT("Failed to parse")));
        TestEqual(TEXT("Problems: broken JSON has no entries"), Broken.Num(), 0);

        const FHDStratagemCatalog NoName = FHDStratagemCatalog::FromJsonString(TEXT(R"([ { "CommandSequence": ["Up"] } ])"));
        for (const FString& Problem : NoName.GetProblems()) { AddInfo(FString::Printf(TEXT("noname problem: %s"), *Problem)); }
        TestTrue(TEXT("Problems: missing name reported"), HasProblemContaining(NoName, TEXT("missing key field")));
        TestEqual(TEXT("Problems: missing name has no entries"), NoName.Num(), 0);
    }

    // 3. 프로젝트 파일 형식 점검
    {
        const FHDStratagemCatalog& Project = FHDStratagemCatalog::GetProjectCatalog();
        for (const FString& Problem : Project.GetProblems()) { AddInfo(FString::Printf(TEXT("project problem: %s"), *Problem)); }
        TestTrue(TEXT("Project file: exists"), FPaths::FileExists(FHDStratagemCatalog::GetProjectFilePath()));
        TestEqual(TEXT("Project file: no problems"), Project.GetProblems().Num(), 0);
        TestTrue(TEXT("Project file: has entries"), Project.Num() > 0);
        bool bAllHaveCommand = true;
        for (const FName& Name : Project.GetNames())
        {
            const FHDStratagemDefinition* Definition = Project.Find(Name);
            bAllHaveCommand &= (Definition && Definition->CommandSequence.Num() > 0);
        }
        TestTrue(TEXT("Project file: every entry has a command"), bAllHaveCommand);

        const TArray<FConflict> ProjectConflicts = Project.FindCommandConflicts();
        AddInfo(FString::Printf(TEXT("project conflicts: %s"), *DescribeConflicts(ProjectConflicts)));
        TestTrue(TEXT("Project file: no command conflicts"), ProjectConflicts.IsEmpty());
    }

    // 4. 커맨드 충돌(같거나 한쪽이 다른 쪽의 앞부분)
    {
        const FString DefaultRows = TEXT(R"(
            { "Name": "EagleAirstrike", "CommandSequence": ["Up", "Right", "Down", "Right"] },
            { "Name": "OrbitalPrecisionStrike", "CommandSequence": ["Up", "Right", "Right"] },
            { "Name": "OrbitalGatlingBarrage", "CommandSequence": ["Right", "Down", "Left", "Up", "Up"] },
            { "Name": "Orbital120mmBarrage", "CommandSequence": ["Right", "Right", "Down", "Left", "Right", "Down"] })");

        const FHDStratagemCatalog Default = FHDStratagemCatalog::FromJsonString(TEXT("[") + DefaultRows + TEXT("]"));
        AddInfo(FString::Printf(TEXT("default conflicts: %s"), *DescribeConflicts(Default.FindCommandConflicts())));
        TestTrue(TEXT("Conflicts: default catalog has none"), Default.Num() == 4 && Default.FindCommandConflicts().IsEmpty());

        const FHDStratagemCatalog Prefix = FHDStratagemCatalog::FromJsonString(TEXT("[") + DefaultRows + TEXT(R"(,
            { "Name": "Bad", "CommandSequence": ["Up", "Right"] } ])"));
        AddInfo(FString::Printf(TEXT("prefix conflicts: %s"), *DescribeConflicts(Prefix.FindCommandConflicts())));
        TestTrue(TEXT("Conflicts: prefix conflicts found"), Prefix.FindCommandConflicts() == TArray<FConflict>({
            FConflict(FName(TEXT("EagleAirstrike")), FName(TEXT("Bad"))),
            FConflict(FName(TEXT("OrbitalPrecisionStrike")), FName(TEXT("Bad"))) }));

        const FHDStratagemCatalog Identical = FHDStratagemCatalog::FromJsonString(TEXT(R"([
            { "Name": "A", "CommandSequence": ["Up", "Down"] },
            { "Name": "B", "CommandSequence": ["Up", "Down"] } ])"));
        AddInfo(FString::Printf(TEXT("identical conflicts: %s"), *DescribeConflicts(Identical.FindCommandConflicts())));
        TestTrue(TEXT("Conflicts: identical commands"), Identical.FindCommandConflicts() == TArray<FConflict>({
            FConflict(FName(TEXT("A")), FName(TEXT("B"))) }));

        const FHDStratagemCatalog EmptyCommand = FHDStratagemCatalog::FromJsonString(TEXT(R"([
            { "Name": "Empty", "CommandSequence": [] },
            { "Name": "EagleAirstrike", "CommandSequence": ["Up", "Right", "Down", "Right"] },
            { "Name": "OrbitalPrecisionStrike", "CommandSequence": ["Up", "Right", "Right"] },
            { "Name": "OrbitalGatlingBarrage", "CommandSequence": ["Right", "Down", "Left", "Up", "Up"] } ])"));
        AddInfo(FString::Printf(TEXT("empty conflicts: %s"), *DescribeConflicts(EmptyCommand.FindCommandConflicts())));
        TestTrue(TEXT("Conflicts: empty command conflicts with all"), EmptyCommand.FindCommandConflicts() == TArray<FConflict>({
            FConflict(FName(TEXT("Empty")), FName(TEXT("EagleAirstrike"))),
            FConflict(FName(TEXT("Empty")), FName(TEXT("OrbitalPrecisionStrike"))),
            FConflict(FName(TEXT("Empty")), FName(TEXT("OrbitalGatlingBarrage"))) }));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
