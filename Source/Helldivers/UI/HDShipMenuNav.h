// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/HDShipMenu.h"

// 격자 화면의 칸 하나(열·행과 차지하는 폭)
struct FHDNavCell
{
    int32               ItemIndex   = INDEX_NONE;   // 줄 번호(고르기 창·무기고 타일은 선택지 번호)
    int32               SubIndex    = 0;            // 함선 관리의 단계 카드(브리프 6)
    int32               Col         = 0;
    int32               Row         = 0;
    int32               ColSpan     = 1;
    int32               RowSpan     = 1;
};

// 칸 사이 방향키 이동. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.FrontEnd.LoadoutScreen)로 고정한다
namespace HDShipMenuNav
{
    // 방향 쪽에 있는 칸 중 가장 가까운 칸의 번호. 없으면 Current. 감지 않는다.
    // Right 기준: 후보는 Col ≥ 지금.Col + 지금.ColSpan, 고르는 기준은 (수직 거리, 수평 거리, 칸 번호) 사전 순서. 나머지 방향은 대칭
    HELLDIVERS_API int32                Move(const TArray<FHDNavCell>& Cells, int32 Current, EHDShipMenuKey Direction);
    // ItemIndex·SubIndex 가 같은 칸의 번호. 없으면 INDEX_NONE
    HELLDIVERS_API int32                FindCell(const TArray<FHDNavCell>& Cells, int32 ItemIndex, int32 SubIndex = 0);
    // 선택지를 분류가 바뀔 때마다 새 행으로, 한 분류 안에서는 Columns 열로
    HELLDIVERS_API TArray<FHDNavCell>   MakePickerCells(const TArray<FHDShipMenuOption>& Options, int32 Columns);
    // Loadout 고정 배치: Slot1~Slot4 = (0~3, 0), Primary·Secondary·Grenade = (0~2, 1), Ready = (4, 0) 행 폭 2. 모르는 Id 는 행 2 에 차례로
    HELLDIVERS_API TArray<FHDNavCell>   MakeLoadoutCells(const TArray<FHDShipMenuItem>& Items);
    // 함선 관리 배치: 분야(Group)를 처음 나온 순서로 열 묶음 g, 가장 많은 단계 수 T 일 때 단계 k 카드 = (g × T + k, 분야 안 순번). SubCount 0 인 줄은 칸이 없다
    HELLDIVERS_API TArray<FHDNavCell>   MakeUpgradeCells(const TArray<FHDShipMenuItem>& Items);
}
