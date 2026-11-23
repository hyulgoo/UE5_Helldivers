// Fill out your copyright notice in the Description page of Project Settings.

#include "HDPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "Component/HDStratagemComponent.h"
#include "Component/HDCombatComponent.h"
#include "Component/HDInputComponent.h"
#include "Component/HDAbilitySystemComponent.h"
#include "UI/HDHUD.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "GameData/HDCharacterControlData.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Cheat/HDCheatManager.h"
#include "Game/HDGameMode.h"
#include "Game/HDGameState.h"
#include "Progression/HDProfileSubsystem.h"
#include "Progression/HDProgressionSettings.h"
#include "Progression/HDDifficulty.h"
#include "Progression/HDMissionReward.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Progression/HDUpgradeRules.h"
#include "Progression/HDArmoryRules.h"
#include "Progression/HDWeaponCatalog.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Stratagem/HDStratagemSettings.h"
#include "Ship/HDShipHubRules.h"
#include "Ship/HDShipInterior.h"
#include "UI/HDShipUILayout.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

namespace HDPlayerControllerShip
{
    // HDShipMenu::BuildItems 의 난이도 줄 Id
    const FName DifficultyItemId(TEXT("Difficulty"));

    // P 함선 메뉴의 단말기 Id(로그용)
    const FName SystemTerminalId(TEXT("System"));

    // HDShipMenu::BuildItems 의 Loadout 칸 줄 Id "Slot1"~"Slot4" → 칸 번호 0~3. 아니면 INDEX_NONE
    int32 GetLoadoutSlotIndex(const FName ItemId)
    {
        const FString IdText = ItemId.ToString();
        const FString Prefix(TEXT("Slot"));
        if (IdText.StartsWith(Prefix, ESearchCase::IgnoreCase) == false)
        {
            return INDEX_NONE;
        }

        const FString NumberText = IdText.Mid(Prefix.Len());
        if (NumberText.IsEmpty() || NumberText.IsNumeric() == false)
        {
            return INDEX_NONE;
        }

        const int32 SlotNumber = FCString::Atoi(*NumberText);
        return (SlotNumber >= 1 && SlotNumber <= FHDStratagemLoadout::MaxSlots) ? SlotNumber - 1 : INDEX_NONE;
    }

    // HDShipMenu::BuildItems 의 Loadout 무기 줄 Id "Primary"·"Secondary"·"Grenade"(HDWeaponCatalog::GetSlotName) → 칸. 아니면 거짓
    bool TryGetLoadoutWeapon(const FName ItemId, EHDLoadoutWeapon& OutSlot)
    {
        for (const EHDLoadoutWeapon Slot : { EHDLoadoutWeapon::Primary, EHDLoadoutWeapon::Secondary, EHDLoadoutWeapon::Grenade })
        {
            if (ItemId == FName(HDWeaponCatalog::GetSlotName(Slot)))
            {
                OutSlot = Slot;
                return true;
            }
        }

        return false;
    }
}

static TAutoConsoleVariable<int32> CVarHDAbilitySystemDebugOnSpawn(
    TEXT("hd.Debug.AbilitySystemOnSpawn"),
    0,
    TEXT("1: run 'showdebug abilitysystem' on first possess. While it is shown, AHUD skips DrawHUD, so the mission HUD is hidden"),
    ECVF_Cheat);

AHDPlayerController::AHDPlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CheatClass = UHDCheatManager::StaticClass();

    static ConstructorHelpers::FObjectFinder<UHDCharacterControlData> ThirdPersonDataRef(TEXT("/Script/Helldivers.HDCharacterControlData'/Game/Helldivers/CharacterControl/HDC_ThirdPerson.HDC_ThirdPerson'"));
    if (ThirdPersonDataRef.Succeeded())
    {
        CharacterControlDataMap.Add(EHDCharacterControlType::ThirdPerson, ThirdPersonDataRef.Object);
    }

    static ConstructorHelpers::FObjectFinder<UHDCharacterControlData> FirstPersonDataRef(TEXT("/Script/Helldivers.HDCharacterControlData'/Game/Helldivers/CharacterControl/HDC_FirstPerson.HDC_FirstPerson'"));
    if (FirstPersonDataRef.Succeeded())
    {
        CharacterControlDataMap.Add(EHDCharacterControlType::FirstPerson, FirstPersonDataRef.Object);
    }
}

UHDAbilitySystemComponent* AHDPlayerController::GetAbilitySystemComponent()
{
    if(AbilitySystemComponent == nullptr)
    {
        AbilitySystemComponent = Cast<UHDAbilitySystemComponent>(FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(GetPawn()));
        NULL_CHECK_WITH_RETURNTYPE(AbilitySystemComponent, nullptr);
    }

    return AbilitySystemComponent;
}

