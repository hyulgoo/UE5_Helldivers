
#include "HDCharacterPlayer.h"
#include "Define/HDMontageSectionNames.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "AbilitySystemComponent.h"
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
#include "Weapon/HDSecondaryWeapon.h"
#include "AbilitySystem/GameAbility/HDGA_Fire.h"
#include "AbilitySystem/GameAbility/HDGA_Reload.h"
#include "AbilitySystem/GameAbility/HDGA_ThrowGrenade.h"
#include "AbilitySystem/GameAbility/HDGA_Stim.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Character/Player/HDStimRules.h"
#include "Weapon/Projectile/HDProjectileGrenade.h"
#include "Define/HDSocketNames.h"
#include "TimerManager.h"
#include "Game/HDGameMode.h"
#include "Engine/World.h"

namespace
{
    // 강하·증원 때 받는 수류탄 수(최대치)
    constexpr float DefaultGrenadeCount = 4.f;
    // 강하·증원 때 받는 스팀 수(최대치)
    constexpr float DefaultStimCount = 4.f;
    // 손에서 조준 방향보다 이만큼 위로 던진다(도)
    constexpr float GrenadeThrowPitchOffset = 10.f;
    constexpr float GrenadeThrowSpeed = 1800.f;
}

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

    // 캐릭터 BP 의 Abilities 배열은 BP 값이 C++ 기본값을 덮으므로 투척 GA 는 여기서 준다. ASC 는 플레이어스테이트 것이라 증원 때 다시 주지 않는다
    if (ASC->FindAbilitySpecFromClass(UHDGA_ThrowGrenade::StaticClass()) == nullptr)
    {
        ASC->GiveAbility(FGameplayAbilitySpec(UHDGA_ThrowGrenade::StaticClass()));
    }

    // 스팀 GA 도 같은 이유로 여기서 준다
    if (ASC->FindAbilitySpecFromClass(UHDGA_Stim::StaticClass()) == nullptr)
    {
        ASC->GiveAbility(FGameplayAbilitySpec(UHDGA_Stim::StaticClass()));
    }

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

    // 스트라타젬 화살표와 무기 숫자 키는 묶지 않는다. AHDPlayerController::InputKey 가 원시 키 순서대로 InputStratagemCommand·SelectWeaponSlot 을 부른다
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
    // 수류탄을 든 채 지원 무기를 주우면 쓰는 칸이 Support 로 바뀐다. 태그가 남으면 사격·재장전이 막힌다
    ASC->SetLooseGameplayTagCount(Tag_Character_State_GrenadeReady, 0);
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

