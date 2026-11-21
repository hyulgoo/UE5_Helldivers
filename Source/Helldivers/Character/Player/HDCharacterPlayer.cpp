
#include "HDCharacterPlayer.h"
#include "Define/HDMontageSectionNames.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Component/HDCombatComponent.h"
#include "Component/HDMovementStateComponent.h"
#include "Component/HDStratagemComponent.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Attribute/HDSpeedAttributeSet.h"
#include "Attribute/HDWeaponAttributeSet.h"
#include "Player/HDGASPlayerState.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "GameData/HDCharacterControlData.h"
#include "Weapon\HDWeapon.h"
#include "Weapon/HDWeaponSlots.h"
#include "TimerManager.h"
#include "Game/HDGameMode.h"
#include "Engine/World.h"

namespace HDCharacterPlayer
{
    // 로그용 방향 이름
    const TCHAR* GetCommandName(const EHDCommandInput Command)
    {
        switch (Command)
        {
        case EHDCommandInput::Up:       return TEXT("Up");
        case EHDCommandInput::Down:     return TEXT("Down");
        case EHDCommandInput::Left:     return TEXT("Left");
        case EHDCommandInput::Right:    return TEXT("Right");
        default:                        return TEXT("None");
        }
    }
}

AHDCharacterPlayer::AHDCharacterPlayer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    GetCharacterMovement()->bOrientRotationToMovement = true;

    // Camera
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bEnableCameraLag = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    Combat              = CreateDefaultSubobject<UHDCombatComponent>(TEXT("Combat"));
    MovementStateComp   = CreateDefaultSubobject<UHDMovementStateComponent>(TEXT("MovementState"));
    Stratagem           = CreateDefaultSubobject<UHDStratagemComponent>(TEXT("Stratagem"));

    // DA_Input·C++ 어디에서도 쓰지 않던 IA_Equipped(E)를 무기 교체에 쓴다
    static ConstructorHelpers::FObjectFinder<UInputAction> SwapWeaponInputActionRef(TEXT("/Script/EnhancedInput.InputAction'/Game/Helldivers/Input/Action/IA_Equipped.IA_Equipped'"));
    if (SwapWeaponInputActionRef.Succeeded())
    {
        SwapWeaponInputAction = SwapWeaponInputActionRef.Object;
    }
}

void AHDCharacterPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AHDCharacterPlayer::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    InitAbilitySystemComponent();

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    ASC->GenericGameplayEventCallbacks.FindOrAdd(Tag_Event_PlayerHUD_Initialize).AddUObject(this, &AHDCharacterPlayer::SpawnDefaultWeapon);
    ASC->GenericGameplayEventCallbacks.FindOrAdd(Tag_Character_Action_HoldStratagem).AddUObject(this, &AHDCharacterPlayer::OnStratagemEventReceived, Tag_Character_Action_HoldStratagem.GetTag());
    ASC->GenericGameplayEventCallbacks.FindOrAdd(Tag_Character_Action_ThrowEnd).AddUObject(this, &AHDCharacterPlayer::OnStratagemEventReceived, Tag_Character_Action_ThrowEnd.GetTag());
    ASC->GenericGameplayEventCallbacks.FindOrAdd(Tag_Character_Action_DetachStratagem).AddUObject(this, &AHDCharacterPlayer::OnStratagemEventReceived, Tag_Character_Action_DetachStratagem.GetTag());
    Stratagem->BindLoadout(GetPlayerState<AHDGASPlayerState>());
}

void AHDCharacterPlayer::UnPossessed()
{
    Super::UnPossessed();

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const FGameplayTag BoundEventTags[] =
    {
        Tag_Event_PlayerHUD_Initialize,
        Tag_Character_Action_HoldStratagem,
        Tag_Character_Action_ThrowEnd,
        Tag_Character_Action_DetachStratagem,
    };

    for (const FGameplayTag& EventTag : BoundEventTags)
    {
        FGameplayEventMulticastDelegate* EventDelegate = ASC->GenericGameplayEventCallbacks.Find(EventTag);
        if (EventDelegate == nullptr)
        {
            continue;
        }

        EventDelegate->RemoveAll(this);
    }

    // 남은 시체가 플레이어스테이트 ASC 를 계속 돌려주면 시체에 맞은 폭발·총알이 새 몸의 체력을 깎는다
    ClearAbilitySystemComponent();
}

void AHDCharacterPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(CodeReloadEventTimerHandle);

    if (Combat)
    {
        TArray<AHDWeapon*> SlotWeapons;
        Combat->GetAllWeapons(SlotWeapons);
        for (AHDWeapon* SlotWeapon : SlotWeapons)
        {
            SlotWeapon->Destroy();
        }
    }

    Super::EndPlay(EndPlayReason);
}

void AHDCharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    NULL_CHECK(EnhancedInputComponent);

    // 스트라타젬 화살표는 묶지 않는다. AHDPlayerController::InputKey 가 원시 키 순서대로 InputStratagemCommand 를 부른다
    NULL_CHECK(SwapWeaponInputAction);
    EnhancedInputComponent->BindAction(SwapWeaponInputAction, ETriggerEvent::Started, this, &AHDCharacterPlayer::SwapWeapon);
}

void AHDCharacterPlayer::SetRagdoll(const bool bRagdoll, const FVector& Impulse)
{
    Super::SetRagdoll(bRagdoll, Impulse);

    if (bRagdoll == false)
    {
        SetCharacterStanceState(EHDCharacterStanceState::Prone, true);
    }
}

void AHDCharacterPlayer::EquipWeapon(AHDWeapon* NewWeapon)
{
    NULL_CHECK(NewWeapon);
    NULL_CHECK(WeaponAttributeSetEffect);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    Combat->EquipWeapon(NewWeapon, ASC);
    ApplyWeaponAttributes(Combat->GetWeapon());

    // HUD 가 이 이벤트로 크로스헤어 위젯을 만든다(AHDHUD::OnEquipWeaponUIEventRecieved)
    FGameplayAbilityHelper::SendGameplayEventToSelf(Tag_Event_PlayerHUD_EquipWeapon, ASC);
}

void AHDCharacterPlayer::ApplyWeaponAttributes(const AHDWeapon* InWeapon)
{
    NULL_CHECK(InWeapon);
    NULL_CHECK(WeaponAttributeSetEffect);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    // GE 수정자는 Ammo 가 MaxAmmo 보다 먼저라, Max 가 0 인 첫 장착에서는 PreAttributeChange 가 Ammo·Capacity current 를 0 으로 자른다.
    // 그래서 Max 를 무기 값으로 먼저 정한다(브리프 14 사용자 결정 (A))
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetMaxAmmoAttribute(), Combat->GetWeaponMaxAmmoCount());
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetMaxCapacityAttribute(), Combat->GetWeaponMaxCapacityCount());

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(WeaponAttributeSetEffect, 1.f, ASC->MakeEffectContext());
    CONDITION_CHECK(SpecHandle.IsValid());

    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_Ammo, Combat->GetWeaponAmmoCount());
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_MaxAmmo, Combat->GetWeaponMaxAmmoCount());
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_Capacity, Combat->GetWeaponCapacityCount());
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_MaxCapacity, Combat->GetWeaponMaxCapacityCount());

    ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void AHDCharacterPlayer::SwapWeapon()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK_WITHOUT_LOG(ASC);

    if (ASC->HasMatchingGameplayTag(Tag_Character_Action_HoldStratagem) || ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead))
    {
        return;
    }

    CONDITION_CHECK_WITHOUT_LOG(Combat->SwapWeapon(ASC));

    const AHDWeapon* ActiveWeapon = Combat->GetWeapon();
    ApplyWeaponAttributes(ActiveWeapon);
    FGameplayAbilityHelper::SendGameplayEventToSelf(Tag_Event_PlayerHUD_EquipWeapon, ASC);

    UE_LOG(LogTemp, Display, TEXT("[HDWeapon] swap -> %s ammo=%.0f/%.0f capacity=%.0f"),
        (ActiveWeapon && ActiveWeapon->IsSupportWeapon()) ? TEXT("Support") : TEXT("Primary"),
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetAmmoAttribute()),
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxAmmoAttribute()),
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetCapacityAttribute()));
}