bool AHDPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    // Enhanced Input 이 키 상태를 그대로 받게 먼저 부른다
    const bool bResult = Super::InputKey(Params);

    // 함선 메뉴 중에는 메뉴 키만 받는다. 방향키가 스트라타젬 명령 큐로 가지 않게 나머지 큐(화살표·숫자·Q·Alt)는 건너뛴다
    if (bShipMenuActive)
    {
        EHDShipMenuKey MenuKey = EHDShipMenuKey::Confirm;
        if (HDShipMenu::TryGetMenuKey(Params.Key, Params.Event, MenuKey))
        {
            PendingShipMenuKeys.Add(MenuKey);
        }

        return bResult;
    }

    // 함선 안(메뉴 닫힘): E·P/Escape·BackSpace 만 본다. 화살표·숫자·Q·Alt·결과 키 큐는 건너뛴다
    if (IsInShipInterior())
    {
        if (HDShipHub::IsInteractKey(Params.Key, Params.Event))
        {
            bPendingInteract = true;
        }

        EHDMissionMenuKey ShipMenuKey = EHDMissionMenuKey::Confirm;
        if (HDDirectKey::TryGetMissionMenuKey(Params.Key, Params.Event, ShipMenuKey))
        {
            const UWorld* World = GetWorld();
            const AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;

            if (ShipMenuKey == EHDMissionMenuKey::ToggleAbandon)
            {
                bPendingShipSystemMenu = true;
            }
            else if (ShipMenuKey == EHDMissionMenuKey::Cancel && HDGameMode && HDGameMode->IsLaunching())
            {
                bPendingLaunchCancel = true;
            }
        }

        return bResult;
    }

    EHDCommandInput Command = EHDCommandInput::Count;
    if (HDDirectKey::TryGetCommand(Params.Key, Params.Event, Command))
    {
        PendingStratagemCommands.Add(Command);
    }

    EHDWeaponSlot WeaponSlot = EHDWeaponSlot::Primary;
    if (HDDirectKey::TryGetWeaponSlot(Params.Key, Params.Event, WeaponSlot))
    {
        PendingWeaponSlots.Add(WeaponSlot);
    }

    if (HDDirectKey::IsStimKey(Params.Key, Params.Event))
    {
        bPendingStim = true;
    }

    if (HDDirectKey::IsDiveKey(Params.Key, Params.Event))
    {
        bPendingDive = true;
    }

    // 작전 포기 확인 창·결과 화면 키. 이 키들은 IMC 에 매핑이 없다
    EHDMissionMenuKey MissionMenuKey = EHDMissionMenuKey::Confirm;
    if (HDDirectKey::TryGetMissionMenuKey(Params.Key, Params.Event, MissionMenuKey))
    {
        PendingMissionMenuKeys.Add(MissionMenuKey);
    }

    return bResult;
}

void AHDPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    // 메뉴 키를 먼저. 출격하면 ExitShipMenu 가 큐를 비우므로 옮겨 놓고 돌고, 메뉴가 닫히면 남은 키는 버린다
    if (PendingShipMenuKeys.IsEmpty() == false)
    {
        const TArray<EHDShipMenuKey> MenuKeys = MoveTemp(PendingShipMenuKeys);
        PendingShipMenuKeys.Reset();

        for (const EHDShipMenuKey MenuKey : MenuKeys)
        {
            if (bShipMenuActive == false)
            {
                break;
            }

            HandleShipMenuKey(MenuKey);
        }
    }

    // 함선 안 요청. E = 범위 안의 가장 가까운 단말기, P = 함선 메뉴, BackSpace = 출격 대기 취소
    if (bPendingInteract)
    {
        bPendingInteract = false;

        const int32 TerminalIndex = FindShipTerminalInRange();
        const TArray<FHDShipTerminalInfo>& Terminals = HDShipHub::GetTerminals();
        if (Terminals.IsValidIndex(TerminalIndex))
        {
            OpenShipTerminal(Terminals[TerminalIndex].RootScreen, Terminals[TerminalIndex].Id);
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("[HDShip] interact none"));
        }
    }

    if (bPendingShipSystemMenu)
    {
        bPendingShipSystemMenu = false;
        OpenShipTerminal(EHDShipScreen::System, HDPlayerControllerShip::SystemTerminalId);
    }

    if (bPendingLaunchCancel)
    {
        bPendingLaunchCancel = false;

        UWorld* World = GetWorld();
        AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
        if (HDGameMode)
        {
            HDGameMode->CancelLaunch();
        }
    }

    // 미션 메뉴 키. 폰이 캐릭터가 아니어도(헬포드·폰 없음), 죽어 있어도 처리한다
    if (PendingMissionMenuKeys.IsEmpty() == false)
    {
        const TArray<EHDMissionMenuKey> MissionMenuKeys = MoveTemp(PendingMissionMenuKeys);
        PendingMissionMenuKeys.Reset();

        for (const EHDMissionMenuKey MissionMenuKey : MissionMenuKeys)
        {
            HandleMissionMenuKey(MissionMenuKey);
        }
    }

    if (PendingStratagemCommands.IsEmpty() && PendingWeaponSlots.IsEmpty() && bPendingStim == false && bPendingDive == false)
    {
        return;
    }

    // 헬포드 등 플레이어 캐릭터가 아니면 버린다
    AHDCharacterPlayer* HDPlayer = GetPawn<AHDCharacterPlayer>();
    if (HDPlayer)
    {
        for (const EHDCommandInput Command : PendingStratagemCommands)
        {
            HDPlayer->InputStratagemCommand(Command);
        }

        for (const EHDWeaponSlot WeaponSlot : PendingWeaponSlots)
        {
            HDPlayer->SelectWeaponSlot(WeaponSlot);
        }

        if (bPendingStim)
        {
            HDPlayer->UseStim();
        }

        if (bPendingDive)
        {
            HDPlayer->Dive();
        }
    }

    PendingStratagemCommands.Reset();
    PendingWeaponSlots.Reset();
    bPendingStim = false;
    bPendingDive = false;
}