void AHDCharacterPlayer::SelectWeaponSlot(const EHDWeaponSlot Slot)
{
    const auto LogRefused = [Slot](const TCHAR* Reason)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDWeapon] select %s refused (%s)"), GetWeaponSlotName(Slot), Reason);
    };

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (ASC == nullptr || ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead))
    {
        LogRefused(TEXT("dead"));
        return;
    }

    if (ASC->HasMatchingGameplayTag(Tag_Character_Action_HoldStratagem) || ASC->HasMatchingGameplayTag(Tag_Character_Action_StratagemInputMode))
    {
        LogRefused(TEXT("stratagem"));
        return;
    }

    if (IsWeaponActionActive())
    {
        LogRefused(TEXT("busy"));
        return;
    }

    if (Slot == EHDWeaponSlot::Grenade)
    {
        const float GrenadeCount = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetGrenadeAttribute());
        if (GrenadeCount <= 0.f)
        {
            LogRefused(TEXT("no grenades"));
            return;
        }

        Combat->SetGrenadeAvailable(true);
        const EHDWeaponSelectResult GrenadeResult = Combat->SelectWeaponSlot(Slot, ASC);
        if (GrenadeResult != EHDWeaponSelectResult::Selected)
        {
            LogRefused(GrenadeResult == EHDWeaponSelectResult::AlreadyActive ? TEXT("active") : TEXT("empty"));
            return;
        }

        // 무기 Attribute 는 마지막 총 것으로 남긴다(돌아갈 때 그대로 쓴다)
        ASC->SetLooseGameplayTagCount(Tag_Character_State_GrenadeReady, 1);
        UE_LOG(LogTemp, Display, TEXT("[HDWeapon] select Grenade -> grenades=%.0f/%.0f"),
            GrenadeCount, ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxGrenadeAttribute()));
        return;
    }

    const EHDWeaponSelectResult SelectResult = Combat->SelectWeaponSlot(Slot, ASC);
    if (SelectResult == EHDWeaponSelectResult::AlreadyActive)
    {
        LogRefused(TEXT("active"));
        return;
    }

    if (SelectResult == EHDWeaponSelectResult::Empty)
    {
        LogRefused(TEXT("empty"));
        return;
    }

    const AHDWeapon* ActiveWeapon = Combat->GetWeapon();
    ApplyWeaponAttributes(ActiveWeapon);
    FGameplayAbilityHelper::SendGameplayEventToSelf(Tag_Event_PlayerHUD_EquipWeapon, ASC);
    // 수류탄을 들고 있었으면 총으로 돌아왔다
    ASC->SetLooseGameplayTagCount(Tag_Character_State_GrenadeReady, 0);

    UE_LOG(LogTemp, Display, TEXT("[HDWeapon] select %s -> %s ammo=%.0f/%.0f capacity=%.0f"),
        GetWeaponSlotName(Slot),
        ActiveWeapon ? *ActiveWeapon->GetClass()->GetName() : TEXT("None"),
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetAmmoAttribute()),
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxAmmoAttribute()),
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetCapacityAttribute()));
}

bool AHDCharacterPlayer::IsWeaponActionActive() const
{
    const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(ASC, false);

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (Spec.IsActive() == false || Spec.Ability == nullptr)
        {
            continue;
        }

        if (Spec.Ability->IsA<UHDGA_Fire>() || Spec.Ability->IsA<UHDGA_Reload>() || Spec.Ability->IsA<UHDGA_ThrowGrenade>())
        {
            return true;
        }
    }

    return false;
}

void AHDCharacterPlayer::LeaveGrenadeSlot(const TCHAR* Reason)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const EHDWeaponSlot GunSlot = Combat->GetLastGunSlot();
    Combat->SelectWeaponSlot(GunSlot, ASC);
    ApplyWeaponAttributes(Combat->GetWeapon());
    FGameplayAbilityHelper::SendGameplayEventToSelf(Tag_Event_PlayerHUD_EquipWeapon, ASC);
    ASC->SetLooseGameplayTagCount(Tag_Character_State_GrenadeReady, 0);

    UE_LOG(LogTemp, Display, TEXT("[HDWeapon] leave Grenade -> %s (%s)"), GetWeaponSlotName(GunSlot), Reason);
}

void AHDCharacterPlayer::ThrowGrenade()
{
    UWorld* World = GetWorld();
    NULL_CHECK(World);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    const FVector ThrowStart = GetMesh()->GetSocketLocation(HDSOCKETNAME_RIGRHTHAND);
    const FVector& HitTarget = Combat->GetHitTarget();

    FVector ThrowDirection = HitTarget.IsZero() ? FVector::ZeroVector : (HitTarget - ThrowStart).GetSafeNormal();
    if (ThrowDirection.IsZero())
    {
        ThrowDirection = GetBaseAimRotation().Vector();
    }

    FRotator ThrowRotation = ThrowDirection.Rotation();
    ThrowRotation.Pitch += GrenadeThrowPitchOffset;
    const FVector ThrowVelocity = ThrowRotation.Vector() * GrenadeThrowSpeed;

    // 회전 없이 만든다. ProjectileMovement 는 bInitialVelocityInLocalSpace 라 생성 회전으로 속도를 한 번 더 돌린다
    const FTransform SpawnTransform(ThrowStart);
    AHDProjectileGrenade* Grenade = World->SpawnActorDeferred<AHDProjectileGrenade>(AHDProjectileGrenade::StaticClass(), SpawnTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    NULL_CHECK(Grenade);

    Grenade->SetThrowVelocity(ThrowVelocity);
    Grenade->FinishSpawning(SpawnTransform);

    const float GrenadeCount = FMath::Max(ASC->GetNumericAttributeBase(UHDWeaponAttributeSet::GetGrenadeAttribute()) - 1.f, 0.f);
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetGrenadeAttribute(), GrenadeCount);

    UE_LOG(LogTemp, Display, TEXT("[HDGrenade] thrown left=%.0f"), ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetGrenadeAttribute()));
}

