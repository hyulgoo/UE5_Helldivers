// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "HDEnemyCharger.generated.h"

// 돌격형: 플레이어에게 달려와 근접 공격한다
UCLASS()
class HELLDIVERS_API AHDEnemyCharger : public AHDCharacterNonPlayer
{
	GENERATED_BODY()

public:
    AHDEnemyCharger(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
