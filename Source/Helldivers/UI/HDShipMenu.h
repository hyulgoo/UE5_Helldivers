// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "Progression/HDProfileTypes.h"
#include "Progression/HDDifficulty.h"

class FHDUpgradeCatalog;
class FHDStratagemCatalog;
class FHDShipMenu;

// 함선 메뉴의 화면
enum class EHDShipScreen : uint8
{
    Title,
    Bridge,
    Upgrades,
    Loadout,
    Armory,
    // 걸어 다니는 함선: 작전 테이블(난이도 → START MISSION → LOADOUT → READY)과 P 함선 메뉴(RESUME·QUIT GAME)
    WarTable,
    System
};

// 메뉴가 받는 키. 원시 키는 HDShipMenu::TryGetMenuKey 가 바꾼다
enum class EHDShipMenuKey : uint8
{
    Up,
    Down,
    Left,
    Right,
    Confirm,
    Back,
    // 무기고 탭(브리프 ship-terminal-ui 7): Q = 이전, E = 다음. 기존 값 뒤에만 더한다
    PrevTab,
    NextTab
};

// 키가 만든 일. OpenScreen 은 FHDShipMenu 가 스스로 처리하고 알리기만 한다
enum class EHDShipActionType : uint8
{
    None,
    OpenScreen,
    Deploy,
    Quit,
    Adjust,
    Select,
    // 단말기 닫기(뿌리 화면에서 Back, RESUME). Id = 뿌리 화면 이름
    CloseTerminal,
    // READY: 출격 대기(카운트다운) 시작
    Launch,
    // Loadout 고르기 창 열기·닫기(Id = 줄 Id)
    OpenPicker,
    ClosePicker,
    // 고르기 창에서 장착(Id = 줄 Id, OptionId = 고른 선택지)
    Equip,
    // 무기고 탭 바꾸기(Id = 탭 이름)
    ChangeTab
};

// 고르기 창의 항목
struct FHDShipMenuOption
{
    FName               Id;
    FString             Label;
    FString             Category;
    bool                bUnlocked       = true;
    // 다른 칸에 끼어 있으면 그 칸 번호(0부터)
    int32               EquippedSlot    = INDEX_NONE;
    bool                bCurrent        = false;
};

// 메뉴 한 줄
struct FHDShipMenuItem
{
    FName               Id;
    FString             Label;
    // 고른 줄의 설명(화면 아래)
    FString             Detail;
    // 거짓이면 흐리게 그리고 확정해도 None
    bool                bEnabled        = true;
    // 참이면 ← → 가 Adjust(−1 / +1)
    bool                bAdjustable     = false;
    EHDShipActionType   ConfirmAction   = EHDShipActionType::Select;
    // ConfirmAction 이 OpenScreen 일 때 갈 화면
    EHDShipScreen       TargetScreen    = EHDShipScreen::Bridge;
    // 카드 아이콘(HDShipIcons::Resolve 의 Id). Loadout 칸 = 스트라타젬 이름, 무기 줄 = 무기 Id
    FName               IconId;
    // Loadout 고르기 창의 선택지(칸 줄 = 카탈로그 전체, 무기 줄 = 그 칸 무기). 비면 창이 없다
    TArray<FHDShipMenuOption> Options;
    // 함선 관리(브리프 6): 분야 이름(비면 OTHER), 단계 수, 산 단계 수([0, SubCount]), 확정을 길게 눌러야 하는가
    FString             Group;
    int32               SubCount        = 0;
    int32               ActiveSub       = 0;
    bool                bHoldToConfirm  = false;
};

// HandleKey 의 결과. Value 는 Adjust 의 −1 / +1
struct FHDShipMenuAction
{
    EHDShipActionType   Type    = EHDShipActionType::None;
    FName               Id;
    int32               Value   = 0;
    // Equip 의 선택지 Id
    FName               OptionId;
};