bool AHDCharacterPlayer::Resupply()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (ASC == nullptr || ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead))
    {
        return false;
    }

    NULL_CHECK_WITH_RETURNTYPE(WeaponAttributeSetEffect, false);

    const float OldAmmo     = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetAmmoAttribute());
    const float OldCapacity = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetCapacityAttribute());
    const float MaxAmmo     = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxAmmoAttribute());
    const float MaxCapacity = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxCapacityAttribute());

    // EquipWeapon 과 같은 GE(BP 값 BPGE_EquipWeapon, SetByCaller 넷을 Override)에 지금 최대값을 넣는다. 수류탄은 채우지 않는다
    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(WeaponAttributeSetEffect, 1.f, ASC->MakeEffectContext());
    CONDITION_CHECK_WITH_RETURNTYPE(SpecHandle.IsValid(), false);

    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_Ammo, MaxAmmo);
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_MaxAmmo, MaxAmmo);
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_Capacity, MaxCapacity);
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_MaxCapacity, MaxCapacity);

    ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] resupply Ammo %.0f->%.0f Capacity %.0f->%.0f"),
        OldAmmo, ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetAmmoAttribute()),
        OldCapacity, ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetCapacityAttribute()));
    return true;
}

UAnimMontage* AHDCharacterPlayer::GetCombatMontage(const EHDCombatMontage MontageType) const
{
    return Combat->GetCombatMontage(MontageType);
}

void AHDCharacterPlayer::PlayWeaponMontage(const EHDCombatMontage MontageType)
{
    USkeletalMeshComponent* WeaponMesh = Combat->GetWeaponMesh();
    NULL_CHECK(WeaponMesh);
    
    FName SectionName = NAME_None;
    if (MontageType == EHDCombatMontage::Fire)
    {
        SectionName = HDMONTAGE_SECTIONNAME_WEAPON_FIRE;
    }
    else if (MontageType == EHDCombatMontage::Reload)
    {
        if(MovementStateComp->GetStanceState() == EHDCharacterStanceState::Prone)
        {
            SectionName = HDMONTAGE_SECTIONNAME_WEAPON_RELOAD_PRONE;
        }
        else
        {
            SectionName = Combat->IsShoulder() ? HDMONTAGE_SECTIONNAME_WEAPON_RELOAD_SHOULDER : HDMONTAGE_SECTIONNAME_WEAPON_RELOAD_HIP;
        }
    }

    CONDITION_CHECK(SectionName != NAME_None);

    UAnimMontage* WeaponMontage = Combat->GetWeaponMontage();
    NULL_CHECK(WeaponMontage);

    float PlayRate = 1.f;

    if(MontageType == EHDCombatMontage::Fire)
    {
        const int32 SectionIndex = WeaponMontage->GetSectionIndex(SectionName);
        const float MontageLength = WeaponMontage->GetSectionLength(SectionIndex); 
        PlayRate = MontageLength / Combat->GetWeaponFireDelay();
    }

    UAnimInstance* WeaponAnimInstance = WeaponMesh->GetAnimInstance();
    NULL_CHECK(WeaponAnimInstance);

    WeaponAnimInstance->Montage_Play(WeaponMontage, PlayRate);
    WeaponAnimInstance->Montage_JumpToSection(SectionName, WeaponMontage);

    // 노티파이가 없는 무기 몽타주는 발사·재장전 이벤트를 C++ 로 보낸다. 소총은 무기 몽타주 노티파이가 보낸다
    const AHDWeapon* ActiveWeapon = Combat->GetWeapon();
    if (ActiveWeapon && ActiveWeapon->SendsMontageEventsFromCode())
    {
        ScheduleCodeMontageEvent(MontageType, WeaponMontage, SectionName);
    }
}

