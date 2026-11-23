// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipMenuView.h"
#include "Progression/HDMissionReward.h"

namespace HDShipMenuViewPrivate
{
    // HDShipMenu::BuildItems 의 난이도 줄 Id
    const FName DifficultyItemId(TEXT("Difficulty"));

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

    Hints.Add(MakeHint(TEXT("UP/DOWN"), TEXT("SELECT"), EHDShipMenuKey::Up, false));

    // FHDShipMenu::HandleKey 처럼 커서를 줄 수로 자른다
    if (Items.IsEmpty() == false)
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
