// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipMenu.h"
#include "Progression/HDArmoryRules.h"
#include "Progression/HDProgressionRules.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Progression/HDUpgradeRules.h"
#include "Progression/HDWeaponCatalog.h"
#include "Stratagem/HDStratagemCatalog.h"

namespace HDShipMenuItemId
{
    const FName Bridge(TEXT("Bridge"));
    const FName Deploy(TEXT("Deploy"));
    const FName Difficulty(TEXT("Difficulty"));
    const FName Loadout(TEXT("Loadout"));
    const FName Upgrades(TEXT("Upgrades"));
    const FName Armory(TEXT("Armory"));
    const FName Quit(TEXT("Quit"));
    const FName StartMission(TEXT("StartMission"));
    const FName Ready(TEXT("Ready"));
    const FName Resume(TEXT("Resume"));
}

namespace HDShipMenuPrivate
{
    // 초 글자: 정수면 "10", 아니면 "0.5"
    FString FormatSeconds(const float Seconds)
    {
        const int32 Rounded = FMath::RoundToInt(Seconds);
        return FMath::IsNearlyEqual(Seconds, static_cast<float>(Rounded))
            ? FString::Printf(TEXT("%d"), Rounded)
            : FString::Printf(TEXT("%.1f"), Seconds);
    }

    // 프로필 로드아웃을 정리한 것(출격 칸과 같은 규칙). 카탈로그가 없으면 빈 배열
    TArray<FName> GetValidatedLoadout(const FHDShipMenuContext& Context)
    {
        if (Context.Stratagems == nullptr)
        {
            return TArray<FName>();
        }

        return HDArmory::ValidateLoadout(Context.Profile.Loadout, Context.Profile, *Context.Stratagems, Context.DefaultLoadout);
    }

    // Difficulty 줄(Bridge·WarTable 공용). ← → 로 프로필 난이도를 바꾼다(컨트롤러가 처리). 확정은 아무것도 하지 않는다.
    // 난이도 표가 비면 넣지 않는다
    void AddDifficultyItem(TArray<FHDShipMenuItem>& Items, const FHDShipMenuContext& Context)
    {
        if (Context.Difficulties.IsEmpty())
        {
            return;
        }

        const int32 Level = HDDifficulty::ClampLevel(Context.Profile.Difficulty, Context.Difficulties.Num());
        const FHDDifficultyLevel Row = HDDifficulty::GetLevel(Context.Difficulties, Level);

        FHDShipMenuItem& DifficultyItem = Items.AddDefaulted_GetRef();
        DifficultyItem.Id               = HDShipMenuItemId::Difficulty;
        DifficultyItem.Label            = HDDifficulty::FormatMenuLabel(Level, Row);
        DifficultyItem.Detail           = HDDifficulty::FormatDetail(Row);
        DifficultyItem.bAdjustable      = true;
        DifficultyItem.ConfirmAction    = EHDShipActionType::None;
    }

    // QUIT GAME 줄(Bridge·System 공용)
    void AddQuitItem(TArray<FHDShipMenuItem>& Items)
    {
        FHDShipMenuItem& QuitItem   = Items.AddDefaulted_GetRef();
        QuitItem.Id                 = HDShipMenuItemId::Quit;
        QuitItem.Label              = TEXT("QUIT GAME");
        QuitItem.Detail             = TEXT("EXIT TO DESKTOP");
        QuitItem.ConfirmAction      = EHDShipActionType::Quit;
    }

    // Loadout 화면의 무기 세 줄(Primary·Secondary·Grenade, 브리프 9): "PRIMARY  < <DisplayName> >"(GetChosen 값).
    // ← → 는 Adjust(컨트롤러가 그 칸의 열린 무기를 돌린다), 확정은 아무것도 하지 않는다
    void AddWeaponLoadoutItems(TArray<FHDShipMenuItem>& Items, const FHDShipMenuContext& Context)
    {
        for (const EHDLoadoutWeapon Slot : { EHDLoadoutWeapon::Primary, EHDLoadoutWeapon::Secondary, EHDLoadoutWeapon::Grenade })
        {
            const FHDWeaponOption* Option = HDWeaponCatalog::Find(HDWeaponCatalog::GetChosen(Context.Profile, Slot));
            if (Option == nullptr)
            {
                continue;
            }

            const FString SlotName  = HDWeaponCatalog::GetSlotName(Slot);

            FHDShipMenuItem& Item   = Items.AddDefaulted_GetRef();
            Item.Id                 = FName(*SlotName);
            Item.Label              = FString::Printf(TEXT("%s  < %s >"), *SlotName.ToUpper(), *Option->DisplayName);
            Item.Detail             = Option->Detail;
            Item.bAdjustable        = true;
            Item.ConfirmAction      = EHDShipActionType::None;
        }
    }

