// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Weapon/HDArmorRules.h"
#include "UI/HDShipMenu.h"
#include "UI/HDShipUILayout.h"
#include "HDHUD.generated.h"

class AHDGameState;
class AHDShipInterior;
class FHDShipUICanvas;
class UFont;
class UTexture2D;
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

    // 함선 메뉴 히트박스: ShipTitle → 타이틀 확정, ShipItem_<i> → 그 줄 확정 / 가리키기, ShipKey_<키> → 그 키(클릭만). 결과 패널 ResultReturn → 함선 복귀 요청
    virtual void                        NotifyHitBoxClick(FName BoxName) override;
    virtual void                        NotifyHitBoxBeginCursorOver(FName BoxName) override;

    // 플레이어가 쏜 탄이 적에 맞은 결과. 조준점에 히트마커를 잠깐 그린다(투사체가 부른다)
    void                                NotifyHitFeedback(EHDHitOutcome Outcome, bool bWeakPoint);

private:
    // 함선 단계: 열기·닫기·화면 바꾸기 전환을 돌리고 타이틀, 메뉴(닫는 중이면 마지막 화면), 함선 안 안내, 출격 대기를 그린다
    void                                DrawShipPhase(const AHDGameState& GameState);
    // 타이틀(HD SOLO OPERATIONS / PRESS ENTER)과 화면 전체 히트박스 ShipTitle. 옛 그림 그대로
    void                                DrawShipTitle();
    // 공통 틀(배경·위 바·목록·아래 키 안내 바). bInteractive 가 거짓이면(닫는 중) 히트박스·로그·검사 없이 그린다
    void                                DrawShipMenu(bool bInteractive);
    // 함선 안(메뉴 닫힘, 배경 없음): 오른쪽 위 자원, 왼쪽 아래 [P] SHIP MENU, 단말기 위치에 투영한 이름 태그(범위 안이면 [E] + 이름)
    void                                DrawShipOverlay();
    // 단말기 태그를 놓고 그린다. 보이는 태그나 활성 단말기가 바뀌면 [HDShipUI] prompt 줄
    void                                DrawShipPromptTags(FHDShipUICanvas& UICanvas, int32 ActiveTerminalIndex);
    // 월드의 함선 방(처음 찾은 것을 캐시). 없으면 nullptr
    AHDShipInterior*                    FindShipInterior();
    // 작전 테이블 화면: 프로필·난이도 표로 화면 데이터를 만들고 방의 지도 캡처(없으면 도식)로 그린다
    void                                DrawWarTable(FHDShipUICanvas& UICanvas, const FHDShipMenu& ShipMenu, const TArray<FHDShipMenuItem>& Items);
    // 맨 위 층: 어둡게 + 가운데 HELLPOD LAUNCH, 큰 숫자, [BACKSPACE] CANCEL
    void                                DrawLaunchCountdown(float RemainingSeconds);
    // 처음 쓸 때 /Engine/EngineFonts/Roboto 를 읽는다. 못 읽으면 nullptr(캔버스 도구가 엔진 중간 글꼴로 대신한다)
    UFont*                              GetShipUIFont();
    // 아이콘 텍스처 캐시(ShipUITextures). 처음 쓸 때 LoadObject, 못 읽으면 nullptr 와 [HDShipUI] icon missing 한 번
    UTexture2D*                         GetShipUITexture(const FString& Path);
    // 캔버스 도구에 위 캐시를 붙인다
    void                                BindShipUITextures(FHDShipUICanvas& UICanvas);
    // 지금 든 무기의 카탈로그 Id 로 Img_Weapon 글자 타일을 바꾼다
    void                                RefreshWeaponIcon();
    // 그린 글자의 겹침·잘림을 센다. (화면, 뷰)마다 한 번 [HDShipUI] check 를 남기고, 그 뒤 (화면, 뷰, 커서)가 처음인 프레임에 문제가 있으면 Warning
    void                                CheckShipTexts(const FString& ScreenName, const FHDShipUICanvas& UICanvas, int32 Cursor);
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

    // 함선 화면 글꼴(엔진 Roboto). 한 번만 읽어 본다
    UPROPERTY()
    TObjectPtr<UFont>                       ShipUIFont;
    bool                                    bShipUIFontLoadTried = false;

    // 함선 화면 아이콘 텍스처(경로 → 텍스처). 못 읽은 경로는 nullptr 로 남겨 다시 읽지 않는다
    UPROPERTY()
    TMap<FString, TObjectPtr<UTexture2D>>   ShipUITextures;

    // 미션 HUD 무기 글자 타일(<Id>_<가로 px> → 텍스처)
    UPROPERTY()
    TMap<FName, TObjectPtr<UTexture2D>>     WeaponIconTextures;

    // 함선 화면 전환과 지난 프레임의 메뉴 열림·화면
    FHDShipUITransition                     ShipTransition;
    bool                                    bLastShipMenuOpen = false;
    EHDShipScreen                           LastShipScreen = EHDShipScreen::Title;

    // 마지막으로 남긴 frame 줄(화면|뷰)과 hints 줄(화면|키 안내)
    FString                                 LastShipFrameKey;
    FString                                 LastShipHintsKey;
    // 검사 줄을 남긴 (화면|뷰)와 (화면|뷰|커서)
    TSet<FString>                           CheckedShipViews;
    TSet<FString>                           CheckedShipCursors;

    // 함선 안 단말기 태그: 처음 찾은 방 액터와 마지막으로 남긴 prompt 줄(보이는 태그|활성)
    TWeakObjectPtr<AHDShipInterior>         CachedShipInterior;
    FString                                 LastPromptTagKey;
};
