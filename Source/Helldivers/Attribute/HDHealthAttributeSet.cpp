// Fill out your copyright notice in the Description page of Project Settings.

#include "HDHealthAttributeSet.h"
#include "Define/HDGameplayTag.h"
#include "Define/HDDefine.h"
#include "AbilitySystem/GameplayAbilityHelper.h"

void UHDHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    if(Attribute == GetCurrentHealthAttribute()) 
    {
        NewValue = NewValue <= 0.f ? 0.f : (NewValue > GetMaxHealth() ? GetMaxHealth() : NewValue);
    }
}

void UHDHealthAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	if (Attribute == GetCurrentHealthAttribute())
	{
		if (OldValue > 0.f && NewValue <= 0.f)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			NULL_CHECK(ASC);

			FGameplayAbilityHelper::AddTagToTarget(ASC, Tag_Character_Action_Dead);
			FGameplayAbilityHelper::AddTagToTarget(ASC, Tag_Trigger_Dead);
		}
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		NULL_CHECK(ASC);

		if (OldValue <= 0.f && NewValue > 0.f)
		{
			// 최대 체력이 처음 정해질 때: 초기 GE 가 CurrentHealth 를 먼저 적용해 현재값만 0 으로 잘린 것을 기준값으로 되살린다
			const float RestoredHealth = FMath::Clamp(CurrentHealth.GetBaseValue(), 0.f, NewValue);
			ASC->ApplyModToAttribute(GetCurrentHealthAttribute(), EGameplayModOp::Override, RestoredHealth);
		}
		else if (GetCurrentHealth() > NewValue)
		{
			// 최대 체력이 줄면 현재 체력을 새 최대로 자른다. 늘 때는 올리지 않는다
			ASC->ApplyModToAttribute(GetCurrentHealthAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
}
