// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "HDStratagemSettings.generated.h"

// Project Settings > Game > HD Stratagem. 바꾸면 Config/DefaultGame.ini 에 저장된다.
// 스트라타젬 자체(커맨드·지연·대기·횟수·효과)는 텍스트 카탈로그 Config/HDStratagems.json 에 있다
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "HD Stratagem"))
class HELLDIVERS_API UHDStratagemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
    UHDStratagemSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual FName       GetCategoryName() const override;

public:
    // 처음 강하할 때 쓰는 로드아웃. 위에서부터 1~4칸, 넘치면 자른다. 이름은 Config/HDStratagems.json 의 "Name"
    UPROPERTY(Config, EditAnywhere, Category = "Loadout")
    TArray<FName>       Loadout;
};
