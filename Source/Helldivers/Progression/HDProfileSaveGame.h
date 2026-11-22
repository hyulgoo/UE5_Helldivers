// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Progression/HDProfileTypes.h"
#include "HDProfileSaveGame.generated.h"

// 프로필 저장 파일(Saved/SaveGames/<슬롯>.sav)의 내용. 읽고 쓰는 곳은 UHDProfileSubsystem 이다
UCLASS()
class HELLDIVERS_API UHDProfileSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
    // SaveGame 지정자를 붙이지 않는다. SaveGameToMemory 는 ArIsSaveGame 을 켜지 않아 보통 UPROPERTY 를 모두 쓴다
    UPROPERTY()
    FHDProfileData      Data;
};
