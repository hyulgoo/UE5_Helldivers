// Fill out your copyright notice in the Description page of Project Settings.

#include "HDHUD.h"
#include "UI/HDGASPlayerUserWidget.h"
#include "AbilitySystemComponent.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Attribute/HDWeaponAttributeSet.h"
#include "UI/HDReticleWidget.h"
#include "Weapon/HDWeapon.h"
#include "Define/HDDefine.h"
#include "Define//HDGameplayTag.h"
#include "Interface/HDWeaponInterface.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Component\HDStratagemComponent.h"
#include "HDStratagemHUDUserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Game/HDGameState.h"
#include "Game/HDMissionFlow.h"
#include "Mission/HDExtractionZone.h"
#include "Mission/HDMissionObjective.h"
#include "Mission/HDSamplePickup.h"
#include "Player/HDGASPlayerState.h"
#include "Controller/HDPlayerController.h"
#include "Progression/HDProfileSubsystem.h"
#include "Progression/HDProgressionSettings.h"
#include "Progression/HDMissionReward.h"
#include "Progression/HDSampleRules.h"
#include "UI/HDShipMenu.h"
#include "Ship/HDShipHubRules.h"
#include "Engine/Canvas.h"

namespace HDHUDMission
{
    const FLinearColor  TextColor(1.f, 0.85f, 0.2f);
    const FLinearColor  FailColor(1.f, 0.f, 0.f);
    // 왼쪽 위는 스트라타젬 패널(위젯)이 차지하므로 미션 글자는 오른쪽 위에 오른쪽 정렬
    constexpr float     TextRight = 40.f;
    constexpr float     TextTop = 40.f;
    constexpr float     LineHeight = 26.f;

    // 화면 가운데 MISSION COMPLETE / MISSION FAILED
    constexpr float     ResultTextScale = 2.f;

    // 화면 가운데 REINFORCING IN <초>
    constexpr float     RespawnTextScale = 2.f;

    // 추출 지점 표식: 위치 + Z 300
    constexpr float     ExtractionMarkerHeight = 300.f;

    // 샘플 표식: 위치 + Z 100
    constexpr float     SampleMarkerHeight = 100.f;

    // 히트마커: 화면 가운데에서 HitMarkerGap 떨어져 HitMarkerLength 길이의 대각선 넷(X)
    constexpr float     HitMarkerDuration = 0.25f;
    constexpr float     HitMarkerGap = 6.f;
    constexpr float     HitMarkerLength = 10.f;
    constexpr float     HitMarkerThickness = 2.f;
    // DEFLECT 글자: X 아래쪽 끝에서 이만큼 아래
    constexpr float     HitMarkerTextGap = 6.f;
    const FLinearColor  HitMarkerFullColor(1.f, 0.1f, 0.1f);
    const FLinearColor  HitMarkerWeakColor(1.f, 0.55f, 0.f);
    const FLinearColor  HitMarkerReducedColor(1.f, 1.f, 1.f);
    const FLinearColor  HitMarkerRicochetColor(0.55f, 0.55f, 0.55f);

    // 추출 호출 안내의 방향 글자: "LEFT UP RIGHT DOWN" (HDMissionRules::GetExtractionCallSequence 순서)
    FString MakeExtractionCallText()
    {
        TArray<FString> Words;
        for (const EHDCommandInput Command : HDMissionRules::GetExtractionCallSequence())
        {
            switch (Command)
            {
            case EHDCommandInput::Up:       Words.Add(TEXT("UP"));      break;
            case EHDCommandInput::Down:     Words.Add(TEXT("DOWN"));    break;
            case EHDCommandInput::Left:     Words.Add(TEXT("LEFT"));    break;
            case EHDCommandInput::Right:    Words.Add(TEXT("RIGHT"));   break;
            default:                        break;
            }
        }
        return FString::Join(Words, TEXT(" "));
    }
}

namespace HDHUDShip
{
    // 화면 전체를 덮는 반투명 검정
    const FLinearColor  BackgroundColor(0.f, 0.f, 0.f, 0.6f);
    const FLinearColor  ItemColor(1.f, 1.f, 1.f);
    const FLinearColor  DisabledItemColor(0.5f, 0.5f, 0.5f);

    // 타이틀: 화면 높이 비율 위치에 가운데 정렬
    constexpr float     TitleTextScale = 2.5f;
    constexpr float     TitleTextHeightRatio = 0.35f;
    constexpr float     PromptTextHeightRatio = 0.6f;

    // Bridge 위쪽: 왼쪽 제목, 오른쪽 자원 줄
    constexpr float     HeaderMargin = 40.f;
    constexpr float     HeaderTextScale = 1.5f;