void AHDPlayerController::SimulateKeyFrames(TArray<TArray<FHDSimulatedKeyEvent>> Frames)
{
    if (SimulatedKeyFrames.IsEmpty() == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HDSimulateKeys] busy"));
        return;
    }

    if (Frames.IsEmpty())
    {
        return;
    }

    SimulatedKeyFrames      = MoveTemp(Frames);
    SimulatedKeyFrameIndex  = 0;
    SendNextSimulatedKeyFrame();
}

void AHDPlayerController::SendNextSimulatedKeyFrame()
{
    if (SimulatedKeyFrames.IsEmpty())
    {
        return;
    }

    const TArray<FHDSimulatedKeyEvent> Frame = SimulatedKeyFrames[0];
    SimulatedKeyFrames.RemoveAt(0);

    FString Tokens;
    for (const FHDSimulatedKeyEvent& KeyEvent : Frame)
    {
        if (Tokens.IsEmpty() == false)
        {
            Tokens += TEXT(" ");
        }
        Tokens += KeyEvent.Key.GetFName().ToString() + (KeyEvent.Event == IE_Pressed ? TEXT("+") : TEXT("-"));
    }

    UE_LOG(LogTemp, Display, TEXT("[HDSimulateKeys] frame %d: %s"), SimulatedKeyFrameIndex, Tokens.IsEmpty() ? TEXT("(wait)") : *Tokens);

    for (const FHDSimulatedKeyEvent& KeyEvent : Frame)
    {
        InputKey(FInputKeyEventArgs::CreateSimulated(KeyEvent.Key, KeyEvent.Event, KeyEvent.Event == IE_Pressed ? 1.f : 0.f));
    }

    ++SimulatedKeyFrameIndex;

    if (SimulatedKeyFrames.IsEmpty())
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSimulateKeys] done (%d frames)"), SimulatedKeyFrameIndex);
        return;
    }

    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        SendNextSimulatedKeyFrame();
    }));
}

void AHDPlayerController::EnterShipMenu(bool bShowTitle)
{
    ShipMenu        = FHDShipMenu(bShowTitle ? EHDShipScreen::Title : EHDShipScreen::Bridge);
    bShipMenuActive = true;

    SetShowMouseCursor(true);
    bEnableClickEvents      = true;
    bEnableMouseOverEvents  = true;
    SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));

    ResetShipMouseAnchor();
}

void AHDPlayerController::ExitShipMenu()
{
    bShipMenuActive = false;
    PendingShipMenuKeys.Reset();

    SetShowMouseCursor(false);
    bEnableClickEvents      = false;
    bEnableMouseOverEvents  = false;
    SetInputMode(FInputModeGameOnly());

    // 단말기가 막은 이동·시점(출격 경로에서도 풀리게)
    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();
}

bool AHDPlayerController::IsInShipInterior() const
{
    const UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    return HDGameState && HDGameState->IsInShip() && Cast<AHDCharacterPlayer>(GetPawn()) != nullptr;
}

int32 AHDPlayerController::FindShipTerminalInRange() const
{
    const UWorld* World = GetWorld();
    const AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    const AHDShipInterior* ShipInterior = HDGameMode ? HDGameMode->GetShipInterior() : nullptr;
    const APawn* ControlledPawn = GetPawn();
    if (ShipInterior == nullptr || ControlledPawn == nullptr)
    {
        return INDEX_NONE;
    }

    return HDShipHub::FindTerminalInRange(ShipInterior->ToLocal(ControlledPawn->GetActorLocation()), HDShipHub::GetTerminals(), GetDefault<UHDProgressionSettings>()->ShipInteractRange);
}