    // Armory 화면의 무기 줄(브리프 9): 표 순서로 가격 > 0 인 무기마다 "<DisplayName>  <가격> REQ" 또는 "<DisplayName>  UNLOCKED".
    // 잠겨 있고 징발이 충분할 때만 켜짐, 해금했으면 꺼짐
    void AddWeaponArmoryItems(TArray<FHDShipMenuItem>& Items, const FHDShipMenuContext& Context)
    {
        for (const FHDWeaponOption& Option : HDWeaponCatalog::GetOptions())
        {
            if (Option.UnlockCost <= 0)
            {
                continue;
            }

            const bool bUnlocked    = HDWeaponCatalog::IsUnlocked(Context.Profile, Option.Id);

            FHDCost Cost;
            Cost.Requisition = Option.UnlockCost;

            FHDShipMenuItem& Item   = Items.AddDefaulted_GetRef();
            Item.Id                 = Option.Id;
            Item.Label              = bUnlocked
                ? FString::Printf(TEXT("%s  UNLOCKED"), *Option.DisplayName)
                : FString::Printf(TEXT("%s  %d REQ"), *Option.DisplayName, Option.UnlockCost);
            Item.Detail             = Option.Detail;
            Item.bEnabled           = bUnlocked == false && HDProgression::CanAfford(Context.Profile, Cost);
            Item.ConfirmAction      = EHDShipActionType::Select;
        }
    }

    // Loadout 화면: 정리한 로드아웃의 칸마다 한 줄(Slot1~Slot4). ← → 는 Adjust(컨트롤러가 해금된 이름을 돌린다), 확정은 아무것도 하지 않는다.
    // 설명의 값은 정의 값이다(업그레이드 적용 전). 칸 줄 뒤에 무기 세 줄을 붙인다
    TArray<FHDShipMenuItem> BuildLoadoutItems(const FHDShipMenuContext& Context)
    {
        TArray<FHDShipMenuItem> Items;
        if (Context.Stratagems == nullptr)
        {
            return Items;
        }

        const TArray<FName> Loadout = GetValidatedLoadout(Context);
        for (int32 SlotIndex = 0; SlotIndex < Loadout.Num(); ++SlotIndex)
        {
            const FName StratagemName = Loadout[SlotIndex];

            FHDShipMenuItem& Item   = Items.AddDefaulted_GetRef();
            Item.Id                 = FName(*FString::Printf(TEXT("Slot%d"), SlotIndex + 1));
            Item.Label              = FString::Printf(TEXT("SLOT %d  < %s >"), SlotIndex + 1, *HDArmory::MakeDisplayName(StratagemName));
            Item.bAdjustable        = true;
            Item.ConfirmAction      = EHDShipActionType::None;

            const FHDStratagemDefinition* Definition = Context.Stratagems->Find(StratagemName);
            if (Definition)
            {
                Item.Detail = FString::Printf(TEXT("COOLDOWN %sS   USES %s   CALL-IN %sS   %s"),
                    *FormatSeconds(Definition->Cooldown),
                    Definition->MaxUses > 0 ? *FString::FromInt(Definition->MaxUses) : TEXT("INF"),
                    *FormatSeconds(Definition->ActivationDelay),
                    *HDArmory::FormatCommand(Definition->CommandSequence));
            }
        }

        AddWeaponLoadoutItems(Items, Context);
        return Items;
    }