    // Bridge 줄 목록과 줄마다 히트박스(글자 위아래로 여유를 둔다)
    constexpr float     ItemLeft = 120.f;
    constexpr float     ItemTop = 180.f;
    constexpr float     ItemSpacing = 44.f;
    constexpr float     ItemHitBoxWidth = 600.f;
    constexpr float     ItemHitBoxHeight = 40.f;
    constexpr float     ItemHitBoxTopOffset = 10.f;

    // Bridge 아래쪽: 화면 아래에서 이만큼 위
    constexpr float     DetailBottom = 140.f;
    constexpr float     HelpBottom = 80.f;

    // 자원 줄과 그 아래 LAST MISSION 줄 사이
    constexpr float     LastResultGap = 8.f;

    const TCHAR* const  TitleText = TEXT("HD SOLO OPERATIONS");
    const TCHAR* const  PromptText = TEXT("PRESS ENTER");
    const TCHAR* const  HelpText = TEXT("[UP/DOWN] SELECT   [LEFT/RIGHT] CHANGE   [ENTER] CONFIRM   [BACKSPACE] BACK");

    // 함선 안(메뉴 닫힘): 아래 가운데 단말기 안내, 왼쪽 아래 키 안내
    constexpr float     InteractPromptScale = 1.5f;
    constexpr float     InteractPromptBottom = 180.f;
    const TCHAR* const  InteriorHelpText = TEXT("[E] USE   [P] SHIP MENU");

    // 출격 대기: 위쪽 가운데 큰 글자와 그 아래 취소 안내
    constexpr float     CountdownTextScale = 2.f;
    constexpr float     CountdownHeightRatio = 0.1f;
    constexpr float     CountdownCancelGap = 8.f;
    const TCHAR* const  CountdownCancelText = TEXT("[BACKSPACE] CANCEL");

    const FName         TitleHitBoxName(TEXT("ShipTitle"));
    const FString       ItemHitBoxPrefix(TEXT("ShipItem_"));

    FName MakeItemHitBoxName(int32 Index)
    {
        return FName(*FString::Printf(TEXT("%s%d"), *ItemHitBoxPrefix, Index));
    }

    // ShipTitle → -1, ShipItem_<i> → i. 함선 메뉴 히트박스가 아니면 거짓
    bool TryParseHitBoxName(FName BoxName, int32& OutIndex)
    {
        if (BoxName == TitleHitBoxName)
        {
            OutIndex = -1;
            return true;
        }

        const FString Name = BoxName.ToString();
        if (Name.StartsWith(ItemHitBoxPrefix, ESearchCase::CaseSensitive) == false)
        {
            return false;
        }

        const FString IndexText = Name.RightChop(ItemHitBoxPrefix.Len());
        if (IndexText.IsNumeric() == false)
        {
            return false;
        }

        OutIndex = FCString::Atoi(*IndexText);
        return true;
    }
}

namespace HDHUDResult
{
    // 가운데 상자(결과 패널·포기 확인 창): 반투명 검정, 안쪽 여백, 줄 사이
    const FLinearColor  PanelColor(0.f, 0.f, 0.f, 0.7f);
    const FLinearColor  LineColor(1.f, 1.f, 1.f);
    constexpr float     PanelPadding = 40.f;
    constexpr float     LineGap = 12.f;

    const TCHAR* const  ReturnText = TEXT("[ENTER] RETURN TO SHIP");
    const FName         ReturnHitBoxName(TEXT("ResultReturn"));

    const TCHAR* const  AbandonTitleText = TEXT("ABANDON MISSION?");
    const TCHAR* const  AbandonHelpText = TEXT("[ENTER] CONFIRM   [P] CANCEL");
    constexpr float     AbandonTitleScale = 2.f;

    // 상자 안 한 줄
    struct FPanelLine
    {
        FString         Text;
        FLinearColor    Color;
        UFont*          Font    = nullptr;
        float           Scale   = 1.f;
        // 참이면 이 줄에 ResultReturn 히트박스를 둔다
        bool            bReturn = false;
    };

