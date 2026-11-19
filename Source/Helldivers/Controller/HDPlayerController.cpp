// Fill out your copyright notice in the Description page of Project Settings.

#include "HDPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "Component/HDStratagemComponent.h"
#include "Component/HDCombatComponent.h"
#include "Component/HDInputComponent.h"
#include "Component/HDAbilitySystemComponent.h"
#include "UI/HDHUD.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "GameData/HDCharacterControlData.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Cheat/HDCheatManager.h"

static TAutoConsoleVariable<int32> CVarHDAbilitySystemDebugOnSpawn(
    TEXT("hd.Debug.AbilitySystemOnSpawn"),
    0,
    TEXT("1: run 'showdebug abilitysystem' on first possess. While it is shown, AHUD skips DrawHUD, so the mission HUD is hidden"),
    ECVF_Cheat);

AHDPlayerController::AHDPlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CheatClass = UHDCheatManager::StaticClass();

    static ConstructorHelpers::FObjectFinder<UHDCharacterControlData> ThirdPersonDataRef(TEXT("/Script/Helldivers.HDCharacterControlData'/Game/Helldivers/CharacterControl/HDC_ThirdPerson.HDC_ThirdPerson'"));
    if (ThirdPersonDataRef.Succeeded())
    {
        CharacterControlDataMap.Add(EHDCharacterControlType::ThirdPerson, ThirdPersonDataRef.Object);
    }

    static ConstructorHelpers::FObjectFinder<UHDCharacterControlData> FirstPersonDataRef(TEXT("/Script/Helldivers.HDCharacterControlData'/Game/Helldivers/CharacterControl/HDC_FirstPerson.HDC_FirstPerson'"));
    if (FirstPersonDataRef.Succeeded())
    {
        CharacterControlDataMap.Add(EHDCharacterControlType::FirstPerson, FirstPersonDataRef.Object);
    }
}

UHDAbilitySystemComponent* AHDPlayerController::GetAbilitySystemComponent()
{
    if(AbilitySystemComponent == nullptr)
    {
        AbilitySystemComponent = Cast<UHDAbilitySystemComponent>(FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(GetPawn()));
        NULL_CHECK_WITH_RETURNTYPE(AbilitySystemComponent, nullptr);
    }

    return AbilitySystemComponent;
}

void AHDPlayerController::BeginPlay()
{
    Super::BeginPlay();
}

void AHDPlayerController::OnPossess(APawn* aPawn)
{
    Super::OnPossess(aPawn);

    if(Cast<AHDCharacterPlayer>(aPawn))
    {
        SetCharacterControl(EHDCharacterControlType::ThirdPerson);

        UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
        NULL_CHECK(ASC);

        AHDHUD* HUD = GetHUD<AHDHUD>();
        NULL_CHECK(HUD);

        HUD->SetAbilitySystemComponent(ASC);
        HUD->CreateDefaultWidget();

        // PlayerState ASC 는 다시 강하해도 같으므로 앞 빙의에서 묶은 것을 먼저 푼다
        FGameplayEventMulticastDelegate& EquipWeaponDelegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(Tag_Event_PlayerHUD_EquipWeapon);
        EquipWeaponDelegate.RemoveAll(HUD);
        EquipWeaponDelegate.AddUObject(HUD, &AHDHUD::OnEquipWeaponUIEventRecieved);

        FGameplayAbilityHelper::SendGameplayEventToSelf(Tag_Event_PlayerHUD_Initialize, GetAbilitySystemComponent());

        // 토글 명령이라 처음 한 번만. 켜져 있으면 AHUD::PostRender 가 DrawHUD 를 건너뛰어 미션 HUD 가 안 보이므로 hd.Debug.AbilitySystemOnSpawn 1 일 때만
        if (bAbilityDebugShown == false && CVarHDAbilitySystemDebugOnSpawn.GetValueOnGameThread() > 0)
        {
            ConsoleCommand(TEXT("showdebug abilitysystem"));
            bAbilityDebugShown = true;
        }
    }
}