bool AHDPlayerController::OpenShipTerminal(EHDShipScreen Root, FName TerminalId)
{
    if (IsInShipInterior() == false || bShipMenuActive)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] terminal refused %s"), *TerminalId.ToString());
        return false;
    }

    ShipMenu.OpenTerminal(Root);
    bShipMenuActive = true;
    OpenTerminalId  = TerminalId;

    // EnterShipMenu 와 같은 커서·클릭·입력 모드
    SetShowMouseCursor(true);
    bEnableClickEvents      = true;
    bEnableMouseOverEvents  = true;
    SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));

    // 단말기를 보는 동안 걷지도 돌아보지도 않는다(APawn::AddMovementInput·APlayerController::AddYawInput 이 본다)
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);

    ResetShipMouseAnchor();

    UE_LOG(LogTemp, Display, TEXT("[HDShip] terminal open %s screen=%s"), *TerminalId.ToString(), HDShipMenu::GetScreenName(Root));
    return true;
}

void AHDPlayerController::CloseShipTerminal()
{
    ExitShipMenu();
    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();

    UE_LOG(LogTemp, Display, TEXT("[HDShip] terminal close %s"), *OpenTerminalId.ToString());
    OpenTerminalId = NAME_None;
}

TArray<FHDShipMenuItem> AHDPlayerController::BuildShipMenuItems() const
{
    FHDShipMenuContext Context;
    const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
    if (ProfileSubsystem)
    {
        Context.Profile = ProfileSubsystem->GetProfile();
    }
    Context.Difficulties = GetDefault<UHDProgressionSettings>()->DifficultyLevels;
    Context.Upgrades = &FHDUpgradeCatalog::GetProjectCatalog();
    Context.Stratagems = &FHDStratagemCatalog::GetProjectCatalog();
    Context.DefaultLoadout = GetDefault<UHDStratagemSettings>()->Loadout;

    return HDShipMenu::BuildItems(ShipMenu.GetScreen(), Context);
}

void AHDPlayerController::HoverShipMenuItem(int32 Index)
{
    if (bShipMenuActive == false)
    {
        return;
    }

    // 화면을 열 때 마우스가 줄 위에 멈춰 있어도 새 히트박스가 가리키기를 부른다. 움직이기 전에는 커서를 그대로 둔다
    if (bShipMouseMoved == false)
    {
        float MouseX = 0.f;
        float MouseY = 0.f;
        if (GetMousePosition(MouseX, MouseY) == false || HDShipUILayout::HasMouseMoved(ShipMouseAnchor, FVector2D(MouseX, MouseY)) == false)
        {
            return;
        }
        bShipMouseMoved = true;
    }

    ShipMenu.SetCursor(Index, BuildShipMenuItems().Num());
}

void AHDPlayerController::ClickShipKeyHint(EHDShipMenuKey Key)
{
    if (bShipMenuActive == false)
    {
        return;
    }

    HandleShipMenuKey(Key);
}

bool AHDPlayerController::IsShipLaunching() const
{
    const UWorld* World = GetWorld();
    const AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    return HDGameMode && HDGameMode->IsLaunching();
}

void AHDPlayerController::ResetShipMouseAnchor()
{
    // 마우스가 뷰포트 밖이면 기준점이 없다. 그 뒤 가리키기는 마우스가 들어와 움직인 것이므로 기준점 (0, 0) 에서 멀어 참이 된다
    float MouseX = 0.f;
    float MouseY = 0.f;
    ShipMouseAnchor = GetMousePosition(MouseX, MouseY) ? FVector2D(MouseX, MouseY) : FVector2D::ZeroVector;
    bShipMouseMoved = false;
}

void AHDPlayerController::ClickShipMenuItem(int32 Index)
{
    if (bShipMenuActive == false)
    {
        return;
    }

    // -1 = 타이틀 화면 전체
    if (Index != -1)
    {
        ShipMenu.SetCursor(Index, BuildShipMenuItems().Num());
    }

    HandleShipMenuKey(EHDShipMenuKey::Confirm);
}

