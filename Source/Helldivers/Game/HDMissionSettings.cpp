// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/HDMissionSettings.h"

FName UHDMissionSettings::GetCategoryName() const
{
    return TEXT("Game");
}

FHDMissionConfig UHDMissionSettings::MakeConfig() const
{
    FHDMissionConfig Config;
    Config.RequiredObjectives = RequiredObjectives;
    Config.MaxReinforcements = MaxReinforcements;
    Config.ExtractionHoldTime = ExtractionHoldTime;
    Config.MissionTimeLimit = MissionTimeLimit;
    return Config;
}
