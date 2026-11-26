
#include "HDCombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Collision/HDCollision.h"
#include "Define/HDDefine.h"
#include "Define/HDSocketNames.h"
#include "Weapon/HDWeapon.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "Component/HDFaceAimRules.h"

#define AIMOFFSET_PITCH_OFFSET 20.f

namespace HDCombatWeaponSlot
{
    // 칸에 넣어 둔 무기는 손에 붙인 채 숨긴다(숨김·틱·충돌)
    void SetWeaponShown(AHDWeapon* InWeapon, const bool bShown)
    {
        InWeapon->SetActorTickEnabled(bShown);
        InWeapon->SetActorHiddenInGame(bShown == false);
        InWeapon->SetActorEnableCollision(bShown);
    }
}

UHDCombatComponent::UHDCombatComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHDCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    CharacterMovement = GetOwner()->GetComponentByClass<UCharacterMovementComponent>();
    check(CharacterMovement);

    SpringArm = GetOwner()->GetComponentByClass<USpringArmComponent>();
    check(SpringArm);

    FOnTimelineFloat ArmLengthTimelineProgress;
    ArmLengthTimelineProgress.BindUFunction(this, FName("OnSpringArmLengthUpdate"));
    SpringArmArmLengthTimeline.AddInterpFloat(DefaultCurve, ArmLengthTimelineProgress);
    SpringArmArmLengthTimeline.SetLooping(false);
}

void UHDCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TraceUnderCrosshairs();
    AimOffset(DeltaTime);
    SpringArmArmLengthTimeline.TickTimeline(DeltaTime);
}

const float UHDCombatComponent::GetAimOffset_Yaw() const
{
    return AimOffset_Yaw;
}

const float UHDCombatComponent::GetAimOffset_Pitch() const
{
    return AimOffset_Pitch;
}

const FVector& UHDCombatComponent::GetHitTarget() const
{
    return HitTarget;
}

const float UHDCombatComponent::GetCurrentFOV() const
{
    return CurrentFOV;
}

void UHDCombatComponent::SetCurrentFOV(const float NewFOV)
{
    CurrentFOV = NewFOV;
}

const float UHDCombatComponent::GetZoomInterpSpeed() const
{
    return ZoomInterpSpeed;
}

const bool UHDCombatComponent::IsCharacterLookingViewport() const
{
    return bIsCharacterLookingViewport;
}

void UHDCombatComponent::SetSpringArmTargetLength(const float TargetArmLength)
{
    SpringArmTargetArmLength = TargetArmLength;
}

const EHDTurningInPlace UHDCombatComponent::GetTurnInPlace() const
{
    return TurningInPlace;
}

const float UHDCombatComponent::GetDefaultFOV() const
{
    return DefaultFOV;
}

const bool UHDCombatComponent::IsUseRotateBone() const
{
    return bUseRotateRootBone;
}

UAnimMontage* UHDCombatComponent::GetCombatMontage(const EHDCombatMontage MontageType)
{
    CONDITION_CHECK_WITH_RETURNTYPE(CombatMontage.Num() == static_cast<int32>(EHDCombatMontage::Count), nullptr);

    return CombatMontage[MontageType];
}

