// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameData/HDStratagemEffectData.h"
#include "HDStratagemDefinition.generated.h"

enum class EHDCommandInput : uint8;

// 스트라타젬 한 개 = 텍스트 카탈로그(Config/HDStratagems.json) 한 행.
// 효과 열은 FHDStratagemEffectData 에서 물려받아 같은 행에 평평하게 쓴다. 행에 적지 않은 열은 기본값(0)이다
USTRUCT(BlueprintType)
struct FHDStratagemDefinition : public FHDStratagemEffectData
{
    GENERATED_BODY()

public:
    // 호출 커맨드. JSON 에는 "Up"·"Down"·"Left"·"Right"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Call")
    TArray<EHDCommandInput>     CommandSequence;

    // 표적에 닿은 뒤 효과가 시작될 때까지(초). DT_StratagemData 의 StratagemDelay 와 같은 뜻
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Call")
    float                       ActivationDelay     = 0.f;

    // 던진 뒤 다시 부를 수 있을 때까지(초). 0 = 대기 없음
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Call")
    float                       Cooldown            = 0.f;

    // 한 판에 부를 수 있는 횟수. 0 = 무제한
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Call")
    int32                       MaxUses             = 0;

    // 횟수(MaxUses)를 다 쓴 뒤 다시 채워질 때까지(초). 0 = 다시 채우지 않는다. MaxUses 가 0 이면 쓰지 않는다
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Call")
    float                       RearmTime           = 0.f;
};
