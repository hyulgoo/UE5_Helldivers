// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "HDProgressionSettings.generated.h"

// Project Settings > Game > HD Progression. 바꾸면 Config/DefaultGame.ini 에 저장된다
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "HD Progression"))
class HELLDIVERS_API UHDProgressionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
    virtual FName       GetCategoryName() const override;

public:
    // 프로필 저장 슬롯 이름. 파일은 Saved/SaveGames/<슬롯>.sav
    UPROPERTY(Config, EditAnywhere, Category = "Save")
    FString             SaveSlotName = TEXT("HDProfile");

    // 참이면 에디터(PIE 포함, GIsEditor)에서는 슬롯 끝에 _PIE 를 붙여 시험이 실제 진행을 덮지 않게 한다
    UPROPERTY(Config, EditAnywhere, Category = "Save")
    bool                bSeparateEditorSlot = true;

    // 새 프로필의 징발 포인트
    UPROPERTY(Config, EditAnywhere, Category = "Profile")
    int32               StartingRequisition = 0;
};
