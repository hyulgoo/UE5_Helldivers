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
#include "Mission/HDMissionObjective.h"

namespace HDHUDMission
{
    const FLinearColor  TextColor(1.f, 0.85f, 0.2f);
    constexpr float     TextLeft = 40.f;
    constexpr float     TextTop = 40.f;
    constexpr float     LineHeight = 26.f;
}

void AHDHUD::SetAbilitySystemComponent(UAbilitySystemComponent* ASC)
{
    NULL_CHECK(ASC);
    AbilitySystemComponent = ASC;
}

void AHDHUD::CreateDefaultWidget()
{
    NULL_CHECK_WITHOUT_LOG(AbilitySystemComponent);

    if(PlayerHUDWidgetClass)
    {
        PlayerHUDWidget = CreateWidget<UHDGASPlayerUserWidget>(GetWorld(), PlayerHUDWidgetClass);
        NULL_CHECK(PlayerHUDWidget);

        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDHealthAttributeSet::GetCurrentHealthAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnHealthAttributeChangeds);
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDHealthAttributeSet::GetMaxHealthAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnHealthAttributeChangeds);
        PlayerHUDWidget->InitializeHealth(AbilitySystemComponent->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()), AbilitySystemComponent->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute()));
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetAmmoAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetMaxAmmoAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetCapacityAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetMaxCapacityAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetGrenadeAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UHDWeaponAttributeSet::GetMaxGrenadeAttribute()).AddUObject(PlayerHUDWidget, &UHDGASPlayerUserWidget::OnWeaponAttributeChangeds);

        PlayerHUDWidget->AddToViewport();
    }

    if (StratagemWidgetClass)
    {
        StratagemWidget = CreateWidget<UHDStratagemHUDUserWidget>(GetWorld(), StratagemWidgetClass, FName("StratagemHUDWidget"));
        NULL_CHECK(StratagemWidget);

        const FGameplayTagContainer& StratagemEventTags = FGameplayAbilityHelper::GetAllChildTag(Tag_Event_StratagemHUD);
        for (const FGameplayTag& StratagemEventTag : StratagemEventTags)
        {
            AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(StratagemEventTag).AddUObject(StratagemWidget, &UHDStratagemHUDUserWidget::OnStratagemEventReceived);
        }

        UHDStratagemComponent* StratagemComponent = PlayerOwner->GetPawn()->GetComponentByClass<UHDStratagemComponent>();
        NULL_CHECK(StratagemComponent);

        StratagemWidget->InitializeStratagemHUD(StratagemComponent);
        StratagemWidget->AddToViewport();
    }
}

void AHDHUD::OnEquipWeaponUIEventRecieved(const FGameplayEventData* Payload)
{
    NULL_CHECK_WITHOUT_LOG(AbilitySystemComponent);
    NULL_CHECK(ReticleWidgetClass);

    if (ReticleWidget)
    {
        NULL_CHECK(CurrentWeapon);
        CurrentWeapon->OnSpreadChanged.Remove(SpreadChangedHandle);
    }
    else
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

    TArray<FString> Lines;
    switch (HDGameState->GetMissionPhase())
    {
    case EHDMissionPhase::Deploy:
        Lines.Add(TEXT("DEPLOYING"));
        break;
    case EHDMissionPhase::InProgress:
        Lines.Add(FString::Printf(TEXT("OBJECTIVE  DESTROY FACILITIES  %d/%d"), HDGameState->GetCompletedObjectives(), HDGameState->GetRequiredObjectives()));
        break;
    case EHDMissionPhase::Extraction:
        Lines.Add(TEXT("EXTRACT"));
        break;
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
    for (TActorIterator<AHDMissionObjective> It(GetWorld()); It; ++It)
    {
        const AHDMissionObjective* Objective = *It;
        if (IsValid(Objective) == false || Objective->IsCompleted())
        {
            continue;
        }

        // 카메라 뒤의 점도 Project 는 화면 좌표를 돌려주므로 앞에 있을 때만 그린다
        const FVector MarkerLocation = Objective->GetMarkerLocation();
        if (FVector::DotProduct(ViewDirection, MarkerLocation - ViewLocation) <= 0.f)
        {
            continue;
        }

        const FVector ScreenLocation = Project(MarkerLocation);
        const int32 DistanceMeters = FMath::RoundToInt(FVector::Distance(PlayerPawn->GetActorLocation(), Objective->GetActorLocation()) / 100.f);
        DrawText(FString::Printf(TEXT("%s  %dm"), *Objective->GetMarkerLabel(), DistanceMeters), TextColor, ScreenLocation.X, ScreenLocation.Y, Font);
    }
}
