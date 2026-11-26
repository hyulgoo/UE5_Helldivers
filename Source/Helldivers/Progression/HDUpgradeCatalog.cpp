// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDUpgradeCatalog.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

FHDUpgradeCatalog FHDUpgradeCatalog::FromJsonString(const FString& JsonText)
{
    FHDUpgradeCatalog Catalog;

    // 엔진의 DataTable JSON 가져오기(런타임에도 있다)를 그대로 쓴다. 표는 읽는 동안만 쓰고 행을 복사한다
    UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
    Table->RowStruct = FHDUpgradeDefinition::StaticStruct();
    // 행에는 쓰는 열만 적는다. 구조체에 없는 열(오타)은 Problems 로 남긴다
    Table->bIgnoreMissingFields = true;
    Table->bIgnoreExtraFields = false;
    Catalog.Problems = Table->CreateTableFromJSONString(JsonText);

    Table->ForeachRow<FHDUpgradeDefinition>(TEXT("HDUpgradeCatalog"), [&Catalog](const FName& Key, const FHDUpgradeDefinition& Value)
    {
        Catalog.Names.Add(Key);
        Catalog.Definitions.Add(Value);

        // 음수 가격은 구매할 때 자원을 늘린다. 막지 않고 알린다
        for (int32 LevelIndex = 0; LevelIndex < Value.Levels.Num(); ++LevelIndex)
        {
            const FHDUpgradeLevel& Level = Value.Levels[LevelIndex];
            if (Level.Requisition < 0 || Level.Common < 0 || Level.Rare < 0 || Level.Super < 0)
            {
                Catalog.Problems.Add(FString::Printf(TEXT("%s: level %d has a negative cost"), *Key.ToString(), LevelIndex + 1));
            }
        }
    });

    return Catalog;
}

FString FHDUpgradeCatalog::GetProjectFilePath()
{
    return FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("HDUpgrades.json"));
}

const FHDUpgradeCatalog& FHDUpgradeCatalog::GetProjectCatalog()
{
    static FHDUpgradeCatalog    CachedCatalog;
    static FDateTime            CachedTimeStamp;
    static bool                 bLoaded = false;

    // 함선 UI 가 아이콘마다 부르므로 매번 파일 시각을 묻지 않는다.
    // 에디터는 표를 고치면 다시 읽도록 1초에 한 번 시각을 보고, 패키징 빌드는 처음 한 번만 읽는다
    const FString FilePath = GetProjectFilePath();
#if WITH_EDITOR
    static double               LastTimeStampCheckSeconds = 0.0;
    const double NowSeconds = FPlatformTime::Seconds();
    if (bLoaded && NowSeconds - LastTimeStampCheckSeconds < 1.0)
    {
        return CachedCatalog;
    }
    LastTimeStampCheckSeconds = NowSeconds;

    const FDateTime TimeStamp = IFileManager::Get().GetTimeStamp(*FilePath);
    if (bLoaded && TimeStamp == CachedTimeStamp)
    {
        return CachedCatalog;
    }
#else
    if (bLoaded)
    {
        return CachedCatalog;
    }
    const FDateTime TimeStamp = FDateTime::MinValue();
#endif

    FString JsonText;
    if (FFileHelper::LoadFileToString(JsonText, *FilePath))
    {
        CachedCatalog = FromJsonString(JsonText);
    }
    else
    {
        CachedCatalog = FHDUpgradeCatalog();
        CachedCatalog.Problems.Add(FString::Printf(TEXT("Cannot read %s"), *FilePath));
    }

    bLoaded = true;
    CachedTimeStamp = TimeStamp;

    UE_LOG(LogTemp, Display, TEXT("[HDUpgrade] catalog %d entries from %s"), CachedCatalog.Num(), *FPaths::ConvertRelativePathToFull(FilePath));
    for (const FString& Problem : CachedCatalog.Problems)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HDUpgrade] catalog problem: %s"), *Problem);
    }

    return CachedCatalog;
}

const FHDUpgradeDefinition* FHDUpgradeCatalog::Find(const FName Name) const
{
    const int32 Index = Names.IndexOfByKey(Name);
    return Definitions.IsValidIndex(Index) ? &Definitions[Index] : nullptr;
}
