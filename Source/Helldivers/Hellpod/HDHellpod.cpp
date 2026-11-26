// Fill out your copyright notice in the Description page of Project Settings.

#include "HDHellpod.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeSourceComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/SpringArmComponent.h"
#include "Define/HDDefine.h"
#include "Player/HDGASPlayerState.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Collision/HDCollision.h"
#include "Controller/HDPlayerController.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Hellpod/HDHellpodLandingRules.h"
#include "Game/HDGameState.h"

namespace
{
    // 메시 기준 꼭대기 해치 높이. hellpod 메시는 피벗이 바닥이고 반경 40cm 안쪽 윗면이 z = 187.2 로 평평하다. hellpod_cap 의 바닥은 -0.58
    const FVector HellpodCapRelativeLocation(0.f, 0.f, 187.8f);

    // 흔들림 축마다 다른 노이즈 구간을 쓰려고 더하는 값
    constexpr float ShakeSeedYaw    = 31.7f;
    constexpr float ShakeSeedRoll   = 57.3f;
    constexpr float ShakeSeedZ      = 13.1f;
}

AHDHellpod::AHDHellpod(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(CollisionBox);
    CollisionBox->SetNotifyRigidBodyCollision(true);
    CollisionBox->SetEnableGravity(true);
    CollisionBox->SetCollisionProfileName(HDCOLLISION_PROFILE_HELLPOD);
    CollisionBox->BodyInstance.bUseCCD = true;
    CollisionBox->OnComponentHit.AddDynamic(this, &AHDHellpod::OnBoxHit);

    HellpodMesh = CreateDefaultSubobject<UStaticMeshComponent>("HellpodMesh");
    HellpodMesh->SetupAttachment(RootComponent);
    HellpodMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    HellpodCap = CreateDefaultSubobject<UStaticMeshComponent>("HellpodCap");
    HellpodCap->SetupAttachment(HellpodMesh);
    HellpodCap->SetRelativeLocation(HellpodCapRelativeLocation);
    HellpodCap->SetCollisionProfileName(HDCOLLISION_PROFILE_NOCOLLISION);
    HellpodCap->SetGenerateOverlapEvents(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CapMeshRef(TEXT("/Script/Engine.StaticMesh'/Game/Asset/MyResources/hellpod_cap.hellpod_cap'"));
    if (CapMeshRef.Succeeded())
    {
        HellpodCap->SetStaticMesh(CapMeshRef.Object);
    }

    // Camera
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    CameraShakeSource = CreateDefaultSubobject<UCameraShakeSourceComponent>(TEXT("CameraShakeSource"));
    CameraShakeSource->SetupAttachment(RootComponent);
    CameraShakeSource->bAutoStart = false;
}

void AHDHellpod::BeginPlay()
{
	Super::BeginPlay();

	bIsLanded = false;

	SetSpawnTimeline();
	ShakeCamera(0);

	MeshDefaultRelativeRotation = HellpodMesh->GetRelativeRotation();
	CollisionBox->AddImpulse(FVector::DownVector * FallSpeed, NAME_None, true);
}

void AHDHellpod::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SpawnCharacterTimeline.TickTimeline(DeltaTime);

	if (bIsLanded == false)
	{
		RotateHellpodByCurrentImpulse(DeltaTime);
	}

	if (bLandingActive)
	{
		TickLanding(DeltaTime);
	}
}

void AHDHellpod::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	NULL_CHECK(PlayerController);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	NULL_CHECK(Subsystem);

	Subsystem->ClearAllMappings();
	NULL_CHECK(InputMappingContext);

	Subsystem->AddMappingContext(InputMappingContext, 0);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	NULL_CHECK(EnhancedInputComponent);
	NULL_CHECK(MoveAction);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHDHellpod::MoveHellpod);
}

void AHDHellpod::OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    CONDITION_CHECK_WITHOUT_LOG(bIsLanded == false);

	bIsLanded = true;

	CONDITION_CHECK(ImpactTag.IsValid());
	NULL_CHECK(OtherActor);

	CameraShakeSource->StopAllCameraShakes();
	HellpodMesh->SetRelativeRotation(MeshDefaultRelativeRotation);

	CollisionBox->SetSimulatePhysics(false);
	CollisionBox->SetEnableGravity(false);

    AHDGASPlayerState* GASPlayerState = GetPlayerState<AHDGASPlayerState>();
    NULL_CHECK(GASPlayerState);

	// 빙의가 바뀌면 붐이 컨트롤 회전을 잃으므로 그 전에 강하 시점을 고정한다
	FreezeLandingCamera();
	SpawnCharacter();
	ComputeRevealCamera();

    UAbilitySystemComponent* PlayerStateASC = GASPlayerState->GetAbilitySystemComponent();
    NULL_CHECK(PlayerStateASC);

    FGameplayAbilityHelper::ExcuteGameplayCue(ImpactTag, Hit.ImpactPoint, Hit.Normal, PlayerStateASC);
}

