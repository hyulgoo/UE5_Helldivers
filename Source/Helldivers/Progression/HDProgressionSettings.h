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

    // 참이면 맵을 열 때 함선 단계(폰 없음, 메뉴)로 시작하고 출격을 골라야 강하한다. 거짓이면 지금처럼 바로 강하한다
    UPROPERTY(Config, EditAnywhere, Category = "Ship")
    bool                bStartInShip = true;

    // 함선 카메라 = 첫 PlayerStart + 이 값, PlayerStart 를 바라본다
    UPROPERTY(Config, EditAnywhere, Category = "Ship")
    FVector             ShipCameraOffset = FVector(-5000.f, 0.f, 6000.f);
};
