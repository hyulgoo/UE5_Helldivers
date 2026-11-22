// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "InputKeyEventArgs.h"
#include "Controller/HDDirectKeyRules.h"
#include "UI/HDShipMenu.h"
#include "HDPlayerController.generated.h"

class ACharacter;
class UInputAction;
class UHDInputData;
class UHDAbilitySystemComponent;
class UHDStratagemHUDUserWidget;
class UHDCharacterControlData;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EHDCharacterInputAction : uint8
{
	ThirdLook,
	ThirdMove,
	FirstLook,
	FirstMove,
	Count
};

UCLASS()
class HELLDIVERS_API AHDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AHDPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
		
    UHDAbilitySystemComponent*              GetAbilitySystemComponent();

    // 키 이벤트는 하나씩 누른 순서대로 온다. Super(Enhanced Input) 를 먼저 부르고, 화살표 누름은 스트라타젬 명령 큐에, 숫자 키 1~4 누름은 무기 칸 큐에 넣는다. Q 누름은 스팀 요청, 왼쪽 Alt 누름은 급강하 요청으로 세운다.
    // 이어서 Enter·P·Escape·BackSpace 누름(HDDirectKey::TryGetMissionMenuKey)은 미션 메뉴 큐에 넣는다.
    // 함선 메뉴 중에는 메뉴 키(HDShipMenu::TryGetMenuKey)만 메뉴 큐에 넣고 나머지 큐는 건너뛴다
    virtual bool                            InputKey(const FInputKeyEventArgs& Params) override;
    // Super 의 입력 처리(여기서 Ctrl 입력 모드 GA 가 켜진다) 뒤에 메뉴 큐를 먼저 비우고, 미션 메뉴 큐(폰이 캐릭터가 아니어도, 죽어 있어도),
    // 명령 큐, 이어서 무기 칸 큐, 스팀 요청, 급강하 요청을 순서대로 폰에 넘긴다
    virtual void                            PlayerTick(float DeltaTime) override;

    // 치트 HDSimulateKeys 용. 첫 프레임을 바로 보내고 남은 프레임은 한 틱에 한 프레임씩 InputKey 로 보낸다. 보내는 중이면 무시
    void                                    SimulateKeyFrames(TArray<TArray<FHDSimulatedKeyEvent>> Frames);

    // 게임모드가 함선 단계에 들어올 때 부른다. 커서를 보이고 클릭·가리키기 이벤트를 켠다
    void                                    EnterShipMenu(bool bShowTitle);
    // 출격할 때 부른다. 커서·클릭 이벤트를 끄고 게임 입력만 받는다
    void                                    ExitShipMenu();
    bool                                    IsShipMenuActive() const { return bShipMenuActive; }
    const FHDShipMenu&                      GetShipMenu() const { return ShipMenu; }
    // 지금 화면의 줄. 문맥은 프로필 서브시스템에서 만든다(없으면 기본값)
    TArray<FHDShipMenuItem>                 BuildShipMenuItems() const;
    // HUD 마우스: 가리킨 줄로 커서를 옮기고, 누르면 그 줄을 확정한다(Index 가 -1 이면 타이틀 화면 전체)
    void                                    HoverShipMenuItem(int32 Index);
    void                                    ClickShipMenuItem(int32 Index);

    // 결과 화면에서 함선으로(Enter·결과 패널 클릭). 결과가 없으면 무시, 결과 뒤 ResultInputDelay 가 안 지났으면 [HDShip] return ignored, 지났으면 AHDGameMode::ReturnToShip
    void                                    RequestReturnToShip();
    // 게임모드가 결과를 낸 뒤 부른다. 확인 창을 닫고 커서·클릭·가리키기 이벤트를 켠다(EnterShipMenu 와 같은 입력 모드, 함선 메뉴는 켜지 않는다)
    void                                    ShowMissionResult();
    // 작전 포기 확인 창(ABANDON MISSION?)이 열려 있는가
    bool                                    IsAbandonPromptOpen() const;

protected:
    virtual void							BeginPlay() override final;
	virtual void							OnPossess(APawn* Pawn) override final;
    virtual void                            OnUnPossess() override final;
    virtual void                            SetupInputComponent() override final;

