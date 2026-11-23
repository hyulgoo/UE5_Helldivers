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
#include "Ship/HDShipInterior.h"
#include "GameFramework/Pawn.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "UI/HDShipUIStyle.h"
#include "UI/HDShipUILayout.h"
#include "UI/HDShipMenuView.h"
#include "UI/HDShipUICanvas.h"
#include "UI/HDShipScreenPainter.h"

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

static TAutoConsoleVariable<int32> CVarHDShipUIDebug(
    TEXT("hd.ShipUI.Debug"),
    0,
    TEXT("1: draw ship UI frame regions and recorded text boxes as thin outlines"),
    ECVF_Cheat);

namespace HDHUDShip
{
    // 타이틀 화면 전체를 덮는 반투명 검정(타이틀은 옛 그림 그대로)
    const FLinearColor  BackgroundColor(0.f, 0.f, 0.f, 0.6f);
    const FLinearColor  ItemColor(1.f, 1.f, 1.f);

    // 타이틀: 화면 높이 비율 위치에 가운데 정렬
    constexpr float     TitleTextScale = 2.5f;
    constexpr float     TitleTextHeightRatio = 0.35f;
    constexpr float     PromptTextHeightRatio = 0.6f;

    const TCHAR* const  TitleText = TEXT("HD SOLO OPERATIONS");
    const TCHAR* const  PromptText = TEXT("PRESS ENTER");

    const FName         TitleHitBoxName(TEXT("ShipTitle"));

    // 함선 화면 전환(계획 U15): 열기 0.15초, 닫기 0.12초, 화면 바꾸기 0.10초
    constexpr float     OpenSeconds = 0.15f;
    constexpr float     CloseSeconds = 0.12f;
    constexpr float     ChangeScreenSeconds = 0.10f;

    const TCHAR* const  FontPath = TEXT("/Engine/EngineFonts/Roboto.Roboto");

    // 검사 로그의 화면 이름(메뉴 화면이 아닌 층)
    const TCHAR* const  InteriorScreenName = TEXT("Interior");
    // 단말기 태그 사이·투영점과의 간격(설계 단위)
    constexpr float     PromptTagGap = 6.f;
    const TCHAR* const  CountdownScreenName = TEXT("Countdown");

    // 프로필 자원과 마지막 미션 줄
    FHDResourceView MakeResourceView(const UObject* WorldContext)
    {
        const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(WorldContext);
        return HDShipMenuView::MakeResourceView(
            ProfileSubsystem ? ProfileSubsystem->GetProfile() : FHDProfileData(),
            HDShipMenuView::FormatLastMission(ProfileSubsystem ? ProfileSubsystem->GetLastResult() : nullptr));
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
        DrawShipPhase(*HDGameState);
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

    if (HDPlayerController == nullptr)
    {
        return;
    }

    if (BoxName == HDHUDShip::TitleHitBoxName)
    {
        HDPlayerController->ClickShipMenuItem(-1);
        return;
    }

    HDShipUILayout::EHDShipHitKind Kind = HDShipUILayout::EHDShipHitKind::None;
    int32 A = 0;
    int32 B = 0;
    if (HDShipUILayout::ParseHitBoxName(BoxName, Kind, A, B) == false)
    {
        return;
    }

    if (Kind == HDShipUILayout::EHDShipHitKind::Item)
    {
        HDPlayerController->ClickShipMenuItem(A);
    }
    else if (Kind == HDShipUILayout::EHDShipHitKind::Key && A <= static_cast<int32>(EHDShipMenuKey::Back))
    {
        HDPlayerController->ClickShipKeyHint(static_cast<EHDShipMenuKey>(A));
    }
}

void AHDHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
    Super::NotifyHitBoxBeginCursorOver(BoxName);

    // 줄만 가리키기로 커서를 옮긴다(키 안내 상자·타이틀은 아니다)
    AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    HDShipUILayout::EHDShipHitKind Kind = HDShipUILayout::EHDShipHitKind::None;
    int32 A = 0;
    int32 B = 0;
    if (HDPlayerController && HDShipUILayout::ParseHitBoxName(BoxName, Kind, A, B) && Kind == HDShipUILayout::EHDShipHitKind::Item)
    {
        HDPlayerController->HoverShipMenuItem(A);
    }
}