    // 초 → "mm:ss"
    FString FormatTime(float Seconds)
    {
        const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(Seconds));
        return FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60);
    }

    // 화면 가운데 상자에 줄마다 가로 가운데 정렬로 그린다. bReturn 줄은 상자 폭의 히트박스(ResultReturn)
    void DrawPanel(AHUD& HUD, const UCanvas& Canvas, const TArray<FPanelLine>& Lines)
    {
        TArray<FVector2D> Sizes;
        float MaxWidth = 0.f;
        float TotalHeight = 0.f;
        for (const FPanelLine& Line : Lines)
        {
            float Width = 0.f;
            float Height = 0.f;
            HUD.GetTextSize(Line.Text, Width, Height, Line.Font, Line.Scale);
            Sizes.Add(FVector2D(Width, Height));
            MaxWidth = FMath::Max(MaxWidth, Width);
            TotalHeight += Height;
        }
        TotalHeight += LineGap * FMath::Max(0, Lines.Num() - 1);

        const float PanelWidth = MaxWidth + PanelPadding * 2.f;
        const float PanelHeight = TotalHeight + PanelPadding * 2.f;
        const float PanelX = (Canvas.ClipX - PanelWidth) * 0.5f;
        const float PanelY = (Canvas.ClipY - PanelHeight) * 0.5f;
        HUD.DrawRect(PanelColor, PanelX, PanelY, PanelWidth, PanelHeight);

        float LineY = PanelY + PanelPadding;
        for (int32 Index = 0; Index < Lines.Num(); ++Index)
        {
            const FPanelLine& Line = Lines[Index];
            const FVector2D& Size = Sizes[Index];
            HUD.DrawText(Line.Text, Line.Color, (Canvas.ClipX - Size.X) * 0.5f, LineY, Line.Font, Line.Scale);

            if (Line.bReturn)
            {
                HUD.AddHitBox(FVector2D(PanelX, LineY - LineGap * 0.5f), FVector2D(PanelWidth, Size.Y + LineGap), ReturnHitBoxName, true);
            }

            LineY += Size.Y + LineGap;
        }
    }
}

void AHDHUD::SetAbilitySystemComponent(UAbilitySystemComponent* ASC)
{
    NULL_CHECK(ASC);
    AbilitySystemComponent = ASC;
}

void AHDHUD::CreateDefaultWidget()
{
    NULL_CHECK_WITHOUT_LOG(AbilitySystemComponent);

    // 빙의마다 불린다. ASC 는 같은 PlayerState ASC 라 위젯과 델리게이트는 처음 한 번만 만든다
    if(PlayerHUDWidgetClass)
    {
        if (PlayerHUDWidget == nullptr)
        {
            PlayerHUDWidget = CreateWidget<UHDGASPlayerUserWidget>(GetWorld(), PlayerHUDWidgetClass);
            NULL_CHECK(PlayerHUDWidget);

            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDHealthAttributeSet::GetCurrentHealthAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnHealthAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDHealthAttributeSet::GetMaxHealthAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnHealthAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetAmmoAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetMaxAmmoAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetCapacityAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetMaxCapacityAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetGrenadeAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetMaxGrenadeAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetStimAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetMaxStimAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);

            PlayerHUDWidget->AddToViewport();
        }

        // 다시 강하하면 초기 GE 를 다시 적용한 값으로 막대를 맞춘다
        PlayerHUDWidget->InitializeHealth(AbilitySystemComponent->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()), AbilitySystemComponent->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute()));
    }

    if (StratagemWidgetClass)
    {
        UHDStratagemComponent* StratagemComponent = PlayerOwner->GetPawn()->GetComponentByClass<UHDStratagemComponent>();
        NULL_CHECK(StratagemComponent);

        if (StratagemWidget)
        {
            StratagemWidget->SetStratagemComponent(StratagemComponent);
        }
        else
        {
            StratagemWidget = CreateWidget<UHDStratagemHUDUserWidget>(GetWorld(), StratagemWidgetClass);
            NULL_CHECK(StratagemWidget);

            const FGameplayTagContainer& StratagemEventTags = FGameplayAbilityHelper::GetAllChildTag(Tag_Event_StratagemHUD);
            for (const FGameplayTag& StratagemEventTag : StratagemEventTags)
            {
                AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(StratagemEventTag).AddUObject(StratagemWidget, &UHDStratagemHUDUserWidget::OnStratagemEventReceived);
            }

            StratagemWidget->InitializeStratagemHUD(StratagemComponent);
            StratagemWidget->AddToViewport();
        }
    }
}

void AHDHUD::OnEquipWeaponUIEventRecieved(const FGameplayEventData* Payload)
{
    // 함선 안에는 조준선이 없다
    const UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    CONDITION_CHECK_WITHOUT_LOG(HDGameState == nullptr || HDGameState->IsInShip() == false);

    NULL_CHECK_WITHOUT_LOG(AbilitySystemComponent);
    NULL_CHECK(ReticleWidgetClass);

    // 다시 강하하거나 무기를 바꿔 두 번째로 불려도 위젯은 하나만 둔다. 확산 연결(아래 주석)이 끊겨 있어 CurrentWeapon 은 비어 있을 수 있다
    if (CurrentWeapon)
    {
        CurrentWeapon->OnSpreadChanged.Remove(SpreadChangedHandle);
    }

    if (ReticleWidget == nullptr)
    {
        ReticleWidget = CreateWidget<UHDReticleWidget>(GetWorld(), ReticleWidgetClass);
        NULL_CHECK(ReticleWidget);

        ReticleWidget->AddToViewport();
    }

    TScriptInterface<IHDWeaponInterface> WeaponInterfae = PlayerOwner->GetPawn();
    NULL_CHECK(WeaponInterfae);

    //CurrentWeapon = WeaponInterfae->GetWeapon();
    //if (CurrentWeapon)
    //{
    //    SpreadChangedHandle = CurrentWeapon->OnSpreadChanged.AddUObject(ReticleWidget, &UHDReticleWidget::UpdateCrosshair);
    //}
}

