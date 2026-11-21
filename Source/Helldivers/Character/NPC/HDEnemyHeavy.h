// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/HDEnemyShooter.h"
#include "HDEnemyHeavy.generated.h"

// 중장갑형: 원거리형을 물려받아 느리게 다가와 길게 점사한다. 몸통은 주무기가 튕기고 머리(약점)·팔다리로 잡는다
UCLASS()
class HELLDIVERS_API AHDEnemyHeavy : public AHDEnemyShooter
{
	GENERATED_BODY()

public:
    AHDEnemyHeavy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
