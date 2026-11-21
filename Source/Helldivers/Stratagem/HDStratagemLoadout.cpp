// Fill out your copyright notice in the Description page of Project Settings.

#include "Stratagem/HDStratagemLoadout.h"

void FHDStratagemLoadout::Initialize(const TArray<FName>& LoadoutNames, const TArray<FHDStratagemEntry>& Catalog)
{
    Slots.Reset();
    bInitialized = true;

    for (const FName& StratagemName : LoadoutNames)
    {
        if (Slots.Num() >= MaxSlots)
        {
            break;
        }

        const FHDStratagemEntry* Entry = Catalog.FindByPredicate([&StratagemName](const FHDStratagemEntry& Candidate) { return Candidate.Name == StratagemName; });
        if (StratagemName.IsNone() || Entry == nullptr || FindSlot(StratagemName) != INDEX_NONE)
        {
            UE_LOG(LogTemp, Display, TEXT("[HDStratagem] loadout skips %s (none, not in catalog, or duplicate)"), *StratagemName.ToString());
            continue;
        }

        FHDStratagemSlot& Slot = Slots.AddDefaulted_GetRef();
        Slot.StratagemName = StratagemName;
        Slot.CommandSequence = Entry->CommandSequence;
        Slot.ActivationDelay = Entry->ActivationDelay;
        Slot.Cooldown = FMath::Max(Entry->Cooldown, 0.f);
        Slot.MaxUses = FMath::Max(Entry->MaxUses, 0);
        Slot.UsesLeft = Slot.MaxUses;
        Slot.RearmTime = FMath::Max(Entry->RearmTime, 0.f);
        Slot.bRearming = false;
    }
}

const FHDStratagemSlot* FHDStratagemLoadout::GetSlot(const int32 SlotIndex) const
{
    return Slots.IsValidIndex(SlotIndex) ? &Slots[SlotIndex] : nullptr;
}

int32 FHDStratagemLoadout::FindSlot(const FName StratagemName) const
{
    return Slots.IndexOfByPredicate([&StratagemName](const FHDStratagemSlot& Slot) { return Slot.StratagemName == StratagemName; });
}

bool FHDStratagemLoadout::HasUsesLeft(const int32 SlotIndex) const
{
    const FHDStratagemSlot* Slot = GetSlot(SlotIndex);
    return Slot && (Slot->MaxUses == 0 || Slot->UsesLeft > 0);
}

bool FHDStratagemLoadout::ConsumeUse(const int32 SlotIndex, float* OutCooldown)
{
    if (HasUsesLeft(SlotIndex) == false)
    {
        return false;
    }

    FHDStratagemSlot& Slot = Slots[SlotIndex];
    float Cooldown = Slot.Cooldown;
    if (Slot.MaxUses > 0)
    {
        --Slot.UsesLeft;
        Slot.bRearming = false;

        // 마지막 한 번을 썼으면 횟수를 되돌리고 긴 대기(재무장)를 건다. 대기가 끝나는 시점을 따로 지켜보지 않는다
        if (Slot.UsesLeft == 0 && Slot.RearmTime > 0.f)
        {
            Slot.UsesLeft = Slot.MaxUses;
            Slot.bRearming = true;
            Cooldown = Slot.RearmTime;
        }
    }

    if (OutCooldown)
    {
        *OutCooldown = Cooldown;
    }

    return true;
}

void FHDStratagemLoadout::ResetUses()
{
    for (FHDStratagemSlot& Slot : Slots)
    {
        Slot.UsesLeft = Slot.MaxUses;
        Slot.bRearming = false;
    }
}

FHDStratagemCommandMatch FHDStratagemLoadout::MatchCommands(const TArray<EHDCommandInput>& Input, const TArray<bool>& bAvailable) const
{
    FHDStratagemCommandMatch Match;
    if (Input.IsEmpty())
    {
        return Match;
    }

    for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
    {
        const TArray<EHDCommandInput>& CommandSequence = Slots[SlotIndex].CommandSequence;
        const bool bSlotAvailable = bAvailable.IsValidIndex(SlotIndex) && bAvailable[SlotIndex];
        if (bSlotAvailable == false || Input.Num() > CommandSequence.Num())
        {
            continue;
        }

        bool bPrefixMatch = true;
        for (int32 InputIndex = 0; InputIndex < Input.Num(); ++InputIndex)
        {
            if (Input[InputIndex] != CommandSequence[InputIndex])
            {
                bPrefixMatch = false;
                break;
            }
        }

        if (bPrefixMatch == false)
        {
            continue;
        }

        Match.MatchedSlots.Add(SlotIndex);
        if (Input.Num() == CommandSequence.Num())
        {
            Match.SelectedSlot = SlotIndex;
        }
    }

    return Match;
}