void AHDHUD::RebuildStratagemList()
{
    NULL_CHECK_WITHOUT_LOG(StratagemWidget);
    StratagemWidget->RebuildStratagemList();
}

void AHDHUD::DrawHUD()
{
    // AHUD::DrawHUD 가 매 프레임 히트박스를 지우므로 함선 메뉴의 히트박스는 그 뒤에 더한다
    Super::DrawHUD();

    // 함선 단계에서는 메뉴(닫혀 있으면 함선 안 안내)와 출격 대기만 그린다(미션 글자·표식·히트마커는 그리지 않는다)
    const UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    if (HDGameState && HDGameState->IsInShip())
    {
        const AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
        if (HDPlayerController && HDPlayerController->IsShipMenuActive())
        {
            DrawShipMenu();
        }
        else
        {
            DrawShipOverlay();
        }

        if (HDGameState->GetLaunchRemaining() >= 0.f)
        {
            DrawLaunchCountdown(HDGameState->GetLaunchRemaining());
        }
        return;
    }

    DrawMissionText();
    DrawObjectiveMarkers();
    DrawHitMarker();
    DrawAbandonPrompt();
}

void AHDHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);

    AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    if (HDPlayerController && BoxName == HDHUDResult::ReturnHitBoxName)
    {
        HDPlayerController->RequestReturnToShip();
        return;
    }

    int32 Index = 0;
    if (HDPlayerController && HDHUDShip::TryParseHitBoxName(BoxName, Index))
    {
        HDPlayerController->ClickShipMenuItem(Index);
    }
}

void AHDHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
    Super::NotifyHitBoxBeginCursorOver(BoxName);

    AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    int32 Index = 0;
    if (HDPlayerController && HDHUDShip::TryParseHitBoxName(BoxName, Index))
    {
        HDPlayerController->HoverShipMenuItem(Index);
    }
}

void AHDHUD::DrawShipMenu()
{
    using namespace HDHUDShip;

    const AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    if (Canvas == nullptr || HDPlayerController == nullptr)
    {
        return;
    }

    const float ScreenWidth = Canvas->ClipX;
    const float ScreenHeight = Canvas->ClipY;
    UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
    UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;

    DrawRect(BackgroundColor, 0.f, 0.f, ScreenWidth, ScreenHeight);

    // 가로 가운데 정렬 글자
    auto DrawCenteredText = [&](const FString& Text, const FLinearColor& Color, float CenterY, UFont* Font, float Scale)
    {
        float TextWidth = 0.f;
        float TextHeight = 0.f;
        GetTextSize(Text, TextWidth, TextHeight, Font, Scale);
        DrawText(Text, Color, (ScreenWidth - TextWidth) * 0.5f, CenterY - TextHeight * 0.5f, Font, Scale);
    };

    const FHDShipMenu& ShipMenu = HDPlayerController->GetShipMenu();
    if (ShipMenu.GetScreen() == EHDShipScreen::Title)
    {
        DrawCenteredText(TitleText, HDHUDMission::TextColor, ScreenHeight * TitleTextHeightRatio, LargeFont, TitleTextScale);
        DrawCenteredText(PromptText, ItemColor, ScreenHeight * PromptTextHeightRatio, MediumFont, 1.f);

        AddHitBox(FVector2D(0.f, 0.f), FVector2D(ScreenWidth, ScreenHeight), TitleHitBoxName, true);
        return;
    }

    // 위쪽: 왼쪽 화면 머리글, 오른쪽 자원 줄과 마지막 결과 줄
    DrawText(HDShipMenu::GetScreenTitle(ShipMenu.GetScreen()), HDHUDMission::TextColor, HeaderMargin, HeaderMargin, LargeFont, HeaderTextScale);
    DrawShipResources();

    // 줄 목록: 고른 줄은 "> " + 노랑, 다른 줄 흰색, 꺼진 줄 회색
    const TArray<FHDShipMenuItem> Items = HDPlayerController->BuildShipMenuItems();
    const int32 Cursor = ShipMenu.GetCursor();
    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        const FHDShipMenuItem& Item = Items[Index];
        const bool bSelected = Index == Cursor;
        const FLinearColor Color = Item.bEnabled == false ? DisabledItemColor : (bSelected ? HDHUDMission::TextColor : ItemColor);
        const float RowY = ItemTop + ItemSpacing * Index;

        DrawText(bSelected ? TEXT("> ") + Item.Label : Item.Label, Color, ItemLeft, RowY, MediumFont);
        AddHitBox(FVector2D(ItemLeft, RowY - ItemHitBoxTopOffset), FVector2D(ItemHitBoxWidth, ItemHitBoxHeight), MakeItemHitBoxName(Index), true);
    }

    // 아래쪽: 고른 줄의 설명, 키 안내
    if (Items.IsValidIndex(Cursor))
    {
        DrawText(Items[Cursor].Detail, ItemColor, ItemLeft, ScreenHeight - DetailBottom, MediumFont);
    }
    DrawText(HelpText, DisabledItemColor, ItemLeft, ScreenHeight - HelpBottom, MediumFont);
}