    // Armory 화면: 카탈로그 파일 순서로 UnlockCost > 0 인 행마다 한 줄. 잠겨 있고 징발이 충분할 때만 켜짐, 해금했으면 꺼짐.
    // 스트라타젬 줄 뒤에 무기 줄을 붙인다. 스트라타젬 카탈로그가 없으면 Loadout 과 같이 무기 줄도 넣지 않는다(헤더 BuildItems 설명 그대로)
    TArray<FHDShipMenuItem> BuildArmoryItems(const FHDShipMenuContext& Context)
    {
        TArray<FHDShipMenuItem> Items;
        if (Context.Stratagems == nullptr)
        {
            return Items;
        }

        for (const FName& StratagemName : Context.Stratagems->GetNames())
        {
            const FHDStratagemDefinition* Definition = Context.Stratagems->Find(StratagemName);
            if (Definition == nullptr || Definition->UnlockCost <= 0)
            {
                continue;
            }

            const bool bUnlocked    = HDArmory::IsUnlocked(Context.Profile, *Context.Stratagems, StratagemName);
            const FString Name      = HDArmory::MakeDisplayName(StratagemName);

            FHDCost Cost;
            Cost.Requisition = Definition->UnlockCost;

            FHDShipMenuItem& Item   = Items.AddDefaulted_GetRef();
            Item.Id                 = StratagemName;
            Item.Label              = bUnlocked
                ? FString::Printf(TEXT("%s  UNLOCKED"), *Name)
                : FString::Printf(TEXT("%s  %d REQ"), *Name, Definition->UnlockCost);
            Item.Detail             = FString::Printf(TEXT("COOLDOWN %sS   CALL-IN %sS   %s"),
                *FormatSeconds(Definition->Cooldown),
                *FormatSeconds(Definition->ActivationDelay),
                *HDArmory::FormatCommand(Definition->CommandSequence));
            Item.bEnabled           = bUnlocked == false && HDProgression::CanAfford(Context.Profile, Cost);
            Item.ConfirmAction      = EHDShipActionType::Select;
        }

        AddWeaponArmoryItems(Items, Context);
        return Items;
    }

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

    if (Screen == EHDShipScreen::Loadout)
    {
        TArray<FHDShipMenuItem> Items = HDShipMenuPrivate::BuildLoadoutItems(Context);

        // 칸 줄 뒤에 READY. 어느 모드(함선 안·Bridge)에서 열어도 출격 대기를 시작한다
        FHDShipMenuItem& ReadyItem  = Items.AddDefaulted_GetRef();
        ReadyItem.Id                = HDShipMenuItemId::Ready;
        ReadyItem.Label             = TEXT("READY");
        ReadyItem.Detail            = TEXT("START THE HELLPOD LAUNCH COUNTDOWN");
        ReadyItem.ConfirmAction     = EHDShipActionType::Launch;
        return Items;
    }

    if (Screen == EHDShipScreen::Armory)
    {
        return HDShipMenuPrivate::BuildArmoryItems(Context);
    }

    TArray<FHDShipMenuItem> Items;
    if (Screen == EHDShipScreen::WarTable)
    {
        HDShipMenuPrivate::AddDifficultyItem(Items, Context);

        FHDShipMenuItem& StartItem  = Items.AddDefaulted_GetRef();
        StartItem.Id                = HDShipMenuItemId::StartMission;
        StartItem.Label             = TEXT("START MISSION");
        StartItem.Detail            = TEXT("CHOOSE YOUR LOADOUT, THEN READY UP");
        StartItem.ConfirmAction     = EHDShipActionType::OpenScreen;
        StartItem.TargetScreen      = EHDShipScreen::Loadout;
        return Items;
    }

    if (Screen == EHDShipScreen::System)
    {
        FHDShipMenuItem& ResumeItem = Items.AddDefaulted_GetRef();
        ResumeItem.Id               = HDShipMenuItemId::Resume;
        ResumeItem.Label            = TEXT("RESUME");
        ResumeItem.Detail           = TEXT("BACK TO THE SHIP");
        ResumeItem.ConfirmAction    = EHDShipActionType::CloseTerminal;

        HDShipMenuPrivate::AddQuitItem(Items);
        return Items;
    }

    if (Screen != EHDShipScreen::Bridge)
    {
        return Items;
    }

    FHDShipMenuItem& DeployItem = Items.AddDefaulted_GetRef();
    DeployItem.Id               = HDShipMenuItemId::Deploy;
    DeployItem.Label            = TEXT("DEPLOY");
    DeployItem.Detail           = TEXT("LAUNCH HELLPOD TO THE MISSION AREA");
    DeployItem.ConfirmAction    = EHDShipActionType::Deploy;

    HDShipMenuPrivate::AddDifficultyItem(Items, Context);