void AHDCharacterPlayer::OnGrenadeThrowFinished()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK_WITHOUT_LOG(ASC);

    // 죽는 순간 투척 GA 가 끝나도(몽타주 끊김) 시체가 총을 다시 꺼내지 않는다
    if (ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead))
    {
        return;
    }

    if (ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetGrenadeAttribute()) <= 0.f && Combat->IsGrenadeSlotActive())
    {
        LeaveGrenadeSlot(TEXT("empty"));
    }
}

void AHDCharacterPlayer::UseStim()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (ASC == nullptr)
    {
        return;
    }

    if (ASC->TryActivateAbilityByClass(UHDGA_Stim::StaticClass()))
    {
        return;
    }

    const EHDStimRefusal Refusal = HDStim::CheckUse(
        ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetStimAttribute()),
        ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()),
        ASC->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute()),
        ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead),
        ASC->HasMatchingGameplayTag(Tag_Character_State_Stimmed));

    // 규칙으로는 쓸 수 있는데 켜지지 않았으면 다른 태그(Block.All 등)에 막힌 것이다
    UE_LOG(LogTemp, Display, TEXT("[HDStim] refused (%s)"), Refusal == EHDStimRefusal::None ? TEXT("blocked") : HDStim::GetRefusalName(Refusal));
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
    const float OldGrenade  = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetGrenadeAttribute());
    const float OldStim     = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetStimAttribute());
    const float MaxAmmo     = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxAmmoAttribute());
    const float MaxCapacity = ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxCapacityAttribute());

    // EquipWeapon 과 같은 GE(BP 값 BPGE_EquipWeapon, SetByCaller 넷을 Override)에 지금 최대값을 넣는다. 수류탄은 아래에서 MaxGrenade 로 채운다
    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(WeaponAttributeSetEffect, 1.f, ASC->MakeEffectContext());
    CONDITION_CHECK_WITH_RETURNTYPE(SpecHandle.IsValid(), false);

    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_Ammo, MaxAmmo);
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_MaxAmmo, MaxAmmo);
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_Capacity, MaxCapacity);
    SpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Attribute_MaxCapacity, MaxCapacity);

    ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetGrenadeAttribute(), ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxGrenadeAttribute()));
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetStimAttribute(), ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxStimAttribute()));

    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] resupply Ammo %.0f->%.0f Capacity %.0f->%.0f Grenade %.0f->%.0f Stim %.0f->%.0f"),
        OldAmmo, ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetAmmoAttribute()),
        OldCapacity, ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetCapacityAttribute()),
        OldGrenade, ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetGrenadeAttribute()),
        OldStim, ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetStimAttribute()));
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

    // 권총 몽타주(SK_AM_Pistol)에는 Reload_Shoulder·Reload_Prone 이 없다. 없는 재장전 섹션은 Reload_Hip 으로 바꾼다
    if (MontageType == EHDCombatMontage::Reload && WeaponMontage->IsValidSectionName(SectionName) == false)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDWeapon] reload section %s missing, using %s"), *SectionName.ToString(), *HDMONTAGE_SECTIONNAME_WEAPON_RELOAD_HIP.ToString());
        SectionName = HDMONTAGE_SECTIONNAME_WEAPON_RELOAD_HIP;
    }

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

    // 보조무기(권총)는 집어넣은 채로 지급한다. 주무기가 쓰는 칸으로 남는다
    AHDSecondaryWeapon* SecondaryWeapon = GetWorld()->SpawnActor<AHDSecondaryWeapon>(AHDSecondaryWeapon::StaticClass());
    NULL_CHECK(SecondaryWeapon);
    Combat->StowWeapon(SecondaryWeapon);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);
    ASC->GenericGameplayEventCallbacks.Remove(Tag_Event_PlayerHUD_Initialize);

    // 강하·증원마다 수류탄을 채운다. PreAttributeChange 가 Grenade 를 MaxGrenade 로 자르므로 Max 를 먼저 정한다
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetMaxGrenadeAttribute(), DefaultGrenadeCount);
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetGrenadeAttribute(), DefaultGrenadeCount);
    // 스팀도 같은 순서로 채운다
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetMaxStimAttribute(), DefaultStimCount);
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetStimAttribute(), DefaultStimCount);
    ASC->SetLooseGameplayTagCount(Tag_Character_State_GrenadeReady, 0);
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

    // PIE 종료·맵 이동 때 플레이어스테이트 ASC 가 사라지면서 켜져 있던 엎드리기·앉기·달리기 GA 를 취소하고,
    // 그 EndAbility 가 ASC 가 이미 빈 캐릭터의 이 함수를 부른다
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK_WITHOUT_LOG(ASC);

    float Speed = ASC->GetNumericAttribute(Attribute);
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
        UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
        NULL_CHECK(ASC);

        // 수류탄 투척(UHDGA_ThrowGrenade)도 같은 AM_Throw 라 이 이벤트가 온다. 스트라타젬을 들고 있을 때만 처리한다
        if (ASC->HasMatchingGameplayTag(Tag_Character_Action_HoldStratagem) == false)
        {
            return;
        }

        Stratagem->ThrowFinished();

        // 손을 떠났으므로 든 상태를 푼다. 사격은 투척 GA 가 끝날 때까지 그 GA 의 Block.Fire 로 막혀 있다
        ASC->SetLooseGameplayTagCount(Tag_Character_Action_HoldStratagem, 0);
    }
    else if (EventTag == Tag_Character_Action_ThrowEnd)
    {
        // 수류탄을 든 채면 총을 보이지 않는다
        const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
        if (ASC && ASC->HasMatchingGameplayTag(Tag_Character_State_GrenadeReady))
        {
            return;
        }

        SetWeaponActive(true);
    }
}