void AHDHUD::DrawShipResources()
{
    using namespace HDHUDShip;

    if (Canvas == nullptr)
    {
        return;
    }

    const float ScreenWidth = Canvas->ClipX;
    UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;

    // 자원 줄(오른쪽 정렬)
    const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
    const FString ResourcesText = HDShipMenu::FormatResources(ProfileSubsystem ? ProfileSubsystem->GetProfile() : FHDProfileData());
    float ResourcesWidth = 0.f;
    float ResourcesHeight = 0.f;
    GetTextSize(ResourcesText, ResourcesWidth, ResourcesHeight, MediumFont);
    DrawText(ResourcesText, ItemColor, ScreenWidth - HeaderMargin - ResourcesWidth, HeaderMargin, MediumFont);

    // 자원 줄 아래: 게임 인스턴스가 기억하는 마지막 미션 결과(오른쪽 정렬)
    const FHDMissionResultSummary* LastResult = ProfileSubsystem ? ProfileSubsystem->GetLastResult() : nullptr;
    if (LastResult)
    {
        const FString LastResultText = FString::Printf(TEXT("LAST MISSION  %s  +%d REQ"),
            LastResult->Outcome.bSuccess ? TEXT("COMPLETE") : TEXT("FAILED"),
            LastResult->Reward.Requisition);

        float LastResultWidth = 0.f;
        float LastResultHeight = 0.f;
        GetTextSize(LastResultText, LastResultWidth, LastResultHeight, MediumFont);
        DrawText(LastResultText, ItemColor, ScreenWidth - HeaderMargin - LastResultWidth, HeaderMargin + ResourcesHeight + LastResultGap, MediumFont);
    }
}

void AHDHUD::DrawShipOverlay()
{
    using namespace HDHUDShip;

    const AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    if (Canvas == nullptr || HDPlayerController == nullptr)
    {
        return;
    }

    const float ScreenWidth = Canvas->ClipX;
    const float ScreenHeight = Canvas->ClipY;
    UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;

    // 오른쪽 위: 자원 줄·마지막 결과 줄(배경 없음)
    DrawShipResources();

    // 아래 가운데: 범위 안 단말기 안내
    const int32 TerminalIndex = HDPlayerController->FindShipTerminalInRange();
    const TArray<FHDShipTerminalInfo>& Terminals = HDShipHub::GetTerminals();
    if (Terminals.IsValidIndex(TerminalIndex))
    {
        const FString InteractText = HDShipHub::FormatInteractPrompt(Terminals[TerminalIndex]);
        float InteractWidth = 0.f;
        float InteractHeight = 0.f;
        GetTextSize(InteractText, InteractWidth, InteractHeight, MediumFont, InteractPromptScale);
        DrawText(InteractText, HDHUDMission::TextColor, (ScreenWidth - InteractWidth) * 0.5f, ScreenHeight - InteractPromptBottom, MediumFont, InteractPromptScale);
    }

    // 왼쪽 아래: 키 안내
    DrawText(InteriorHelpText, DisabledItemColor, HeaderMargin, ScreenHeight - HelpBottom, MediumFont);
}

void AHDHUD::DrawLaunchCountdown(float RemainingSeconds)
{
    using namespace HDHUDShip;

    if (Canvas == nullptr)
    {
        return;
    }

    const float ScreenWidth = Canvas->ClipX;
    const float ScreenHeight = Canvas->ClipY;
    UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
    UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;

    // 위쪽 가운데: 큰 글자 HELLPOD LAUNCH IN n, 그 아래 취소 안내
    const FString CountdownText = HDShipHub::FormatCountdown(RemainingSeconds);
    float CountdownWidth = 0.f;
    float CountdownHeight = 0.f;
    GetTextSize(CountdownText, CountdownWidth, CountdownHeight, LargeFont, CountdownTextScale);
    const float CountdownY = ScreenHeight * CountdownHeightRatio;
    DrawText(CountdownText, HDHUDMission::TextColor, (ScreenWidth - CountdownWidth) * 0.5f, CountdownY, LargeFont, CountdownTextScale);

    float CancelWidth = 0.f;
    float CancelHeight = 0.f;
    GetTextSize(CountdownCancelText, CancelWidth, CancelHeight, MediumFont);
    DrawText(CountdownCancelText, ItemColor, (ScreenWidth - CancelWidth) * 0.5f, CountdownY + CountdownHeight + CountdownCancelGap, MediumFont);
}

