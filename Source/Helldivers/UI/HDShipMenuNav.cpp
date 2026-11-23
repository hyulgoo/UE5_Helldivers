// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipMenuNav.h"

namespace HDShipMenuNavPrivate
{
    // 두 칸 범위 [A1, A2]·[B1, B2] 의 거리. 겹치면 0
    int32 RangeGap(int32 A1, int32 A2, int32 B1, int32 B2)
    {
        return FMath::Max(0, FMath::Max(B1 - A2, A1 - B2));
    }

    FHDNavCell MakeCell(int32 ItemIndex, int32 Col, int32 Row, int32 RowSpan = 1)
    {
        FHDNavCell Cell;
        Cell.ItemIndex  = ItemIndex;
        Cell.Col        = Col;
        Cell.Row        = Row;
        Cell.RowSpan    = RowSpan;
        return Cell;
    }
}

int32 HDShipMenuNav::Move(const TArray<FHDNavCell>& Cells, int32 Current, EHDShipMenuKey Direction)
{
    using namespace HDShipMenuNavPrivate;

    if (Cells.IsValidIndex(Current) == false)
    {
        return Current;
    }

    const FHDNavCell& From = Cells[Current];
    const int32 FromColEnd = From.Col + From.ColSpan - 1;
    const int32 FromRowEnd = From.Row + From.RowSpan - 1;

    int32 Best = Current;
    int32 BestCross = MAX_int32;
    int32 BestAlong = MAX_int32;
    for (int32 Index = 0; Index < Cells.Num(); ++Index)
    {
        if (Index == Current)
        {
            continue;
        }

        const FHDNavCell& To = Cells[Index];
        const int32 ToColEnd = To.Col + To.ColSpan - 1;
        const int32 ToRowEnd = To.Row + To.RowSpan - 1;

        // Along = 방향 쪽 거리(음수면 후보 아님), Cross = 가로지르는 쪽 범위 거리
        int32 Along = -1;
        int32 Cross = 0;
        switch (Direction)
        {
        case EHDShipMenuKey::Right:
            Along = To.Col - (FromColEnd + 1);
            Cross = RangeGap(From.Row, FromRowEnd, To.Row, ToRowEnd);
            break;
        case EHDShipMenuKey::Left:
            Along = From.Col - (ToColEnd + 1);
            Cross = RangeGap(From.Row, FromRowEnd, To.Row, ToRowEnd);
            break;
        case EHDShipMenuKey::Down:
            Along = To.Row - (FromRowEnd + 1);
            Cross = RangeGap(From.Col, FromColEnd, To.Col, ToColEnd);
            break;
        case EHDShipMenuKey::Up:
            Along = From.Row - (ToRowEnd + 1);
            Cross = RangeGap(From.Col, FromColEnd, To.Col, ToColEnd);
            break;
        default:
            return Current;
        }

        if (Along < 0)
        {
            continue;
        }

        // 사전 순서 (Cross, Along, 칸 번호). 번호는 오름차순으로 돌므로 같으면 앞 칸이 남는다
        if (Cross < BestCross || (Cross == BestCross && Along < BestAlong))
        {
            Best        = Index;
            BestCross   = Cross;
            BestAlong   = Along;
        }
    }

    return Best;
}

int32 HDShipMenuNav::FindCell(const TArray<FHDNavCell>& Cells, int32 ItemIndex, int32 SubIndex)
{
    return Cells.IndexOfByPredicate([ItemIndex, SubIndex](const FHDNavCell& Cell) { return Cell.ItemIndex == ItemIndex && Cell.SubIndex == SubIndex; });
}

TArray<FHDNavCell> HDShipMenuNav::MakePickerCells(const TArray<FHDShipMenuOption>& Options, int32 Columns)
{
    using namespace HDShipMenuNavPrivate;

    const int32 ColumnCount = FMath::Max(1, Columns);
    TArray<FHDNavCell> Cells;
    int32 Row = -1;
    int32 Col = 0;
    for (int32 Index = 0; Index < Options.Num(); ++Index)
    {
        const bool bNewCategory = Index == 0 || Options[Index].Category != Options[Index - 1].Category;
        if (bNewCategory || Col >= ColumnCount)
        {
            ++Row;
            Col = 0;
        }

        Cells.Add(MakeCell(Index, Col, Row));
        ++Col;
    }
    return Cells;
}

TArray<FHDNavCell> HDShipMenuNav::MakeLoadoutCells(const TArray<FHDShipMenuItem>& Items)
{
    using namespace HDShipMenuNavPrivate;

    // HDShipMenu::BuildItems 의 Loadout 줄 Id
    static const FName SlotIds[4] = { FName(TEXT("Slot1")), FName(TEXT("Slot2")), FName(TEXT("Slot3")), FName(TEXT("Slot4")) };
    static const FName WeaponIds[3] = { FName(TEXT("Primary")), FName(TEXT("Secondary")), FName(TEXT("Grenade")) };
    static const FName ReadyId(TEXT("Ready"));

    TArray<FHDNavCell> Cells;
    int32 UnknownCol = 0;
    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        const FName Id = Items[Index].Id;
        int32 Col = INDEX_NONE;
        int32 Row = 0;
        int32 RowSpan = 1;
        for (int32 Slot = 0; Slot < 4; ++Slot)
        {
            if (Id == SlotIds[Slot])
            {
                Col = Slot;
            }
        }
        for (int32 Weapon = 0; Weapon < 3; ++Weapon)
        {
            if (Id == WeaponIds[Weapon])
            {
                Col = Weapon;
                Row = 1;
            }
        }
        if (Id == ReadyId)
        {
            Col = 4;
            RowSpan = 2;
        }
        if (Col == INDEX_NONE)
        {
            Col = UnknownCol++;
            Row = 2;
        }

        Cells.Add(MakeCell(Index, Col, Row, RowSpan));
    }
    return Cells;
}
