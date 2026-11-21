// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HDArmorRules.generated.h"

UENUM(BlueprintType)
enum class EHDHitZone : uint8
{
    Body,
    Head,
    Arm,
    Leg,
};

UENUM(BlueprintType)
enum class EHDHitOutcome : uint8
{
    Full,       // 관통 > 장갑: 피해 전부
    Reduced,    // 관통 == 장갑: HDArmor::ReducedDamageRatio 만큼
    Ricochet,   // 관통 < 장갑: 피해 없음
};

// 부위별 장갑 등급(0 = 장갑 없음)과 피해 배율. 기본값은 장갑·부위 개념이 없던 때의 동작(전부 0, 전부 1배)
USTRUCT(BlueprintType)
struct FHDArmorProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    int32           BodyArmor               = 0;

    UPROPERTY(EditAnywhere)
    int32           HeadArmor               = 0;

    UPROPERTY(EditAnywhere)
    int32           ArmArmor                = 0;

    UPROPERTY(EditAnywhere)
    int32           LegArmor                = 0;

    UPROPERTY(EditAnywhere)
    float           BodyDamageMultiplier    = 1.f;

    UPROPERTY(EditAnywhere)
    float           HeadDamageMultiplier    = 1.f;

    UPROPERTY(EditAnywhere)
    float           ArmDamageMultiplier     = 1.f;

    UPROPERTY(EditAnywhere)
    float           LegDamageMultiplier     = 1.f;
};

struct FHDHitResolution
{
    EHDHitZone      Zone        = EHDHitZone::Body;
    EHDHitOutcome   Outcome     = EHDHitOutcome::Full;
    int32           Penetration = 0;
    int32           Armor       = 0;
    float           Damage      = 0.f;
};

// 장갑·관통·부위 판정 규칙. 월드·액터를 참조하지 않는다(자동 테스트 Helldivers.Combat.Armor)
namespace HDArmor
{
    constexpr float ReducedDamageRatio  = 0.65f;    // 관통 == 장갑
    constexpr float GrazingCos          = 0.5f;     // 입사각 60도. 이보다 비스듬하면 관통 -1

    // 본 이름(대소문자 무시)의 접두사로 부위를 가른다. 모르는 이름·None 은 Body
    HELLDIVERS_API EHDHitZone       ZoneFromBone(FName BoneName);

    // 관통 > 장갑 전부, == 감소, < 도탄. 관통 등급은 1 이상이라 장갑 0 인 대상은 늘 전부다
    HELLDIVERS_API EHDHitOutcome    ResolveOutcome(int32 Penetration, int32 Armor);

    // CosIncidence(탄이 온 방향과 표면 법선의 내적) < GrazingCos 면 관통 -1. 1 밑으로는 내리지 않는다
    HELLDIVERS_API int32            ApplyIncidence(int32 Penetration, float CosIncidence);

    // 부위 → 그 부위 장갑·배율 → 결과 → 피해. 입사각은 호출하는 쪽이 ApplyIncidence 로 먼저 적용한다
    HELLDIVERS_API FHDHitResolution ResolveHit(const FHDArmorProfile& Profile, FName BoneName, int32 Penetration, float BaseDamage);

    // 폭발: 몸통 장갑으로 판정하고 부위 배율은 쓰지 않는다
    HELLDIVERS_API FHDHitResolution ResolveExplosion(const FHDArmorProfile& Profile, int32 Penetration, float BaseDamage);

    HELLDIVERS_API const TCHAR*     GetZoneName(EHDHitZone Zone);
    HELLDIVERS_API const TCHAR*     GetOutcomeName(EHDHitOutcome Outcome);
}