private:
    void		                            AbilityInputTriggered(const FGameplayTag Tag);
    void		                            AbilityInputReleased(const FGameplayTag Tag);
    void		                            AbilityInputToggled(const FGameplayTag Tag);

    void                                    ChangeCharacterControlType();
    void                                    SetCharacterControl(const EHDCharacterControlType NewCharacterControlType);

    void									Look(const FInputActionValue& Value);
    void									Move(const FInputActionValue& Value);

    void                                    SendNextSimulatedKeyFrame();

    // 메뉴 키 하나: 줄을 만들고 FHDShipMenu::HandleKey → [HDShip] key 로그 → ExecuteShipMenuAction
    void                                    HandleShipMenuKey(EHDShipMenuKey Key);
    // Deploy → AHDGameMode::RequestDeploy, Quit → UKismetSystemLibrary::QuitGame. 뒤 브리프가 Adjust·Select 갈래를 더한다
    void                                    ExecuteShipMenuAction(const FHDShipMenuAction& Action);

    // 미션 메뉴 키 하나. 결과가 있으면 Confirm = RequestReturnToShip, 확인 창이 열려 있으면 Confirm = 닫고 AHDGameMode::RequestAbandon / 나머지 = 닫기,
    // 미션 진행 중(Deploy·InProgress·Extraction, 함선 아님)이면 ToggleAbandon = 열기. 그 밖은 무시
    void                                    HandleMissionMenuKey(EHDMissionMenuKey Key);
    // 확인 창을 열고 닫는다. 바뀔 때만 [HDMission] abandon prompt open / closed
    void                                    SetAbandonPromptOpen(bool bOpen);

private:
	UPROPERTY()
	TObjectPtr<UHDAbilitySystemComponent>   AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UHDInputData>			    AbilityInputData;
    
    EHDCharacterControlType			        CurrentCharacterControlType = EHDCharacterControlType::ThirdPerson;

	UPROPERTY(EditDefaultsOnly)
	TMap<EHDCharacterInputAction, TObjectPtr<UInputAction>> InputActionMap;

    UPROPERTY(EditAnywhere)
    TMap<EHDCharacterControlType, TObjectPtr<UHDCharacterControlData>> CharacterControlDataMap;	

    // showdebug abilitysystem 을 이미 켰는가(다시 강하할 때 꺼지지 않게)
    bool                                    bAbilityDebugShown = false;

    // InputKey 가 받은 화살표 누름. PlayerTick 이 비운다
    TArray<EHDCommandInput>                 PendingStratagemCommands;

    // InputKey 가 받은 숫자 키 누름. PlayerTick 이 명령 큐 다음에 비운다
    TArray<EHDWeaponSlot>                   PendingWeaponSlots;

    // InputKey 가 받은 스팀 키(Q) 누름. PlayerTick 이 무기 칸 큐 다음에 처리하고 내린다
    bool                                    bPendingStim = false;

    // InputKey 가 받은 급강하 키(왼쪽 Alt) 누름. PlayerTick 이 스팀 요청 다음에 처리하고 내린다
    bool                                    bPendingDive = false;

    // HDSimulateKeys 가 아직 보내지 않은 프레임과 다음에 보낼 프레임 번호
    TArray<TArray<FHDSimulatedKeyEvent>>    SimulatedKeyFrames;
    int32                                   SimulatedKeyFrameIndex = 0;

    // 함선 메뉴의 화면·커서
    FHDShipMenu                             ShipMenu;
    bool                                    bShipMenuActive = false;
    // InputKey 가 받은 메뉴 키. PlayerTick 이 다른 큐보다 먼저 비운다
    TArray<EHDShipMenuKey>                  PendingShipMenuKeys;

    // InputKey 가 받은 미션 메뉴 키(Enter·P·Escape·BackSpace). PlayerTick 이 함선 메뉴 키 다음에 비운다
    TArray<EHDMissionMenuKey>               PendingMissionMenuKeys;
    // 작전 포기 확인 창. 열려 있어도 미션은 멈추지 않는다
    bool                                    bAbandonPromptOpen = false;
};