void UHDCombatComponent::TraceUnderCrosshairs()
{
    CONDITION_CHECK(GEngine && GEngine->GameViewport);

    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);

    const FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
    FVector CrosshairWorldPosition;
    FVector CrosshairWorldDirection;
    const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
        UGameplayStatics::GetPlayerController(this, 0),
        CrosshairLocation,
        CrosshairWorldPosition,
        CrosshairWorldDirection
    );

    if (bScreenToWorld)
    {
        // 캐릭터를 시선 위에 내린 지점에서 시작한다. 카메라와 캐릭터 사이(등 뒤)는 보지 않고, 캐릭터 바로 앞 적은 건너뛰지 않는다
        const float DistanceToCharacter = FVector::DotProduct(GetOwner()->GetActorLocation() - CrosshairWorldPosition, CrosshairWorldDirection);
        const FVector Start = CrosshairWorldPosition + CrosshairWorldDirection * FMath::Max(0.f, DistanceToCharacter);

        const FVector End = Start + CrosshairWorldDirection * HITSCAN_TRACE_LENGTH;
        UWorld* World = GetWorld();
        VALID_CHECK(World);

        // 탄과 같은 충돌 프로필로 찾아야 조준점이 곧 탄착점이 된다. Visibility 는 캐릭터(캡슐·메시)가 무시해서 적을 지나 뒤 땅에 맞았고,
        // 총구에서 그 먼 점으로 쏜 탄이 어깨 카메라와의 시차만큼 적을 비껴갔다. 탄처럼 쏜 캐릭터와 들고 있는 무기는 무시한다
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDAimTrace), false, GetOwner());
        if (Weapon)
        {
            QueryParams.AddIgnoredActor(Weapon.Get());
        }

        FHitResult TraceHitResult;
        const bool bHit = World->LineTraceSingleByProfile(TraceHitResult, Start, End, HDCOLLISION_PROFILE_PROJECTILE, QueryParams);

        HitTarget = bHit ? TraceHitResult.ImpactPoint : End;

        NULL_CHECK(Weapon);
        Weapon->TraceEndWithScatter(HitTarget);
    }
}

void UHDCombatComponent::AimOffset(const float DeltaTime)
{
    NULL_CHECK(CharacterMovement);
    VALID_CHECK_WITHOUT_LOG(Weapon);

	APawn* Owner = GetOwner<APawn>();
    NULL_CHECK(Owner);

    if (bIsShoulder == false && IsFacingAim())
    {
        FaceAim(Owner, DeltaTime);
        return;
    }

    FVector Velocity = Owner->GetVelocity();
    Velocity.Z = 0.f;
    const bool bIsMoving = Velocity.Size() > 0.1f;
    const bool bIsFalling = CharacterMovement->IsFalling();
    const FRotator BaseAimRoatation = Owner->GetBaseAimRotation();
    const FRotator ControlRotation = Owner->GetControlRotation();

    if (bIsShoulder)
    {
        bIsCharacterLookingViewport = true;
        bUseRotateRootBone = false;
        Owner->bUseControllerRotationYaw = true;
        CharacterMovement->bOrientRotationToMovement = false;

        const FRotator TargetRotation(0.f, ControlRotation.Yaw, 0.f);
        const FRotator SmoothRotation = FMath::RInterpTo(Owner->GetActorRotation(), TargetRotation, DeltaTime, 20.f);
        Owner->SetActorRotation(SmoothRotation);

        if (bIsMoving == false && bIsFalling == false)
        {
            const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, StartingAimRotation);
            AimOffset_Yaw = DeltaRotation.Yaw;

            if (TurningInPlace == EHDTurningInPlace::NotTurning)
            {
                InterpAimOffset_Yaw = AimOffset_Yaw;
            }

            TurnInPlace(DeltaTime);
        }

        if (bIsMoving || bIsFalling)
        {
            AimOffset_Yaw = 0.f;
            InterpAimOffset_Yaw = 0.f;
            StartingAimRotation = TargetRotation;
        }
    }
    else
    {
        bIsCharacterLookingViewport = true;
        bUseRotateRootBone = true;
        Owner->bUseControllerRotationYaw = false;
        CharacterMovement->bOrientRotationToMovement = false;
        const FRotator TargetRotation(0.f, BaseAimRoatation.Yaw, 0.f);
        const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, StartingAimRotation);
        AimOffset_Yaw = DeltaRotation.Yaw;

        if (bIsMoving == false && bIsFalling == false)
        {
            if (TurningInPlace == EHDTurningInPlace::NotTurning)
            {
                InterpAimOffset_Yaw = AimOffset_Yaw;
            }

            bUseRotateRootBone = false;
            Owner->bUseControllerRotationYaw = true;
            TurnInPlace(DeltaTime);
        }

        if (bIsMoving || bIsFalling)
        {
            bIsCharacterLookingViewport = false;
            bUseRotateRootBone = false;
            AimOffset_Yaw = 0.f;
            StartingAimRotation = BaseAimRoatation;
            TurningInPlace = EHDTurningInPlace::NotTurning;
            CharacterMovement->bOrientRotationToMovement = true;
        }
    }

    AimOffset_Pitch = BaseAimRoatation.Pitch - AIMOFFSET_PITCH_OFFSET;
}