void AHDPlayerController::HandleShipMenuKey(EHDShipMenuKey Key)
{
    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;

    // 출격 대기 중에는 Back(대기 취소)만 받는다. 메뉴는 그대로(원작처럼 로드아웃 화면에서 카운트다운을 본다)
    if (HDGameMode && HDGameMode->IsLaunching())
    {
        if (Key == EHDShipMenuKey::Back)
        {
            HDGameMode->CancelLaunch();
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("[HDShip] key %s ignored (launching)"), HDShipMenu::GetKeyName(Key));
        }
        return;
    }

    const EHDShipScreen OldScreen = ShipMenu.GetScreen();
    const FHDShipMenuAction Action = ShipMenu.HandleKey(Key, BuildShipMenuItems());

    // 키 처리 뒤의 화면·커서와 그 줄(화면이 바뀌었으면 새 화면의 줄)
    const EHDShipScreen NewScreen = ShipMenu.GetScreen();
    const TArray<FHDShipMenuItem> Items = BuildShipMenuItems();
    const int32 Cursor = ShipMenu.GetCursor();
    const FString ItemLabel = Items.IsValidIndex(Cursor) ? Items[Cursor].Label : FString(TEXT("-"));

    UE_LOG(LogTemp, Display, TEXT("[HDShip] key %s screen=%s cursor=%d item=%s action=%s"),
        HDShipMenu::GetKeyName(Key),
        HDShipMenu::GetScreenName(NewScreen),
        Cursor,
        *ItemLabel,
        HDShipMenu::GetActionName(Action.Type));

    if (Action.Type == EHDShipActionType::OpenScreen)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] screen %s -> %s"), HDShipMenu::GetScreenName(OldScreen), HDShipMenu::GetScreenName(NewScreen));
    }

    // 화면이 바뀌면(타이틀 → Bridge 포함) 가리키기 기준점을 다시 잡는다
    if (NewScreen != OldScreen)
    {
        ResetShipMouseAnchor();
    }

    // 함선 모드에서 타이틀을 넘기면 Bridge 대신 함선 안으로 들어간다(메뉴는 게임모드가 닫는다)
    if (OldScreen == EHDShipScreen::Title && NewScreen != EHDShipScreen::Title && GetDefault<UHDProgressionSettings>()->bShipInterior)
    {
        NULL_CHECK(HDGameMode);
        HDGameMode->EnterShipInterior(this);
        return;
    }

    ExecuteShipMenuAction(Action);
}

