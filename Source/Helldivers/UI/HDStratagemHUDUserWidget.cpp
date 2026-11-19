// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDStratagemHUDUserWidget.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "GameData/HDStratagemData.h"
#include "Components/VerticalBox.h"
#include "Component/HDStratagemComponent.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "UI/HDStratagemInfoUserWidget.h"
#include "Abilities/GameplayAbilityTypes.h"

void UHDStratagemHUDUserWidget::InitializeStratagemHUD(UHDStratagemComponent* InStratagemComponent)
{
    NULL_CHECK(InStratagemComponent);
    StratagemComponent = InStratagemComponent;

    // 화면 밖(왼쪽)에서 시작한다. 목록을 다시 만들 때는 위치를 건드리지 않는다
    FVector2D ViewportSize = FVector2D();
    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
    }

    WidgetAppearDistance = (ViewportSize.X / 2.f);

    FVector2D CurrentPos = GetRenderTransform().Translation;
    CurrentPos.X -= WidgetAppearDistance;
    SetRenderTranslation(CurrentPos);

    RebuildStratagemList();
}

void UHDStratagemHUDUserWidget::SetStratagemComponent(UHDStratagemComponent* InStratagemComponent)
{
    NULL_CHECK(InStratagemComponent);
    StratagemComponent = InStratagemComponent;
}

void UHDStratagemHUDUserWidget::RebuildStratagemList()
{
    NULL_CHECK(VB_StratagemList);
    VB_StratagemList->ClearChildren();
    StratagemInfoWidgetList.Empty();

    // 스트라타젬 아이콘은 없어도 된다(이름만 보인다). 커맨드 아이콘은 WBP 에 네 방향이 다 있다
    CONDITION_CHECK(CommandIconList.IsEmpty() == false);
    NULL_CHECK(StratagemComponent);
    NULL_CHECK(StratagemInfoWidgetClass);

    const FHDStratagemLoadout* Loadout = StratagemComponent->GetLoadout();
    NULL_CHECK(Loadout);

    for (int32 SlotIndex = 0; SlotIndex < Loadout->Num(); ++SlotIndex)
    {
        const FHDStratagemSlot* LoadoutSlot = Loadout->GetSlot(SlotIndex);
        NULL_CHECK(LoadoutSlot);

        UHDStratagemInfoUserWidget* StratagemIndoWidget = CreateWidget<UHDStratagemInfoUserWidget>(this, StratagemInfoWidgetClass);
        NULL_CHECK(StratagemIndoWidget);

        UTexture2D* const* IconTexture = StratagemIconList.Find(LoadoutSlot->StratagemName);
        StratagemIndoWidget->InitializeStratagemInfoWidget(IconTexture ? *IconTexture : nullptr, LoadoutSlot->CommandSequence, CommandIconList, LoadoutSlot->StratagemName);
        StratagemInfoWidgetList.Add(StratagemIndoWidget);

        VB_StratagemList->AddChildToVerticalBox(StratagemIndoWidget);
    }
}

void UHDStratagemHUDUserWidget::SetHUDActiveByCurrentInputMatchList(const TArray<FName>& MatchStratagemList, const int32 CurrentInputNum)
{
    for(UHDStratagemInfoUserWidget* StratagemInfoWidget : StratagemInfoWidgetList)
    {
        if(MatchStratagemList.Find(StratagemInfoWidget->GetStratagemName()) != INDEX_NONE)
        {
            StratagemInfoWidget->ActiveCommandIconByNum(CurrentInputNum);
        }
        else
        {
            StratagemInfoWidget->SetAllWidgetCapacity(0.5f);
        }
    }
}

void UHDStratagemHUDUserWidget::SetAllWidgetOpacity(const float Opacity)
{
    for(UHDStratagemInfoUserWidget* StratagemInfoWidget : StratagemInfoWidgetList)
    {
        StratagemInfoWidget->SetAllWidgetCapacity(Opacity);
    }
}

void UHDStratagemHUDUserWidget::WidgetAppear(const bool bAppear)
{
    MoveState = bAppear ? EHDStratagemWidgetMoveState::APPEAR : EHDStratagemWidgetMoveState::DISAPPEAR;

    SetAllWidgetOpacity(bAppear ? 1.f : 0.5f);
}

void UHDStratagemHUDUserWidget::OnStratagemEventReceived(const FGameplayEventData* Payload)
{
    NULL_CHECK(Payload);

    if (Payload->EventTag == Tag_Event_StratagemHUD_Active)
    {
        WidgetAppear(true);
    }
    else if (Payload->EventTag == Tag_Event_StratagemHUD_Deactive)
    {
        WidgetAppear(false);
    }
    else if (Payload->EventTag == Tag_Event_StratagemHUD_AddCommand)
    {
        NULL_CHECK(StratagemComponent);

        const TArray<FName>& MatchStratagemList = StratagemComponent->GetCommandMatchStratagemNameList();
        const int32 CurrentInputNum = StratagemComponent->GetCurrentInputNum();

        // 틀린 입력·대기 중인 칸의 입력이면 후보가 없다. 오류가 아니라 목록을 흐리게 한다
        if (MatchStratagemList.IsEmpty())
        {
            SetAllWidgetOpacity(0.5f);
            return;
        }

        SetHUDActiveByCurrentInputMatchList(MatchStratagemList, CurrentInputNum);
    }
}

void UHDStratagemHUDUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 칸 i <-> 위젯 i. 남은 횟수 0 이면 EMPTY, 아니면 남은 대기(올림 초)와 남은/최대 횟수, 둘 다 없으면 이름만
    const FHDStratagemLoadout* Loadout = StratagemComponent ? StratagemComponent->GetLoadout() : nullptr;
    if (Loadout)
    {
        const int32 SlotNum = FMath::Min(Loadout->Num(), StratagemInfoWidgetList.Num());
        for (int32 SlotIndex = 0; SlotIndex < SlotNum; ++SlotIndex)
        {
            const FHDStratagemSlot* LoadoutSlot = Loadout->GetSlot(SlotIndex);
            UHDStratagemInfoUserWidget* StratagemInfoWidget = StratagemInfoWidgetList[SlotIndex];
            if (LoadoutSlot == nullptr || StratagemInfoWidget == nullptr)
            {
                continue;
            }

            FString Status;
            if (Loadout->HasUsesLeft(SlotIndex) == false)
            {
                Status = TEXT("  EMPTY");
            }
            else
            {
                const float CooldownRemaining = StratagemComponent->GetSlotCooldownRemaining(SlotIndex);
                if (CooldownRemaining > 0.f)
                {
                    Status += FString::Printf(TEXT("  %ds"), FMath::CeilToInt(CooldownRemaining));
                }

                if (LoadoutSlot->MaxUses > 0)
                {
                    Status += FString::Printf(TEXT("  %d/%d"), LoadoutSlot->UsesLeft, LoadoutSlot->MaxUses);
                }
            }

            StratagemInfoWidget->SetStatusText(Status);
        }
    }

    if(MoveState != EHDStratagemWidgetMoveState::NONE)
    {
        const FVector2D CurrentPos = GetRenderTransform().Translation;
        const FVector2D DesiredPos = MoveState == EHDStratagemWidgetMoveState::APPEAR ? FVector2D(0, 0) : FVector2D(-WidgetAppearDistance, 0);

        if (CurrentPos == DesiredPos)
        {
            MoveState = EHDStratagemWidgetMoveState::NONE;
            return;
        }

        const FVector2D NewPos = FMath::Vector2DInterpTo(CurrentPos, DesiredPos, InDeltaTime, 20.f);
        SetRenderTranslation(NewPos);
    }   
}