void AHDPlayerController::OnUnPossess()
{
    Super::OnUnPossess();

    AbilitySystemComponent = nullptr;
}

void AHDPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UHDInputComponent* HDInput = Cast<UHDInputComponent>(InputComponent);
    NULL_CHECK(HDInput);

    CONDITION_CHECK(InputActionMap.Num() == static_cast<int32>(EHDCharacterInputAction::Count));

    HDInput->SetTaggedInputActionDataAsset(AbilityInputData, this, &ThisClass::AbilityInputTriggered, &ThisClass::AbilityInputReleased, &ThisClass::AbilityInputToggled);
    HDInput->BindAction(InputActionMap[EHDCharacterInputAction::ThirdMove], ETriggerEvent::Triggered, this, &ThisClass::Move);
    HDInput->BindAction(InputActionMap[EHDCharacterInputAction::ThirdLook], ETriggerEvent::Triggered, this, &ThisClass::Look);
}

void AHDPlayerController::AbilityInputTriggered(const FGameplayTag InputTag)
{
    UHDAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const FGameplayTag& AssetTag = AbilityInputData->GetTriggerTagByInputTag(InputTag);
    ASC->AbilityInputTagTriggered(AssetTag);
}

void AHDPlayerController::AbilityInputReleased(const FGameplayTag InputTag)
{
    UHDAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const FGameplayTag& AssetTag = AbilityInputData->GetTriggerTagByInputTag(InputTag);
    ASC->AbilityInputTagReleased(AssetTag);
}

void AHDPlayerController::AbilityInputToggled(const FGameplayTag InputTag)
{
    UHDAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const FGameplayTag& AssetTag = AbilityInputData->GetTriggerTagByInputTag(InputTag);
    ASC->AbilityInputTagToggled(AssetTag);
}

void AHDPlayerController::ChangeCharacterControlType()
{
    CurrentCharacterControlType = CurrentCharacterControlType == EHDCharacterControlType::FirstPerson 
        ? EHDCharacterControlType::ThirdPerson : EHDCharacterControlType::FirstPerson;

    SetCharacterControl(CurrentCharacterControlType);
}

void AHDPlayerController::SetCharacterControl(const EHDCharacterControlType NewCharacterControlType)
{
    CONDITION_CHECK(CharacterControlDataMap.Contains(NewCharacterControlType));
    
    CurrentCharacterControlType = NewCharacterControlType;

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    NULL_CHECK(Subsystem);

    Subsystem->ClearAllMappings();

    UInputMappingContext* NewMappingContext = CharacterControlDataMap[CurrentCharacterControlType]->InputMappingContext;
    NULL_CHECK(NewMappingContext);

    Subsystem->AddMappingContext(NewMappingContext, 0);

    AHDCharacterPlayer* HDPlayer = GetPawn<AHDCharacterPlayer>();
    NULL_CHECK(HDPlayer);

    HDPlayer->SetCharacterControlData(CharacterControlDataMap[CurrentCharacterControlType]);
}

void AHDPlayerController::Look(const FInputActionValue& Value)
{
    const FVector2D& LookAxisVector = Value.Get<FVector2D>();

    AddYawInput(LookAxisVector.X);
    AddPitchInput(LookAxisVector.Y);
}

void AHDPlayerController::Move(const FInputActionValue& Value)
{
    const FVector2D& MovementVector = Value.Get<FVector2D>();
    const FRotator& Rotation = GetControlRotation();

    const FRotator YawRotation(0, Rotation.Yaw, 0);

    const FVector& ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector& RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    APawn* ControlledPawn = GetPawn<APawn>();
    if (ControlledPawn)
    {
        ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.X);
        ControlledPawn->AddMovementInput(RightDirection, MovementVector.Y);
    }
}
