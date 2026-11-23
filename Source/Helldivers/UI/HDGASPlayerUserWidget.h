// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "HDGASPlayerUserWidget.generated.h"

class UProgressBar;
class UImage;
class UTextBlock;
struct FGameplayEventData;

UCLASS()
class HELLDIVERS_API UHDGASPlayerUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    void                        InitializeHealth(const float InCurrentHealth, const float InMaxHealth);
    void                        OnHealthAttributeChangeds(const FOnAttributeChangeData& ChangeData);
    void                        OnWeaponAttributeChangeds(const FOnAttributeChangeData& ChangeData);
    // 장비 칸 이미지 브러시만 바꾼다(크기는 위젯 슬롯을 따른다)
    void                        SetWeaponIcon(UTexture2D* Texture);
    void                        SetGrenadeIcon(UTexture2D* Texture);
    void                        SetStimIcon(UTexture2D* Texture);
    // Img_Weapon 슬롯(배치 전이면 브러시) 크기의 가로 / 세로. 알 수 없으면 1
    float                       GetWeaponIconAspect() const;
private:

    void                        RefreshHealthBar();
    void                        OnCapacityCountChanged(const int32 NewCapacityCount);
    void                        RefreshGrenadeCount();
    void                        RefreshStimCount();

    void                        UpdateProgressbar(UProgressBar* Progressbar, const float Value);
    void                        UpdateTextblock(UTextBlock* TextBlock, const FText& Text);
    void                        UpdateImage(UImage* ImageWidget, UTexture2D* Texture);

protected:
    // HP Section
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage>          Img_HPbarBackGround;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar>    Pb_HPbar;
    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage>          Img_Steam;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock>      Text_SteamCount;

    float                       CurrentHealth = 0.f;
    float                       CurrentMaxHealth = 0.f;
    int32                       CurrentStimCount = 0;
    int32                       MaxStimCount = 1;

    // Equip Section
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage>          Img_EquipBackground;
    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage>          Img_Grenade;
    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock>      Text_GrenadeCount;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage>          Img_Backpack;
    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar>    Pb_Backpack;
    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar>    Pb_Ammo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock>      Text_CapacityCount;
    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage>          Img_Weapon;

    int32                       MaxAmmoCount = 1;
    int32                       MaxCapacityCount = 1;
    int32                       MaxGrenadeCount = 1;
    int32                       CurrentGrenadeCount = 0;
};
