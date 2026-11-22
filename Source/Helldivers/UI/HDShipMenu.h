// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "Progression/HDProfileTypes.h"
#include "Progression/HDDifficulty.h"

class FHDUpgradeCatalog;
class FHDStratagemCatalog;

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
    Back
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
    Launch
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
};

// HandleKey 의 결과. Value 는 Adjust 의 −1 / +1
struct FHDShipMenuAction
{
    EHDShipActionType   Type    = EHDShipActionType::None;
    FName               Id;
    int32               Value   = 0;
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
    // 눌림만 받는다: 방향키 = Up/Down/Left/Right, Enter = Confirm, BackSpace·Escape = Back
    HELLDIVERS_API bool                     TryGetMenuKey(const FKey& Key, EInputEvent Event, EHDShipMenuKey& OutKey);
    // Title 은 빈 배열(글자는 HUD 가 그린다). Bridge 는 Deploy, Difficulty(Context.Difficulties 가 있을 때, ← → 로 조정), Loadout, Upgrades, Armory, Quit.
    // Upgrades 는 카탈로그 행마다 "<DisplayName>  <산 단계>/<단계 수>"(확정 Select, 지불할 수 있을 때만 켜짐, 다 샀으면 꺼짐).
    // Loadout 은 정리한 로드아웃(HDArmory::ValidateLoadout)의 칸마다 Slot1~Slot4 "SLOT <n>  < <표시 이름> >"(← → 로 조정, 확정 None), 마지막에 READY(확정 Launch, 카탈로그가 없어도 있다).
    // Armory 는 UnlockCost > 0 인 스트라타젬 행마다 "<표시 이름>  <가격> REQ" 또는 "<표시 이름>  UNLOCKED"(확정 Select, 잠겨 있고 징발이 충분할 때만 켜짐).
    // WarTable 은 Difficulty(Bridge 와 같은 줄, 표가 있을 때), StartMission(확정 OpenScreen → Loadout). System 은 Resume(확정 CloseTerminal), Quit
    HELLDIVERS_API TArray<FHDShipMenuItem>  BuildItems(EHDShipScreen Screen, const FHDShipMenuContext& Context);
    // 함선 자원 줄: "REQ <징발>   COMMON <일반>   RARE <희귀>   SUPER <최상급>"(사이 공백 세 칸)
    HELLDIVERS_API FString                  FormatResources(const FHDProfileData& Profile);
    // 로그용: "Title"·"Bridge"·"Upgrades"·"Loadout"·"Armory"·"WarTable"·"System" / "Up"… / "none"·"open"·"deploy"·"quit"·"adjust"·"select"·"close"·"launch"
    HELLDIVERS_API const TCHAR*             GetScreenName(EHDShipScreen Screen);
    // 화면 머리글: "SHIP BRIDGE"·"SHIP UPGRADES"·"LOADOUT"·"ARMORY"·"WAR TABLE"·"SHIP MENU"(Title 은 빈 글자)
    HELLDIVERS_API const TCHAR*             GetScreenTitle(EHDShipScreen Screen);
    HELLDIVERS_API const TCHAR*             GetKeyName(EHDShipMenuKey Key);
    HELLDIVERS_API const TCHAR*             GetActionName(EHDShipActionType Type);
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
    FHDShipMenuAction   HandleKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items);

private:
    EHDShipScreen       Screen;
    int32               Cursor          = 0;
    // 하위 화면에서 Back 하면 돌아갈 뿌리 화면의 커서(뿌리에서 하위 화면을 열 때 기억한다)
    int32               RootCursor      = 0;
    // 하위 화면에서 Back 하면 돌아갈 화면. 기본 Bridge, 단말기는 OpenTerminal 이 정한다
    EHDShipScreen       RootScreen      = EHDShipScreen::Bridge;
};
