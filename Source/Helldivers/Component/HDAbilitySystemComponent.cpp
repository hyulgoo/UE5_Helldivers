// Fill out your copyright notice in the Description page of Project Settings.

#include "HDAbilitySystemComponent.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Attribute/HDSpeedAttributeSet.h"
#include "Attribute\HDWeaponAttributeSet.h"
#include "GameData/HDCharacterAttributeData.h"
#include "Define/HDDefine.h"

void UHDAbilitySystemComponent::AbilityInputTagTriggered(const FGameplayTag AssetTag)
{
    // 활성화가 능력을 주거나 지우면 배열이 다시 잡힐 수 있다. 엔진 입력 처리처럼 순회 동안 목록 변경을 미룬다
    ABILITYLIST_SCOPE_LOCK();

    TArray<FGameplayAbilitySpec>& ActivatebleAbilities = GetActivatableAbilities();
    for (FGameplayAbilitySpec& Spec : ActivatebleAbilities)
    {
        if (Spec.Ability == nullptr)
        {
            continue;
        }

        const FGameplayTagContainer& TagContainer = Spec.Ability->GetAssetTags();
        if (TagContainer.IsValid() == false || TagContainer.HasTagExact(AssetTag) == false)
        {
            continue;
        }

        if (Spec.IsActive())
        {
            AbilitySpecInputPressed(Spec);
        }
        else
        {
            TryActivateAbility(Spec.Handle);
        }
    }
}

void UHDAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag AssetTag)
{
    // 활성화가 능력을 주거나 지우면 배열이 다시 잡힐 수 있다. 엔진 입력 처리처럼 순회 동안 목록 변경을 미룬다
    ABILITYLIST_SCOPE_LOCK();

    TArray<FGameplayAbilitySpec>& ActivatebleAbilities = GetActivatableAbilities();
    for (FGameplayAbilitySpec& Spec : ActivatebleAbilities)
    {
        if (Spec.Ability == nullptr)
        {
            continue;
        }

        const FGameplayTagContainer& TagContainer = Spec.Ability->GetAssetTags();
        if (TagContainer.IsValid() == false || TagContainer.HasTagExact(AssetTag) == false)
        {
            continue;
        }

        if (Spec.IsActive())
        {
            AbilitySpecInputReleased(Spec);
        }
    }
}

void UHDAbilitySystemComponent::AbilityInputTagToggled(const FGameplayTag AssetTag)
{
    // 활성화가 능력을 주거나 지우면 배열이 다시 잡힐 수 있다. 엔진 입력 처리처럼 순회 동안 목록 변경을 미룬다
    ABILITYLIST_SCOPE_LOCK();

    TArray<FGameplayAbilitySpec>& ActivatebleAbilities = GetActivatableAbilities();
    for (FGameplayAbilitySpec& Spec : ActivatebleAbilities)
    {
        if (Spec.Ability == nullptr)
        {
            continue;
        }

        const FGameplayTagContainer& TagContainer = Spec.Ability->GetAssetTags();
        if (TagContainer.IsValid() == false || TagContainer.HasTagExact(AssetTag) == false)
        {
            continue;
        }

        if (Spec.IsActive())
        {
            AbilitySpecInputReleased(Spec);
        }
        else
        {
            TryActivateAbility(Spec.Handle);
        }
    }
}

void UHDAbilitySystemComponent::InitAttributeSet()
{
    AddSet<UHDHealthAttributeSet>();
    AddSet<UHDSpeedAttributeSet>();
    AddSet<UHDWeaponAttributeSet>();
}

void UHDAbilitySystemComponent::SetAttributeSetStat(FHDCharacterAttributeData* StatData, TSubclassOf<UGameplayEffect> InitAttributeStatEffect)
{
    NULL_CHECK(StatData);
    NULL_CHECK(InitAttributeStatEffect);

    FGameplayEffectContextHandle Context = MakeEffectContext();
    FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(InitAttributeStatEffect, 1.f, Context);
    CONDITION_CHECK(SpecHandle.IsValid());

    for(const TPair<FGameplayTag, float>& TaggedStat : StatData->Data)
    {
        SpecHandle.Data->SetSetByCallerMagnitude(TaggedStat.Key, TaggedStat.Value);
    }

    ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
