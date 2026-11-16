// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HDEnemyTypes.generated.h"

USTRUCT(BlueprintType)
struct FHDEnemyStats
{
    GENERATED_BODY()

    // 체력 초기화
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   MaxHealth           = 100.f;

    // CharacterMovement MaxWalkSpeed
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   WalkSpeed           = 450.f;

    // 상태 판정·근접 판정
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   AttackRange         = 180.f;

    // 근접 피해
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   AttackDamage        = 15.f;

    // 예비 동작(초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   AttackWindup        = 0.45f;

    // 공격 준비(초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   AttackCooldown      = 1.6f;

    // 대상 놓침 반경(하드 상한). 감지 시간과 상관없이 이 밖이면 놓는다
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   LoseTargetRadius    = 6000.f;

    // 시야 반경. 잃는 반경은 +1000
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   SightRadius         = 4000.f;

    // 청각 반경
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   HearingRange        = 5000.f;

    // 이 안의 플레이어는 시야 밖이어도 안다
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   AutoAwareRadius     = 500.f;

    // 이 시간(초) 동안 감지가 없으면 대상을 놓친다
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   LoseTargetTime      = 6.f;

    // 이 거리 안이면 다가가지 않고 멈춘다. 0 = 근접형
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   PreferredRange      = 0.f;

    // 한 번 공격에 쏘는 발 수
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32   BurstCount          = 1;

    // 점사 간격(초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   BurstInterval       = 0.15f;

    // 탄 퍼짐 원뿔 반각(도)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   AimSpreadDegrees    = 3.f;
};
