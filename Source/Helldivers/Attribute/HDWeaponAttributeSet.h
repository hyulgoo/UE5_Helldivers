// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "HDWeaponAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class HELLDIVERS_API UHDWeaponAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	ATTRIBUTE_ACCESSORS(ThisClass, Ammo);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxAmmo);
	ATTRIBUTE_ACCESSORS(ThisClass, Capacity);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxCapacity);
	ATTRIBUTE_ACCESSORS(ThisClass, Grenade);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxGrenade);
	ATTRIBUTE_ACCESSORS(ThisClass, Stim);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxStim);

	virtual void            PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void            PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
    //virtual bool            PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
    //virtual void            PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
public:
	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Ammo;	
    
	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData MaxAmmo;	

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Capacity;	

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData MaxCapacity;

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Grenade;

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData MaxGrenade;

	// 스팀(회복 주사) 개수. 다 써도 Block.* 태그는 없다(UHDGA_Stim 이 개수를 직접 본다)
	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Stim;

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData MaxStim;
};
