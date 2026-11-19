// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGE_StratagemCooldown.h"
#include "AbilitySystemComponent.h"
#include "Define/HDGameplayTag.h"

UHDGE_StratagemCooldown::UHDGE_StratagemCooldown(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
}

FGameplayTag UHDGE_StratagemCooldown::GetSlotTag(const int32 SlotIndex)
{
    switch (SlotIndex)
    {
    case 0: return Tag_Cooldown_Stratagem_Slot1.GetTag();
    case 1: return Tag_Cooldown_Stratagem_Slot2.GetTag();
    case 2: return Tag_Cooldown_Stratagem_Slot3.GetTag();
    case 3: return Tag_Cooldown_Stratagem_Slot4.GetTag();
    default: return FGameplayTag();
    }
}

void UHDGE_StratagemCooldown::ApplyToSlot(UAbilitySystemComponent* ASC, const int32 SlotIndex, const float Seconds)
{
    const FGameplayTag SlotTag = GetSlotTag(SlotIndex);
    if (ASC == nullptr || SlotTag.IsValid() == false || Seconds <= 0.f)
    {
        return;
    }

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UHDGE_StratagemCooldown::StaticClass(), 1.f, ASC->MakeEffectContext());
    if (SpecHandle.IsValid() == false)
    {
        return;
    }

    // 지속 시간을 적용 전에 고정한다(빼면 태그는 붙어도 남은 시간이 0 이다 — 기획 실측 W1)
    SpecHandle.Data->SetDuration(Seconds, true);
    SpecHandle.Data->DynamicGrantedTags.AddTag(SlotTag);
    ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

bool UHDGE_StratagemCooldown::IsSlotOnCooldown(const UAbilitySystemComponent* ASC, const int32 SlotIndex)
{
    const FGameplayTag SlotTag = GetSlotTag(SlotIndex);
    return ASC && SlotTag.IsValid() && ASC->HasMatchingGameplayTag(SlotTag);
}

float UHDGE_StratagemCooldown::GetSlotRemaining(const UAbilitySystemComponent* ASC, const int32 SlotIndex)
{
    const FGameplayTag SlotTag = GetSlotTag(SlotIndex);
    if (ASC == nullptr || SlotTag.IsValid() == false)
    {
        return 0.f;
    }

    const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(SlotTag));
    float Remaining = 0.f;
    for (const float TimeRemaining : ASC->GetActiveEffectsTimeRemaining(Query))
    {
        Remaining = FMath::Max(Remaining, TimeRemaining);
    }

    return Remaining;
}

void UHDGE_StratagemCooldown::ClearAll(UAbilitySystemComponent* ASC)
{
    if (ASC == nullptr)
    {
        return;
    }

    FGameplayTagContainer SlotTags;
    for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
    {
        SlotTags.AddTag(GetSlotTag(SlotIndex));
    }

    ASC->RemoveActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(SlotTags));
}
