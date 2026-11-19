// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"

// 카탈로그 한 항목. 텍스트 카탈로그 한 행에서 로드아웃이 쓰는 값만 담는다(UHDStratagemComponent::BuildCatalog)
struct FHDStratagemEntry
{
    FName                   Name                = NAME_None;
    TArray<EHDCommandInput> CommandSequence;
    float                   ActivationDelay     = 0.f;
    // 0 = 대기 없음
    float                   Cooldown            = 0.f;
    // 0 = 무제한
    int32                   MaxUses             = 0;
};

// 로드아웃 한 칸
struct FHDStratagemSlot
{
    FName                   StratagemName       = NAME_None;
    TArray<EHDCommandInput> CommandSequence;
    float                   ActivationDelay     = 0.f;
    float                   Cooldown            = 0.f;
    int32                   MaxUses             = 0;
    int32                   UsesLeft            = 0;

    // 로그·치트용. 무제한이면 inf, 아니면 남은/최대
    FString                 GetUsesText() const { return MaxUses > 0 ? FString::Printf(TEXT("%d/%d"), UsesLeft, MaxUses) : FString(TEXT("inf")); }
};

// 커맨드 입력 대조 결과
struct FHDStratagemCommandMatch
{
    // 입력이 커맨드 앞부분과 같은 칸(칸 순서)
    TArray<int32>           MatchedSlots;

    // 입력과 커맨드가 정확히 같은 칸. 없으면 INDEX_NONE
    int32                   SelectedSlot        = INDEX_NONE;
};

// 로드아웃(최대 4칸)과 칸마다 남은 횟수. 월드와 무관한 순수 로직이라 자동 테스트(Helldivers.Stratagem.Loadout)로 고정한다.
// AHDGASPlayerState 가 들고 있어 다시 강하해도 남은 횟수가 그대로다. 재사용 대기는 UHDGE_StratagemCooldown 이 맡는다
class HELLDIVERS_API FHDStratagemLoadout
{
public:
    static constexpr int32  MaxSlots = 4;

    // LoadoutNames 순서대로 칸을 만든다. 카탈로그에 없는 이름·None·중복은 건너뛰고 MaxSlots 칸에서 자른다. 대기·횟수는 카탈로그 항목 값(음수는 0)
    void                        Initialize(const TArray<FName>& LoadoutNames, const TArray<FHDStratagemEntry>& Catalog);

    bool                        IsInitialized() const { return bInitialized; }
    int32                       Num() const { return Slots.Num(); }
    const FHDStratagemSlot*     GetSlot(const int32 SlotIndex) const;
    int32                       FindSlot(const FName StratagemName) const;

    // 무제한이면 참
    bool                        HasUsesLeft(const int32 SlotIndex) const;

    // 한 번 쓴다. 남은 횟수가 없거나 칸이 없으면 거짓이고 바뀌지 않는다. 무제한이면 참이고 그대로다
    bool                        ConsumeUse(const int32 SlotIndex);

    // 모든 칸의 남은 횟수를 MaxUses 로
    void                        ResetUses();

    // bAvailable[칸] 이 거짓인 칸(대기 중·다 씀)은 후보에서 뺀다. bAvailable 이 칸 수보다 짧으면 모자란 칸은 거짓. 입력이 비면 빈 결과
    FHDStratagemCommandMatch    MatchCommands(const TArray<EHDCommandInput>& Input, const TArray<bool>& bAvailable) const;

private:
    TArray<FHDStratagemSlot>    Slots;
    bool                        bInitialized    = false;
};
