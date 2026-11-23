// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipMenu.h"
#include "UI/HDShipMenuNav.h"
#include "UI/HDShipIcons.h"
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
    // 고르기 창 타일 열 수
    constexpr int32 PickerColumns = 4;

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
            Item.IconId             = Option->Id;

            // 고르기 창: 표 순서로 그 칸의 무기, 분류 = 칸 이름
            for (const FHDWeaponOption& Candidate : HDWeaponCatalog::GetOptions())
            {
                if (Candidate.Slot != Slot)
                {
                    continue;
                }

                FHDShipMenuOption& MenuOption = Item.Options.AddDefaulted_GetRef();
                MenuOption.Id           = Candidate.Id;
                MenuOption.Label        = Candidate.DisplayName;
                MenuOption.Category     = SlotName.ToUpper();
                MenuOption.bUnlocked    = HDWeaponCatalog::IsUnlocked(Context.Profile, Candidate.Id);
                MenuOption.bCurrent     = Candidate.Id == Option->Id;
            }
        }
    }

    // 칸 고르기 창의 선택지: 카탈로그 전체를 분류 순서(HDShipIcons::GetCategoryOrder), 같으면 카탈로그 순서로.
    // EquippedSlot = 다른 칸에 있으면 그 칸 번호, bCurrent = 이 칸의 지금 이름
    TArray<FHDShipMenuOption> MakeSlotOptions(const FHDShipMenuContext& Context, const TArray<FName>& Loadout, int32 SlotIndex)
    {
        struct FSortEntry
        {
            int32   Order;
            int32   CatalogIndex;
            FName   Name;
            FString Category;
        };

        TArray<FSortEntry> Entries;
        const TArray<FName>& Names = Context.Stratagems->GetNames();
        for (int32 CatalogIndex = 0; CatalogIndex < Names.Num(); ++CatalogIndex)
        {
            const FHDStratagemDefinition* Definition = Context.Stratagems->Find(Names[CatalogIndex]);
            if (Definition == nullptr)
            {
                continue;
            }

            const FString Category = HDShipIcons::GetStratagemCategory(*Definition);
            Entries.Add({ HDShipIcons::GetCategoryOrder(Category), CatalogIndex, Names[CatalogIndex], Category });
        }
        Entries.Sort([](const FSortEntry& A, const FSortEntry& B)
        {
            return A.Order != B.Order ? A.Order < B.Order : A.CatalogIndex < B.CatalogIndex;
        });

        TArray<FHDShipMenuOption> Options;
        for (const FSortEntry& Entry : Entries)
        {
            FHDShipMenuOption& Option = Options.AddDefaulted_GetRef();
            Option.Id           = Entry.Name;
            Option.Label        = HDArmory::MakeDisplayName(Entry.Name);
            Option.Category     = Entry.Category;
            Option.bUnlocked    = HDArmory::IsUnlocked(Context.Profile, *Context.Stratagems, Entry.Name);
            Option.bCurrent     = Loadout.IsValidIndex(SlotIndex) && Loadout[SlotIndex] == Entry.Name;
            for (int32 OtherIndex = 0; OtherIndex < Loadout.Num(); ++OtherIndex)
            {
                if (OtherIndex != SlotIndex && Loadout[OtherIndex] == Entry.Name)
                {
                    Option.EquippedSlot = OtherIndex;
                    break;
                }
            }
        }
        return Options;
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
            // 무기고 탭·타일(브리프 ship-terminal-ui 7): 탭 = 칸 이름(수류탄은 복수), 해금은 길게 눌러 확정
            Item.Group              = Option.Slot == EHDLoadoutWeapon::Grenade ? FString(TEXT("GRENADES")) : FString(HDWeaponCatalog::GetSlotName(Option.Slot)).ToUpper();
            Item.IconId             = Option.Id;
            Item.bHoldToConfirm     = true;
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
            Item.IconId             = StratagemName;
            Item.Options            = MakeSlotOptions(Context, Loadout, SlotIndex);

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
            Item.Group              = TEXT("STRATAGEMS");
            Item.IconId             = StratagemName;
            Item.bHoldToConfirm     = true;
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
            // 함선 관리 카드(브리프 ship-terminal-ui 6): 분야 열, 단계 수, 산 단계, 구매는 길게 눌러 확정
            Item.Group              = Definition->Group.IsEmpty() ? FString(TEXT("OTHER")) : Definition->Group;
            Item.SubCount           = LevelCount;
            Item.ActiveSub          = Level;
            Item.bHoldToConfirm     = true;

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

    // 무기고 탭(메뉴가 열려 있을 때만 불린다. 닫혀 있을 때 E 는 상호작용)
    if (Key == EKeys::Q)
    {
        OutKey = EHDShipMenuKey::PrevTab;
        return true;
    }

    if (Key == EKeys::E)
    {
        OutKey = EHDShipMenuKey::NextTab;
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
    case EHDShipMenuKey::PrevTab:   return TEXT("PrevTab");
    case EHDShipMenuKey::NextTab:   return TEXT("NextTab");
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
    case EHDShipActionType::OpenPicker:     return TEXT("picker-open");
    case EHDShipActionType::ClosePicker:    return TEXT("picker-close");
    case EHDShipActionType::Equip:          return TEXT("equip");
    case EHDShipActionType::ChangeTab:      return TEXT("tab");
    default:                                return TEXT("?");
    }
}

