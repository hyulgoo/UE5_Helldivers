// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipMenuView.h"
#include "Progression/HDMissionReward.h"
#include "Progression/HDDifficulty.h"
#include "Progression/HDUpgradeRules.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "UI/HDShipIcons.h"

namespace HDShipMenuViewPrivate
{
    // HDShipMenu::BuildItems 의 난이도 줄·READY 줄 Id
    const FName DifficultyItemId(TEXT("Difficulty"));
    const FName ReadyItemId(TEXT("Ready"));

    FHDKeyHint MakeHint(const TCHAR* Key, const TCHAR* Action, EHDShipMenuKey MenuKey, bool bClickable)
    {
        FHDKeyHint Hint;
        Hint.Key        = Key;
        Hint.Action     = Action;
        Hint.MenuKey    = MenuKey;
        Hint.bClickable = bClickable;
        return Hint;
    }
}

TArray<FHDKeyHint> HDShipMenuView::GetKeyHints(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items, bool bLaunching)
{
    using namespace HDShipMenuViewPrivate;

    TArray<FHDKeyHint> Hints;
    const EHDShipScreen Screen = Menu.GetScreen();
    if (Screen == EHDShipScreen::Title)
    {
        return Hints;
    }

    // 출격 대기 중에는 Back(대기 취소)만 받는다(AHDPlayerController::HandleShipMenuKey)
    if (bLaunching)
    {
        Hints.Add(MakeHint(TEXT("BACKSPACE"), TEXT("CANCEL"), EHDShipMenuKey::Back, true));
        return Hints;
    }

    // Upgrades 에 단계가 있는 줄이 있으면 카드 화면(FHDShipMenu::HandleKey 와 같은 조건)
    const bool bUpgradeCards = Screen == EHDShipScreen::Upgrades
        && Items.ContainsByPredicate([](const FHDShipMenuItem& Candidate) { return Candidate.SubCount > 0; });

    // Loadout: 칸 이동(방향키 넷)과 고르기 창
    if (Screen == EHDShipScreen::Loadout)
    {
        Hints.Add(MakeHint(TEXT("ARROWS"), TEXT("MOVE"), EHDShipMenuKey::Up, false));
        const FHDShipMenuItem* Item = Items.IsEmpty() ? nullptr : &Items[FMath::Clamp(Menu.GetCursor(), 0, Items.Num() - 1)];
        if (Menu.IsPickerOpen() && Item && Item->Options.IsEmpty() == false)
        {
            const FHDShipMenuOption& Option = Item->Options[FMath::Clamp(Menu.GetPickerCursor(), 0, Item->Options.Num() - 1)];
            if (Option.bCurrent)
            {
                Hints.Add(MakeHint(TEXT("ENTER"), TEXT("KEEP"), EHDShipMenuKey::Confirm, true));
            }
            else if (Option.bUnlocked && Option.EquippedSlot == INDEX_NONE)
            {
                Hints.Add(MakeHint(TEXT("ENTER"), TEXT("EQUIP"), EHDShipMenuKey::Confirm, true));
            }
            Hints.Add(MakeHint(TEXT("BACKSPACE"), TEXT("CLOSE"), EHDShipMenuKey::Back, true));
            return Hints;
        }

        if (Item && Item->bEnabled)
        {
            if (Item->Options.IsEmpty() == false)
            {
                Hints.Add(MakeHint(TEXT("ENTER"), TEXT("CHANGE"), EHDShipMenuKey::Confirm, true));
            }
            else if (Item->ConfirmAction == EHDShipActionType::Launch)
            {
                Hints.Add(MakeHint(TEXT("ENTER"), TEXT("READY"), EHDShipMenuKey::Confirm, true));
            }
            else if (Item->ConfirmAction != EHDShipActionType::None)
            {
                Hints.Add(MakeHint(TEXT("ENTER"), TEXT("CONFIRM"), EHDShipMenuKey::Confirm, true));
            }
        }
    }
    else if (bUpgradeCards)
    {
        // Upgrades 카드: 다음에 살 수 있는 카드에서만 길게 눌러 구매(안내만, 클릭 상자 아님)
        Hints.Add(MakeHint(TEXT("ARROWS"), TEXT("MOVE"), EHDShipMenuKey::Up, false));
        const FHDShipMenuItem& Item = Items[FMath::Clamp(Menu.GetCursor(), 0, Items.Num() - 1)];
        if (GetTierState(Item, Menu.GetSubCursor()) == EHDTierState::Available)
        {
            Hints.Add(MakeHint(TEXT("HOLD ENTER"), TEXT("BUY"), EHDShipMenuKey::Confirm, false));
        }
    }
    else
    {
        Hints.Add(MakeHint(TEXT("UP/DOWN"), TEXT("SELECT"), EHDShipMenuKey::Up, false));
    }

    // FHDShipMenu::HandleKey 처럼 커서를 줄 수로 자른다
    if (Screen != EHDShipScreen::Loadout && bUpgradeCards == false && Items.IsEmpty() == false)
    {
        const FHDShipMenuItem& Item = Items[FMath::Clamp(Menu.GetCursor(), 0, Items.Num() - 1)];
        if (Item.bAdjustable && Item.bEnabled)
        {
            const bool bDifficulty = Screen == EHDShipScreen::WarTable && Item.Id == DifficultyItemId;
            Hints.Add(MakeHint(TEXT("LEFT/RIGHT"), bDifficulty ? TEXT("DIFFICULTY") : TEXT("CHANGE"), EHDShipMenuKey::Left, false));
        }

        if (Item.bEnabled && Item.ConfirmAction != EHDShipActionType::None)
        {
            Hints.Add(MakeHint(TEXT("ENTER"), TEXT("CONFIRM"), EHDShipMenuKey::Confirm, true));
        }
    }

    // Back: 하위 화면이면 뿌리로, 단말기 뿌리면 닫기(System 은 Esc 로 안내), Bridge 뿌리면 아무것도 하지 않는다
    const EHDShipScreen Root = Menu.GetRootScreen();
    if (Screen != Root)
    {
        Hints.Add(MakeHint(TEXT("BACKSPACE"), TEXT("BACK"), EHDShipMenuKey::Back, true));
    }
    else if (Root == EHDShipScreen::System)
    {
        Hints.Add(MakeHint(TEXT("ESC"), TEXT("CLOSE"), EHDShipMenuKey::Back, true));
    }
    else if (Root != EHDShipScreen::Bridge)
    {
        Hints.Add(MakeHint(TEXT("BACKSPACE"), TEXT("CLOSE"), EHDShipMenuKey::Back, true));
    }

    return Hints;
}