void AHDHellpod::SpawnCharacter()
{
	NULL_CHECK(CharacterClass);

	UWorld* World = GetWorld();

	APlayerController* PlayerController = GetController<AHDPlayerController>();
	NULL_CHECK(PlayerController);

	// 강하 때 보던 방향을 바라보고 나오게 해서 블렌드가 끝날 때 카메라가 캐릭터 등 뒤에 온다
	const float FacingYaw = LandingCameraStart.Rotator().Yaw;
	const FTransform SpawnTransform(FRotator(0.f, FacingYaw, 0.f), GetActorLocation());

	SpawnedCharacter = World->SpawnActorDeferred<AHDCharacterPlayer>(
		CharacterClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	NULL_CHECK(SpawnedCharacter);

	PlayerController->Possess(Cast<APawn>(SpawnedCharacter));
	UGameplayStatics::FinishSpawningActor(SpawnedCharacter, SpawnTransform);

	PlayerController = SpawnedCharacter->GetController<APlayerController>();
	NULL_CHECK(PlayerController);
	SpawnedCharacter->DisableInput(PlayerController);

	// Possess 가 시점을 캐릭터로 바로 넘기므로 같은 프레임에 헬포드로 되돌린다. 캐릭터로는 ViewBlendDelay 뒤 블렌드로 넘어간다
	PlayerController->SetControlRotation(FRotator(ArrivalPitch, FacingYaw, 0.f));
	PlayerController->SetViewTarget(this);
	LandingController = PlayerController;

	// 상승 구간: 머리가 해치 아래에 있는 높이 → 캡슐 바닥이 지면(루트)에 닿는 높이. 상승 전에는 떨어지지 않게 이동을 멈춘다
	FVector CharacterOrigin, CharacterExtent;
	SpawnedCharacter->GetActorBounds(true, CharacterOrigin, CharacterExtent);

	const float RootZ = GetActorLocation().Z;
	RiseStartZ	= RootZ - CharacterExtent.Z;
	RiseEndZ	= RootZ + CharacterExtent.Z;

	SpawnedCharacter->GetCharacterMovement()->DisableMovement();
	OnSpawnTimelineUpdate(0.f);

	LandingElapsed		= 0.f;
	bCapEjected			= false;
	bViewBlendStarted	= false;
	bLandingActive		= true;
	ControlReturnAt		= HDHellpodLanding::ControlReturnTime(ViewBlendDelay, ViewBlendTime, CapEjectDelay, SpawnCharacterTimeline.GetTimelineLength());
}

void AHDHellpod::OnSpawnTimelineUpdate(const float Value)
{
	NULL_CHECK(SpawnedCharacter);

	const FVector& CharacterLocation	= SpawnedCharacter->GetActorLocation();
	const float Interpolated			= FMath::Lerp(RiseStartZ, RiseEndZ, Value);
	SpawnedCharacter->SetActorLocation(FVector(CharacterLocation.X, CharacterLocation.Y, Interpolated));
}

void AHDHellpod::SpawnCharacterEnd()
{
	NULL_CHECK(SpawnedCharacter);
	SpawnedCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	// 캐릭터 기본 캡슐 프로필(AHDCharacterBase). Pawn 프로필은 투사체·스트라타젬을 막아 탄이 메시(부위) 대신 캡슐에 맞고 던진 비컨이 자기 캡슐에 부딪힌다
	SpawnedCharacter->GetCapsuleComponent()->SetCollisionProfileName(HDCOLLISION_PROFILE_CAPSULE);
	SpawnedCharacter->GetMesh()->SetCollisionProfileName(HDCOLLISION_PROFILE_PLAYER);

	SetLifeSpan(30.f);
}

void AHDHellpod::SetSpawnTimeline()
{
	FOnTimelineFloat SpawnCharacterTimelineProgress;
	SpawnCharacterTimelineProgress.BindUFunction(this, FName("OnSpawnTimelineUpdate"));
	SpawnCharacterTimeline.AddInterpFloat(SpawnCurveFloat, SpawnCharacterTimelineProgress);

	FOnTimelineEventStatic SpawnCharacterTimelineEventStatic;
	SpawnCharacterTimelineEventStatic.BindUObject(this, &AHDHellpod::SpawnCharacterEnd);
	SpawnCharacterTimeline.SetTimelineFinishedFunc(SpawnCharacterTimelineEventStatic);
	SpawnCharacterTimeline.SetLooping(false);
}

void AHDHellpod::FreezeLandingCamera()
{
	// 지금 보이는 시점 그대로 떼어 내고, 이후로는 UpdateLandingCamera 가 월드 기준으로 직접 움직인다
	LandingCameraStart = FollowCamera->GetComponentTransform();
	CameraBoom->bUsePawnControlRotation = false;
	FollowCamera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

void AHDHellpod::ComputeRevealCamera()
{
	// 포드 뒤 조금 위, 오른쪽으로 비킨 숄더 뷰. 지형에 막히면 앞으로 당긴다(포드·캐릭터는 무시)
	RevealRotation = FRotator(RevealPitch, LandingCameraStart.Rotator().Yaw, 0.f);

	const FRotationMatrix RevealMatrix(RevealRotation);
	const FVector Forward	= RevealMatrix.GetUnitAxis(EAxis::X);
	const FVector Right		= RevealMatrix.GetUnitAxis(EAxis::Y);
	const FVector Pivot		= GetActorLocation() + FVector::UpVector * RevealFocusHeight + Right * RevealSideOffset;
	const FVector Desired	= Pivot - Forward * RevealDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDHellpodRevealCamera), false, this);
	QueryParams.AddIgnoredActor(SpawnedCharacter);

	FHitResult Hit;
	const bool bBlocked		= GetWorld()->LineTraceSingleByChannel(Hit, Pivot, Desired, ECC_Camera, QueryParams);
	const float Distance	= HDHellpodLanding::ClampCameraDistance(RevealDistance, bBlocked, Hit.Distance, CameraProbeMargin, CameraMinDistance);

	RevealLocation = Pivot - Forward * Distance;
}