FHDShipMenu::FHDShipMenu(EHDShipScreen StartScreen)
    : Screen(StartScreen)
{
}

void FHDShipMenu::SetScreen(EHDShipScreen NewScreen)
{
    Screen      = NewScreen;
    Cursor      = 0;
    SubCursor   = 0;
    ActiveTab   = 0;
    bPickerOpen = false;
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

    Cursor      = NewCursor;
    SubCursor   = 0;
    bPickerOpen = false;
}

void FHDShipMenu::SetPickerCursor(int32 NewCursor, int32 OptionCount)
{
    if (bPickerOpen == false || NewCursor < 0 || NewCursor >= OptionCount)
    {
        return;
    }

    PickerCursor = NewCursor;
}

bool FHDShipMenu::HandleLoadoutKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items, FHDShipMenuAction& OutAction)
{
    using namespace HDShipMenuPrivate;

    const FHDShipMenuItem& Item = Items[Cursor];
    const bool bDirection = Key == EHDShipMenuKey::Up || Key == EHDShipMenuKey::Down || Key == EHDShipMenuKey::Left || Key == EHDShipMenuKey::Right;

    if (bPickerOpen)
    {
        const TArray<FHDShipMenuOption>& Options = Item.Options;
        if (Options.IsEmpty())
        {
            bPickerOpen = false;
            return true;
        }
        PickerCursor = FMath::Clamp(PickerCursor, 0, Options.Num() - 1);

        if (bDirection)
        {
            const TArray<FHDNavCell> Cells = HDShipMenuNav::MakePickerCells(Options, PickerColumns);
            const int32 Next = HDShipMenuNav::Move(Cells, HDShipMenuNav::FindCell(Cells, PickerCursor), Key);
            if (Cells.IsValidIndex(Next))
            {
                PickerCursor = Cells[Next].ItemIndex;
            }
            return true;
        }

        if (Key == EHDShipMenuKey::Back)
        {
            bPickerOpen     = false;
            OutAction.Type  = EHDShipActionType::ClosePicker;
            OutAction.Id    = Item.Id;
            return true;
        }

        // Confirm: 지금 낀 것 = 닫기, 열려 있고 다른 칸에 없으면 장착, 그 밖은 아무것도 하지 않는다
        const FHDShipMenuOption& Option = Options[PickerCursor];
        if (Option.bCurrent)
        {
            bPickerOpen     = false;
            OutAction.Type  = EHDShipActionType::ClosePicker;
            OutAction.Id    = Item.Id;
        }
        else if (Option.bUnlocked && Option.EquippedSlot == INDEX_NONE)
        {
            bPickerOpen         = false;
            OutAction.Type      = EHDShipActionType::Equip;
            OutAction.Id        = Item.Id;
            OutAction.OptionId  = Option.Id;
        }
        return true;
    }

    if (bDirection)
    {
        const TArray<FHDNavCell> Cells = HDShipMenuNav::MakeLoadoutCells(Items);
        const int32 Next = HDShipMenuNav::Move(Cells, HDShipMenuNav::FindCell(Cells, Cursor), Key);
        if (Cells.IsValidIndex(Next))
        {
            Cursor = Cells[Next].ItemIndex;
        }
        return true;
    }

    if (Key == EHDShipMenuKey::Confirm && Item.bEnabled && Item.Options.IsEmpty() == false)
    {
        const int32 CurrentOption = Item.Options.IndexOfByPredicate([](const FHDShipMenuOption& Option) { return Option.bCurrent; });
        bPickerOpen     = true;
        PickerCursor    = CurrentOption != INDEX_NONE ? CurrentOption : 0;
        OutAction.Type  = EHDShipActionType::OpenPicker;
        OutAction.Id    = Item.Id;
        return true;
    }

    // 그 밖의 Confirm(READY → Launch)과 Back 은 HandleKey 의 공통 규칙
    return false;
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

    // Loadout: 칸 이동과 고르기 창(브리프 ship-terminal-ui 5). 처리하지 않은 키만 아래 공통 규칙으로
    if (Screen == EHDShipScreen::Loadout && ItemCount > 0 && HandleLoadoutKey(Key, Items, Action))
    {
        return Action;
    }

    // Upgrades: 단계가 있는 줄이 있으면 카드 이동과 단계 커서를 본 Confirm(브리프 ship-terminal-ui 6). 없으면 목록 규칙
    const bool bUpgradeCards = Screen == EHDShipScreen::Upgrades
        && Items.ContainsByPredicate([](const FHDShipMenuItem& Candidate) { return Candidate.SubCount > 0; });
    if (bUpgradeCards && HandleUpgradeKey(Key, Items, Action))
    {
        return Action;
    }

    // Armory: 줄에 탭(Group)이 있으면 탭과 타일 이동(브리프 ship-terminal-ui 7). Confirm·Back 은 공통 규칙
    const bool bArmoryTabs = Screen == EHDShipScreen::Armory
        && Items.ContainsByPredicate([](const FHDShipMenuItem& Candidate) { return Candidate.Group.IsEmpty() == false; });
    if (bArmoryTabs && HandleArmoryKey(Key, Items, Action))
    {
        return Action;
    }

    // 탭 키는 무기고 탭 화면에서만 뜻이 있다
    if (Key == EHDShipMenuKey::PrevTab || Key == EHDShipMenuKey::NextTab)
    {
        return Action;
    }

    if (Key == EHDShipMenuKey::Back)
    {
        // 하위 화면에서는 뿌리 화면으로, 들어갈 때의 커서로 돌아간다(Id = 뿌리 이름, Bridge 면 "Bridge")
        if (Screen != RootScreen)
        {
            Screen      = RootScreen;
            Cursor      = RootCursor;
            SubCursor   = 0;
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

const TArray<FString>& HDShipMenu::GetArmoryTabs()
{
    static const TArray<FString> Tabs = { TEXT("STRATAGEMS"), TEXT("PRIMARY"), TEXT("SECONDARY"), TEXT("GRENADES") };
    return Tabs;
}

namespace HDShipMenuPrivate
{
    // 그 탭(Group)의 줄 번호를 줄 순서로
    TArray<int32> GetTabRows(const TArray<FHDShipMenuItem>& Items, int32 Tab)
    {
        TArray<int32> Rows;
        const TArray<FString>& Tabs = HDShipMenu::GetArmoryTabs();
        if (Tabs.IsValidIndex(Tab) == false)
        {
            return Rows;
        }

        for (int32 Index = 0; Index < Items.Num(); ++Index)
        {
            if (Items[Index].Group == Tabs[Tab])
            {
                Rows.Add(Index);
            }
        }
        return Rows;
    }

    // 무기고 타일 열 수
    constexpr int32 ArmoryColumns = 4;
}

void FHDShipMenu::SetActiveTab(int32 Tab, const TArray<FHDShipMenuItem>& Items)
{
    if (HDShipMenu::GetArmoryTabs().IsValidIndex(Tab) == false)
    {
        return;
    }

    ActiveTab = Tab;
    const TArray<int32> Rows = HDShipMenuPrivate::GetTabRows(Items, Tab);
    if (Rows.IsEmpty() == false)
    {
        Cursor = Rows[0];
    }
}

bool FHDShipMenu::HandleArmoryKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items, FHDShipMenuAction& OutAction)
{
    using namespace HDShipMenuPrivate;

    if (Key == EHDShipMenuKey::PrevTab || Key == EHDShipMenuKey::NextTab)
    {
        const int32 NewTab = ActiveTab + (Key == EHDShipMenuKey::NextTab ? 1 : -1);
        // 끝에서 감지 않는다
        if (HDShipMenu::GetArmoryTabs().IsValidIndex(NewTab) == false)
        {
            return true;
        }

        SetActiveTab(NewTab, Items);
        OutAction.Type  = EHDShipActionType::ChangeTab;
        OutAction.Id    = FName(*HDShipMenu::GetArmoryTabs()[NewTab]);
        return true;
    }

    const bool bDirection = Key == EHDShipMenuKey::Up || Key == EHDShipMenuKey::Down || Key == EHDShipMenuKey::Left || Key == EHDShipMenuKey::Right;
    if (bDirection == false)
    {
        return false;
    }

    const TArray<int32> Rows = GetTabRows(Items, ActiveTab);
    if (Rows.IsEmpty())
    {
        return true;
    }

    // 커서가 탭 밖이면 먼저 탭 첫 줄로
    int32 Current = Rows.IndexOfByKey(Cursor);
    if (Current == INDEX_NONE)
    {
        Current = 0;
    }

    // 탭 타일을 왼쪽 위부터 행 순서로 4열(고르기 창 배치에 분류 하나를 준 것과 같다)
    TArray<FHDShipMenuOption> Tiles;
    Tiles.SetNum(Rows.Num());
    const TArray<FHDNavCell> Cells = HDShipMenuNav::MakePickerCells(Tiles, ArmoryColumns);
    const int32 Next = HDShipMenuNav::Move(Cells, HDShipMenuNav::FindCell(Cells, Current), Key);
    Cursor = Rows[Cells.IsValidIndex(Next) ? Cells[Next].ItemIndex : Current];
    return true;
}

bool HDShipMenu::NeedsHold(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items)
{
    if (Items.IsEmpty())
    {
        return false;
    }

    // 사본으로 Confirm 을 눌러 본다(원래 메뉴의 화면·커서는 그대로)
    FHDShipMenu Probe = Menu;
    const FHDShipMenuItem& Item = Items[FMath::Clamp(Menu.GetCursor(), 0, Items.Num() - 1)];
    return Probe.HandleKey(EHDShipMenuKey::Confirm, Items).Type != EHDShipActionType::None && Item.bHoldToConfirm;
}

void FHDShipMenu::SetSubCursor(int32 NewSub, const TArray<FHDShipMenuItem>& Items)
{
    if (Items.IsValidIndex(Cursor) == false)
    {
        return;
    }

    SubCursor = FMath::Clamp(NewSub, 0, FMath::Max(0, Items[Cursor].SubCount - 1));
}

bool FHDShipMenu::HandleUpgradeKey(EHDShipMenuKey Key, const TArray<FHDShipMenuItem>& Items, FHDShipMenuAction& OutAction)
{
    const FHDShipMenuItem& Item = Items[Cursor];
    const bool bDirection = Key == EHDShipMenuKey::Up || Key == EHDShipMenuKey::Down || Key == EHDShipMenuKey::Left || Key == EHDShipMenuKey::Right;

    if (bDirection)
    {
        // 줄 수·단계 수가 바뀌었을 수 있으므로 단계 커서도 자른다
        SubCursor = FMath::Clamp(SubCursor, 0, FMath::Max(0, Item.SubCount - 1));

        const TArray<FHDNavCell> Cells = HDShipMenuNav::MakeUpgradeCells(Items);
        const int32 Next = HDShipMenuNav::Move(Cells, HDShipMenuNav::FindCell(Cells, Cursor, SubCursor), Key);
        if (Cells.IsValidIndex(Next))
        {
            Cursor      = Cells[Next].ItemIndex;
            SubCursor   = Cells[Next].SubIndex;
        }
        return true;
    }

    // 다음에 살 단계 카드만 공통 Confirm(켜진 줄이면 Select)으로 넘긴다. 산 단계·뒤 단계 카드는 None
    if (Key == EHDShipMenuKey::Confirm)
    {
        return SubCursor != Item.ActiveSub;
    }

    // Back 은 공통 규칙
    return false;
}