void AHDCharacterPlayer::ScheduleCodeMontageEvent(const EHDCombatMontage MontageType, const UAnimMontage* WeaponMontage, const FName SectionName)
{
    NULL_CHECK(WeaponMontage);

    UWorld* World = GetWorld();
    NULL_CHECK(World);

    if (MontageType == EHDCombatMontage::Fire)
    {
        // 한 활성에 한 발. FireDelay 안에 다시 오면 보내지 않고, 사격 GA 는 입력을 뗄 때 취소로 끝난다
        const double Now = World->GetTimeSeconds();
        const float FireDelay = Combat->GetWeaponFireDelay();
        if (HDWeaponMontageEvent::IsFireReady(LastCodeFireEventTime, Now, FireDelay) == false)
        {
            UE_LOG(LogTemp, Display, TEXT("[HDWeapon] code fire skipped (%.2fs < %.2fs)"), Now - LastCodeFireEventTime, FireDelay);
            return;
        }

        LastCodeFireEventTime = Now;
        GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() { SendCodeMontageEvent(Tag_Event_Spawn_Projectile); }));
        UE_LOG(LogTemp, Display, TEXT("[HDWeapon] code event Event.Spawn.Projectile next tick"));
    }
    else if (MontageType == EHDCombatMontage::Reload)
    {
        // 재장전 대기 태스크는 몸 몽타주가 끝나면 끝나므로 그 전에 보낸다
        const int32 SectionIndex = WeaponMontage->GetSectionIndex(SectionName);
        const float WeaponSectionLength = SectionIndex != INDEX_NONE ? WeaponMontage->GetSectionLength(SectionIndex) : 0.f;
        const UAnimMontage* BodyReloadMontage = GetCombatMontage(EHDCombatMontage::Reload);
        // 재장전 GA 는 섹션 이름 없이 재생해 첫 섹션만 돈다(AM_Reload 는 섹션 7개, 첫 섹션 Rifle_Hip 은 다음 섹션 없음)
        const float BodyMontageLength = BodyReloadMontage ? BodyReloadMontage->GetSectionLength(0) : 0.f;
        const float Delay = HDWeaponMontageEvent::GetReloadEventDelay(WeaponSectionLength, BodyMontageLength);

        FTimerManager& TimerManager = GetWorldTimerManager();
        TimerManager.ClearTimer(CodeReloadEventTimerHandle);

        const FTimerDelegate ReloadEventDelegate = FTimerDelegate::CreateWeakLambda(this, [this]() { SendCodeMontageEvent(Tag_Event_Reload); });
        if (Delay <= 0.f)
        {
            TimerManager.SetTimerForNextTick(ReloadEventDelegate);
        }
        else
        {
            TimerManager.SetTimer(CodeReloadEventTimerHandle, ReloadEventDelegate, Delay, false);
        }

        UE_LOG(LogTemp, Display, TEXT("[HDWeapon] code event Event.Reload in %.2fs (weapon %.2f body %.2f)"), Delay, WeaponSectionLength, BodyMontageLength);
    }
}

void AHDCharacterPlayer::SendCodeMontageEvent(const FGameplayTag EventTag)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK_WITHOUT_LOG(ASC);

    FGameplayAbilityHelper::SendGameplayEventToSelf(EventTag, ASC);
}

bool AHDCharacterPlayer::IsEquippedWeapon() const
{
    return Combat->IsEquippedWeapon();
}

bool AHDCharacterPlayer::IsShoulder() const
{
    return Combat->IsShoulder();
}

bool AHDCharacterPlayer::IsWeaponAutoFire() const
{
    return Combat->IsWeaponAutoFire();
}

void AHDCharacterPlayer::SpawnProjectile()
{
    Combat->SpawnProjectile();
}

void AHDCharacterPlayer::SetWeaponActive(const bool bActive)
{
    Combat->SetWeaponActive(bActive);
}

void AHDCharacterPlayer::SpawnDefaultWeapon(const FGameplayEventData* Payload)
{
    AHDWeapon* Weapon = Combat->SpawnDefaultWeapon();
    NULL_CHECK(Weapon);
    EquipWeapon(Weapon);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);
    ASC->GenericGameplayEventCallbacks.Remove(Tag_Event_PlayerHUD_Initialize);
}