void AHDCharacterPlayer::TryHoldStratagem()
{
    const bool CanHoldStrategem = Stratagem->IsSelectedStratagemExist();
    if (CanHoldStrategem)
    {
        // 수류탄을 든 채면 먼저 총으로 돌아간다. 아래에서 총을 숨기고, 던진 뒤 ThrowEnd 가 다시 보인다
        const UAbilitySystemComponent* GrenadeCheckASC = GetAbilitySystemComponent();
        if (GrenadeCheckASC && GrenadeCheckASC->HasMatchingGameplayTag(Tag_Character_State_GrenadeReady))
        {
            LeaveGrenadeSlot(TEXT("stratagem"));
        }

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
        // 수류탄을 든 상태도 푼다(같은 이유로 증원 뒤에도 남는다)
        ASC->SetLooseGameplayTagCount(Tag_Character_State_GrenadeReady, 0);

        // 좌클릭 직후 죽으면 AM_Throw 가 시체에서 계속 돌아 0.427초에 수류탄이 나간다. 투척 GA 를 취소해 몽타주를 멈춘다
        const FGameplayAbilitySpec* ThrowSpec = ASC->FindAbilitySpecFromClass(UHDGA_ThrowGrenade::StaticClass());
        if (ThrowSpec && ThrowSpec->IsActive())
        {
            UE_LOG(LogTemp, Display, TEXT("[HDGrenade] throw cancelled (dead)"));
            ASC->CancelAbilityHandle(ThrowSpec->Handle);
        }

        // 회복 중에 죽으면 남은 회복이 시체의 체력을 다시 올린다. 스팀 GA 를 취소한다
        const FGameplayAbilitySpec* StimSpec = ASC->FindAbilitySpecFromClass(UHDGA_Stim::StaticClass());
        if (StimSpec && StimSpec->IsActive())
        {
            UE_LOG(LogTemp, Display, TEXT("[HDStim] cancelled (dead)"));
            ASC->CancelAbilityHandle(StimSpec->Handle);
        }
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