void AHDHUD::NotifyHitFeedback(EHDHitOutcome Outcome, bool bWeakPoint)
{
    const UWorld* World = GetWorld();
    NULL_CHECK_WITHOUT_LOG(World);

    LastHitOutcome = Outcome;
    bLastHitWeakPoint = bWeakPoint;
    LastHitTime = World->GetTimeSeconds();

    UE_LOG(LogTemp, Display, TEXT("[HDHUD] hit marker %s%s"), HDArmor::GetOutcomeName(Outcome), bWeakPoint ? TEXT(" weak") : TEXT(""));
}

void AHDHUD::DrawHitMarker()
{
    using namespace HDHUDMission;

    const UWorld* World = GetWorld();
    if (World == nullptr || Canvas == nullptr || LastHitTime < 0.0)
    {
        return;
    }

    if (World->GetTimeSeconds() - LastHitTime > HitMarkerDuration)
    {
        return;
    }

    FLinearColor Color = HitMarkerFullColor;
    switch (LastHitOutcome)
    {
    case EHDHitOutcome::Full:
        Color = bLastHitWeakPoint ? HitMarkerWeakColor : HitMarkerFullColor;
        break;
    case EHDHitOutcome::Reduced:
        Color = HitMarkerReducedColor;
        break;
    case EHDHitOutcome::Ricochet:
        Color = HitMarkerRicochetColor;
        break;
    default:
        break;
    }

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;

    // 대각선 방향 넷. 가운데는 비워 조준점을 가리지 않는다
    const float Inner = HitMarkerGap * UE_INV_SQRT_2;
    const float Outer = (HitMarkerGap + HitMarkerLength) * UE_INV_SQRT_2;
    for (const float SignX : { -1.f, 1.f })
    {
        for (const float SignY : { -1.f, 1.f })
        {
            DrawLine(CenterX + SignX * Inner, CenterY + SignY * Inner, CenterX + SignX * Outer, CenterY + SignY * Outer, Color, HitMarkerThickness);
        }
    }

    if (LastHitOutcome == EHDHitOutcome::Ricochet)
    {
        const FString DeflectText = TEXT("DEFLECT");
        UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;

        float TextWidth = 0.f;
        float TextHeight = 0.f;
        GetTextSize(DeflectText, TextWidth, TextHeight, Font);
        DrawText(DeflectText, Color, CenterX - TextWidth * 0.5f, CenterY + Outer + HitMarkerTextGap, Font);
    }
}

