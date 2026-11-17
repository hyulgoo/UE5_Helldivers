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
#include "Mission/HDExtractionZone.h"
#include "Mission/HDMissionObjective.h"
#include "Engine/Canvas.h"

namespace HDHUDMission
{
    const FLinearColor  TextColor(1.f, 0.85f, 0.2f);
    const FLinearColor  FailColor(1.f, 0.f, 0.f);
    constexpr float     TextLeft = 40.f;
    constexpr float     TextTop = 40.f;
    constexpr float     LineHeight = 26.f;

    // 화면 가운데 MISSION COMPLETE / MISSION FAILED
    constexpr float     ResultTextScale = 2.f;

    // 화면 가운데 REINFORCING IN <초>
    constexpr float     RespawnTextScale = 2.f;

    // 추출 지점 표식: 위치 + Z 300
    constexpr float     ExtractionMarkerHeight = 300.f;
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

void AHDHUD::DrawHUD()
{
    Super::DrawHUD();

    DrawMissionText();
    DrawObjectiveMarkers();
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
        // 남은 시간 mm:ss
        const int32 RemainingSeconds = FMath::Max(0, FMath::FloorToInt(HDGameState->GetExtractionRemaining()));
        Lines.Add(FString::Printf(TEXT("EXTRACTION  HOLD  %02d:%02d"), RemainingSeconds / 60, RemainingSeconds % 60));
        Lines.Add(HDGameState->IsPlayerInExtractionZone() ? TEXT("HOLD THE ZONE") : TEXT("ENTER EXTRACTION ZONE"));
        break;
    }
    default:
        break;
    }
    Lines.Add(FString::Printf(TEXT("REINFORCEMENTS  %d"), HDGameState->GetReinforcementsLeft()));

    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    float LineY = TextTop;
    for (const FString& Line : Lines)
    {
        DrawText(Line, TextColor, TextLeft, LineY, Font);
        LineY += LineHeight;
    }

    // 끝나면 화면 가운데 결과
    if ((Phase == EHDMissionPhase::Success || Phase == EHDMissionPhase::Fail) && Canvas)
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
}