void AHDPlayerController::ExecuteShipMenuAction(const FHDShipMenuAction& Action)
{
    switch (Action.Type)
    {
    case EHDShipActionType::Deploy:
    {
        UWorld* World = GetWorld();
        AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
        NULL_CHECK(HDGameMode);

        HDGameMode->RequestDeploy(this);
        break;
    }
    case EHDShipActionType::Quit:
        UE_LOG(LogTemp, Display, TEXT("[HDShip] quit"));
        UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
        break;
    case EHDShipActionType::CloseTerminal:
        CloseShipTerminal();
        break;
    case EHDShipActionType::Launch:
    {
        // 메뉴는 열어 둔다. 대기 중 키는 HandleShipMenuKey 가 막는다
        UWorld* World = GetWorld();
        AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
        NULL_CHECK(HDGameMode);

        HDGameMode->RequestLaunch(this);
        break;
    }
    case EHDShipActionType::Adjust:
    {
        // Loadout 화면의 칸 줄: 정리한 로드아웃에서 그 칸만 해금된 다음 이름으로 바꿔 프로필에 저장한다(다음 출격부터). 저장 로그는 서브시스템이 찍는다
        const int32 SlotIndex = HDPlayerControllerShip::GetLoadoutSlotIndex(Action.Id);
        if (SlotIndex != INDEX_NONE)
        {
            UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
            NULL_CHECK(ProfileSubsystem);

            const FHDStratagemCatalog& Catalog = FHDStratagemCatalog::GetProjectCatalog();
            const TArray<FName>& DefaultLoadout = GetDefault<UHDStratagemSettings>()->Loadout;
            const int32 Delta = Action.Value;
            FName NewName = NAME_None;
            ProfileSubsystem->ModifyProfile([&Catalog, &DefaultLoadout, &NewName, SlotIndex, Delta](FHDProfileData& Data)
            {
                TArray<FName> Loadout = HDArmory::ValidateLoadout(Data.Loadout, Data, Catalog, DefaultLoadout);
                if (Loadout.IsValidIndex(SlotIndex) == false)
                {
                    return;
                }

                Loadout[SlotIndex] = HDArmory::CycleSlot(Loadout, SlotIndex, Delta, Data, Catalog);
                NewName = Loadout[SlotIndex];
                Data.Loadout = Loadout;
            });

            UE_LOG(LogTemp, Display, TEXT("[HDLoadout] slot %d -> %s"), SlotIndex + 1, *NewName.ToString());
            break;
        }

        // Loadout 화면의 무기 줄: 그 칸의 열린 다음 무기를 프로필에 저장한다(다음 스폰부터, 브리프 9). 저장 로그는 서브시스템이 찍는다
        EHDLoadoutWeapon WeaponSlot = EHDLoadoutWeapon::Primary;
        if (HDPlayerControllerShip::TryGetLoadoutWeapon(Action.Id, WeaponSlot))
        {
            UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
            NULL_CHECK(ProfileSubsystem);

            const int32 Delta = Action.Value;
            FName NewWeapon = NAME_None;
            ProfileSubsystem->ModifyProfile([&NewWeapon, WeaponSlot, Delta](FHDProfileData& Data)
            {
                NewWeapon = HDWeaponCatalog::Cycle(Data, WeaponSlot, Delta);
                HDWeaponCatalog::SetChosen(Data, WeaponSlot, NewWeapon);
            });

            UE_LOG(LogTemp, Display, TEXT("[HDLoadout] %s -> %s"), *FString(HDWeaponCatalog::GetSlotName(WeaponSlot)).ToLower(), *NewWeapon.ToString());
            break;
        }

        if (Action.Id != HDPlayerControllerShip::DifficultyItemId)
        {
            break;
        }

        UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
        NULL_CHECK(ProfileSubsystem);

        // 끝 단계에서 더 누르면 값이 그대로다(감기지 않는다). 저장 로그는 서브시스템이 찍는다
        const TArray<FHDDifficultyLevel>& DifficultyTable = GetDefault<UHDProgressionSettings>()->DifficultyLevels;
        const int32 Delta = Action.Value;
        ProfileSubsystem->ModifyProfile([&DifficultyTable, Delta](FHDProfileData& Data)
        {
            Data.Difficulty = HDDifficulty::StepLevel(Data.Difficulty, Delta, DifficultyTable.Num());
        });

        const int32 Level = ProfileSubsystem->GetProfile().Difficulty;
        UE_LOG(LogTemp, Display, TEXT("[HDShip] difficulty %d %s"), Level, *HDDifficulty::GetLevel(DifficultyTable, Level).Label);
        break;
    }
    case EHDShipActionType::Select:
    {
        // Armory 화면의 줄 = 스트라타젬 이름 또는 무기 Id. 꺼진 줄(해금함·징발 모자람)은 메뉴가 None 을 돌려주므로 여기까지 오지 않는다
        if (ShipMenu.GetScreen() == EHDShipScreen::Armory)
        {
            UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
            NULL_CHECK(ProfileSubsystem);

            // 무기 표에 있으면 무기 해금(브리프 9). 해금한 무기는 다음에 LOADOUT 무기 줄 후보로 나온다
            if (HDWeaponCatalog::Find(Action.Id))
            {
                const FName WeaponId = Action.Id;
                const int32 OldRequisition = ProfileSubsystem->GetProfile().Requisition;
                EHDUnlockResult Result = EHDUnlockResult::Unknown;
                ProfileSubsystem->ModifyProfile([&Result, WeaponId](FHDProfileData& Data)
                {
                    Result = HDWeaponCatalog::TryUnlock(Data, WeaponId);
                });

                UE_LOG(LogTemp, Display, TEXT("[HDArmory] unlock weapon %s (%s) requisition %d -> %d"),
                    *WeaponId.ToString(),
                    HDArmory::GetUnlockResultName(Result),
                    OldRequisition,
                    ProfileSubsystem->GetProfile().Requisition);
                break;
            }

            // 해금한 스트라타젬은 다음에 LOADOUT 후보로 나온다. 저장 로그는 서브시스템이 찍는다
            const FHDStratagemCatalog& Catalog = FHDStratagemCatalog::GetProjectCatalog();
            const FName StratagemName = Action.Id;
            const int32 OldRequisition = ProfileSubsystem->GetProfile().Requisition;
            EHDUnlockResult Result = EHDUnlockResult::Unknown;
            ProfileSubsystem->ModifyProfile([&Result, &Catalog, StratagemName](FHDProfileData& Data)
            {
                Result = HDArmory::TryUnlock(Data, Catalog, StratagemName);
            });

            UE_LOG(LogTemp, Display, TEXT("[HDArmory] unlock %s (%s) requisition %d -> %d"),
                *StratagemName.ToString(),
                HDArmory::GetUnlockResultName(Result),
                OldRequisition,
                ProfileSubsystem->GetProfile().Requisition);
            break;
        }

        // Upgrades 화면의 줄 = 업그레이드 이름. 꺼진 줄은 메뉴가 None 을 돌려주므로 여기까지 오지 않는다
        if (ShipMenu.GetScreen() != EHDShipScreen::Upgrades)
        {
            break;
        }

        UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
        NULL_CHECK(ProfileSubsystem);

        // 효과는 다음 출격부터 들어간다. 저장 로그는 서브시스템이 찍는다
        const FHDUpgradeCatalog& Catalog = FHDUpgradeCatalog::GetProjectCatalog();
        const FName UpgradeName = Action.Id;
        const int32 OldRequisition = ProfileSubsystem->GetProfile().Requisition;
        EHDUpgradeBuyResult Result = EHDUpgradeBuyResult::Unknown;
        ProfileSubsystem->ModifyProfile([&Result, &Catalog, UpgradeName](FHDProfileData& Data)
        {
            Result = HDUpgrade::TryBuy(Data, UpgradeName, Catalog);
        });

        const FHDProfileData& Profile = ProfileSubsystem->GetProfile();
        UE_LOG(LogTemp, Display, TEXT("[HDUpgrade] buy %s -> %d (%s) requisition %d -> %d"),
            *UpgradeName.ToString(),
            HDUpgrade::GetLevel(Profile, UpgradeName),
            HDUpgrade::GetBuyResultName(Result),
            OldRequisition,
            Profile.Requisition);
        break;
    }
    default:
        break;
    }
}