void AHDCharacterPlayer::SetShouldering(const bool bShoulder)
{
    Combat->SetShoulder(bShoulder);   
}

float AHDCharacterPlayer::GetWeaponFireDelay() const
{
    return Combat->GetWeaponFireDelay();
}

void AHDCharacterPlayer::SetCharacterControlData(UHDCharacterControlData* CharacterControlData)
{
    bUseControllerRotationYaw = CharacterControlData->bUseControllerRotationYaw;

    UCharacterMovementComponent* CharacterMovementComponent     = GetCharacterMovement();
    CharacterMovementComponent->bOrientRotationToMovement       = CharacterControlData->bOrientRotationToMovement;
    CharacterMovementComponent->bUseControllerDesiredRotation   = CharacterControlData->bUseControllerDesiredRotation;
    CharacterMovementComponent->RotationRate                    = CharacterControlData->RotationRate;

    SpringArm->TargetArmLength         = CharacterControlData->TargetArmLength;
    SpringArm->TargetOffset            = CharacterControlData->TargetOffset;
    SpringArm->SocketOffset            = CharacterControlData->SocketOffset;
    SpringArm->bUsePawnControlRotation = CharacterControlData->bUsePawnControlRotation;
    SpringArm->bInheritPitch           = CharacterControlData->bInheritPitch;
    SpringArm->bInheritYaw             = CharacterControlData->bInheritYaw;
    SpringArm->bInheritRoll            = CharacterControlData->bInheritRoll;
    SpringArm->bDoCollisionTest        = CharacterControlData->bDoCollisionTest;

    Combat->SetSpringArmTargetLength(CharacterControlData->TargetArmLength);
    MovementStateComp->SetSpringArmDefaultZOffset(CharacterControlData->TargetOffset.Z);
}

void AHDCharacterPlayer::UpdateSpeed(const EHDCharacterStanceState StanceState, const EHDCharacterMovementState MoveState)
{
    const bool bSprint = MoveState == EHDCharacterMovementState::Sprint;
    FGameplayAttribute Attribute;
    switch (StanceState)
    {
    case EHDCharacterStanceState::Idle:
        Attribute = bSprint ? UHDSpeedAttributeSet::GetSprintSpeedAttribute() : UHDSpeedAttributeSet::GetWalkSpeedAttribute();
        break;
    case EHDCharacterStanceState::Crouch:
        Attribute = UHDSpeedAttributeSet::GetCrouchSpeedAttribute();
        break;
    case EHDCharacterStanceState::Prone:
        Attribute = UHDSpeedAttributeSet::GetCrawlingSpeedAttribute();
        break;
    }

    CONDITION_CHECK(Attribute.IsValid());
    float Speed = GetAbilitySystemComponent()->GetNumericAttribute(Attribute);
    if (bSprint && StanceState == EHDCharacterStanceState::Crouch)
    {
        Speed *= 1.5f;
    }

    GetCharacterMovement()->MaxWalkSpeed = Speed;
}

void AHDCharacterPlayer::InputStratagemCommand(const EHDCommandInput Command)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (ASC == nullptr || ASC->HasMatchingGameplayTag(Tag_Character_Action_StratagemInputMode) == false)
    {
        return;
    }

    Stratagem->AddStratagemCommand(Command);
    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] command %s count=%d"), HDCharacterPlayer::GetCommandName(Command), Stratagem->GetCurrentInputNum());

    // HUD 연동용 GAS Event
    FGameplayAbilityHelper::SendGameplayEventToTarget(Tag_Event_StratagemHUD_AddCommand, this, ASC);
}

void AHDCharacterPlayer::SetMovementState(const EHDCharacterMovementState NewState)
{
    MovementStateComp->SetMovementState(NewState);
    
    UpdateSpeed(MovementStateComp->GetStanceState(), NewState);
}

void AHDCharacterPlayer::SetCharacterStanceState(const EHDCharacterStanceState NewState, const bool bForced)
{
    MovementStateComp->SetStanceState(NewState, bForced);

    UpdateSpeed(NewState, MovementStateComp->GetMovementState());
}