void AHDHellpod::TickLanding(const float DeltaTime)
{
	LandingElapsed += DeltaTime;

	// 시점 블렌드 중에도 헬포드 쪽 시점은 계속 계산되므로 조작 반환까지 갱신한다
	UpdateLandingCamera();

	if (bCapEjected == false && LandingElapsed >= CapEjectDelay)
	{
		EjectCap();
		SpawnCharacterTimeline.PlayFromStart();
	}

	if (bViewBlendStarted == false && LandingElapsed >= ViewBlendDelay)
	{
		bViewBlendStarted = true;

		APlayerController* PlayerController = LandingController.Get();
		if (PlayerController && SpawnedCharacter)
		{
			PlayerController->SetViewTargetWithBlend(SpawnedCharacter, ViewBlendTime, EViewTargetBlendFunction::VTBlend_EaseInOut, ViewBlendExp);
		}
	}

	if (LandingElapsed >= ControlReturnAt)
	{
		ReturnControl();
	}
}

float AHDHellpod::GetCameraShakeWeight() const
{
	const float ImpactWeight	= HDHellpodLanding::ImpactShakeWeight(LandingElapsed, ImpactShakeTime);
	const float CapWeight		= HDHellpodLanding::ImpactShakeWeight(LandingElapsed - CapEjectDelay, CapEjectShakeTime) * CapEjectShakeScale;
	return FMath::Max(ImpactWeight, CapWeight);
}

void AHDHellpod::UpdateLandingCamera()
{
	const float Alpha = HDHellpodLanding::RevealAlpha(LandingElapsed, RevealTime);

	FVector Location	= FMath::Lerp(LandingCameraStart.GetLocation(), RevealLocation, Alpha);
	FRotator Rotation	= FQuat::Slerp(LandingCameraStart.GetRotation(), RevealRotation.Quaternion(), Alpha).Rotator();

	// 충격: 착지 순간 아래로 덜컥 내려앉았다가 감쇠 흔들림과 함께 돌아온다
	const float ImpactWeight	= HDHellpodLanding::ImpactShakeWeight(LandingElapsed, ImpactShakeTime);
	const float ShakeWeight		= GetCameraShakeWeight();
	const float NoiseTime		= LandingElapsed * ImpactShakeFrequency;

	Location.Z			-= ImpactDip * ImpactWeight;
	Location.Z			+= ImpactShakeOffset * ShakeWeight * FMath::PerlinNoise1D(NoiseTime + ShakeSeedZ);
	Rotation.Pitch		+= ImpactShakeAngle * ShakeWeight * FMath::PerlinNoise1D(NoiseTime);
	Rotation.Yaw		+= ImpactShakeAngle * 0.5f * ShakeWeight * FMath::PerlinNoise1D(NoiseTime + ShakeSeedYaw);
	Rotation.Roll		+= ImpactShakeAngle * ShakeWeight * FMath::PerlinNoise1D(NoiseTime + ShakeSeedRoll);

	FollowCamera->SetWorldLocationAndRotation(Location, Rotation);
}

