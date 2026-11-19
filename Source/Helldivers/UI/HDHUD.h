// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HDHUD.generated.h"

class UAbilitySystemComponent;
class UHDGASPlayerUserWidget;
class UHDReticleWidget;
class UHDStratagemHUDUserWidget;
class AHDWeapon;
struct FOnAttributeChangeData;
struct FGameplayEventData;

UCLASS()
class HELLDIVERS_API AHDHUD : public AHUD
{
	GENERATED_BODY()

public:
    void                                SetAbilitySystemComponent(UAbilitySystemComponent* ASC);
    // 빙의마다 불린다. 위젯은 처음 한 번만 만들고, 다시 강하하면 체력 막대와 스트라타젬 컴포넌트만 바꾼다
    void                                CreateDefaultWidget();
    void                                OnEquipWeaponUIEventRecieved(const FGameplayEventData* Payload);
    // 로드아웃을 바꾼 뒤 스트라타젬 목록을 다시 만든다. 위젯이 아직 없으면 아무것도 하지 않는다
    void                                RebuildStratagemList();

    // 왼쪽 위 미션 글자, 미완료 목표 표식(라벨·거리), 증원 대기 중이면 가운데 REINFORCING IN <초> 를 캔버스에 그린다
    virtual void                        DrawHUD() override;

private:
    void                                DrawMissionText();
    void                                DrawObjectiveMarkers();

private:
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent>     AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHDGASPlayerUserWidget>	    PlayerHUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHDReticleWidget>		    ReticleWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHDStratagemHUDUserWidget>	StratagemWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UHDGASPlayerUserWidget>	    PlayerHUDWidget;
    	
	UPROPERTY()
	TObjectPtr<UHDReticleWidget>		    ReticleWidget;
    
	UPROPERTY()
	TObjectPtr<UHDStratagemHUDUserWidget>	StratagemWidget;

	UPROPERTY()
    TObjectPtr<AHDWeapon>                   CurrentWeapon;

    FDelegateHandle                         SpreadChangedHandle;  
};