void UHDCombatComponent::BeginFaceAim()
{
    ++FaceAimRequestCount;
}

void UHDCombatComponent::EndFaceAim()
{
    FaceAimRequestCount = FMath::Max(FaceAimRequestCount - 1, 0);

    const UWorld* World = GetWorld();
    if (World)
    {
        FaceAimHoldUntil = World->GetTimeSeconds() + FaceAimLingerTime;
    }
}

float UHDCombatComponent::GetYawToAim() const
{
    const APawn* Owner = GetOwner<APawn>();
    if (Owner == nullptr)
    {
        return 0.f;
    }

    return HDFaceAim::YawToAim(Owner->GetActorRotation().Yaw, Owner->GetControlRotation().Yaw);
}

bool UHDCombatComponent::IsFacingAim() const
{
    if (FaceAimRequestCount > 0)
    {
        return true;
    }

    const UWorld* World = GetWorld();
    return World && World->GetTimeSeconds() < FaceAimHoldUntil;
}

void UHDCombatComponent::FaceAim(APawn* Owner, const float DeltaTime)
{
    // 견착처럼 몸을 조준 방향에 두되, 한 번에 돌지 않고 FaceAimTurnRate 로 돈다(뒤돌기 모션). 이동은 조준 방향 기준 스트레이프가 된다
    bIsCharacterLookingViewport = true;
    bUseRotateRootBone = false;
    Owner->bUseControllerRotationYaw = false;
    CharacterMovement->bOrientRotationToMovement = false;

    const FRotator TargetRotation(0.f, Owner->GetControlRotation().Yaw, 0.f);
    Owner->SetActorRotation(FMath::RInterpConstantTo(Owner->GetActorRotation(), TargetRotation, DeltaTime, FaceAimTurnRate));

    // 끝난 뒤 제자리 회전이 튀지 않게 기준을 조준 방향에 맞춰 둔다
    AimOffset_Yaw = 0.f;
    InterpAimOffset_Yaw = 0.f;
    TurningInPlace = EHDTurningInPlace::NotTurning;
    StartingAimRotation = TargetRotation;

    AimOffset_Pitch = Owner->GetBaseAimRotation().Pitch - AIMOFFSET_PITCH_OFFSET;
}

void UHDCombatComponent::TurnInPlace(const float DeltaTime)
{
    if (AimOffset_Yaw > TurnThreshold)
    {
        TurningInPlace = EHDTurningInPlace::Turn_Right;
    }
    else if (AimOffset_Yaw < -TurnThreshold)
    {
        TurningInPlace = EHDTurningInPlace::Turn_Left;
    }

    if (TurningInPlace != EHDTurningInPlace::NotTurning)
    {
        InterpAimOffset_Yaw = FMath::FInterpTo(InterpAimOffset_Yaw, 0.f, DeltaTime, 4.f);
        AimOffset_Yaw = InterpAimOffset_Yaw;
        if (FMath::Abs(AimOffset_Yaw) < 15.f)
        {
            StartingAimRotation = FRotator(0.f, GetOwner<APawn>()->GetBaseAimRotation().Yaw, 0.f);
            TurningInPlace = EHDTurningInPlace::NotTurning;
        }
    }
}

void UHDCombatComponent::OnSpringArmLengthUpdate(const float Value)
{
    const float Interpolated = FMath::Lerp(SpringArmTargetArmLength, SpringArmTargetArmLength / 2.f, Value);
    SpringArm->TargetArmLength = Interpolated;
}

