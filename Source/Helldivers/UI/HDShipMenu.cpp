// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipMenu.h"

namespace HDShipMenuItemId
{
    const FName Bridge(TEXT("Bridge"));
    const FName Deploy(TEXT("Deploy"));
    const FName Difficulty(TEXT("Difficulty"));
    const FName Quit(TEXT("Quit"));
}

bool HDShipMenu::TryGetMenuKey(const FKey& Key, EInputEvent Event, EHDShipMenuKey& OutKey)
{
    if (Event != IE_Pressed)
    {
        return false;
    }

    if (Key == EKeys::Up)
    {
        OutKey = EHDShipMenuKey::Up;
        return true;
    }

    if (Key == EKeys::Down)
    {
        OutKey = EHDShipMenuKey::Down;
        return true;
    }

    if (Key == EKeys::Left)
    {
        OutKey = EHDShipMenuKey::Left;
        return true;
    }

    if (Key == EKeys::Right)
    {
        OutKey = EHDShipMenuKey::Right;
        return true;
    }

    if (Key == EKeys::Enter)
    {
        OutKey = EHDShipMenuKey::Confirm;
        return true;
    }

    if (Key == EKeys::BackSpace || Key == EKeys::Escape)
    {
        OutKey = EHDShipMenuKey::Back;
        return true;
    }

    return false;
}

TArray<FHDShipMenuItem> HDShipMenu::BuildItems(EHDShipScreen Screen, const FHDShipMenuContext& Context)
{
    TArray<FHDShipMenuItem> Items;
    if (Screen != EHDShipScreen::Bridge)
    {
        return Items;
    }

    FHDShipMenuItem& DeployItem = Items.AddDefaulted_GetRef();
    DeployItem.Id               = HDShipMenuItemId::Deploy;
    DeployItem.Label            = TEXT("DEPLOY");
    DeployItem.Detail           = TEXT("LAUNCH HELLPOD TO THE MISSION AREA");
    DeployItem.ConfirmAction    = EHDShipActionType::Deploy;

    // ← → 로 프로필 난이도를 바꾼다(컨트롤러가 처리). 확정은 아무것도 하지 않는다
    if (Context.Difficulties.IsEmpty() == false)
    {
        const int32 Level = HDDifficulty::ClampLevel(Context.Profile.Difficulty, Context.Difficulties.Num());
        const FHDDifficultyLevel Row = HDDifficulty::GetLevel(Context.Difficulties, Level);

        FHDShipMenuItem& DifficultyItem = Items.AddDefaulted_GetRef();
        DifficultyItem.Id               = HDShipMenuItemId::Difficulty;
        DifficultyItem.Label            = HDDifficulty::FormatMenuLabel(Level, Row);
        DifficultyItem.Detail           = HDDifficulty::FormatDetail(Row);
        DifficultyItem.bAdjustable      = true;
        DifficultyItem.ConfirmAction    = EHDShipActionType::None;
    }

    // 뒤 브리프(6·7)가 Difficulty 와 Quit 사이에 줄을 더한다

    FHDShipMenuItem& QuitItem   = Items.AddDefaulted_GetRef();
    QuitItem.Id                 = HDShipMenuItemId::Quit;
    QuitItem.Label              = TEXT("QUIT GAME");
    QuitItem.Detail             = TEXT("EXIT TO DESKTOP");
    QuitItem.ConfirmAction      = EHDShipActionType::Quit;

    return Items;
}

FString HDShipMenu::FormatResources(const FHDProfileData& Profile)
{
    return FString::Printf(TEXT("REQ %d   COMMON %d   RARE %d   SUPER %d"),
        Profile.Requisition,
        Profile.Samples.Common,
        Profile.Samples.Rare,
        Profile.Samples.Super);
}

const TCHAR* HDShipMenu::GetScreenName(EHDShipScreen Screen)
{
    switch (Screen)
    {
    case EHDShipScreen::Title:      return TEXT("Title");
    case EHDShipScreen::Bridge:     return TEXT("Bridge");
    default:                        return TEXT("?");
    }
}