void AHDCharacterPlayer::RestoreStanceState()
{
    MovementStateComp->RestoreStanceState();

    UpdateSpeed(MovementStateComp->GetStanceState(), MovementStateComp->GetMovementState());
}

void AHDCharacterPlayer::OnStratagemEventReceived(const FGameplayEventData* Payload, const FGameplayTag EventTag)
{
    if (EventTag == Tag_Character_Action_HoldStratagem)
    {
        TryHoldStratagem();
    }
    else if (EventTag == Tag_Character_Action_DetachStratagem)
    {
        Stratagem->ThrowFinished();

        // 손을 떠났으므로 든 상태를 푼다. 사격은 투척 GA 가 끝날 때까지 그 GA 의 Block.Fire 로 막혀 있다
        UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
        NULL_CHECK(ASC);
        ASC->SetLooseGameplayTagCount(Tag_Character_Action_HoldStratagem, 0);
    }
    else if (EventTag == Tag_Character_Action_ThrowEnd)
    {
        SetWeaponActive(true);
    }
}

void AHDCharacterPlayer::TryHoldStratagem()
{   
    const bool CanHoldStrategem = Stratagem->IsSelectedStratagemExist();
    if (CanHoldStrategem)
    {
        Stratagem->HoldStratagem(GetMesh(), Combat->GetHitTarget());
        SetWeaponActive(false);
        UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
        NULL_CHECK(ASC);
        // 루즈 태그는 개수를 세므로, 들기가 겹쳐도 한 번 풀면 없어지게 1 로 둔다
        ASC->SetLooseGameplayTagCount(Tag_Character_Action_HoldStratagem, 1);
    }
    else
    {
        Stratagem->CancelStratagem();
    }

    Stratagem->ClearCommand();
}

void AHDCharacterPlayer::InitAbilitySystemComponent()
{
    AHDGASPlayerState* GASPlayerState = GetPlayerState<AHDGASPlayerState>();
    NULL_CHECK(GASPlayerState);

    SetAbilitySystemComponent(GASPlayerState, GASPlayerState->GetAbilitySystemComponent());
}

void AHDCharacterPlayer::InterpFOV(float DeltaSeconds)
{
    const bool bIsShoulder = Combat->IsShoulder();
    const float TargetFOV = bIsShoulder ? Combat->GetWeaponZoomedFOV() : Combat->GetDefaultFOV();
    const float InterpSpeed = bIsShoulder ? Combat->GetWeaponZoomInterpSpeed() : Combat->GetZoomInterpSpeed();
    const float NewInterpFOV = FMath::FInterpTo(Combat->GetCurrentFOV(), TargetFOV, DeltaSeconds, InterpSpeed);
    Combat->SetCurrentFOV(NewInterpFOV);
    
    FollowCamera->SetFieldOfView(Combat->GetCurrentFOV());
}

void AHDCharacterPlayer::SetDead()
{
    Super::SetDead();

    // 시체가 조준 선 검사와 디버그 구를 계속 그리지 않게
    Combat->SetComponentTickEnabled(false);

    // 들고 있던 스트라타젬은 버리고 든 상태를 푼다. ASC 는 플레이어스테이트 것이라 증원 뒤에도 남는다
    Stratagem->CancelStratagem();
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        ASC->SetLooseGameplayTagCount(Tag_Character_Action_HoldStratagem, 0);
    }

    APlayerController* PlayerController = GetController<APlayerController>();
    NULL_CHECK(PlayerController);

    if (IsLocallyControlled())
    {
        DisableInput(PlayerController);
    }

    MovementStateComp->SetStanceState(EHDCharacterStanceState::Prone);

    // 남은 증원이 0 이면 미션 실패. 남아 있으면 죽은 채로 기다린다(다시 강하는 브리프 9)
    AHDGameMode* HDGameMode = GetWorld()->GetAuthGameMode<AHDGameMode>();
    if (HDGameMode)
    {
        HDGameMode->NotifyPlayerDied(GetController());
    }
}
