// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "UI/HDShipMenu.h"

// 걸어 다니는 함선의 단말기
enum class EHDShipTerminal : uint8
{
    WarTable,
    ShipManagement,
    Armory
};

// 함선 방 기준(바닥 윗면 가운데가 원점, +X 가 창 쪽) 단말기 한 개
struct FHDShipTerminalInfo
{
    EHDShipTerminal     Terminal        = EHDShipTerminal::WarTable;
    FName               Id;
    FString             Label;
    EHDShipScreen       RootScreen      = EHDShipScreen::WarTable;
    // 본체 가운데(바닥 높이). 범위도 여기서 잰다
    FVector             LocalLocation   = FVector::ZeroVector;
    // 화면(앞면)이 보는 방향
    float               LocalYaw        = 0.f;
};

// 함선 방 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.FrontEnd.ShipHub)로 고정한다
namespace HDShipHub
{
    constexpr float                                 DefaultInteractRange = 300.f;

    // 작전 테이블·함선 관리·무기고(이 순서)
    HELLDIVERS_API const TArray<FHDShipTerminalInfo>& GetTerminals();
    // 표에서 Id 가 같은 행. 없으면 nullptr
    HELLDIVERS_API const FHDShipTerminalInfo*       FindTerminal(FName Id);
    // 방 안쪽 반 크기: X 2000, Y 1200, Z = 천장 높이 700
    HELLDIVERS_API FVector                          GetRoomHalfExtent();
    // (−1500, 0, 100), 방향 0(작전 테이블을 본다)
    HELLDIVERS_API FVector                          GetCrewSpawnLocation();
    HELLDIVERS_API float                            GetCrewSpawnYaw();
    // XY 거리만 본다. Range 안에서 가장 가까운 단말기의 번호, 없으면 INDEX_NONE
    HELLDIVERS_API int32                            FindTerminalInRange(const FVector& LocalLocation, const TArray<FHDShipTerminalInfo>& Terminals, float Range);
    // E 누름만
    HELLDIVERS_API bool                             IsInteractKey(const FKey& Key, EInputEvent Event);
    // "[E] WAR TABLE"
    HELLDIVERS_API FString                          FormatInteractPrompt(const FHDShipTerminalInfo& Terminal);
    // "HELLPOD LAUNCH IN <올림한 초>"
    HELLDIVERS_API FString                          FormatCountdown(float RemainingSeconds);
}

// 화면 태그 하나의 입력. HUD 가 매 프레임 만든다
struct FHDPromptTagInput
{
    FName               Id;
    FVector2D           ScreenAnchor    = FVector2D::ZeroVector;
    bool                bProjected      = false;    // 카메라 앞에서 투영됐다
    float               Distance2D      = 0.f;      // 폰 → 단말기 XY 거리
    bool                bInRange        = false;    // FindTerminalInRange 가 고른 단말기
    FVector2D           Size            = FVector2D::ZeroVector;   // 그릴 상자(px)
};

struct FHDPromptTag
{
    FName               Id;
    FBox2D              Rect            = FBox2D(ForceInit);
    bool                bVisible        = false;
    bool                bActive         = false;
    bool                bEdgeClamped    = false;
};

namespace HDShipHub
{
    constexpr float                 TableAnchorZ            = 300.f;
    constexpr float                 ConsoleAnchorZ          = 230.f;
    constexpr float                 DefaultTagShowDistance  = 1800.f;
    constexpr int32                 MaxTagPushes            = 4;

    // 방 기준 태그 기준점(단말기 위치 + Z). 작전 테이블 TableAnchorZ, 콘솔 ConsoleAnchorZ
    HELLDIVERS_API FVector                  GetPromptAnchor(const FHDShipTerminalInfo& Terminal);
    // 태그 자리 정하기(자동 테스트 Helldivers.FrontEnd.ShipPrompt). 결과는 입력과 같은 순서.
    // 활성(bInRange)은 늘 보이고 먼저 놓인다. 수동은 투영됐고 SafeRect 안이고 Distance2D <= ShowDistance 일 때만 보인다.
    // 상자 가운데 X = 투영점 X, 아래 변 = 투영점 Y - Gap 에서 시작해 SafeRect 안으로 밀고, 앞서 놓은 상자와 겹치면 위로(넘치면 아래로) 민다
    HELLDIVERS_API TArray<FHDPromptTag>     PlacePromptTags(const TArray<FHDPromptTagInput>& Inputs, const FBox2D& SafeRect, float Gap, float ShowDistance);
}

// 출격 대기(READY 뒤 카운트다운). 게임 모드가 Tick 에서 돌린다
class HELLDIVERS_API FHDLaunchCountdown
{
public:
    // 이미 돌고 있으면 거짓. 음수는 0 으로 자른다
    bool                Start(float Seconds);
    // 돌고 있었으면 참
    bool                Cancel();
    // 남은 시간이 0 이하가 된 틱에만 참을 돌려주고 꺼진다
    bool                Tick(float DeltaSeconds);
    bool                IsActive() const;
    // 꺼져 있으면 −1
    float               GetRemaining() const;
    // 올림. 꺼져 있으면 0
    int32               GetDisplaySeconds() const;

private:
    bool                bActive         = false;
    float               Remaining       = -1.f;
};
