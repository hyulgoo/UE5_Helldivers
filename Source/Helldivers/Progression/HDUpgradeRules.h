// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Progression/HDProfileTypes.h"
#include "Progression/HDUpgradeTypes.h"

class FHDUpgradeCatalog;
struct FHDStratagemEntry;
struct FHDStratagemDefinition;

enum class EHDUpgradeBuyResult : uint8
{
    Bought,
    Unknown,
    MaxLevel,
    CannotAfford
};

// 산 단계를 모두 합친 효과. 기본값 = 효과 없음
struct FHDUpgradeModifiers
{
    int32               EagleExtraUses              = 0;
    float               EagleRearmScale             = 1.f;
    float               OrbitalCooldownScale        = 1.f;
    float               SupportCooldownScale        = 1.f;
    float               ActivationDelayReduction    = 0.f;
    int32               ExtraStimsAndGrenades       = 0;
    int32               ExtraReinforcements         = 0;
};

// 함선 업그레이드 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Progression.Upgrades)로 고정한다.
// 효과는 다음 출격 때 칸 값(UHDStratagemComponent::BuildCatalog)·스팀·수류탄·증원 수에 넣고, 미션 중에는 바꾸지 않는다
namespace HDUpgrade
{
    // 호출 지연을 줄여도 이 값 밑으로는 내리지 않는다(원래 이보다 짧던 값은 그대로)
    constexpr float                         MinActivationDelay = 0.5f;

    // 프로필의 산 단계(없으면 0)
    HELLDIVERS_API int32                    GetLevel(const FHDProfileData& Data, FName UpgradeName);
    // 다음에 살 단계. 다 샀으면 null
    HELLDIVERS_API const FHDUpgradeLevel*   GetNextLevel(const FHDUpgradeDefinition& Definition, int32 CurrentLevel);
    // 위에서부터: 카탈로그에 없음 → Unknown, 다 삼 → MaxLevel, 모자람 → CannotAfford(그대로), 그 밖 → 지불하고 단계 + 1
    HELLDIVERS_API EHDUpgradeBuyResult      TryBuy(FHDProfileData& Data, FName UpgradeName, const FHDUpgradeCatalog& Catalog);
    // 단계는 [0, Levels.Num()] 으로 자르고 그 단계의 값을 쓴다. 같은 효과가 둘이면 배율은 곱하고 더하기는 더한다. 카탈로그에 없는 이름은 무시
    HELLDIVERS_API FHDUpgradeModifiers      ComputeModifiers(const FHDProfileData& Data, const FHDUpgradeCatalog& Catalog);
    // 칸 값(정의를 옮긴 Entry)에 효과를 넣는다. 종류는 정의의 Kind·bIsEagle 로 가른다. 지연은 max(min(원래, 하한), 원래 − 감소)
    HELLDIVERS_API void                     ApplyToEntry(FHDStratagemEntry& Entry, const FHDStratagemDefinition& Definition, const FHDUpgradeModifiers& Modifiers);
    // "EAGLE REARM -40%"·"+1 EAGLE USES"·"CALL-IN TIME -0.5S"·"+2 STIMS AND GRENADES"·"+1 REINFORCEMENTS" …
    HELLDIVERS_API FString                  FormatEffect(EHDUpgradeEffect Effect, float Value);
    // "200 REQ  10 C  3 R"(0 인 항목은 뺀다, 모두 0 이면 "FREE")
    HELLDIVERS_API FString                  FormatCost(const FHDCost& Cost);
    // 로그용 "eagleUses=+1 eagleRearm=0.60 orbital=0.90 support=0.90 delay=-0.5 consumables=+1 reinforcements=+1"
    HELLDIVERS_API FString                  DescribeModifiers(const FHDUpgradeModifiers& Modifiers);
    // "bought"·"unknown"·"max level"·"cannot afford"
    HELLDIVERS_API const TCHAR*             GetBuyResultName(EHDUpgradeBuyResult Result);
}