void AHDHellpod::EjectCap()
{
	bCapEjected = true;
	NULL_CHECK(HellpodCap);

	HellpodCap->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// 지형·구조물에만 부딪힌다. 캐릭터·카메라·탄·포드 본체와는 엉키지 않는다
	HellpodCap->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	HellpodCap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	HellpodCap->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	HellpodCap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	HellpodCap->SetCollisionResponseToChannel(HDCOLLISION_CHANNEL_PLAYER, ECR_Ignore);
	HellpodCap->SetCollisionResponseToChannel(HDCOLLISION_CHANNEL_PROJECTILE, ECR_Ignore);
	HellpodCap->SetCollisionResponseToChannel(HDCOLLISION_CHANNEL_HELLPOD, ECR_Ignore);
	HellpodCap->SetCollisionResponseToChannel(HDCOLLISION_CHANNEL_ACTION, ECR_Ignore);
	HellpodCap->BodyInstance.bUseCCD = true;
	HellpodCap->SetSimulatePhysics(true);
	HellpodCap->SetLinearDamping(CapLinearDamping);
	HellpodCap->SetAngularDamping(CapAngularDamping);

	const FVector Velocity = HDHellpodLanding::CapEjectVelocity(RevealRotation.Yaw, CapEjectUpSpeed, CapEjectSideSpeed, CapEjectSpreadDeg, FMath::FRandRange(-1.f, 1.f));
	HellpodCap->SetPhysicsLinearVelocity(Velocity);
	HellpodCap->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * CapEjectSpinDeg);

	if (CapEjectTag.IsValid() == false)
	{
		return;
	}

	APlayerController* PlayerController = LandingController.Get();
	AHDGASPlayerState* GASPlayerState = PlayerController ? PlayerController->GetPlayerState<AHDGASPlayerState>() : nullptr;
	NULL_CHECK(GASPlayerState);

	FGameplayAbilityHelper::ExcuteGameplayCue(CapEjectTag, HellpodCap->GetComponentLocation(), FVector::UpVector, GASPlayerState->GetAbilitySystemComponent());
}

void AHDHellpod::ReturnControl()
{
	bLandingActive = false;

	APlayerController* PlayerController = LandingController.Get();
	NULL_CHECK(PlayerController);
	NULL_CHECK(SpawnedCharacter);

	// 착지 연출 중 미션이 끝났으면 AHDGameMode::StopForMissionEnd 가 막은 입력을 다시 열지 않는다
	const AHDGameState* HDGameState = GetWorld()->GetGameState<AHDGameState>();
	const bool bMissionOver = HDGameState && (HDGameState->GetMissionPhase() == EHDMissionPhase::Success || HDGameState->GetMissionPhase() == EHDMissionPhase::Fail);
	if (bMissionOver)
	{
		UE_LOG(LogTemp, Display, TEXT("[HDHellpod] control kept off (mission over) t=%.2f"), LandingElapsed);
		return;
	}

	SpawnedCharacter->EnableInput(PlayerController);

	UE_LOG(LogTemp, Display, TEXT("[HDHellpod] control returned t=%.2f (cap %.2f, blend %.2f+%.2f)"), LandingElapsed, CapEjectDelay, ViewBlendDelay, ViewBlendTime);
}

void AHDHellpod::MoveHellpod(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	CurrentInput = MovementVector;

	const FRotator& Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector& ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector& RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// Triggered 는 누르는 동안 매 프레임 온다. 프레임 시간을 곱해 프레임률과 상관없이 같은 만큼 민다.
	// MaxMoveSpeed 는 60fps 에서 프레임마다 준 충격량으로 맞춰 둔 값이라 그 기준을 곱한다
	constexpr float TunedFrameRate = 60.f;
	const float FrameScale = GetWorld()->GetDeltaSeconds() * TunedFrameRate;

	CollisionBox->AddImpulse(RightDirection * MovementVector.Y * MaxMoveSpeed * FrameScale);
	CollisionBox->AddImpulse(ForwardDirection * MovementVector.X * MaxMoveSpeed * FrameScale);
}

void AHDHellpod::RotateHellpodByCurrentImpulse(const float DeltaTime)
{
	const float TargetPitch = CurrentInput.X * MaxPitchAngle;
	const float TargetRoll = -CurrentInput.Y * MaxRollAngle;

	const FRotator& CurrentRotator = HellpodMesh->GetRelativeRotation();
	const FRotator TargetRotation(
		MeshDefaultRelativeRotation.Pitch + TargetPitch,
		MeshDefaultRelativeRotation.Yaw,
		MeshDefaultRelativeRotation.Roll + TargetRoll);

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotator,
		TargetRotation,
		DeltaTime,
		5.f
	);

	HellpodMesh->SetRelativeRotation(NewRotation);
	ShakeCamera(CameraShakeScaleWhenFalling);

	CurrentInput = FVector2D::ZeroVector;
}

void AHDHellpod::ShakeCamera(const float Scale)
{
	CameraShakeSource->CameraShake = FallCameraShakeClass;
	CameraShakeSource->Start();
}