// 줄을 만드는 데 쓰는 값. 뒤 브리프가 필드를 더한다
struct FHDShipMenuContext
{
    FHDProfileData              Profile;
    // 난이도 표(UHDProgressionSettings::DifficultyLevels). 비면 Difficulty 줄을 넣지 않는다
    TArray<FHDDifficultyLevel>  Difficulties;
    // 업그레이드 카탈로그(컨트롤러가 FHDUpgradeCatalog::GetProjectCatalog 로 채운다). 없거나 비면 Upgrades 화면에 줄이 없다
    const FHDUpgradeCatalog*    Upgrades = nullptr;
    // 스트라타젬 카탈로그(컨트롤러가 FHDStratagemCatalog::GetProjectCatalog 로 채운다). 없으면 Loadout 화면은 READY 줄만, Armory 화면은 줄이 없다
    const FHDStratagemCatalog*  Stratagems = nullptr;
    // 프로필 로드아웃이 비거나 모자랄 때 채우는 값(컨트롤러가 UHDStratagemSettings::Loadout 으로 채운다)
    TArray<FName>               DefaultLoadout;
};

// 함선 메뉴 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.FrontEnd.ShipMenu)로 고정한다
namespace HDShipMenu
{
    // 눌림만 받는다: 방향키 = Up/Down/Left/Right, Enter = Confirm, BackSpace·Escape = Back, Q = PrevTab, E = NextTab
    HELLDIVERS_API bool                     TryGetMenuKey(const FKey& Key, EInputEvent Event, EHDShipMenuKey& OutKey);
    // Title 은 빈 배열(글자는 HUD 가 그린다). Bridge 는 Deploy, Difficulty(Context.Difficulties 가 있을 때, ← → 로 조정), Loadout, Upgrades, Armory, Quit.
    // Upgrades 는 카탈로그 행마다 "<DisplayName>  <산 단계>/<단계 수>"(확정 Select, 지불할 수 있을 때만 켜짐, 다 샀으면 꺼짐).
    // Loadout 은 정리한 로드아웃(HDArmory::ValidateLoadout)의 칸마다 Slot1~Slot4 "SLOT <n>  < <표시 이름> >"(← → 로 조정, 확정 None), 마지막에 READY(확정 Launch, 카탈로그가 없어도 있다).
    // 칸·무기 줄은 IconId 와 고르기 창 Options(칸 = 카탈로그 전체를 분류 순서로, 무기 = 그 칸 무기)를 채운다.
    // Armory 는 UnlockCost > 0 인 스트라타젬 행마다 "<표시 이름>  <가격> REQ" 또는 "<표시 이름>  UNLOCKED"(확정 Select, 잠겨 있고 징발이 충분할 때만 켜짐).
    // WarTable 은 Difficulty(Bridge 와 같은 줄, 표가 있을 때), StartMission(확정 OpenScreen → Loadout). System 은 Resume(확정 CloseTerminal), Quit
    HELLDIVERS_API TArray<FHDShipMenuItem>  BuildItems(EHDShipScreen Screen, const FHDShipMenuContext& Context);
    // 함선 자원 줄: "REQ <징발>   COMMON <일반>   RARE <희귀>   SUPER <최상급>"(사이 공백 세 칸)
    HELLDIVERS_API FString                  FormatResources(const FHDProfileData& Profile);
    // 로그용: "Title"·"Bridge"·"Upgrades"·"Loadout"·"Armory"·"WarTable"·"System" / "Up"… / "none"·"open"·"deploy"·"quit"·"adjust"·"select"·"close"·"launch"·"picker-open"·"picker-close"·"equip"
    HELLDIVERS_API const TCHAR*             GetScreenName(EHDShipScreen Screen);
    // 화면 머리글: "SHIP BRIDGE"·"SHIP UPGRADES"·"LOADOUT"·"ARMORY"·"WAR TABLE"·"SHIP MENU"(Title 은 빈 글자)
    HELLDIVERS_API const TCHAR*             GetScreenTitle(EHDShipScreen Screen);
    HELLDIVERS_API const TCHAR*             GetKeyName(EHDShipMenuKey Key);
    HELLDIVERS_API const TCHAR*             GetActionName(EHDShipActionType Type);
    // 지금 Confirm 이 행동을 만들고(None 아님) 그 줄이 bHoldToConfirm 이면 참(메뉴 사본으로 HandleKey 를 불러 본다)
    HELLDIVERS_API bool                     NeedsHold(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items);
    // 무기고 탭 이름(줄의 Group 값): STRATAGEMS·PRIMARY·SECONDARY·GRENADES
    HELLDIVERS_API const TArray<FString>&   GetArmoryTabs();
}

