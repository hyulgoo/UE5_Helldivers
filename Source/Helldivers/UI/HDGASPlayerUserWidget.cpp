// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDGASPlayerUserWidget.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Attribute/HDWeaponAttributeSet.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Define/HDDefine.h"

void UHDGASPlayerUserWidget::InitializeHealth(const float InCurrentHealth, const float InMaxHealth)
{
    CurrentHealth = InCurrentHealth;
    CurrentMaxHealth = InMaxHealth;

    RefreshHealthBar();
}

void UHDGASPlayerUserWidget::OnHealthAttributeChangeds(const FOnAttributeChangeData& ChangeData)
{
    if(ChangeData.Attribute == UHDHealthAttributeSet::GetCurrentHealthAttribute())
    {
        CurrentHealth = ChangeData.NewValue;
    }
    else if(ChangeData.Attribute == UHDHealthAttributeSet::GetMaxHealthAttribute())
    {
        CurrentMaxHealth = ChangeData.NewValue;
    }

    RefreshHealthBar();
}

void UHDGASPlayerUserWidget::RefreshHealthBar()
{
    // 최대 체력이 아직 0 이면 0 나누기를 피한다
    if (CurrentMaxHealth <= 0.f)
    {
        UpdateProgressbar(Pb_HPbar, 0.f);
        return;
    }

    UpdateProgressbar(Pb_HPbar, CurrentHealth / CurrentMaxHealth);
}

void UHDGASPlayerUserWidget::OnWeaponAttributeChangeds(const FOnAttributeChangeData& ChangeData)
{
    if (ChangeData.Attribute == UHDWeaponAttributeSet::GetAmmoAttribute())
    {
        const float CurrentAmmoCount = ChangeData.NewValue;
        const float Ratio = CurrentAmmoCount / static_cast<float>(MaxAmmoCount);
        UpdateProgressbar(Pb_Ammo, Ratio);
    }
    else if (ChangeData.Attribute == UHDWeaponAttributeSet::GetMaxAmmoAttribute())
    {
        MaxAmmoCount = static_cast<int32>(ChangeData.NewValue);
        UpdateProgressbar(Pb_Ammo, 1.f);
    }
    else if (ChangeData.Attribute == UHDWeaponAttributeSet::GetCapacityAttribute())
    {
        OnCapacityCountChanged(static_cast<int32>(ChangeData.NewValue));
    }
    else if (ChangeData.Attribute == UHDWeaponAttributeSet::GetMaxCapacityAttribute())
    {
        MaxCapacityCount = static_cast<int32>(ChangeData.NewValue);
        OnCapacityCountChanged(MaxCapacityCount);
    }
    else if (ChangeData.Attribute == UHDWeaponAttributeSet::GetGrenadeAttribute())
    {
        CurrentGrenadeCount = static_cast<int32>(ChangeData.NewValue);
        RefreshGrenadeCount();
    }
    else if (ChangeData.Attribute == UHDWeaponAttributeSet::GetMaxGrenadeAttribute())
    {
        MaxGrenadeCount = static_cast<int32>(ChangeData.NewValue);
        RefreshGrenadeCount();
    }
    else if (ChangeData.Attribute == UHDWeaponAttributeSet::GetStimAttribute())
    {
        CurrentStimCount = static_cast<int32>(ChangeData.NewValue);
        RefreshStimCount();
    }
    else if (ChangeData.Attribute == UHDWeaponAttributeSet::GetMaxStimAttribute())
    {
        MaxStimCount = static_cast<int32>(ChangeData.NewValue);
        RefreshStimCount();
    }
}

void UHDGASPlayerUserWidget::OnCapacityCountChanged(const int32 NewCapacityCount)
{
    const FText CapacityCountText = FText::Format(NSLOCTEXT("Ammo", "AmmoForamt", "{0}/{1}"),
        FText::AsNumber(NewCapacityCount), FText::AsNumber(MaxCapacityCount));
    UpdateTextblock(Text_CapacityCount, CapacityCountText);
}

void UHDGASPlayerUserWidget::RefreshGrenadeCount()
{
    const FText GrenadeCountText = FText::Format(NSLOCTEXT("Grenade", "GrenadeForamt", "{0}/{1}"),
        FText::AsNumber(CurrentGrenadeCount), FText::AsNumber(MaxGrenadeCount));
    UpdateTextblock(Text_GrenadeCount, GrenadeCountText);
}

void UHDGASPlayerUserWidget::RefreshStimCount()
{
    const FText StimCountText = FText::Format(NSLOCTEXT("Stim", "StimFormat", "{0}/{1}"),
        FText::AsNumber(CurrentStimCount), FText::AsNumber(MaxStimCount));
    UpdateTextblock(Text_SteamCount, StimCountText);
}

void UHDGASPlayerUserWidget::UpdateProgressbar(UProgressBar* Progressbar, const float Value)
{
    NULL_CHECK(Progressbar);
    CONDITION_CHECK(Value >= 0.f && Value <= 1.f);

    Progressbar->SetPercent(Value);
}

void UHDGASPlayerUserWidget::UpdateTextblock(UTextBlock* TextBlock, const FText& Text)
{
    NULL_CHECK(TextBlock);

    TextBlock->SetText(Text);
}

void UHDGASPlayerUserWidget::SetWeaponIcon(UTexture2D* Texture)
{
    UpdateImage(Img_Weapon, Texture);
}

void UHDGASPlayerUserWidget::SetGrenadeIcon(UTexture2D* Texture)
{
    UpdateImage(Img_Grenade, Texture);
}

void UHDGASPlayerUserWidget::SetStimIcon(UTexture2D* Texture)
{
    UpdateImage(Img_Steam, Texture);
}

float UHDGASPlayerUserWidget::GetWeaponIconAspect() const
{
    NULL_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(Img_Weapon, 1.f);

    // 배치가 끝났으면 슬롯의 실제 크기, 아니면 브러시 크기
    const FVector2D SlotSize = Img_Weapon->GetCachedGeometry().GetLocalSize();
    if (SlotSize.X > 0.0 && SlotSize.Y > 0.0)
    {
        return static_cast<float>(SlotSize.X / SlotSize.Y);
    }

    const FVector2D BrushSize = Img_Weapon->GetBrush().GetImageSize();
    return (BrushSize.X > 0.0 && BrushSize.Y > 0.0) ? static_cast<float>(BrushSize.X / BrushSize.Y) : 1.f;
}

void UHDGASPlayerUserWidget::UpdateImage(UImage* ImageWidget, UTexture2D* Texture)
{
    NULL_CHECK(ImageWidget);

    ImageWidget->SetBrushFromTexture(Texture);
}
