// Fill out your copyright notice in the Description page of Project Settings.

#include "Stratagem/HDStratagemCatalog.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

FHDStratagemCatalog FHDStratagemCatalog::FromJsonString(const FString& JsonText)
{
    FHDStratagemCatalog Catalog;

    // 엔진의 DataTable JSON 가져오기(런타임에도 있다)를 그대로 쓴다. 표는 읽는 동안만 쓰고 행을 복사한다
    UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
    Table->RowStruct = FHDStratagemDefinition::StaticStruct();
    // 행에는 쓰는 열만 적는다. 구조체에 없는 열(오타)은 Problems 로 남긴다
    Table->bIgnoreMissingFields = true;
    Table->bIgnoreExtraFields = false;
    Catalog.Problems = Table->CreateTableFromJSONString(JsonText);

    Table->ForeachRow<FHDStratagemDefinition>(TEXT("HDStratagemCatalog"), [&Catalog](const FName& Key, const FHDStratagemDefinition& Value)
    {
        Catalog.Names.Add(Key);
        Catalog.Definitions.Add(Value);
    });

    return Catalog;
}

FString FHDStratagemCatalog::GetProjectFilePath()
{
    return FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("HDStratagems.json"));
}

const FHDStratagemCatalog& FHDStratagemCatalog::GetProjectCatalog()
{
    static FHDStratagemCatalog  CachedCatalog;
    static FDateTime            CachedTimeStamp;
    static bool                 bLoaded = false;

    const FString FilePath = GetProjectFilePath();
    const FDateTime TimeStamp = IFileManager::Get().GetTimeStamp(*FilePath);
    if (bLoaded && TimeStamp == CachedTimeStamp)
    {
        return CachedCatalog;
    }

    FString JsonText;
    if (FFileHelper::LoadFileToString(JsonText, *FilePath))
    {
        CachedCatalog = FromJsonString(JsonText);
    }
    else
    {
        CachedCatalog = FHDStratagemCatalog();
        CachedCatalog.Problems.Add(FString::Printf(TEXT("Cannot read %s"), *FilePath));
    }

    bLoaded = true;
    CachedTimeStamp = TimeStamp;

    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] catalog %d entries from %s"), CachedCatalog.Num(), *FPaths::ConvertRelativePathToFull(FilePath));
    for (const FString& Problem : CachedCatalog.Problems)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HDStratagem] catalog problem: %s"), *Problem);
    }

    // 한 커맨드가 다른 커맨드의 앞부분이면 짧은 쪽이 먼저 선택돼 긴 쪽을 부르지 못할 수 있다. 막지 않고 알리기만 한다
    for (const TPair<FName, FName>& Conflict : CachedCatalog.FindCommandConflicts())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HDStratagem] command conflict: %s / %s"), *Conflict.Key.ToString(), *Conflict.Value.ToString());
    }

    return CachedCatalog;
}

const FHDStratagemDefinition* FHDStratagemCatalog::Find(const FName Name) const
{
    const int32 Index = Names.IndexOfByKey(Name);
    return Definitions.IsValidIndex(Index) ? &Definitions[Index] : nullptr;
}

TArray<TPair<FName, FName>> FHDStratagemCatalog::FindCommandConflicts() const
{
    TArray<TPair<FName, FName>> Conflicts;
    for (int32 First = 0; First < Definitions.Num(); ++First)
    {
        const TArray<EHDCommandInput>& FirstCommand = Definitions[First].CommandSequence;
        for (int32 Second = First + 1; Second < Definitions.Num(); ++Second)
        {
            const TArray<EHDCommandInput>& SecondCommand = Definitions[Second].CommandSequence;

            // 짧은 쪽 길이까지 같으면 충돌이다. 짧은 쪽이 비어 있으면 그대로 같다
            const int32 CompareNum = FMath::Min(FirstCommand.Num(), SecondCommand.Num());
            bool bSamePrefix = true;
            for (int32 InputIndex = 0; InputIndex < CompareNum; ++InputIndex)
            {
                if (FirstCommand[InputIndex] != SecondCommand[InputIndex])
                {
                    bSamePrefix = false;
                    break;
                }
            }

            if (bSamePrefix)
            {
                Conflicts.Emplace(Names[First], Names[Second]);
            }
        }
    }

    return Conflicts;
}