void AHDPlayerController::RequestReturnToShip()
{
    UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    if (HDGameState == nullptr || HDGameState->HasMissionResult() == false)
    {
        return;
    }

    // 결과가 나온 직후의 남은 Enter 연타로 바로 넘어가지 않게 한다
    const double Now = World->GetTimeSeconds();
    const double ResultTime = HDGameState->GetMissionResult().ResultWorldTime;
    const float Delay = GetDefault<UHDProgressionSettings>()->ResultInputDelay;
    if (HDProgression::CanLeaveResult(Now, ResultTime, Delay) == false)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] return ignored (wait %.1fs)"), FMath::Max(0.0, Delay - (Now - ResultTime)));
        return;
    }

    AHDGameMode* HDGameMode = World->GetAuthGameMode<AHDGameMode>();
    NULL_CHECK(HDGameMode);

    HDGameMode->ReturnToShip();
}

void AHDPlayerController::ShowMissionResult()
{
    SetAbandonPromptOpen(false);

    // 결과 패널의 [ENTER] RETURN TO SHIP 을 마우스로도 누를 수 있게 한다. 함선 메뉴(bShipMenuActive)는 켜지 않는다
    SetShowMouseCursor(true);
    bEnableClickEvents      = true;
    bEnableMouseOverEvents  = true;
    SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
}

bool AHDPlayerController::IsAbandonPromptOpen() const
{
    return bAbandonPromptOpen;
}

void AHDPlayerController::HandleMissionMenuKey(EHDMissionMenuKey Key)
{
    UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    NULL_CHECK(HDGameState);

    // 결과가 있으면 Confirm 만 받는다(함선 복귀)
    if (HDGameState->HasMissionResult())
    {
        if (Key == EHDMissionMenuKey::Confirm)
        {
            RequestReturnToShip();
        }
        return;
    }

    // 확인 창: Confirm = 포기, ToggleAbandon·Cancel = 닫기
    if (bAbandonPromptOpen)
    {
        SetAbandonPromptOpen(false);

        if (Key == EHDMissionMenuKey::Confirm)
        {
            AHDGameMode* HDGameMode = World->GetAuthGameMode<AHDGameMode>();
            NULL_CHECK(HDGameMode);

            HDGameMode->RequestAbandon();
        }
        return;
    }

    // 미션 진행 중(함선 아님)에만 창을 연다
    const EHDMissionPhase Phase = HDGameState->GetMissionPhase();
    const bool bMissionRunning = HDGameState->IsInShip() == false
        && (Phase == EHDMissionPhase::Deploy || Phase == EHDMissionPhase::InProgress || Phase == EHDMissionPhase::Extraction);
    if (bMissionRunning && Key == EHDMissionMenuKey::ToggleAbandon)
    {
        SetAbandonPromptOpen(true);
    }
}

void AHDPlayerController::SetAbandonPromptOpen(bool bOpen)
{
    if (bAbandonPromptOpen == bOpen)
    {
        return;
    }

    bAbandonPromptOpen = bOpen;
    UE_LOG(LogTemp, Display, TEXT("[HDMission] abandon prompt %s"), bOpen ? TEXT("open") : TEXT("closed"));
}

void AHDPlayerController::BeginPlay()
{
    Super::BeginPlay();
}

void AHDPlayerController::OnPossess(APawn* aPawn)
{
    Super::OnPossess(aPawn);

    if(Cast<AHDCharacterPlayer>(aPawn))
    {
        SetCharacterControl(EHDCharacterControlType::ThirdPerson);

        UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
        NULL_CHECK(ASC);

        AHDHUD* HUD = GetHUD<AHDHUD>();
        NULL_CHECK(HUD);

        HUD->SetAbilitySystemComponent(ASC);

        // 함선 속 헬다이버에는 체력 막대·스트라타젬 패널을 만들지 않는다. 착지한 캐릭터가 처음 만들어 출격 때의 로드아웃으로 목록을 짓는다
        const UWorld* World = GetWorld();
        const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
        if (HDGameState && HDGameState->IsInShip())
        {
            UE_LOG(LogTemp, Display, TEXT("[HDShip] crew possessed (combat hud skipped)"));
        }
        else
        {
            HUD->CreateDefaultWidget();
        }

        // PlayerState ASC 는 다시 강하해도 같으므로 앞 빙의에서 묶은 것을 먼저 푼다
        FGameplayEventMulticastDelegate& EquipWeaponDelegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(Tag_Event_PlayerHUD_EquipWeapon);
        EquipWeaponDelegate.RemoveAll(HUD);
        EquipWeaponDelegate.AddUObject(HUD, &AHDHUD::OnEquipWeaponUIEventRecieved);

        FGameplayAbilityHelper::SendGameplayEventToSelf(Tag_Event_PlayerHUD_Initialize, GetAbilitySystemComponent());

        // 토글 명령이라 처음 한 번만. 켜져 있으면 AHUD::PostRender 가 DrawHUD 를 건너뛰어 미션 HUD 가 안 보이므로 hd.Debug.AbilitySystemOnSpawn 1 일 때만
        if (bAbilityDebugShown == false && CVarHDAbilitySystemDebugOnSpawn.GetValueOnGameThread() > 0)
        {
            ConsoleCommand(TEXT("showdebug abilitysystem"));
            bAbilityDebugShown = true;
        }
    }
}