void UHDCombatComponent::EquipWeapon(AHDWeapon* NewWeapon, UAbilitySystemComponent* ASC)
{
    VALID_CHECK(NewWeapon);

    using namespace HDCombatWeaponSlot;

    // 쓰던 무기: 지금 탄약 Attribute 를 무기에 적어 두고 손에 붙인 채 숨긴다
    if (IsValid(Weapon) && Weapon != NewWeapon)
    {
        Weapon->AmmoData = HDWeaponAmmo::ReadFromAttributes(ASC);
        SetWeaponShown(Weapon, false);
    }

    CONDITION_CHECK(PutWeaponInSlot(NewWeapon, true));

    Weapon = NewWeapon;
    ErgonomicFactor = Weapon->ErgoData.ErgonomicFactor;
    ZoomedFOV = Weapon->ErgoData.ZoomedFOV;
    ZoomInterpSpeed = Weapon->ErgoData.ZoomInterpSpeed;
    bIsEquipped = true;
    SetWeaponShown(NewWeapon, true);
}

void UHDCombatComponent::StowWeapon(AHDWeapon* NewWeapon)
{
    VALID_CHECK(NewWeapon);

    CONDITION_CHECK(PutWeaponInSlot(NewWeapon, false));
    HDCombatWeaponSlot::SetWeaponShown(NewWeapon, false);
}

bool UHDCombatComponent::PutWeaponInSlot(AHDWeapon* NewWeapon, const bool bMakeActive)
{
    const EHDWeaponSlot Slot = NewWeapon->GetWeaponSlot();
    TObjectPtr<AHDWeapon>* SlotWeapon = GetSlotWeapon(Slot);
    NULL_CHECK_WITH_RETURNTYPE(SlotWeapon, false);

    // 같은 칸에 있던 무기는 앞에 버린다
    const FHDWeaponPickupResult PickupResult = WeaponSlots.Pickup(Slot, bMakeActive);
    if (PickupResult.bReplaced && IsValid(*SlotWeapon) && *SlotWeapon != NewWeapon)
    {
        (*SlotWeapon)->DropAt(GetWeaponDropLocation());
    }

    *SlotWeapon = NewWeapon;
    NewWeapon->EquipWeapon(GetOwner());

    USkeletalMeshComponent* SkeletalMesh = GetOwner()->GetComponentByClass<USkeletalMeshComponent>();
    NULL_CHECK_WITH_RETURNTYPE(SkeletalMesh, false);

    const USkeletalMeshSocket* RightHandSocket = SkeletalMesh->GetSocketByName(HDSOCKETNAME_RIGRHTHAND);
    NULL_CHECK_WITH_RETURNTYPE(RightHandSocket, false);
    RightHandSocket->AttachActor(NewWeapon, SkeletalMesh);
    return true;
}

EHDWeaponSelectResult UHDCombatComponent::SelectWeaponSlot(const EHDWeaponSlot Slot, UAbilitySystemComponent* ASC)
{
    // 무기 액터가 없을 때 되돌리려고 칸 상태 전체를 복사해 둔다(쓰는 칸과 LastGunSlot 을 함께 되돌린다)
    const FHDWeaponSlots PreviousSlots = WeaponSlots;

    const EHDWeaponSelectResult SelectResult = WeaponSlots.Select(Slot);
    if (SelectResult != EHDWeaponSelectResult::Selected)
    {
        return SelectResult;
    }

    // 수류탄 칸에는 무기 액터가 없다. 쓰던 총은 탄약을 적어 두고 숨기되 Weapon 은 그대로 둔다(여러 getter 가 Weapon 을 본다)
    if (Slot == EHDWeaponSlot::Grenade)
    {
        if (IsValid(Weapon))
        {
            Weapon->AmmoData = HDWeaponAmmo::ReadFromAttributes(ASC);
            HDCombatWeaponSlot::SetWeaponShown(Weapon, false);
        }

        bIsEquipped = false;
        return EHDWeaponSelectResult::Selected;
    }

    TObjectPtr<AHDWeapon>* SlotWeapon = GetSlotWeapon(Slot);
    if (SlotWeapon == nullptr || IsValid(*SlotWeapon) == false)
    {
        // 칸은 찼다고 하는데 무기 액터가 없다. 칸 상태를 되돌린다
        WeaponSlots = PreviousSlots;
        return EHDWeaponSelectResult::Empty;
    }

    ActivateSlotWeapon(ASC);
    return EHDWeaponSelectResult::Selected;
}

