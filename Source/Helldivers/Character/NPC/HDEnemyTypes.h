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

    // 대상 찾기 반경
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   AggroRadius         = 3500.f;

    // 대상 놓침 반경
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float   LoseTargetRadius    = 6000.f;
};