void AHDHUD::DrawMissionText()
{
    using namespace HDHUDMission;

    UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    NULL_CHECK_WITHOUT_LOG(HDGameState);

    const EHDMissionPhase Phase = HDGameState->GetMissionPhase();

    TArray<FString> Lines;
    switch (Phase)
    {
    case EHDMissionPhase::Deploy:
        Lines.Add(TEXT("DEPLOYING"));
        break;
    case EHDMissionPhase::InProgress:
        Lines.Add(FString::Printf(TEXT("OBJECTIVE  DESTROY FACILITIES  %d/%d"), HDGameState->GetCompletedObjectives(), HDGameState->GetRequiredObjectives()));
        break;
    case EHDMissionPhase::Extraction:
    {
        // 부르기 전(호출이 필요한 설정일 때만): 지점 밖이면 들어가라, 안이면 호출 커맨드
        if (HDGameState->IsExtractionCalled() == false)
        {
            Lines.Add(HDGameState->IsPlayerInExtractionZone()
                ? FString::Printf(TEXT("EXTRACTION  CALL  CTRL + %s"), *MakeExtractionCallText())
                : FString(TEXT("EXTRACTION  ENTER EXTRACTION ZONE")));
            break;
        }

        // 부른 뒤: 남은 시간 mm:ss
        const int32 RemainingSeconds = FMath::Max(0, FMath::FloorToInt(HDGameState->GetExtractionRemaining()));
        Lines.Add(FString::Printf(TEXT("EXTRACTION  HOLD  %02d:%02d"), RemainingSeconds / 60, RemainingSeconds % 60));
        Lines.Add(HDGameState->IsPlayerInExtractionZone() ? TEXT("HOLD THE ZONE") : TEXT("ENTER EXTRACTION ZONE"));
        break;
    }
    default:
        break;
    }
    Lines.Add(FString::Printf(TEXT("REINFORCEMENTS  %d"), HDGameState->GetReinforcementsLeft()));

    // 이번 미션에서 지닌 샘플(플레이어스테이트가 없으면 줄을 넣지 않는다)
    const AHDGASPlayerState* HDPlayerState = PlayerOwner ? PlayerOwner->GetPlayerState<AHDGASPlayerState>() : nullptr;
    if (HDPlayerState)
    {
        Lines.Add(HDSamples::FormatCarried(HDPlayerState->GetCarriedSamples()));
    }

    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    float LineY = TextTop;
    for (const FString& Line : Lines)
    {
        float LineWidth = 0.f;
        float LineTextHeight = 0.f;
        GetTextSize(Line, LineWidth, LineTextHeight, Font);
        const float LineX = Canvas ? Canvas->ClipX - TextRight - LineWidth : TextRight;
        DrawText(Line, TextColor, LineX, LineY, Font);
        LineY += LineHeight;
    }

    // 끝나면 화면 가운데 결과. 정산한 결과가 있으면 결과 패널, 없으면 글자만
    if (HDGameState->HasMissionResult())
    {
        DrawResultPanel();
    }
    else if ((Phase == EHDMissionPhase::Success || Phase == EHDMissionPhase::Fail) && Canvas)
    {
        const bool bSuccess = Phase == EHDMissionPhase::Success;
        const FString ResultText = bSuccess ? TEXT("MISSION COMPLETE") : TEXT("MISSION FAILED");
        UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;

        float TextWidth = 0.f;
        float TextHeight = 0.f;
        GetTextSize(ResultText, TextWidth, TextHeight, LargeFont, ResultTextScale);
        DrawText(ResultText, bSuccess ? TextColor : FailColor, (Canvas->ClipX - TextWidth) * 0.5f, (Canvas->ClipY - TextHeight) * 0.5f, LargeFont, ResultTextScale);
    }

    // 증원 대기 중이면 화면 가운데 남은 초(올림)
    const float RespawnRemaining = HDGameState->GetRespawnRemaining();
    if (RespawnRemaining >= 0.f && Canvas)
    {
        const FString RespawnText = FString::Printf(TEXT("REINFORCING IN %d"), FMath::CeilToInt(RespawnRemaining));
        UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;

        float TextWidth = 0.f;
        float TextHeight = 0.f;
        GetTextSize(RespawnText, TextWidth, TextHeight, LargeFont, RespawnTextScale);
        DrawText(RespawnText, TextColor, (Canvas->ClipX - TextWidth) * 0.5f, (Canvas->ClipY - TextHeight) * 0.5f, LargeFont, RespawnTextScale);
    }
}

void AHDHUD::DrawResultPanel()
{
    using namespace HDHUDResult;

    const UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    if (Canvas == nullptr || HDGameState == nullptr || HDGameState->HasMissionResult() == false)
    {
        return;
    }

    const FHDMissionResultSummary& Summary = HDGameState->GetMissionResult();
    const FHDMissionOutcome& Outcome = Summary.Outcome;
    const FHDMissionReward& Reward = Summary.Reward;

    UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
    UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;

    TArray<FPanelLine> Lines;

    // 맨 위 글자는 결과 패널 전의 가운데 글자와 같다(큰 글꼴 ×2)
    Lines.Add({ Outcome.bSuccess ? TEXT("MISSION COMPLETE") : TEXT("MISSION FAILED"),
        Outcome.bSuccess ? HDHUDMission::TextColor : HDHUDMission::FailColor, LargeFont, HDHUDMission::ResultTextScale });

    if (Outcome.bAbandoned)
    {
        Lines.Add({ TEXT("ABANDONED"), HDHUDMission::FailColor, MediumFont });
    }

    Lines.Add({ FString::Printf(TEXT("DIFFICULTY  %d  %s"), Outcome.Difficulty, *Outcome.DifficultyLabel), LineColor, MediumFont });
    Lines.Add({ FString::Printf(TEXT("OBJECTIVES  %d/%d"), Outcome.CompletedObjectives, Outcome.RequiredObjectives), LineColor, MediumFont });
    Lines.Add({ FString::Printf(TEXT("MISSION TIME  %s"), *FormatTime(Outcome.ElapsedSeconds)), LineColor, MediumFont });
    Lines.Add({ FString::Printf(TEXT("REQUISITION  +%d   (TOTAL %d)"), Reward.Requisition, Summary.TotalRequisition), LineColor, MediumFont });

    // 잃은 샘플이 있으면 그 줄을 빨강으로
    if (Reward.LostSamples.IsEmpty() == false)
    {
        Lines.Add({ FString::Printf(TEXT("SAMPLES LOST  %d / %d / %d"), Reward.LostSamples.Common, Reward.LostSamples.Rare, Reward.LostSamples.Super), HDHUDMission::FailColor, MediumFont });
    }
    else
    {
        Lines.Add({ FString::Printf(TEXT("SAMPLES  +%d / +%d / +%d"), Reward.Samples.Common, Reward.Samples.Rare, Reward.Samples.Super), LineColor, MediumFont });
    }

    // 입력 지연이 지난 뒤에만 복귀 안내와 히트박스
    if (HDProgression::CanLeaveResult(World->GetTimeSeconds(), Summary.ResultWorldTime, GetDefault<UHDProgressionSettings>()->ResultInputDelay))
    {
        Lines.Add({ ReturnText, HDHUDMission::TextColor, MediumFont, 1.f, true });
    }

    DrawPanel(*this, *Canvas, Lines);
}

