// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "InputKeyEventArgs.h"
#include "Controller/HDDirectKeyRules.h"
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

    // 키 이벤트는 하나씩 누른 순서대로 온다. Super(Enhanced Input) 를 먼저 부르고, 화살표 누름은 스트라타젬 명령 큐에, 숫자 키 1~4 누름은 무기 칸 큐에 넣는다. Q 누름은 스팀 요청으로 세운다
    virtual bool                            InputKey(const FInputKeyEventArgs& Params) override;
    // Super 의 입력 처리(여기서 Ctrl 입력 모드 GA 가 켜진다) 뒤에 명령 큐, 이어서 무기 칸 큐, 스팀 요청을 순서대로 폰에 넘긴다
    virtual void                            PlayerTick(float DeltaTime) override;

    // 치트 HDSimulateKeys 용. 첫 프레임을 바로 보내고 남은 프레임은 한 틱에 한 프레임씩 InputKey 로 보낸다. 보내는 중이면 무시
    void                                    SimulateKeyFrames(TArray<TArray<FHDSimulatedKeyEvent>> Frames);

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

    // HDSimulateKeys 가 아직 보내지 않은 프레임과 다음에 보낼 프레임 번호
    TArray<TArray<FHDSimulatedKeyEvent>>    SimulatedKeyFrames;
    int32                                   SimulatedKeyFrameIndex = 0;
};