void UHDCombatComponent::SetGrenadeAvailable(const bool bAvailable)
{
    WeaponSlots.SetGrenadeAvailable(bAvailable);
}

bool UHDCombatComponent::IsGrenadeSlotActive() const
{
    return WeaponSlots.GetActiveSlot() == EHDWeaponSlot::Grenade;
}

void UHDCombatComponent::ActivateSlotWeapon(UAbilitySystemComponent* ASC)
{
    using namespace HDCombatWeaponSlot;

    TObjectPtr<AHDWeapon>* SlotWeapon = GetSlotWeapon(WeaponSlots.GetActiveSlot());
    NULL_CHECK(SlotWeapon);
    VALID_CHECK(*SlotWeapon);

    // 수류탄에서 같은 총으로 돌아와도 탄약은 다시 적는다. 수류탄을 든 동안 Attribute 는 마지막 총의 값이고, 그 사이 받은 보급이 여기 남아 있다
    if (IsValid(Weapon))
    {
        Weapon->AmmoData = HDWeaponAmmo::ReadFromAttributes(ASC);

        if (Weapon != *SlotWeapon)
        {
            SetWeaponShown(Weapon, false);
        }
    }

    Weapon = *SlotWeapon;
    SetWeaponShown(Weapon, true);
    ErgonomicFactor = Weapon->ErgoData.ErgonomicFactor;
    ZoomedFOV = Weapon->ErgoData.ZoomedFOV;
    ZoomInterpSpeed = Weapon->ErgoData.ZoomInterpSpeed;
    bIsEquipped = true;
}

TObjectPtr<AHDWeapon>* UHDCombatComponent::GetSlotWeapon(const EHDWeaponSlot Slot)
{
    switch (Slot)
    {
    case EHDWeaponSlot::Primary:    return &PrimaryWeapon;
    case EHDWeaponSlot::Secondary:  return &SecondaryWeapon;
    case EHDWeaponSlot::Support:    return &SupportWeapon;
    default:                        return nullptr;
    }
}

EHDWeaponSlot UHDCombatComponent::GetActiveWeaponSlot() const
{
    return WeaponSlots.GetActiveSlot();
}

EHDWeaponSlot UHDCombatComponent::GetLastGunSlot() const
{
    return WeaponSlots.GetLastGunSlot();
}

void UHDCombatComponent::GetAllWeapons(TArray<AHDWeapon*>& OutWeapons) const
{
    OutWeapons.Reset();

    if (IsValid(PrimaryWeapon))
    {
        OutWeapons.Add(PrimaryWeapon);
    }

    if (IsValid(SecondaryWeapon))
    {
        OutWeapons.Add(SecondaryWeapon);
    }

    if (IsValid(SupportWeapon))
    {
        OutWeapons.Add(SupportWeapon);
    }
}

void UHDCombatComponent::RefillStowedWeaponAmmo()
{
    TArray<AHDWeapon*> Weapons;
    GetAllWeapons(Weapons);

    // 넣어 둔 총은 꺼낼 때 AmmoData 를 Attribute 로 불러온다(ActivateSlotWeapon). 여기 채워 두지 않으면 보급 전 탄약 그대로 나온다
    for (AHDWeapon* SlotWeapon : Weapons)
    {
        if (SlotWeapon != Weapon)
        {
            SlotWeapon->AmmoData.Ammo       = SlotWeapon->AmmoData.MaxAmmo;
            SlotWeapon->AmmoData.Capacity   = SlotWeapon->AmmoData.MaxCapacity;
        }
    }
}