FString HDShipMenuView::JoinKeyHints(const TArray<FHDKeyHint>& Hints)
{
    TArray<FString> Parts;
    for (const FHDKeyHint& Hint : Hints)
    {
        Parts.Add(FString::Printf(TEXT("[%s] %s"), *Hint.Key, *Hint.Action));
    }
    return FString::Join(Parts, TEXT("   "));
}

FHDResourceView HDShipMenuView::MakeResourceView(const FHDProfileData& Profile, const FString& LastMission)
{
    FHDResourceView View;
    View.Requisition    = Profile.Requisition;
    View.Samples        = Profile.Samples;
    View.LastMission    = LastMission;
    return View;
}

FString HDShipMenuView::FormatLastMission(const FHDMissionResultSummary* LastResult)
{
    if (LastResult == nullptr)
    {
        return FString();
    }

    // 옛 AHDHUD::DrawShipResources 의 글자 그대로
    return FString::Printf(TEXT("LAST MISSION  %s  +%d REQ"),
        LastResult->Outcome.bSuccess ? TEXT("COMPLETE") : TEXT("FAILED"),
        LastResult->Reward.Requisition);
}

FString HDShipMenuView::FormatMapName(const FString& LevelName)
{
    // PIE 사본 레벨은 UEDPIE_<n>_ 접두사가 붙는다
    FString Name = LevelName;
    if (Name.StartsWith(TEXT("UEDPIE_")))
    {
        const int32 Underscore = Name.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 7);
        Name = Underscore != INDEX_NONE ? Name.Mid(Underscore + 1) : FString();
    }

    // 소문자(또는 숫자) 다음 대문자 앞에 공백, 밑줄은 공백
    FString Spaced;
    for (int32 Index = 0; Index < Name.Len(); ++Index)
    {
        const TCHAR Char = Name[Index];
        if (Char == TEXT('_'))
        {
            Spaced.AppendChar(TEXT(' '));
            continue;
        }

        if (Index > 0 && FChar::IsUpper(Char) && (FChar::IsLower(Name[Index - 1]) || FChar::IsDigit(Name[Index - 1])))
        {
            Spaced.AppendChar(TEXT(' '));
        }
        Spaced.AppendChar(Char);
    }

    Spaced.TrimStartAndEndInline();
    return Spaced.IsEmpty() ? FString(TEXT("UNKNOWN SECTOR")) : Spaced.ToUpper();
}