void AHDHUD::DrawShipPhase(const AHDGameState& GameState)
{
    const AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    if (Canvas == nullptr || HDPlayerController == nullptr)
    {
        return;
    }

    const bool bMenuOpen = HDPlayerController->IsShipMenuActive();
    const EHDShipScreen Screen = HDPlayerController->GetShipMenu().GetScreen();

    // 앞 상태를 먼저 돌리고 이번 프레임의 열림·닫힘·화면 바뀜을 건다. 타이틀은 전환 없이 그린다
    ShipTransition.Tick(RenderDelta);
    if (bMenuOpen && Screen != EHDShipScreen::Title)
    {
        if (bLastShipMenuOpen == false || LastShipScreen == EHDShipScreen::Title)
        {
            ShipTransition.Open(HDHUDShip::OpenSeconds);
        }
        else if (Screen != LastShipScreen)
        {
            ShipTransition.ChangeScreen(HDHUDShip::ChangeScreenSeconds);
        }
    }
    else if (bMenuOpen == false && bLastShipMenuOpen && LastShipScreen != EHDShipScreen::Title)
    {
        ShipTransition.Close(HDHUDShip::CloseSeconds);
    }
    else if (bMenuOpen && Screen == EHDShipScreen::Title)
    {
        ShipTransition = FHDShipUITransition();
    }
    bLastShipMenuOpen   = bMenuOpen;
    LastShipScreen      = Screen;

    if (bMenuOpen && Screen == EHDShipScreen::Title)
    {
        DrawShipTitle();
    }
    else
    {
        if (bMenuOpen == false)
        {
            DrawShipOverlay();
        }

        // 닫는 중이면 마지막 화면을 그 투명도로(히트박스 없음, 입력은 이미 게임으로 돌아갔다)
        if (bMenuOpen || ShipTransition.IsVisible())
        {
            DrawShipMenu(bMenuOpen);
        }
    }

    if (GameState.GetLaunchRemaining() >= 0.f)
    {
        DrawLaunchCountdown(GameState.GetLaunchRemaining());
    }
}

UFont* AHDHUD::GetShipUIFont()
{
    if (ShipUIFont == nullptr && bShipUIFontLoadTried == false)
    {
        bShipUIFontLoadTried = true;
        ShipUIFont = LoadObject<UFont>(nullptr, HDHUDShip::FontPath);
        if (ShipUIFont == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HDShipUI] font fallback (%s not found)"), HDHUDShip::FontPath);
        }
    }

    return ShipUIFont;
}

void AHDHUD::DrawShipTitle()
{
    using namespace HDHUDShip;

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

    DrawCenteredText(TitleText, HDHUDMission::TextColor, ScreenHeight * TitleTextHeightRatio, LargeFont, TitleTextScale);
    DrawCenteredText(PromptText, ItemColor, ScreenHeight * PromptTextHeightRatio, MediumFont, 1.f);

    AddHitBox(FVector2D(0.f, 0.f), FVector2D(ScreenWidth, ScreenHeight), TitleHitBoxName, true);
}

void AHDHUD::DrawShipMenu(bool bInteractive)
{
    const AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    if (Canvas == nullptr || HDPlayerController == nullptr)
    {
        return;
    }

    const FHDShipMenu& ShipMenu = HDPlayerController->GetShipMenu();
    const EHDShipScreen Screen = ShipMenu.GetScreen();
    const FHDShipUIFrame Frame = HDShipUILayout::ComputeFrame(FVector2D(Canvas->ClipX, Canvas->ClipY));
    const FVector2D Offset(0.f, HDShipUIStyle::OpenSlide * Frame.Scale * ShipTransition.GetSlide());

    FHDShipUICanvas UICanvas(this, Canvas, GetShipUIFont(), Frame, ShipTransition.GetAlpha(), ShipTransition.GetContentAlpha(), Offset, bInteractive);

    const TArray<FHDShipMenuItem> Items = HDPlayerController->BuildShipMenuItems();
    const TArray<FHDKeyHint> Hints = HDShipMenuView::GetKeyHints(ShipMenu, Items, HDPlayerController->IsShipLaunching());

    // 배경 → 위 바 → 본문(이 브리프는 타이틀이 아니면 모두 목록) → 아래 바
    FHDShipScreenPainter::PaintBackdrop(UICanvas);
    FHDShipScreenPainter::PaintTopBar(UICanvas, HDShipMenu::GetScreenTitle(Screen), HDHUDShip::MakeResourceView(this));
    FHDShipScreenPainter::PaintList(UICanvas, ShipMenu, Items);
    FHDShipScreenPainter::PaintBottomBar(UICanvas, Hints);

    if (bInteractive)
    {
        const FString ScreenName = HDShipMenu::GetScreenName(Screen);
        const int32 ViewWidth = FMath::RoundToInt(Canvas->ClipX);
        const int32 ViewHeight = FMath::RoundToInt(Canvas->ClipY);

        const FString FrameKey = FString::Printf(TEXT("%s|%dx%d"), *ScreenName, ViewWidth, ViewHeight);
        if (FrameKey != LastShipFrameKey)
        {
            LastShipFrameKey = FrameKey;
            UE_LOG(LogTemp, Display, TEXT("[HDShipUI] frame screen=%s view=%dx%d scale=%.3f"), *ScreenName, ViewWidth, ViewHeight, Frame.Scale);
            UE_LOG(LogTemp, Display, TEXT("[HDShipUI] canvas dpi=%.3f font=%s"), UICanvas.GetDPIScale(), UICanvas.IsUsingFallbackFont() ? TEXT("fallback") : TEXT("Roboto"));
        }

        const FString HintsText = HDShipMenuView::JoinKeyHints(Hints);
        const FString HintsKey = ScreenName + TEXT("|") + HintsText;
        if (HintsKey != LastShipHintsKey)
        {
            LastShipHintsKey = HintsKey;
            UE_LOG(LogTemp, Display, TEXT("[HDShipUI] hints screen=%s %s"), *ScreenName, *HintsText);
        }

        // 다 열려 멈춘 프레임에서만 센다(미끄러지는 중에는 아래 바가 화면 밖으로 나간다)
        if (ShipTransition.IsSettled())
        {
            CheckShipTexts(ScreenName, UICanvas, ShipMenu.GetCursor());
        }
    }
    else
    {
        // 닫았다가 다시 열면 frame·hints 줄을 다시 남긴다
        LastShipFrameKey.Reset();
        LastShipHintsKey.Reset();
    }

    if (CVarHDShipUIDebug.GetValueOnGameThread() > 0)
    {
        UICanvas.DrawDebug();
    }
}

