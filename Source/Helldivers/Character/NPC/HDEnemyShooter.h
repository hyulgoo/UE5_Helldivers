// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "HDEnemyShooter.generated.h"

// 원거리형: 선호 거리에서 멈춰 소총으로 점사한다
UCLASS()
class HELLDIVERS_API AHDEnemyShooter : public AHDCharacterNonPlayer
{
	GENERATED_BODY()

public:
    AHDEnemyShooter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 무기 메시의 MuzzleFlash 소켓. 없으면 액터 위치 + 위 60 + 앞 80
    FVector     GetMuzzleLocation() const;
};
