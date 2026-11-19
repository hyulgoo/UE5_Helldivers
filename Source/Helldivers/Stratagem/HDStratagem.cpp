// Fill out your copyright notice in the Description page of Project Settings.

#include "Stratagem/HDStratagem.h"
#include "NiagaraFunctionLibrary.h"  
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "BattleShip/HDBattleShip.h"
#include "Kismet/GameplayStatics.h"
#include "Game/HDGameState.h"
#include "Define/HDDefine.h"
#include "Collision/HDCollision.h"

AHDStratagem::AHDStratagem(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
    CollisionSphere->SetCollisionProfileName(HDCOLLISION_PROFILE_STRATAGEM);
    CollisionSphere->IgnoreActorWhenMoving(GetInstigator(), true);
    CollisionSphere->OnComponentHit.AddDynamic(this, &AHDStratagem::OnHit);

    StratagemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StratagemMesh"));
    StratagemMesh->SetupAttachment(RootComponent);
    StratagemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHDStratagem::SetStratagemInfo(const FName SelectedStratagemName, const float SelectedStratagemActiveDelay)
{
    StratagemName = SelectedStratagemName;
    StratagemActiveDelay = SelectedStratagemActiveDelay;
}

void AHDStratagem::AddImpulseToStratagem(const FVector& NewThrowDirection)
{
    CONDITION_CHECK(NewThrowDirection.IsNormalized());

    DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
    SetOwner(nullptr);

    ThrowDirection = NewThrowDirection;
    // 반지름 8cm 공이 한 프레임에 지름보다 멀리 가므로 CCD 없이는 지형을 뚫고 떨어진다
    CollisionSphere->SetUseCCD(true);
    CollisionSphere->SetSimulatePhysics(true);
    CollisionSphere->SetNotifyRigidBodyCollision(true);
    // 질량과 무관하게 ThrowImpulse(cm/s)를 처음 속도로 준다. BP 의 질량 덮어쓰기(100kg)로 나누면 초속 30cm 라 발밑에 떨어진다
    CollisionSphere->AddImpulse(ThrowDirection * ThrowImpulse, NAME_None, true);
}

void AHDStratagem::BeginPlay()
{
    Super::BeginPlay();
}

void AHDStratagem::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherComp->GetCollisionObjectType() != ECC_WorldStatic)
    {
        return;
    }

    UWorld* World = GetWorld();

    AHDGameState* GameState = Cast<AHDGameState>(UGameplayStatics::GetGameState(World));
    NULL_CHECK(GameState);

    AHDBattleShip* BattleShip = GameState->GetBattleShip();
    NULL_CHECK(BattleShip);

    ThrowDirection.Z = 0.f;
    ThrowDirection.Normalize();

    const FTransform Transform(ThrowDirection.Rotation(), Hit.ImpactPoint);
    BattleShip->ActivateStratagem(StratagemName, Transform, StratagemActiveDelay);

    CollisionSphere->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    CollisionSphere->SetSimulatePhysics(false);
    SpawnPointLaser();

    SetLifeSpan(StratagemActiveDelay);
}

void AHDStratagem::SpawnPointLaser()
{
    UWorld* World = GetWorld();

    NULL_CHECK(PointLaserNiagara);

    const FVector Start = FVector::ZeroVector;
    const FVector End = FVector::UpVector * 80000.f;
    NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, PointLaserNiagara, GetActorLocation());
    NULL_CHECK(NiagaraComponent);

    NiagaraComponent->SetVectorParameter(TEXT("Beam Start"), Start);
    NiagaraComponent->SetVectorParameter(TEXT("Beam End"),   End);
    NiagaraComponent->SetFloatParameter(TEXT("VisibleTime"), StratagemActiveDelay);
}
