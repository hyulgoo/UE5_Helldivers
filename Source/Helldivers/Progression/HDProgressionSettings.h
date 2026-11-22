// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Progression/HDDifficulty.h"
#include "Progression/HDMissionReward.h"
#include "HDProgressionSettings.generated.h"

// Project Settings > Game > HD Progression. 바꾸면 Config/DefaultGame.ini 에 저장된다
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "HD Progression"))
class HELLDIVERS_API UHDProgressionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
    // 난이도 표를 기본 표로 채운다
    UHDProgressionSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual FName               GetCategoryName() const override;

    // 미션 보상 규칙(HDProgression::ComputeReward 에 넘긴다)
    FHDRewardRules              MakeRewardRules() const
    {
        FHDRewardRules Rules;
        Rules.RequisitionPerObjective = RequisitionPerObjective;
        Rules.SuccessRequisitionBonus = SuccessRequisitionBonus;
        return Rules;
    }

public:
    // 프로필 저장 슬롯 이름. 파일은 Saved/SaveGames/<슬롯>.sav
    UPROPERTY(Config, EditAnywhere, Category = "Save")
    FString                     SaveSlotName = TEXT("HDProfile");

    // 참이면 에디터(PIE 포함, GIsEditor)에서는 슬롯 끝에 _PIE 를 붙여 시험이 실제 진행을 덮지 않게 한다
    UPROPERTY(Config, EditAnywhere, Category = "Save")
    bool                        bSeparateEditorSlot = true;

    // 새 프로필의 징발 포인트
    UPROPERTY(Config, EditAnywhere, Category = "Profile")
    int32                       StartingRequisition = 0;

    // 참이면 맵을 열 때 함선 단계(폰 없음, 메뉴)로 시작하고 출격을 골라야 강하한다. 거짓이면 지금처럼 바로 강하한다
    UPROPERTY(Config, EditAnywhere, Category = "Ship")
    bool                        bStartInShip = true;

    // 함선 카메라 = 첫 PlayerStart + 이 값, PlayerStart 를 바라본다
    UPROPERTY(Config, EditAnywhere, Category = "Ship")
    FVector                     ShipCameraOffset = FVector(-5000.f, 0.f, 6000.f);

    // 난이도 표(1 단계부터). 함선 메뉴 DIFFICULTY 줄에서 고르고 출격할 때 미션에 들어간다.
    // Config 에 줄이 없으면 생성자가 채운 기본 표(HDDifficulty::GetDefaultTable)가 남는다
    UPROPERTY(Config, EditAnywhere, Category = "Difficulty")
    TArray<FHDDifficultyLevel>  DifficultyLevels;

    // 완료한 목표 하나당 징발. 실패·포기도 받는다
    UPROPERTY(Config, EditAnywhere, Category = "Reward")
    int32                       RequisitionPerObjective = 40;

    // 추출 성공 보너스 징발
    UPROPERTY(Config, EditAnywhere, Category = "Reward")
    int32                       SuccessRequisitionBonus = 100;

    // 결과가 나온 뒤 이 시간(초) 동안 Enter·클릭을 받지 않는다(남은 Enter 연타로 바로 넘어가지 않게)
    UPROPERTY(Config, EditAnywhere, Category = "Reward")
    float                       ResultInputDelay = 1.f;

    // 일반 샘플을 놓는 링: 강하 지점(첫 PlayerStart)에서의 거리(cm) (최소, 최대)
    UPROPERTY(Config, EditAnywhere, Category = "Samples")
    FVector2D                   CommonSampleRing = FVector2D(3000.f, 12000.f);

    // 희귀 샘플을 놓는 링(cm)
    UPROPERTY(Config, EditAnywhere, Category = "Samples")
    FVector2D                   RareSampleRing = FVector2D(6000.f, 15000.f);

    // 최상급 샘플을 놓는 링(cm)
    UPROPERTY(Config, EditAnywhere, Category = "Samples")
    FVector2D                   SuperSampleRing = FVector2D(10000.f, 18000.f);

    // 같은 종류끼리 떨어뜨리려는 거리(cm)
    UPROPERTY(Config, EditAnywhere, Category = "Samples")
    float                       SampleMinSeparation = 1500.f;

    // 줍기 구 반지름(cm)
    UPROPERTY(Config, EditAnywhere, Category = "Samples")
    float                       SamplePickupRadius = 200.f;

    // 폰에서 이 거리(cm) 안의 샘플만 HUD 표식을 그린다(떨군 묶음은 늘 그린다)
    UPROPERTY(Config, EditAnywhere, Category = "Samples")
    float                       SampleMarkerDistance = 5000.f;
};