FHDWarTableView HDShipMenuView::BuildWarTable(const FHDShipMenuContext& Context, const FString& LevelName, const FHDRewardRules& Rules)
{
    FHDWarTableView View;
    View.OperationName  = FormatMapName(LevelName);
    View.OperationType  = TEXT("DESTROY FACILITIES");
    View.bHasDifficulty = Context.Difficulties.IsEmpty() == false;

    if (View.bHasDifficulty == false)
    {
        View.Objectives = { TEXT("DESTROY FACILITIES"), TEXT("REACH EXTRACTION"), TEXT("OPTIONAL: RECOVER SAMPLES") };
        return View;
    }

    const int32 NumLevels = Context.Difficulties.Num();
    const int32 Level = HDDifficulty::ClampLevel(Context.Profile.Difficulty, NumLevels);
    const FHDDifficultyLevel Row = HDDifficulty::GetLevel(Context.Difficulties, Level);

    for (int32 Index = 0; Index < NumLevels; ++Index)
    {
        FHDDifficultySegment Segment;
        Segment.Level       = Index + 1;
        Segment.Label       = Context.Difficulties[Index].Label;
        Segment.bCurrent    = Segment.Level == Level;
        View.Segments.Add(Segment);
    }

    const int32 Required = Row.RequiredObjectives;
    View.Objectives = {
        FString::Printf(TEXT("DESTROY %d %s"), Required, Required == 1 ? TEXT("FACILITY") : TEXT("FACILITIES")),
        TEXT("REACH EXTRACTION"),
        TEXT("OPTIONAL: RECOVER SAMPLES")
    };
    View.DifficultyLabel    = FString::Printf(TEXT("%d  %s"), Level, *Row.Label);
    View.RewardMultiplier   = TEXT("X") + FString::SanitizeFloat(Row.RewardMultiplier);
    View.MapSamples         = Row.SampleCounts;

    // 목표를 모두 부수고 성공했을 때(소지 샘플은 0 으로 본다)
    FHDMissionOutcome Outcome;
    Outcome.bSuccess            = true;
    Outcome.CompletedObjectives = Required;
    Outcome.RequiredObjectives  = Required;
    Outcome.Difficulty          = Level;
    Outcome.DifficultyLabel     = Row.Label;
    Outcome.RewardMultiplier    = Row.RewardMultiplier;
    View.SuccessRequisition = HDProgression::ComputeReward(Outcome, Rules).Requisition;
    return View;
}

namespace HDShipMenuViewPrivate
{
    // 선택지 상태 줄: 지금 낀 것 → EQUIPPED, 잠김 → LOCKED  <가격> REQ IN ARMORY, 다른 칸 → IN SLOT n
    FString MakeStatus(const FHDShipMenuOption& Option, int32 UnlockCost)
    {
        if (Option.bCurrent)
        {
            return TEXT("EQUIPPED");
        }
        if (Option.bUnlocked == false)
        {
            return FString::Printf(TEXT("LOCKED  %d REQ IN ARMORY"), UnlockCost);
        }
        if (Option.EquippedSlot != INDEX_NONE)
        {
            return FString::Printf(TEXT("IN SLOT %d"), Option.EquippedSlot + 1);
        }
        return FString();
    }

    FHDDetailLine MakeLine(const TCHAR* Label, const FString& Value)
    {
        FHDDetailLine Line;
        Line.Label  = Label;
        Line.Value  = Value;
        return Line;
    }