void AHDHUD::DrawShipOverlay()
{
    const AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerOwner);
    if (Canvas == nullptr || HDPlayerController == nullptr)
    {
        return;
    }

    const FHDShipUIFrame Frame = HDShipUILayout::ComputeFrame(FVector2D(Canvas->ClipX, Canvas->ClipY));
    FHDShipUICanvas UICanvas(this, Canvas, GetShipUIFont(), Frame, 1.f, 1.f, FVector2D::ZeroVector, true);

    const int32 TerminalIndex = HDPlayerController->FindShipTerminalInRange();
    const TArray<FHDShipTerminalInfo>& Terminals = HDShipHub::GetTerminals();

    FHDShipScreenPainter::PaintInteriorOverlay(UICanvas, HDHUDShip::MakeResourceView(this));
    DrawShipPromptTags(UICanvas, TerminalIndex);
    CheckShipTexts(HDHUDShip::InteriorScreenName, UICanvas, Terminals.IsValidIndex(TerminalIndex) ? TerminalIndex : INDEX_NONE);

    if (CVarHDShipUIDebug.GetValueOnGameThread() > 0)
    {
        UICanvas.DrawDebug();
    }
}

void AHDHUD::DrawShipPromptTags(FHDShipUICanvas& UICanvas, int32 ActiveTerminalIndex)
{
    UWorld* World = GetWorld();
    if (World == nullptr || PlayerOwner == nullptr)
    {
        return;
    }

    if (CachedShipInterior.IsValid() == false)
    {
        for (TActorIterator<AHDShipInterior> It(World); It; ++It)
        {
            CachedShipInterior = *It;
            break;
        }
    }

    const AHDShipInterior* ShipInterior = CachedShipInterior.Get();
    const APawn* Pawn = PlayerOwner->GetPawn();
    if (ShipInterior == nullptr || Pawn == nullptr)
    {
        return;
    }

    // 카메라 뒤 점을 거르기 위한 시점
    FVector ViewLocation = FVector::ZeroVector;
    FRotator ViewRotation = FRotator::ZeroRotator;
    PlayerOwner->GetPlayerViewPoint(ViewLocation, ViewRotation);
    const FVector ViewForward = ViewRotation.Vector();

    const FHDShipUIFrame& Frame = UICanvas.GetFrame();
    const FVector PawnLocation = Pawn->GetActorLocation();
    const TArray<FHDShipTerminalInfo>& Terminals = HDShipHub::GetTerminals();

    TArray<FHDPromptTagInput> Inputs;
    TArray<FString> Labels;
    for (int32 Index = 0; Index < Terminals.Num(); ++Index)
    {
        const FHDShipTerminalInfo& Terminal = Terminals[Index];
        const FVector WorldAnchor = ShipInterior->GetActorTransform().TransformPosition(HDShipHub::GetPromptAnchor(Terminal));
        const FVector WorldTerminal = ShipInterior->GetTerminalWorldLocation(Index);

        FHDPromptTagInput Input;
        Input.Id            = Terminal.Id;
        Input.bInRange      = Index == ActiveTerminalIndex;
        Input.Distance2D    = static_cast<float>(FVector::Dist2D(PawnLocation, WorldTerminal));
        Input.bProjected    = PlayerOwner->ProjectWorldLocationToScreen(WorldAnchor, Input.ScreenAnchor, true)
            && FVector::DotProduct(WorldAnchor - ViewLocation, ViewForward) > 0.0;
        Input.Size          = FHDShipScreenPainter::MeasurePromptTag(UICanvas, Terminal.Label, Input.bInRange);
        Inputs.Add(Input);
        Labels.Add(Terminal.Label);
    }

    const TArray<FHDPromptTag> PromptTags = HDShipHub::PlacePromptTags(Inputs, Frame.Safe, UICanvas.Px(HDHUDShip::PromptTagGap), HDShipHub::DefaultTagShowDistance);
    FHDShipScreenPainter::PaintPromptTags(UICanvas, PromptTags, Inputs, Labels);

    // 보이는 태그 집합이나 활성 단말기가 바뀌면 남긴다. 겹침 수는 보이는 상자에 FindTextProblems
    TArray<HDShipUILayout::FHDDrawnText> Boxes;
    FString VisibleIds;
    for (const FHDPromptTag& Tag : PromptTags)
    {
        if (Tag.bVisible == false)
        {
            continue;
        }
        HDShipUILayout::FHDDrawnText Drawn;
        Drawn.Rect  = Tag.Rect;
        Drawn.Text  = Tag.Id.ToString();
        Boxes.Add(Drawn);
        VisibleIds += Tag.Id.ToString() + TEXT(",");
    }

    const FString ActiveName = Terminals.IsValidIndex(ActiveTerminalIndex) ? Terminals[ActiveTerminalIndex].Id.ToString() : FString(TEXT("none"));
    const FString Key = VisibleIds + TEXT("|") + ActiveName;
    if (Key != LastPromptTagKey)
    {
        LastPromptTagKey = Key;
        const HDShipUILayout::FHDTextProblems Problems = HDShipUILayout::FindTextProblems(Boxes, Frame.View);
        UE_LOG(LogTemp, Display, TEXT("[HDShipUI] prompt tags=%d visible=%d active=%s overlaps=%d"), PromptTags.Num(), Boxes.Num(), *ActiveName, Problems.Overlaps);
    }
}