void AHDPlayerController::OnUnPossess()
{
    Super::OnUnPossess();

    AbilitySystemComponent = nullptr;
}

void AHDPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UHDInputComponent* HDInput = Cast<UHDInputComponent>(InputComponent);
    NULL_CHECK(HDInput);

    CONDITION_CHECK(InputActionMap.Num() == static_cast<int32>(EHDCharacterInputAction::Count));

    HDInput->SetTaggedInputActionDataAsset(AbilityInputData, this, &ThisClass::AbilityInputTriggered, &ThisClass::AbilityInputReleased, &ThisClass::AbilityInputToggled);
    HDInput->BindAction(InputActionMap[EHDCharacterInputAction::ThirdMove], ETriggerEvent::Triggered, this, &ThisClass::Move);
    HDInput->BindAction(InputActionMap[EHDCharacterInputAction::ThirdLook], ETriggerEvent::Triggered, this, &ThisClass::Look);
}

void AHDPlayerController::AbilityInputTriggered(const FGameplayTag InputTag)
{
    // 함선 안에서는 사격·조준·재장전·자세·달리기·점프·스트라타젬 입력 모드를 모두 막는다
    CONDITION_CHECK_WITHOUT_LOG(IsInShipInterior() == false);

    UHDAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const FGameplayTag& AssetTag = AbilityInputData->GetTriggerTagByInputTag(InputTag);
    ASC->AbilityInputTagTriggered(AssetTag);
}

void AHDPlayerController::AbilityInputReleased(const FGameplayTag InputTag)
{
    CONDITION_CHECK_WITHOUT_LOG(IsInShipInterior() == false);

    UHDAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const FGameplayTag& AssetTag = AbilityInputData->GetTriggerTagByInputTag(InputTag);
    ASC->AbilityInputTagReleased(AssetTag);
}

void AHDPlayerController::AbilityInputToggled(const FGameplayTag InputTag)
{
    CONDITION_CHECK_WITHOUT_LOG(IsInShipInterior() == false);

    UHDAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const FGameplayTag& AssetTag = AbilityInputData->GetTriggerTagByInputTag(InputTag);
    ASC->AbilityInputTagToggled(AssetTag);
}

void AHDPlayerController::ChangeCharacterControlType()
{
    CurrentCharacterControlType = CurrentCharacterControlType == EHDCharacterControlType::FirstPerson 
        ? EHDCharacterControlType::ThirdPerson : EHDCharacterControlType::FirstPerson;

    SetCharacterControl(CurrentCharacterControlType);
}

void AHDPlayerController::SetCharacterControl(const EHDCharacterControlType NewCharacterControlType)
{
    CONDITION_CHECK(CharacterControlDataMap.Contains(NewCharacterControlType));
    
    CurrentCharacterControlType = NewCharacterControlType;

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    NULL_CHECK(Subsystem);

    Subsystem->ClearAllMappings();

    UInputMappingContext* NewMappingContext = CharacterControlDataMap[CurrentCharacterControlType]->InputMappingContext;
    NULL_CHECK(NewMappingContext);

    Subsystem->AddMappingContext(NewMappingContext, 0);

    AHDCharacterPlayer* HDPlayer = GetPawn<AHDCharacterPlayer>();
    NULL_CHECK(HDPlayer);

    HDPlayer->SetCharacterControlData(CharacterControlDataMap[CurrentCharacterControlType]);
}

void AHDPlayerController::Look(const FInputActionValue& Value)
{
    const FVector2D& LookAxisVector = Value.Get<FVector2D>();

    AddYawInput(LookAxisVector.X);
    AddPitchInput(LookAxisVector.Y);
}

void AHDPlayerController::Move(const FInputActionValue& Value)
{
    const FVector2D& MovementVector = Value.Get<FVector2D>();
    const FRotator& Rotation = GetControlRotation();

    const FRotator YawRotation(0, Rotation.Yaw, 0);

    const FVector& ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector& RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    APawn* ControlledPawn = GetPawn<APawn>();
    if (ControlledPawn)
    {
        ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.X);
        ControlledPawn->AddMovementInput(RightDirection, MovementVector.Y);
    }
}
