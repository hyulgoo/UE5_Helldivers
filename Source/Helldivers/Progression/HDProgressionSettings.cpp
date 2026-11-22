// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDProgressionSettings.h"

UHDProgressionSettings::UHDProgressionSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // DefaultGame.ini 에 DifficultyLevels 줄이 없으면 이 값이 남는다(배열 Config 는 줄이 있을 때만 덮어쓴다)
    DifficultyLevels = HDDifficulty::GetDefaultTable();
}

FName UHDProgressionSettings::GetCategoryName() const
{
    return TEXT("Game");
}