FVector UHDCombatComponent::GetWeaponDropLocation() const
{
    const AActor* Owner = GetOwner();
    const FVector DropColumn = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 150.f;

    UWorld* World = GetWorld();
    NULL_CHECK_WITH_RETURNTYPE(World, DropColumn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDWeaponDrop), false, Owner);
    FHitResult GroundHit;
    const FVector TraceStart = DropColumn + FVector(0.f, 0.f, 200.f);
    const FVector TraceEnd = DropColumn - FVector(0.f, 0.f, 2000.f);
    if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, QueryParams))
    {
        return GroundHit.ImpactPoint;
    }

    return DropColumn;
}

void UHDCombatComponent::SpawnProjectile()
{
    VALID_CHECK(Weapon);

    // 쏘는 순간 다시 잰다. 몽타주 노티파이는 이 컴포넌트 Tick 보다 먼저 와서, 그대로 두면 한 프레임 전 카메라로 잰 조준점을 쓴다
    TraceUnderCrosshairs();
    Weapon->SpawnProjectile(HitTarget);

    // 총성: 최대 거리 0 = 듣는 쪽 HearingRange 로 거른다
    UAISense_Hearing::ReportNoiseEvent(this, GetOwner()->GetActorLocation(), 1.f, GetOwner(), 0.f, NAME_None);
}

USkeletalMeshComponent* UHDCombatComponent::GetWeaponMesh() const
{
    VALID_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(Weapon, nullptr);

    return Weapon->GetComponentByClass<USkeletalMeshComponent>();;
}

void UHDCombatComponent::SetWeaponActive(const bool bActive)
{
    VALID_CHECK(Weapon);
    Weapon->SetActorTickEnabled(bActive);
    Weapon->SetActorHiddenInGame(bActive == false);
    Weapon->SetActorEnableCollision(bActive);
    bIsEquipped = bActive;
}

UAnimMontage* UHDCombatComponent::GetWeaponMontage() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, nullptr);

    return Weapon->WeaponAnimMontage;
}

const EHDFireType UHDCombatComponent::GetWeaponFireType() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, EHDFireType::Count);

    return Weapon->FireType;
}

const float UHDCombatComponent::GetWeaponFireDelay() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, 0.f);

    return Weapon->FireDelay;
}

const int32 UHDCombatComponent::GetWeaponAmmoCount() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, 0);

    return Weapon->AmmoData.Ammo;
}

const int32 UHDCombatComponent::GetWeaponMaxAmmoCount() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, 0);

    return Weapon->AmmoData.MaxAmmo;
}

const int32 UHDCombatComponent::GetWeaponCapacityCount() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, 0);

    return Weapon->AmmoData.Capacity;
}

const int32 UHDCombatComponent::GetWeaponMaxCapacityCount() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, 0);

    return Weapon->AmmoData.MaxCapacity;
}

const float UHDCombatComponent::GetWeaponZoomedFOV() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, 0.f);

    return Weapon->ErgoData.ZoomedFOV;
}

const float UHDCombatComponent::GetWeaponZoomInterpSpeed() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, 0.f);

    return Weapon->ErgoData.ZoomInterpSpeed;
}

const bool UHDCombatComponent::IsWeaponAutoFire() const
{
    VALID_CHECK_WITH_RETURNTYPE(Weapon, false);

    return Weapon->bIsAutoFire;
}

const bool UHDCombatComponent::IsEquippedWeapon() const
{
    return bIsEquipped;
}

AHDWeapon* UHDCombatComponent::SpawnDefaultWeapon()
{
    NULL_CHECK_WITH_RETURNTYPE(DefaultWeaponClass, nullptr);
    return GetWorld()->SpawnActor<AHDWeapon>(DefaultWeaponClass);
}

const bool UHDCombatComponent::IsShoulder() const
{
    return bIsShoulder;
}

void UHDCombatComponent::SetShoulder(const bool bShoudler)
{
    bIsShoulder = bShoudler;

	if (bIsShoulder)
	{
		SpringArmArmLengthTimeline.PlayFromStart();
	}
	else
	{
		SpringArmArmLengthTimeline.ReverseFromEnd();
	}
}

AHDWeapon* UHDCombatComponent::GetWeapon() const
{
    return Weapon;
}