void AHDHUD::DrawAbandonPrompt()
{
    using namespace HDHUDResult;

    const AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    if (Canvas == nullptr || HDPlayerController == nullptr || HDPlayerController->IsAbandonPromptOpen() == false)
    {
        return;
    }

    UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
    UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;

    TArray<FPanelLine> Lines;
    Lines.Add({ AbandonTitleText, HDHUDMission::FailColor, LargeFont, AbandonTitleScale });
    Lines.Add({ AbandonHelpText, LineColor, MediumFont });

    DrawPanel(*this, *Canvas, Lines);
}

void AHDHUD::DrawObjectiveMarkers()
{
    using namespace HDHUDMission;

    NULL_CHECK_WITHOUT_LOG(PlayerOwner);

    const APawn* PlayerPawn = PlayerOwner->GetPawn();
    NULL_CHECK_WITHOUT_LOG(PlayerPawn);

    FVector ViewLocation;
    FRotator ViewRotation;
    PlayerOwner->GetPlayerViewPoint(ViewLocation, ViewRotation);
    const FVector ViewDirection = ViewRotation.Vector();

    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;

    // "<라벨>  <거리>m". 카메라 뒤의 점도 Project 는 화면 좌표를 돌려주므로 앞에 있을 때만 그린다
    auto DrawMarker = [&](const FString& Label, const FVector& MarkerLocation, const FVector& ActorLocation)
    {
        if (FVector::DotProduct(ViewDirection, MarkerLocation - ViewLocation) <= 0.f)
        {
            return;
        }

        const FVector ScreenLocation = Project(MarkerLocation);
        const int32 DistanceMeters = FMath::RoundToInt(FVector::Distance(PlayerPawn->GetActorLocation(), ActorLocation) / 100.f);
        DrawText(FString::Printf(TEXT("%s  %dm"), *Label, DistanceMeters), TextColor, ScreenLocation.X, ScreenLocation.Y, Font);
    };

    for (TActorIterator<AHDMissionObjective> It(GetWorld()); It; ++It)
    {
        const AHDMissionObjective* Objective = *It;
        if (IsValid(Objective) == false || Objective->IsCompleted())
        {
            continue;
        }

        DrawMarker(Objective->GetMarkerLabel(), Objective->GetMarkerLocation(), Objective->GetActorLocation());
    }

    for (TActorIterator<AHDExtractionZone> It(GetWorld()); It; ++It)
    {
        const AHDExtractionZone* ExtractionZone = *It;
        if (IsValid(ExtractionZone) == false)
        {
            continue;
        }

        const FVector ZoneLocation = ExtractionZone->GetActorLocation();
        DrawMarker(TEXT("EXTRACT"), ZoneLocation + FVector(0.f, 0.f, ExtractionMarkerHeight), ZoneLocation);
    }

    // 샘플: 떨군 묶음은 늘, 그 밖은 폰에서 SampleMarkerDistance 안일 때만
    const float SampleMarkerDistance = GetDefault<UHDProgressionSettings>()->SampleMarkerDistance;
    for (TActorIterator<AHDSamplePickup> It(GetWorld()); It; ++It)
    {
        const AHDSamplePickup* Sample = *It;
        if (IsValid(Sample) == false)
        {
            continue;
        }

        const FVector SampleLocation = Sample->GetActorLocation();
        if (Sample->IsDroppedBundle() == false && FVector::Distance(PlayerPawn->GetActorLocation(), SampleLocation) > SampleMarkerDistance)
        {
            continue;
        }

        DrawMarker(Sample->GetMarkerLabel(), SampleLocation + FVector(0.f, 0.f, SampleMarkerHeight), SampleLocation);
    }
}
