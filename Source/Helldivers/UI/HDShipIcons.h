// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FHDStratagemCatalog;
struct FHDStratagemDefinition;
enum class EHDCommandInput : uint8;

// 아이콘 한 개. 텍스처 경로가 있으면 그 텍스처, 없으면 약칭 글자 타일
struct FHDIconRef
{
    // 비면 글자 타일
    FString             TexturePath;
    // 2~3자, 대문자
    FString             Abbrev;
    FLinearColor        Tint            = FLinearColor::White;
    // 텍스처 오른쪽 아래 작은 글자(비어도 된다)
    FString             Badge;
};

// 아이콘 표(스트라타젬·무기·자원·업그레이드)와 스트라타젬 분류. 순수 함수라 자동 테스트(Helldivers.FrontEnd.ShipIcons)로 고정한다
namespace HDShipIcons
{
    // 표에 있으면 그 값, 없으면 Id 를 대문자 경계로 띄운 글자의 약칭 + IconUpgrade 색.
    // 스트라타젬은 Catalog 에서 정의를 찾았을 때만 분류 색, 못 찾으면 IconWeapon
    HELLDIVERS_API FHDIconRef   Resolve(FName Id, const FHDStratagemCatalog* Catalog = nullptr);
    // 낱말이 하나면 앞 두 글자, 여럿이면 앞 세 낱말의 첫 글자(소문자 → 대문자 경계도 낱말 경계). 빈 글자면 "?"
    HELLDIVERS_API FString      MakeAbbrev(const FString& DisplayName);
    // EAGLE·ORBITAL·SUPPLY·DEFENSIVE·SUPPORT WEAPON
    HELLDIVERS_API FString      GetStratagemCategory(const FHDStratagemDefinition& Definition);
    // 위 순서로 0~4, 모르는 이름 99
    HELLDIVERS_API int32        GetCategoryOrder(const FString& Category);
    HELLDIVERS_API FLinearColor GetCategoryTint(const FString& Category);
    // /Game/Asset/Stratagem/Command/Arrow_<방향>.Arrow_<방향>
    HELLDIVERS_API FString      GetCommandArrowTexturePath(EHDCommandInput Input);
}
