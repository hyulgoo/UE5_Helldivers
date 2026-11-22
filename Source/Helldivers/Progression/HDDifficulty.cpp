// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDDifficulty.h"

namespace HDDifficulty
{
    namespace
    {
        FHDDifficultyLevel MakeLevel(const TCHAR* Label, const int32 RequiredObjectives, const float SpawnIntensity, const float HeavyRatio, const float RewardMultiplier,
            const int32 CommonSamples, const int32 RareSamples, const int32 SuperSamples)
        {
            FHDDifficultyLevel Level;
            Level.Label                 = Label;
            Level.RequiredObjectives    = RequiredObjectives;
            Level.SpawnIntensity        = SpawnIntensity;
            Level.HeavyRatio            = HeavyRatio;
            Level.RewardMultiplier      = RewardMultiplier;
            Level.SampleCounts.Common   = CommonSamples;
            Level.SampleCounts.Rare     = RareSamples;
            Level.SampleCounts.Super    = SuperSamples;
            return Level;
        }
    }

    const TArray<FHDDifficultyLevel>& GetDefaultTable()
    {
        // 강도 1 미만은 디렉터 수 계산이 1 로 본다(HDSpawnRules::ScaledCount). 그래서 EASY 의 쉬운 차이는 목표 수와 중장갑 0 에서 온다
        static const TArray<FHDDifficultyLevel> Table =
        {
            //         Label            목표  강도    중장갑   보상    샘플 일반/희귀/최상급
            MakeLevel(TEXT("EASY"),     1,    1.f,    0.f,     1.f,    4, 0, 0),
            MakeLevel(TEXT("NORMAL"),   2,    1.f,    0.1f,    1.25f,  6, 2, 0),
            MakeLevel(TEXT("HARD"),     2,    1.25f,  0.15f,   1.5f,   6, 3, 1),
            MakeLevel(TEXT("VETERAN"),  3,    1.5f,   0.2f,    2.f,    8, 4, 1),
            MakeLevel(TEXT("ELITE"),    3,    2.f,    0.25f,   2.5f,   8, 5, 2),
        };
        return Table;
    }

    int32 ClampLevel(int32 Level, int32 NumLevels)
    {
        return FMath::Clamp(Level, 1, FMath::Max(1, NumLevels));
    }

    int32 StepLevel(int32 Level, int32 Delta, int32 NumLevels)
    {
        return ClampLevel(ClampLevel(Level, NumLevels) + Delta, NumLevels);
    }

    FHDDifficultyLevel GetLevel(const TArray<FHDDifficultyLevel>& Table, int32 Level)
    {
        const int32 Index = ClampLevel(Level, Table.Num()) - 1;
        return Table.IsValidIndex(Index) ? Table[Index] : FHDDifficultyLevel();
    }

    FHDMissionConfig ApplyToConfig(const FHDMissionConfig& Base, const FHDDifficultyLevel& Level)
    {
        FHDMissionConfig Config = Base;
        if (Level.RequiredObjectives > 0)
        {
            Config.RequiredObjectives = Level.RequiredObjectives;
        }
        return Config;
    }

    float GetExtractionIntensity(float BaseExtractionIntensity, const FHDDifficultyLevel& Level)
    {
        return BaseExtractionIntensity * Level.SpawnIntensity;
    }

    FString FormatMenuLabel(int32 Level, const FHDDifficultyLevel& Row)
    {
        return FString::Printf(TEXT("DIFFICULTY  < %d  %s >"), Level, *Row.Label);
    }

    FString FormatDetail(const FHDDifficultyLevel& Row)
    {
        return FString::Printf(TEXT("OBJECTIVES %d   ENEMY INTENSITY %s   HEAVY %d%%   REWARD x%s"),
            Row.RequiredObjectives,
            *FString::SanitizeFloat(Row.SpawnIntensity),
            FMath::RoundToInt(Row.HeavyRatio * 100.f),
            *FString::SanitizeFloat(Row.RewardMultiplier));
    }
}
