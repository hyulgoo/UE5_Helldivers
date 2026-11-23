// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDArmoryRules.h"
#include "Progression/HDProgressionRules.h"
#include "Stratagem/HDStratagemCatalog.h"

namespace HDArmory
{
    bool IsUnlocked(const FHDProfileData& Data, const FHDStratagemCatalog& Catalog, FName StratagemName)
    {
        const FHDStratagemDefinition* Definition = Catalog.Find(StratagemName);
        if (Definition == nullptr)
        {
            return false;
        }

        return Definition->UnlockCost <= 0 || Data.UnlockedStratagems.Contains(StratagemName);
    }

    EHDUnlockResult TryUnlock(FHDProfileData& Data, const FHDStratagemCatalog& Catalog, FName StratagemName)
    {
        const FHDStratagemDefinition* Definition = Catalog.Find(StratagemName);
        if (Definition == nullptr)
        {
            return EHDUnlockResult::Unknown;
        }

        // 처음부터 열린 행(UnlockCost ≤ 0)도 여기서 돌아가므로 해금 목록에 들어가지 않는다
        if (IsUnlocked(Data, Catalog, StratagemName))
        {
            return EHDUnlockResult::AlreadyUnlocked;
        }

        FHDCost Cost;
        Cost.Requisition = Definition->UnlockCost;

        // 모자라면 TrySpend 가 아무것도 바꾸지 않는다
        if (HDProgression::TrySpend(Data, Cost) == false)
        {
            return EHDUnlockResult::CannotAfford;
        }

        Data.UnlockedStratagems.AddUnique(StratagemName);
        return EHDUnlockResult::Unlocked;
    }

    TArray<FName> ValidateLoadout(const TArray<FName>& Wanted, const FHDProfileData& Data, const FHDStratagemCatalog& Catalog, const TArray<FName>& Fallback, int32 MaxSlots)
    {
        TArray<FName> Loadout;

        // 잠긴 이름·카탈로그에 없는 이름·이미 넣은 이름은 건너뛴다
        const auto AddFrom = [&Loadout, &Data, &Catalog, MaxSlots](const TArray<FName>& Names)
        {
            for (const FName& Name : Names)
            {
                if (Loadout.Num() >= MaxSlots)
                {
                    return;
                }

                if (Loadout.Contains(Name) || IsUnlocked(Data, Catalog, Name) == false)
                {
                    continue;
                }

                Loadout.Add(Name);
            }
        };

        AddFrom(Wanted);
        AddFrom(Fallback);
        AddFrom(Catalog.GetNames());
        return Loadout;
    }

    FName CycleSlot(const TArray<FName>& Loadout, int32 SlotIndex, int32 Delta, const FHDProfileData& Data, const FHDStratagemCatalog& Catalog)
    {
        if (Loadout.IsValidIndex(SlotIndex) == false)
        {
            return NAME_None;
        }

        const FName CurrentName = Loadout[SlotIndex];

        TArray<FName> Candidates;
        for (const FName& Name : Catalog.GetNames())
        {
            if (IsUnlocked(Data, Catalog, Name) == false)
            {
                continue;
            }

            // 다른 칸에 있는 이름은 뺀다(이 칸의 지금 이름은 남는다)
            bool bInOtherSlot = false;
            for (int32 OtherIndex = 0; OtherIndex < Loadout.Num(); ++OtherIndex)
            {
                if (OtherIndex != SlotIndex && Loadout[OtherIndex] == Name)
                {
                    bInOtherSlot = true;
                    break;
                }
            }

            if (bInOtherSlot)
            {
                continue;
            }

            Candidates.Add(Name);
        }

        if (Candidates.IsEmpty())
        {
            return CurrentName;
        }

        const int32 CurrentIndex = Candidates.IndexOfByKey(CurrentName);
        if (CurrentIndex == INDEX_NONE)
        {
            // 지금 이름이 후보가 아니면(잠김·없는 이름) + 는 첫 후보, − 는 끝 후보
            return Delta >= 0 ? Candidates[0] : Candidates.Last();
        }

        const int32 CandidateCount = Candidates.Num();
        const int32 NextIndex = ((CurrentIndex + Delta) % CandidateCount + CandidateCount) % CandidateCount;
        return Candidates[NextIndex];
    }

    bool TrySetSlot(TArray<FName>& Loadout, int32 SlotIndex, FName Name, const FHDProfileData& Data, const FHDStratagemCatalog& Catalog)
    {
        if (Loadout.IsValidIndex(SlotIndex) == false || IsUnlocked(Data, Catalog, Name) == false)
        {
            return false;
        }

        // 다른 칸에 있으면 맞바꾸지 않고 거절한다(CycleSlot 의 중복 금지와 같다)
        for (int32 OtherIndex = 0; OtherIndex < Loadout.Num(); ++OtherIndex)
        {
            if (OtherIndex != SlotIndex && Loadout[OtherIndex] == Name)
            {
                return false;
            }
        }

        Loadout[SlotIndex] = Name;
        return true;
    }

    FString MakeDisplayName(FName StratagemName)
    {
        const FString Source = StratagemName.ToString();

        FString DisplayName;
        DisplayName.Reserve(Source.Len() * 2);
        for (int32 Index = 0; Index < Source.Len(); ++Index)
        {
            const TCHAR Char = Source[Index];
            if (Index > 0)
            {
                const TCHAR PrevChar = Source[Index - 1];
                const bool bUpperAfterLowerOrDigit = FChar::IsUpper(Char) && (FChar::IsLower(PrevChar) || FChar::IsDigit(PrevChar));
                const bool bDigitAfterLetter = FChar::IsDigit(Char) && FChar::IsAlpha(PrevChar);
                if (bUpperAfterLowerOrDigit || bDigitAfterLetter)
                {
                    DisplayName.AppendChar(TEXT(' '));
                }
            }

            DisplayName.AppendChar(FChar::ToUpper(Char));
        }

        return DisplayName;
    }

    FString FormatCommand(const TArray<EHDCommandInput>& Command)
    {
        TArray<FString> Parts;
        for (const EHDCommandInput Input : Command)
        {
            switch (Input)
            {
            case EHDCommandInput::Up:       Parts.Add(TEXT("UP")); break;
            case EHDCommandInput::Down:     Parts.Add(TEXT("DOWN")); break;
            case EHDCommandInput::Left:     Parts.Add(TEXT("LEFT")); break;
            case EHDCommandInput::Right:    Parts.Add(TEXT("RIGHT")); break;
            default:                        Parts.Add(TEXT("?")); break;
            }
        }

        return FString::Join(Parts, TEXT(" "));
    }

    const TCHAR* GetUnlockResultName(EHDUnlockResult Result)
    {
        switch (Result)
        {
        case EHDUnlockResult::Unlocked:         return TEXT("unlocked");
        case EHDUnlockResult::Unknown:          return TEXT("unknown");
        case EHDUnlockResult::AlreadyUnlocked:  return TEXT("already unlocked");
        case EHDUnlockResult::CannotAfford:     return TEXT("cannot afford");
        default:                                return TEXT("?");
        }
    }
}
