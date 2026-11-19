// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HDGE_StratagemCooldown.generated.h"

class UAbilitySystemComponent;

// 로드아웃 칸의 재사용 대기. 지속 시간은 Spec 의 SetDuration, 칸 태그(Cooldown.Stratagem.SlotN)는 Spec 의 DynamicGrantedTags 로 준다.
// 생성자에서 GE 컴포넌트를 붙이지 않는다(볼트 errors/gameplayeffect-component-ctor-newobject-crash: 생성자 FindOrAddComponent 는 CDO 생성 중 Fatal)
UCLASS()
class HELLDIVERS_API UHDGE_StratagemCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
    UHDGE_StratagemCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // SlotIndex(0~3) 칸 태그를 Seconds 동안 붙인다. 0 이하이거나 칸 번호가 범위 밖이면 아무것도 하지 않는다
    static void             ApplyToSlot(UAbilitySystemComponent* ASC, const int32 SlotIndex, const float Seconds);

    static bool             IsSlotOnCooldown(const UAbilitySystemComponent* ASC, const int32 SlotIndex);

    // 남은 대기(초). 대기 중이 아니면 0
    static float            GetSlotRemaining(const UAbilitySystemComponent* ASC, const int32 SlotIndex);

    // 모든 칸의 대기를 지운다(치트)
    static void             ClearAll(UAbilitySystemComponent* ASC);

    // 0~3 밖이면 빈 태그
    static FGameplayTag     GetSlotTag(const int32 SlotIndex);
};