    // 스트라타젬 값: UHDStratagemComponent::BuildCatalog 와 같은 필드에 업그레이드를 넣는다
    void FillStratagem(FHDLoadoutDetail& Detail, FName Name, const FHDStratagemDefinition& Definition, const FHDShipMenuContext& Context)
    {
        FHDStratagemEntry Entry;
        Entry.Name              = Name;
        Entry.CommandSequence   = Definition.CommandSequence;
        Entry.ActivationDelay   = Definition.ActivationDelay;
        Entry.Cooldown          = Definition.Cooldown;
        Entry.MaxUses           = Definition.MaxUses;
        Entry.RearmTime         = Definition.RearmTime;
        const FHDUpgradeModifiers Modifiers = Context.Upgrades ? HDUpgrade::ComputeModifiers(Context.Profile, *Context.Upgrades) : FHDUpgradeModifiers();
        HDUpgrade::ApplyToEntry(Entry, Definition, Modifiers);

        Detail.IconId   = Name;
        Detail.Title    = HDArmory::MakeDisplayName(Name);
        Detail.Category = HDShipIcons::GetStratagemCategory(Definition);
        Detail.Command  = Entry.CommandSequence;
        Detail.Lines.Add(MakeLine(TEXT("CALL-IN"), HDShipMenuView::FormatSeconds(Entry.ActivationDelay)));
        Detail.Lines.Add(MakeLine(TEXT("COOLDOWN"), HDShipMenuView::FormatSeconds(Entry.Cooldown)));
        Detail.Lines.Add(MakeLine(TEXT("USES"), Entry.MaxUses > 0 ? FString::FromInt(Entry.MaxUses) : FString(TEXT("UNLIMITED"))));
        if (Entry.MaxUses > 0 && Entry.RearmTime > 0.f)
        {
            Detail.Lines.Add(MakeLine(TEXT("REARM"), HDShipMenuView::FormatSeconds(Entry.RearmTime)));
        }
    }

    void FillWeapon(FHDLoadoutDetail& Detail, const FHDWeaponOption& Weapon)
    {
        Detail.IconId   = Weapon.Id;
        Detail.Title    = Weapon.DisplayName;
        Detail.Category = FString(HDWeaponCatalog::GetSlotName(Weapon.Slot)).ToUpper();
        Detail.Bars     = HDWeaponCatalog::MakeStatBars(Weapon);
    }
}

FHDLoadoutDetail HDShipMenuView::BuildLoadoutDetail(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items, const FHDShipMenuContext& Context)
{
    using namespace HDShipMenuViewPrivate;

    FHDLoadoutDetail Detail;
    if (Items.IsEmpty())
    {
        return Detail;
    }

    const FHDShipMenuItem& Item = Items[FMath::Clamp(Menu.GetCursor(), 0, Items.Num() - 1)];

    // READY: 칸마다 한 줄(스트라타젬 넷, 무기 셋)
    if (Item.Id == ReadyItemId)
    {
        Detail.Title = TEXT("READY");
        for (const FHDShipMenuItem& Other : Items)
        {
            if (Other.IconId.IsNone())
            {
                continue;
            }

            FString Label = Other.Id.ToString().ToUpper();
            FString Value = HDArmory::MakeDisplayName(Other.IconId);
            if (const FHDWeaponOption* Weapon = HDWeaponCatalog::Find(Other.IconId))
            {
                Value = Weapon->DisplayName;
            }
            else if (Label.StartsWith(TEXT("SLOT")))
            {
                Label = TEXT("SLOT ") + Label.Mid(4);
            }
            Detail.Lines.Add(MakeLine(*Label, Value));
        }
        return Detail;
    }

    // 창이 열려 있으면 창의 커서 항목, 아니면 카드(지금 낀 것)
    FHDShipMenuOption Shown;
    Shown.Id        = Item.IconId;
    Shown.bCurrent  = true;
    if (Menu.IsPickerOpen() && Item.Options.IsEmpty() == false)
    {
        Shown = Item.Options[FMath::Clamp(Menu.GetPickerCursor(), 0, Item.Options.Num() - 1)];
    }

    const FHDStratagemDefinition* Definition = Context.Stratagems ? Context.Stratagems->Find(Shown.Id) : nullptr;
    if (Definition)
    {
        FillStratagem(Detail, Shown.Id, *Definition, Context);
        Detail.Status = MakeStatus(Shown, Definition->UnlockCost);
        return Detail;
    }

    if (const FHDWeaponOption* Weapon = HDWeaponCatalog::Find(Shown.Id))
    {
        FillWeapon(Detail, *Weapon);
        Detail.Status = MakeStatus(Shown, Weapon->UnlockCost);
        return Detail;
    }

    Detail.Title = Item.Label;
    return Detail;
}

