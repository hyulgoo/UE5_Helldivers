// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Progression/HDProfileTypes.h"
#include "HDUpgradeTypes.generated.h"

// 함선 업그레이드 효과. 값은 FHDUpgradeLevel::Value(그 단계까지의 누적 값). Count 는 개수 표시용이다
UENUM(BlueprintType)
enum class EHDUpgradeEffect : uint8
{
    // 이글(bIsEagle) 칸의 한 판 횟수 + 값(MaxUses 가 0 이면 쓰지 않는다)
    EagleExtraUses,
    // 이글 재무장 시간 × 값
    EagleRearmScale,
    // 궤도(Kind Projectile, 이글 아님) 재사용 대기 × 값
    OrbitalCooldownScale,
    // 보급·포탑·지원 무기(Kind Resupply·Sentry·SupportWeapon) 재사용 대기 × 값
    SupportCooldownScale,
    // 모든 칸의 호출 지연 − 값(하한 HDUpgrade::MinActivationDelay)
    ActivationDelayReduction,
    // 강하·증원 때 받는 스팀·수류탄 + 값
    ExtraStimsAndGrenades,
    // 미션 증원 수 + 값
    ExtraReinforcements,
    Count
};

// 한 단계: 가격과 그 단계까지의 누적 효과 값.
// 가격 열을 FHDCost 로 묶지 않고 평평하게 둔 것은 JSON 행을 스트라타젬 카탈로그처럼 한 줄로 읽히게 하려는 것이다
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDUpgradeLevel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    int32               Requisition = 0;

    UPROPERTY(EditAnywhere)
    int32               Common      = 0;

    UPROPERTY(EditAnywhere)
    int32               Rare        = 0;

    UPROPERTY(EditAnywhere)
    int32               Super       = 0;

    UPROPERTY(EditAnywhere)
    float               Value       = 0.f;

    FHDCost             ToCost() const;
};

// 업그레이드 하나 = Config/HDUpgrades.json 한 행
USTRUCT(BlueprintType)
struct HELLDIVERS_API FHDUpgradeDefinition : public FTableRowBase
{
    GENERATED_BODY()

    // 메뉴에 그리는 이름(영어)
    UPROPERTY(EditAnywhere)
    FString                 DisplayName;

    // 한 줄 영어 설명
    UPROPERTY(EditAnywhere)
    FString                 Description;

    UPROPERTY(EditAnywhere)
    EHDUpgradeEffect        Effect = EHDUpgradeEffect::EagleExtraUses;

    // 1 단계부터 차례로. 산 단계 수 = 프로필 UpgradeLevels 의 값
    UPROPERTY(EditAnywhere)
    TArray<FHDUpgradeLevel> Levels;
};
