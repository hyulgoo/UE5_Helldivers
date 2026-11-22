// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Weapon/HDArmorRules.h"
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

    // 왼쪽 위 미션 글자, 미완료 목표 표식(라벨·거리), 증원 대기 중이면 가운데 REINFORCING IN <초>, 결과가 나오면 가운데 결과 패널,
    // 작전 포기 확인 창이 열려 있으면 가운데 ABANDON MISSION? 을 캔버스에 그린다. 함선 단계면 함선 메뉴(닫혀 있으면 함선 안 안내)와 출격 대기만 그린다
    virtual void                        DrawHUD() override;

    // 함선 메뉴 히트박스: ShipTitle → 타이틀 확정, ShipItem_<i> → 그 줄 확정 / 가리키기. 결과 패널 ResultReturn → 함선 복귀 요청
    virtual void                        NotifyHitBoxClick(FName BoxName) override;
    virtual void                        NotifyHitBoxBeginCursorOver(FName BoxName) override;

    // 플레이어가 쏜 탄이 적에 맞은 결과. 조준점에 히트마커를 잠깐 그린다(투사체가 부른다)
    void                                NotifyHitFeedback(EHDHitOutcome Outcome, bool bWeakPoint);

private:
    // 반투명 배경 + 타이틀(HD SOLO OPERATIONS / PRESS ENTER) 또는 화면(머리글·자원 줄·마지막 결과 줄·줄 목록·설명·키 안내)과 히트박스
    void                                DrawShipMenu();
    // 오른쪽 위 자원 줄과 그 아래 LAST MISSION 줄(함선 메뉴·함선 안 안내 공용)
    void                                DrawShipResources();
    // 함선 안(메뉴 닫힘, 배경 없음): 자원 줄, 아래 가운데 범위 안 단말기 [E] <이름>, 왼쪽 아래 [E] USE   [P] SHIP MENU
    void                                DrawShipOverlay();
    // 위쪽 가운데 HELLPOD LAUNCH IN <초> 와 [BACKSPACE] CANCEL
    void                                DrawLaunchCountdown(float RemainingSeconds);
    void                                DrawMissionText();
    // 게임스테이트의 결과를 가운데 반투명 상자에: 성공·실패, 포기, 난이도, 목표, 시간, 징발, 샘플, 입력 지연 뒤 [ENTER] RETURN TO SHIP(히트박스 ResultReturn)
    void                                DrawResultPanel();
    // 확인 창이 열려 있으면 가운데 ABANDON MISSION? 과 키 안내
    void                                DrawAbandonPrompt();
    void                                DrawObjectiveMarkers();
    // 마지막 알림 뒤 잠깐 화면 가운데 X. 전부 빨강(약점 주황), 감소 흰색, 도탄 회색 + DEFLECT
    void                                DrawHitMarker();

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

    // 마지막 히트마커 알림. 시각은 월드 시간(초), 음수면 아직 없음
    EHDHitOutcome                           LastHitOutcome = EHDHitOutcome::Full;
    bool                                    bLastHitWeakPoint = false;
    double                                  LastHitTime = -1.0;
};