FString HDShipMenuView::FormatSeconds(float Seconds)
{
    const int32 Rounded = FMath::RoundToInt(Seconds);
    return FMath::IsNearlyEqual(Seconds, static_cast<float>(Rounded), 0.01f)
        ? FString::Printf(TEXT("%d S"), Rounded)
        : FString::Printf(TEXT("%.1f S"), Seconds);
}

TArray<FString> HDShipMenuView::GetUpgradeGroups(const FHDUpgradeCatalog& Catalog)
{
    TArray<FString> Groups;
    for (const FName& Name : Catalog.GetNames())
    {
        const FHDUpgradeDefinition* Definition = Catalog.Find(Name);
        if (Definition == nullptr)
        {
            continue;
        }

        Groups.AddUnique(Definition->Group.IsEmpty() ? FString(TEXT("OTHER")) : Definition->Group);
    }
    return Groups;
}

EHDTierState HDShipMenuView::GetTierState(const FHDShipMenuItem& Item, int32 SubIndex)
{
    if (SubIndex < Item.ActiveSub)
    {
        return EHDTierState::Owned;
    }
    if (SubIndex == Item.ActiveSub)
    {
        return Item.bEnabled ? EHDTierState::Available : EHDTierState::Insufficient;
    }
    return EHDTierState::NeedsPrevious;
}

FString HDShipMenuView::GetTierStateLabel(const FHDShipMenuItem& Item, int32 SubIndex)
{
    switch (GetTierState(Item, SubIndex))
    {
    case EHDTierState::Owned:           return TEXT("OWNED");
    case EHDTierState::Available:       return TEXT("AVAILABLE");
    case EHDTierState::Insufficient:    return TEXT("NOT ENOUGH");
    default:                            return FString::Printf(TEXT("REQUIRES TIER %d"), SubIndex);
    }
}

FHDUpgradeDetail HDShipMenuView::BuildUpgradeDetail(const FHDShipMenu& Menu, const TArray<FHDShipMenuItem>& Items, const FHDShipMenuContext& Context, int32 ItemIndex, int32 SubIndex)
{
    FHDUpgradeDetail Detail;
    if (Items.IsValidIndex(ItemIndex) == false || Context.Upgrades == nullptr)
    {
        return Detail;
    }

    const FHDShipMenuItem& Item = Items[ItemIndex];
    const FHDUpgradeDefinition* Definition = Context.Upgrades->Find(Item.Id);
    if (Definition == nullptr || Definition->Levels.IsValidIndex(SubIndex) == false)
    {
        return Detail;
    }

    const FHDUpgradeLevel& Level = Definition->Levels[SubIndex];
    Detail.IconId       = Item.Id;
    Detail.Title        = Definition->DisplayName;
    Detail.Group        = Item.Group;
    Detail.Description  = Definition->Description;
    Detail.TierLabel    = FString::Printf(TEXT("TIER %d / %d"), SubIndex + 1, Definition->Levels.Num());
    Detail.State        = GetTierState(Item, SubIndex);
    Detail.StateLabel   = GetTierStateLabel(Item, SubIndex);
    Detail.Current      = Definition->Levels.IsValidIndex(Item.ActiveSub - 1)
        ? HDUpgrade::FormatEffect(Definition->Effect, Definition->Levels[Item.ActiveSub - 1].Value)
        : FString(TEXT("NONE"));
    Detail.ThisTier     = HDUpgrade::FormatEffect(Definition->Effect, Level.Value);

    // 비용: 0 인 항목은 빼고, 가진 양과 비교한다
    const FHDProfileData& Profile = Context.Profile;
    auto AddCost = [&Detail](const TCHAR* IconId, int32 Amount, int32 Owned)
    {
        if (Amount <= 0)
        {
            return;
        }

        FHDCostPart& Part = Detail.Cost.AddDefaulted_GetRef();
        Part.IconId     = FName(IconId);
        Part.Amount     = Amount;
        Part.bEnough    = Owned >= Amount;
    };
    AddCost(TEXT("Requisition"), Level.Requisition, Profile.Requisition);
    AddCost(TEXT("SampleCommon"), Level.Common, Profile.Samples.Common);
    AddCost(TEXT("SampleRare"), Level.Rare, Profile.Samples.Rare);
    AddCost(TEXT("SampleSuper"), Level.Super, Profile.Samples.Super);
    return Detail;
}