// 화면·커서·키 처리. 월드와 무관한 순수 로직이라 자동 테스트(Helldivers.FrontEnd.ShipMenu)로 고정한다
class HELLDIVERS_API FHDShipMenu
{
public:
    explicit            FHDShipMenu(EHDShipScreen StartScreen = EHDShipScreen::Title);

    EHDShipScreen       GetScreen() const { return Screen; }
    int32               GetCursor() const { return Cursor; }
    // 화면을 바꾸고 커서를 0 으로
    void                SetScreen(EHDShipScreen NewScreen);
    // 단말기를 연다: 화면·뿌리 = Root, 커서·뿌리 커서 = 0
    void                OpenTerminal(EHDShipScreen Root);
    // Back 이 돌아가는 화면. 기본은 Bridge
    EHDShipScreen       GetRootScreen() const { return RootScreen; }
    // 마우스가 가리킨 줄. [0, ItemCount) 밖이면 무시
    void                SetCursor(int32 NewCursor, int32 ItemCount);
    // Items = 지금 화면의 BuildItems 결과. 먼저 커서를 [0, Num−1] 로 자른다(빈 목록이면 0).
    // Back: 뿌리가 아니면 뿌리 화면·뿌리 커서로(OpenScreen, Id = 뿌리 이름), 뿌리에서는 Bridge 면 None, 단말기면 CloseTerminal(Id = 뿌리 이름)
    // Loadout 은 칸 이동(HDShipMenuNav)과 고르기 창 규칙이다. 다른 화면은 위 규칙 그대로
    FHDShipMenuAction   HandleKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items);

    // Loadout 고르기 창. SetScreen·OpenTerminal·SetCursor(받아들였을 때)가 닫는다
    bool                IsPickerOpen() const { return bPickerOpen; }
    int32               GetPickerCursor() const { return PickerCursor; }
    // 창이 열려 있고 [0, OptionCount) 안일 때만 바꾼다
    void                SetPickerCursor(int32 NewCursor, int32 OptionCount);
    // 함선 관리의 단계 카드(브리프 6). SetScreen·OpenTerminal·SetCursor 가 0 으로
    int32               GetSubCursor() const { return SubCursor; }
    // 마우스가 가리킨 단계 카드. 지금 줄의 [0, SubCount − 1] 로 자른다
    void                SetSubCursor(int32 NewSub, const TArray<FHDShipMenuItem>& Items);
    // 무기고 탭(브리프 7). SetScreen·OpenTerminal 이 0 으로
    int32               GetActiveTab() const { return ActiveTab; }
    // 마우스로 누른 탭. [0, 탭 수) 밖이면 무시. 커서를 그 탭 첫 줄로(빈 탭이면 그대로)
    void                SetActiveTab(int32 Tab, const TArray<FHDShipMenuItem>& Items);

private:
    // Upgrades 의 방향키(HDShipMenuNav::MakeUpgradeCells)와 단계 커서를 본 Confirm. 공통 규칙(Back, 산 단계의 Confirm)으로 넘길 키면 거짓
    bool                HandleUpgradeKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items, FHDShipMenuAction& OutAction);

    // Loadout 의 방향키·창 열기·창 안 키. 공통 규칙(READY Confirm, 창이 닫힌 Back)으로 넘길 키면 거짓
    bool                HandleLoadoutKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items, FHDShipMenuAction& OutAction);

    // Armory 탭·타일 이동. 공통 규칙(Confirm, Back)으로 넘길 키면 거짓
    bool                HandleArmoryKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items, FHDShipMenuAction& OutAction);

private:
    EHDShipScreen       Screen;
    int32               Cursor          = 0;
    bool                bPickerOpen     = false;
    int32               PickerCursor    = 0;
    int32               SubCursor       = 0;
    int32               ActiveTab       = 0;
    // 하위 화면에서 Back 하면 돌아갈 뿌리 화면의 커서(뿌리에서 하위 화면을 열 때 기억한다)
    int32               RootCursor      = 0;
    // 하위 화면에서 Back 하면 돌아갈 화면. 기본 Bridge, 단말기는 OpenTerminal 이 정한다
    EHDShipScreen       RootScreen      = EHDShipScreen::Bridge;
};
