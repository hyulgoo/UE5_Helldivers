// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Progression/HDProfileTypes.h"
#include "Stratagem/HDStratagemLoadout.h"

class FHDStratagemCatalog;

enum class EHDUnlockResult : uint8
{
    Unlocked,
    Unknown,
    AlreadyUnlocked,
    CannotAfford
};

// 무기고(스트라타젬 해금)와 로드아웃 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Progression.Armory)로 고정한다.
// 해금 가격은 카탈로그 열 UnlockCost(Config/HDStratagems.json)이고, 고른 로드아웃은 프로필 Loadout 에 저장돼 출격할 때 칸이 된다
namespace HDArmory
{
    // 카탈로그에 있고 (UnlockCost ≤ 0 이거나 해금 목록에 있음)
    HELLDIVERS_API bool             IsUnlocked(const FHDProfileData& Data, const FHDStratagemCatalog& Catalog, FName StratagemName);
    // 위에서부터: 없음 → Unknown, 이미 열림 → AlreadyUnlocked, 징발 모자람 → CannotAfford(그대로), 그 밖 → 징발을 내고 해금 목록에 넣는다
    HELLDIVERS_API EHDUnlockResult  TryUnlock(FHDProfileData& Data, const FHDStratagemCatalog& Catalog, FName StratagemName);
    // 원하는 목록 → 대체 목록 → 카탈로그 순서로, 열린 것만·중복 없이 MaxSlots 까지 채운다
    HELLDIVERS_API TArray<FName>    ValidateLoadout(const TArray<FName>& Wanted, const FHDProfileData& Data, const FHDStratagemCatalog& Catalog, const TArray<FName>& Fallback, int32 MaxSlots = FHDStratagemLoadout::MaxSlots);
    // SlotIndex 칸의 다음 이름. 후보 = 카탈로그 순서의 열린 이름 중 다른 칸에 없는 것(지금 이름 포함), 끝에서 감긴다
    HELLDIVERS_API FName            CycleSlot(const TArray<FName>& Loadout, int32 SlotIndex, int32 Delta, const FHDProfileData& Data, const FHDStratagemCatalog& Catalog);
    // 낙타 표기 → 대문자 단어: 소문자·숫자 뒤의 대문자, 글자 뒤의 숫자에서 나눈다
    HELLDIVERS_API FString          MakeDisplayName(FName StratagemName);
    // "UP RIGHT DOWN RIGHT"
    HELLDIVERS_API FString          FormatCommand(const TArray<EHDCommandInput>& Command);
    // "unlocked"·"unknown"·"already unlocked"·"cannot afford"
    HELLDIVERS_API const TCHAR*     GetUnlockResultName(EHDUnlockResult Result);
}
