// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipMenu.h"
#include "Progression/HDProgressionRules.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Progression/HDUpgradeRules.h"

namespace HDShipMenuItemId
{
    const FName Bridge(TEXT("Bridge"));
    const FName Deploy(TEXT("Deploy"));
    const FName Difficulty(TEXT("Difficulty"));
    const FName Upgrades(TEXT("Upgrades"));
    const FName Quit(TEXT("Quit"));
}

namespace HDShipMenuPrivate
{
    // Upgrades 화면: 카탈로그 파일 순서로 행마다 한 줄. 다음 단계가 있으면 지불할 수 있을 때만 켜짐, 다 샀으면 꺼짐
    TArray<FHDShipMenuItem> BuildUpgradeItems(const FHDShipMenuContext& Context)
    {
        TArray<FHDShipMenuItem> Items;
        if (Context.Upgrades == nullptr)
        {
            return Items;
        }

        for (const FName& UpgradeName : Context.Upgrades->GetNames())
        {
            const FHDUpgradeDefinition* Definition = Context.Upgrades->Find(UpgradeName);
            if (Definition == nullptr)
            {
                continue;
            }

            const int32 LevelCount = Definition->Levels.Num();
            const int32 Level = FMath::Clamp(HDUpgrade::GetLevel(Context.Profile, UpgradeName), 0, LevelCount);

            FHDShipMenuItem& Item   = Items.AddDefaulted_GetRef();
            Item.Id                 = UpgradeName;
            Item.Label              = FString::Printf(TEXT("%s  %d/%d"), *Definition->DisplayName, Level, LevelCount);
            Item.ConfirmAction      = EHDShipActionType::Select;

            const FHDUpgradeLevel* NextLevel = HDUpgrade::GetNextLevel(*Definition, Level);
            if (NextLevel)
            {
                const FHDCost Cost  = NextLevel->ToCost();
                Item.Detail         = FString::Printf(TEXT("NEXT  %s   COST %s"), *HDUpgrade::FormatEffect(Definition->Effect, NextLevel->Value), *HDUpgrade::FormatCost(Cost));
                Item.bEnabled       = HDProgression::CanAfford(Context.Profile, Cost);
            }
            else
            {
                Item.Detail         = TEXT("MAX LEVEL");
                if (Level > 0)
                {
                    Item.Detail += FString::Printf(TEXT("   %s"), *HDUpgrade::FormatEffect(Definition->Effect, Definition->Levels[Level - 1].Value));
                }
                Item.bEnabled       = false;
            }
        }

        return Items;
    }
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
    if (Screen == EHDShipScreen::Upgrades)
    {
        return HDShipMenuPrivate::BuildUpgradeItems(Context);
    }

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

    FHDShipMenuItem& UpgradesItem   = Items.AddDefaulted_GetRef();
    UpgradesItem.Id                 = HDShipMenuItemId::Upgrades;
    UpgradesItem.Label              = TEXT("SHIP UPGRADES");
    UpgradesItem.Detail             = TEXT("SPEND REQUISITION AND SAMPLES ON SHIP MODULES");
    UpgradesItem.ConfirmAction      = EHDShipActionType::OpenScreen;
    UpgradesItem.TargetScreen       = EHDShipScreen::Upgrades;

    // 뒤 브리프(7)가 Upgrades 와 Quit 사이에 줄을 더한다

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
    case EHDShipScreen::Upgrades:   return TEXT("Upgrades");
    default:                        return TEXT("?");
    }
}

const TCHAR* HDShipMenu::GetScreenTitle(EHDShipScreen Screen)
{
    switch (Screen)
    {
    case EHDShipScreen::Bridge:     return TEXT("SHIP BRIDGE");
    case EHDShipScreen::Upgrades:   return TEXT("SHIP UPGRADES");
    default:                        return TEXT("");
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