    // 설명 = 지금 로드아웃(출격할 때의 칸)의 표시 이름
    TArray<FString> LoadoutNames;
    for (const FName& StratagemName : HDShipMenuPrivate::GetValidatedLoadout(Context))
    {
        LoadoutNames.Add(HDArmory::MakeDisplayName(StratagemName));
    }

    FHDShipMenuItem& LoadoutItem    = Items.AddDefaulted_GetRef();
    LoadoutItem.Id                  = HDShipMenuItemId::Loadout;
    LoadoutItem.Label               = TEXT("LOADOUT");
    LoadoutItem.Detail              = FString::Join(LoadoutNames, TEXT(" / "));
    LoadoutItem.ConfirmAction       = EHDShipActionType::OpenScreen;
    LoadoutItem.TargetScreen        = EHDShipScreen::Loadout;

    FHDShipMenuItem& UpgradesItem   = Items.AddDefaulted_GetRef();
    UpgradesItem.Id                 = HDShipMenuItemId::Upgrades;
    UpgradesItem.Label              = TEXT("SHIP UPGRADES");
    UpgradesItem.Detail             = TEXT("SPEND REQUISITION AND SAMPLES ON SHIP MODULES");
    UpgradesItem.ConfirmAction      = EHDShipActionType::OpenScreen;
    UpgradesItem.TargetScreen       = EHDShipScreen::Upgrades;

    FHDShipMenuItem& ArmoryItem     = Items.AddDefaulted_GetRef();
    ArmoryItem.Id                   = HDShipMenuItemId::Armory;
    ArmoryItem.Label                = TEXT("ARMORY");
    ArmoryItem.Detail               = TEXT("UNLOCK NEW STRATAGEMS WITH REQUISITION");
    ArmoryItem.ConfirmAction        = EHDShipActionType::OpenScreen;
    ArmoryItem.TargetScreen         = EHDShipScreen::Armory;

    HDShipMenuPrivate::AddQuitItem(Items);

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
    case EHDShipScreen::Loadout:    return TEXT("Loadout");
    case EHDShipScreen::Armory:     return TEXT("Armory");
    case EHDShipScreen::WarTable:   return TEXT("WarTable");
    case EHDShipScreen::System:     return TEXT("System");
    default:                        return TEXT("?");
    }
}

const TCHAR* HDShipMenu::GetScreenTitle(EHDShipScreen Screen)
{
    switch (Screen)
    {
    case EHDShipScreen::Bridge:     return TEXT("SHIP BRIDGE");
    case EHDShipScreen::Upgrades:   return TEXT("SHIP UPGRADES");
    case EHDShipScreen::Loadout:    return TEXT("LOADOUT");
    case EHDShipScreen::Armory:     return TEXT("ARMORY");
    case EHDShipScreen::WarTable:   return TEXT("WAR TABLE");
    case EHDShipScreen::System:     return TEXT("SHIP MENU");
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
    case EHDShipActionType::None:           return TEXT("none");
    case EHDShipActionType::OpenScreen:     return TEXT("open");
    case EHDShipActionType::Deploy:         return TEXT("deploy");
    case EHDShipActionType::Quit:           return TEXT("quit");
    case EHDShipActionType::Adjust:         return TEXT("adjust");
    case EHDShipActionType::Select:         return TEXT("select");
    case EHDShipActionType::CloseTerminal:  return TEXT("close");
    case EHDShipActionType::Launch:         return TEXT("launch");
    default:                                return TEXT("?");
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

void FHDShipMenu::OpenTerminal(EHDShipScreen Root)
{
    SetScreen(Root);
    RootScreen  = Root;
    RootCursor  = 0;
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
        // 하위 화면에서는 뿌리 화면으로, 들어갈 때의 커서로 돌아간다(Id = 뿌리 이름, Bridge 면 "Bridge")
        if (Screen != RootScreen)
        {
            Screen      = RootScreen;
            Cursor      = RootCursor;
            Action.Type = EHDShipActionType::OpenScreen;
            Action.Id   = FName(HDShipMenu::GetScreenName(RootScreen));
        }
        // 뿌리에서: Bridge 면 아무것도 하지 않고(옛 동작), 단말기면 닫는다
        else if (RootScreen != EHDShipScreen::Bridge)
        {
            Action.Type = EHDShipActionType::CloseTerminal;
            Action.Id   = FName(HDShipMenu::GetScreenName(RootScreen));
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
            if (Screen == RootScreen)
            {
                RootCursor = Cursor;
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