void AHDHUD::DrawLaunchCountdown(float RemainingSeconds)
{
    if (Canvas == nullptr)
    {
        return;
    }

    // 메뉴 위 맨 위 층. 메뉴 투명도와 상관없이 다 보인다. 숫자는 FHDLaunchCountdown::GetDisplaySeconds 와 같은 올림
    const FHDShipUIFrame Frame = HDShipUILayout::ComputeFrame(FVector2D(Canvas->ClipX, Canvas->ClipY));
    FHDShipUICanvas UICanvas(this, Canvas, GetShipUIFont(), Frame, 1.f, 1.f, FVector2D::ZeroVector, true);
    FHDShipScreenPainter::PaintCountdown(UICanvas, FMath::CeilToInt(RemainingSeconds));
    CheckShipTexts(HDHUDShip::CountdownScreenName, UICanvas, 0);
}

void AHDHUD::CheckShipTexts(const FString& ScreenName, const FHDShipUICanvas& UICanvas, int32 Cursor)
{
    const FHDShipUIFrame& Frame = UICanvas.GetFrame();
    const int32 ViewWidth = FMath::RoundToInt(Frame.View.Max.X);
    const int32 ViewHeight = FMath::RoundToInt(Frame.View.Max.Y);
    const FString ViewKey = FString::Printf(TEXT("%s|%dx%d"), *ScreenName, ViewWidth, ViewHeight);
    const FString CursorKey = FString::Printf(TEXT("%s|%d"), *ViewKey, Cursor);

    // (화면, 뷰)마다 한 번은 늘 남기고, 그 뒤에는 (화면, 뷰, 커서)가 처음인 프레임에서 문제가 있을 때만 남긴다
    const bool bFirstView = CheckedShipViews.Contains(ViewKey) == false;
    const bool bFirstCursor = CheckedShipCursors.Contains(CursorKey) == false;
    if (bFirstView == false && bFirstCursor == false)
    {
        return;
    }
    CheckedShipViews.Add(ViewKey);
    CheckedShipCursors.Add(CursorKey);

    const HDShipUILayout::FHDTextProblems Problems = HDShipUILayout::FindTextProblems(UICanvas.GetDrawnTexts(), Frame.View);
    const bool bProblem = Problems.Overlaps > 0 || Problems.Clipped > 0;
    if (bFirstView == false && bProblem == false)
    {
        return;
    }

    const FString Line = FString::Printf(TEXT("[HDShipUI] check screen=%s view=%dx%d texts=%d overlaps=%d clipped=%d truncated=%d"),
        *ScreenName, ViewWidth, ViewHeight, Problems.Texts, Problems.Overlaps, Problems.Clipped, Problems.Truncated);
    if (bProblem)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s cursor=%d first=\"%s\""), *Line, Cursor, *Problems.FirstProblem);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
    }
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