const TCHAR* HDShipMenu::GetKeyName(EHDShipMenuKey Key)
{
    switch (Key)
    {
    case EHDShipMenuKey::Up:        return TEXT("Up");
    case EHDShipMenuKey::Down:      return TEXT("Down");
    case EHDShipMenuKey::Left:      return TEXT("Left");
    case EHDShipMenuKey::Right:     return TEXT("Right");
    case EHDShipMenuKey::Confirm:   return TEXT("Confirm");
    case EHDShipMenuKey::Back:      return TEXT("Back");
    default:                        return TEXT("?");
    }
}

const TCHAR* HDShipMenu::GetActionName(EHDShipActionType Type)
{
    switch (Type)
    {
    case EHDShipActionType::None:       return TEXT("none");
    case EHDShipActionType::OpenScreen: return TEXT("open");
    case EHDShipActionType::Deploy:     return TEXT("deploy");
    case EHDShipActionType::Quit:       return TEXT("quit");
    case EHDShipActionType::Adjust:     return TEXT("adjust");
    case EHDShipActionType::Select:     return TEXT("select");
    default:                            return TEXT("?");
    }
}

FHDShipMenu::FHDShipMenu(EHDShipScreen StartScreen)
    : Screen(StartScreen)
{
}

void FHDShipMenu::SetScreen(EHDShipScreen NewScreen)
{
    Screen = NewScreen;
    Cursor = 0;
}

void FHDShipMenu::SetCursor(int32 NewCursor, int32 ItemCount)
{
    if (NewCursor < 0 || NewCursor >= ItemCount)
    {
        return;
    }

    Cursor = NewCursor;
}

FHDShipMenuAction FHDShipMenu::HandleKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items)
{
    // 줄 수가 바뀌었을 수 있으므로 먼저 자른다
    const int32 ItemCount = Items.Num();
    Cursor = ItemCount > 0 ? FMath::Clamp(Cursor, 0, ItemCount - 1) : 0;

    FHDShipMenuAction Action;

    // 타이틀은 Confirm 만 받는다
    if (Screen == EHDShipScreen::Title)
    {
        if (Key == EHDShipMenuKey::Confirm)
        {
            SetScreen(EHDShipScreen::Bridge);
            Action.Type = EHDShipActionType::OpenScreen;
            Action.Id   = HDShipMenuItemId::Bridge;
        }
        return Action;
    }

    if (Key == EHDShipMenuKey::Back)
    {
        // 하위 화면에서는 Bridge 로, 들어갈 때의 커서로 돌아간다. Bridge 에서는 아무것도 하지 않는다
        if (Screen != EHDShipScreen::Bridge)
        {
            Screen      = EHDShipScreen::Bridge;
            Cursor      = BridgeCursor;
            Action.Type = EHDShipActionType::OpenScreen;
            Action.Id   = HDShipMenuItemId::Bridge;
        }
        return Action;
    }

    if (ItemCount == 0)
    {
        return Action;
    }

    const FHDShipMenuItem& Item = Items[Cursor];
    switch (Key)
    {
    case EHDShipMenuKey::Up:
        Cursor = (Cursor + ItemCount - 1) % ItemCount;
        break;

    case EHDShipMenuKey::Down:
        Cursor = (Cursor + 1) % ItemCount;
        break;

    case EHDShipMenuKey::Left:
    case EHDShipMenuKey::Right:
        if (Item.bAdjustable && Item.bEnabled)
        {
            Action.Type     = EHDShipActionType::Adjust;
            Action.Id       = Item.Id;
            Action.Value    = Key == EHDShipMenuKey::Right ? 1 : -1;
        }
        break;

    case EHDShipMenuKey::Confirm:
        if (Item.bEnabled == false)
        {
            break;
        }

        if (Item.ConfirmAction == EHDShipActionType::OpenScreen)
        {
            if (Screen == EHDShipScreen::Bridge)
            {
                BridgeCursor = Cursor;
            }

            // SetScreen 이 커서를 바꾸기 전에 Id 를 담는다
            Action.Type = EHDShipActionType::OpenScreen;
            Action.Id   = Item.Id;
            SetScreen(Item.TargetScreen);
            break;
        }

        Action.Type     = Item.ConfirmAction;
        Action.Id       = Item.Id;
        Action.Value    = Item.ConfirmAction == EHDShipActionType::Adjust ? 1 : 0;
        break;

    default:
        break;
    }

    return Action;
}
