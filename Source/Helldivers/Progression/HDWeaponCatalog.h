// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Progression/HDProfileTypes.h"
#include "Progression/HDArmoryRules.h"

// 로드아웃에서 고르는 무기 칸(지원 무기는 스트라타젬이라 없다)
enum class EHDLoadoutWeapon : uint8 { Primary, Secondary, Grenade };

// 상세 패널 수치. 총은 클래스 생성자(돌격소총은 BP_Rifle), 수류탄은 투사체 클래스 값
struct FHDWeaponStats
{
    // 탄 하나
    float               Damage          = 0.f;
    int32               Pellets         = 1;
    // 60 / FireDelay. 수류탄 0
    float               RoundsPerMinute = 0.f;
    int32               Magazine        = 0;
    int32               SpareMagazines  = 0;
    int32               ArmorPen        = 0;
    // 수류탄 폭발 반경(cm)
    float               Radius          = 0.f;
    // 수류탄 신관. 0 = 닿으면 터짐
    float               FuseSeconds     = 0.f;
};

// 상세 패널 막대 한 줄
struct FHDStatBar
{
    FString             Label;
    FString             Value;
    // 0~1, -1 이면 막대 없이 글자만
    float               Fraction        = 0.f;
};

// 무기 표 한 행
struct FHDWeaponOption
{
    FName               Id;
    EHDLoadoutWeapon    Slot            = EHDLoadoutWeapon::Primary;
    FString             DisplayName;
    // 메뉴 설명 줄
    FString             Detail;
    // 무기고 해금 징발. 0 이면 처음부터
    int32               UnlockCost      = 0;
    FHDWeaponStats      Stats;
};

// 무기 표와 순수 규칙. 월드와 무관해 자동 테스트(Helldivers.Progression.Weapons)로 고정한다.
// 표를 텍스트 카탈로그(JSON)로 두지 않는다: 행마다 C++ 클래스가 붙고, 무기를 늘리면 어차피 빌드한다
namespace HDWeaponCatalog
{
    HELLDIVERS_API const TArray<FHDWeaponOption>&   GetOptions();
    HELLDIVERS_API const FHDWeaponOption*          Find(FName Id);
    HELLDIVERS_API FName                           GetDefault(EHDLoadoutWeapon Slot);
    // 표에 있고 (가격 ≤ 0 이거나 UnlockedWeapons 에 있음)
    HELLDIVERS_API bool                            IsUnlocked(const FHDProfileData& Data, FName Id);
    // HDArmory::TryUnlock 과 같은 순서: 없음 → Unknown, 열림 → AlreadyUnlocked, 모자람 → CannotAfford, 그 밖 → 징발을 내고 목록에 넣는다
    HELLDIVERS_API EHDUnlockResult                 TryUnlock(FHDProfileData& Data, FName Id);
    // 프로필 값이 표에 있고, 칸이 맞고, 열려 있으면 그 값. 아니면 기본
    HELLDIVERS_API FName                           GetChosen(const FHDProfileData& Data, EHDLoadoutWeapon Slot);
    // 그 칸의 열린 무기(표 순서) 중 GetChosen 의 다음. 끝에서 감긴다
    HELLDIVERS_API FName                           Cycle(const FHDProfileData& Data, EHDLoadoutWeapon Slot, int32 Delta);
    // 프로필 필드에 쓴다(검사는 GetChosen 이 한다)
    HELLDIVERS_API void                            SetChosen(FHDProfileData& Data, EHDLoadoutWeapon Slot, FName Id);
    // 스폰할 클래스. nullptr 이면 캐릭터 기본(돌격소총 = BP_Rifle)
    HELLDIVERS_API UClass*                         GetWeaponClass(FName Id);
    // "Primary"·"Secondary"·"Grenade"
    HELLDIVERS_API const TCHAR*                    GetSlotName(EHDLoadoutWeapon Slot);
    // "primary=<Id> secondary=<Id> grenade=<Id> unlocked=<쉼표로>"(GetChosen 값)
    HELLDIVERS_API FString                         Describe(const FHDProfileData& Data);
    // 총: DAMAGE·FIRE RATE·CAPACITY·ARMOR PEN. 수류탄: DAMAGE·RADIUS·ARMOR PEN·FUSE(막대 없음)
    HELLDIVERS_API TArray<FHDStatBar>              MakeStatBars(const FHDWeaponOption& Option);
}
