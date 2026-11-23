// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/HDShipMenu.h"
#include "Progression/HDProfileTypes.h"
#include "Progression/HDWeaponCatalog.h"

struct FHDMissionResultSummary;
struct FHDRewardRules;

// 아래 바 키 안내 하나: [Key] Action
struct FHDKeyHint
{
    FString             Key;                    // "UP/DOWN", "ENTER", "BACKSPACE", "ESC", "LEFT/RIGHT"
    FString             Action;                 // "SELECT", "CONFIRM", "CLOSE", "BACK", "CANCEL", "CHANGE", "DIFFICULTY"
    EHDShipMenuKey      MenuKey     = EHDShipMenuKey::Confirm;
    bool                bClickable  = true;     // UP/DOWN·LEFT/RIGHT 은 거짓(방향이 둘이라)
};

// 위 바 오른쪽 자원과 그 아래 마지막 미션 줄
struct FHDResourceView
{
    int32               Requisition = 0;
    FHDSampleCounts     Samples;
    FString             LastMission;
};

// 작전 테이블 난이도 막대 한 칸
struct FHDDifficultySegment
{
    int32               Level       = 1;
    FString             Label;
    bool                bCurrent    = false;
};

// 작전 테이블 화면의 그릴 내용(작전 카드·목표·난이도·보상·맵 샘플)
struct FHDWarTableView
{
    FString                         OperationName;
    FString                         OperationType;              // "DESTROY FACILITIES"
    TArray<FString>                 Objectives;
    bool                            bHasDifficulty      = false;
    TArray<FHDDifficultySegment>    Segments;
    FString                         DifficultyLabel;            // "2  NORMAL"
    FString                         RewardMultiplier;           // "X" + FString::SanitizeFloat(배율)
    int32                           SuccessRequisition  = 0;    // 목표 전부 + 성공을 가정한 ComputeReward
    FHDSampleCounts                 MapSamples;
};

// 로드아웃 상세 패널의 한 줄(라벨, 값)
struct FHDDetailLine
{
    FString                 Label;
    FString                 Value;
};

// 로드아웃 상세 패널(커서 카드, 고르기 창이 열려 있으면 창의 커서 항목)
struct FHDLoadoutDetail
{
    FName                   IconId;
    FString                 Title;
    FString                 Category;
    TArray<FHDDetailLine>   Lines;
    TArray<EHDCommandInput> Command;
    TArray<FHDStatBar>      Bars;
    FString                 Status;
};

// 함선 화면의 "그릴 내용". 월드와 무관한 순수 함수라 자동 테스트(Helldivers.FrontEnd.ShipUI)로 고정한다
namespace HDShipMenuView
{
    // 그 화면·상태에서 실제로 동작하는 키만(왼쪽부터). Title 은 빈 배열, 출격 대기 중이면 [BACKSPACE] CANCEL 하나.
    // Loadout: [ARROWS] MOVE, 창이 닫혀 있으면 카드 [ENTER] CHANGE·READY [ENTER] READY 와 Back 규칙,
    // 창이 열려 있으면 지금 낀 것 [ENTER] KEEP·고를 수 있는 것 [ENTER] EQUIP(잠김·다른 칸은 없음) 뒤 [BACKSPACE] CLOSE.
    // 목록 화면: [UP/DOWN] SELECT, 고른 줄이 켜진 조절 줄이면 [LEFT/RIGHT] CHANGE(WarTable 의 Difficulty 줄은 DIFFICULTY),
    // 고른 줄이 켜져 있고 확정이 None 이 아니면 [ENTER] CONFIRM, 뿌리가 아니면 [BACKSPACE] BACK, 단말기 뿌리면 [BACKSPACE] CLOSE(System 은 [ESC] CLOSE), Bridge 뿌리면 없음
    HELLDIVERS_API TArray<FHDKeyHint>   GetKeyHints(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items, bool bLaunching);
    // "[KEY] ACTION" 을 공백 셋으로 잇는다
    HELLDIVERS_API FString              JoinKeyHints(const TArray<FHDKeyHint>& Hints);
    HELLDIVERS_API FHDResourceView      MakeResourceView(const FHDProfileData& Profile, const FString& LastMission);
    // "LAST MISSION  COMPLETE|FAILED  +<징발> REQ". 결과가 없으면 빈 글자
    HELLDIVERS_API FString              FormatLastMission(const FHDMissionResultSummary* LastResult);
    // 레벨 이름을 띄어 쓴 대문자. PIE 접두사(UEDPIE_<n>_)를 뗀다. "DesertMountains" → "DESERT MOUNTAINS", 빈 글자 → "UNKNOWN SECTOR"
    HELLDIVERS_API FString              FormatMapName(const FString& LevelName);
    // 프로필 난이도를 표로 자른 행으로 채운다. 표가 비면 bHasDifficulty 거짓, 목표 수 없이 "DESTROY FACILITIES", 징발 0
    // 스트라타젬: 업그레이드를 반영한 CALL-IN·COOLDOWN·USES(0 = UNLIMITED)·REARM(횟수가 있고 재무장 > 0 일 때), 호출 화살표.
    // 무기: 능력치 막대. READY: 칸마다 한 줄. 상태 = EQUIPPED·IN SLOT n·LOCKED  <가격> REQ IN ARMORY
    HELLDIVERS_API FHDLoadoutDetail     BuildLoadoutDetail(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items, const FHDShipMenuContext& Context);
    // 정수면 "10 S", 아니면 소수 한 자리 "2.5 S"
    HELLDIVERS_API FString              FormatSeconds(float Seconds);
    HELLDIVERS_API FHDWarTableView      BuildWarTable(const FHDShipMenuContext& Context, const FString& LevelName, const FHDRewardRules& Rules);
}
