// Fill out your copyright notice in the Description page of Project Settings.

#include "Stratagem/Deployable/HDSentryTurret.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Perception/AISense_Hearing.h"
#include "UObject/ConstructorHelpers.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "Stratagem/Deployable/HDSentryRules.h"
#include "Weapon/Projectile/HDProjectileBase.h"
#include "Define/HDDefine.h"

namespace HDSentryTurretPrivate
{
    // 받침: 엔진 Cube(100cm) x 0.6x0.6x0.5 = 60x60x50cm. 피벗이 가운데라 GA 가 지면 + 25 에 놓는다
    const FVector BaseScale(0.6f, 0.6f, 0.5f);

    // 총열: 엔진 Cylinder(지름 100, 높이 100) x 0.15x0.15x0.8 = 지름 15, 길이 80cm. 피치 90 으로 눕힌다.
    // 받침의 비균일 배율을 물려받지 않게 절대 배율로 둔다
    const FVector BarrelScale(0.15f, 0.15f, 0.8f);
    constexpr float BarrelHalfLength = 40.f;

    // 총열 가운데 = 받침 윗면(+25) + 총열 반지름(7.5). 받침 로컬 좌표라 받침 Z 배율(0.5)로 나눈다
    const FVector BarrelRelativeLocation(0.f, 0.f, 32.5f / 0.5f);
    constexpr float BarrelPitch = 90.f;

    // 적 캡슐 가운데쯤을 노린다
    const FVector AimOffset(0.f, 0.f, 50.f);

    // 표적이 없을 때 다시 찾는 간격(초). 매 틱 모든 적을 돌며 선 검사하지 않게 한다
    constexpr float IdleSearchInterval = 0.2f;
}

AHDSentryTurret::AHDSentryTurret(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    using namespace HDSentryTurretPrivate;

    PrimaryActorTick.bCanEverTick = true;

    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
    SetRootComponent(BaseMesh);
    BaseMesh->SetRelativeScale3D(BaseScale);
    BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
    BarrelMesh->SetupAttachment(RootComponent);
    BarrelMesh->SetUsingAbsoluteScale(true);
    BarrelMesh->SetRelativeScale3D(BarrelScale);
    BarrelMesh->SetRelativeLocation(BarrelRelativeLocation);
    BarrelMesh->SetRelativeRotation(FRotator(BarrelPitch, 0.f, 0.f));
    BarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshRef(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMeshRef.Succeeded())
    {
        BaseMesh->SetStaticMesh(CubeMeshRef.Object);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshRef(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMeshRef.Succeeded())
    {
        BarrelMesh->SetStaticMesh(CylinderMeshRef.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialRef(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterialRef.Succeeded())
    {
        BaseMesh->SetMaterial(0, BasicMaterialRef.Object);
        BarrelMesh->SetMaterial(0, BasicMaterialRef.Object);
    }

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
}

UAbilitySystemComponent* AHDSentryTurret::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AHDSentryTurret::InitializeTurret(const float Lifetime, const float Range, const float FireInterval, const int32 MaxShots, TSubclassOf<AHDProjectileBase> InProjectileClass)
{
    TurretLifetime = Lifetime;
    TurretRange = Range;
    TurretFireInterval = FireInterval;
    TurretMaxShots = MaxShots;
    ProjectileClass = InProjectileClass;
}

void AHDSentryTurret::BeginPlay()
{
    Super::BeginPlay();

    NULL_CHECK(AbilitySystemComponent);

    // 탄의 피해 GE 를 이 ASC 가 건다. 주인·아바타 모두 자신
    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] sentry deployed range=%.1f lifetime=%.1f"), TurretRange, TurretLifetime);
}

void AHDSentryTurret::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bFinished)
    {
        return;
    }

    ElapsedTime += DeltaSeconds;
    if (TurretLifetime > 0.f && ElapsedTime >= TurretLifetime)
    {
        FinishTurret();
        return;
    }

    FireCooldown -= DeltaSeconds;
    if (FireCooldown > 0.f)
    {
        return;
    }

    // 표적이 없으면 잠깐 뒤에 다시 찾는다
    if (TryFire() == false)
    {
        FireCooldown = HDSentryTurretPrivate::IdleSearchInterval;
        return;
    }

    ++ShotCount;
    FireCooldown = TurretFireInterval;
    if (TurretMaxShots > 0 && ShotCount >= TurretMaxShots)
    {
        FinishTurret();
    }
}

bool AHDSentryTurret::TryFire()
{
    using namespace HDSentryTurretPrivate;

    UWorld* World = GetWorld();
    if (World == nullptr || ProjectileClass == nullptr)
    {
        return false;
    }

    const FVector Origin = GetActorLocation();
    const double RangeSquared = FMath::Square(static_cast<double>(TurretRange));

    FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(SentryVisibility), false, this);

    TArray<FHDSentryCandidate> Candidates;
    TArray<FVector> AimPoints;
    for (TActorIterator<AHDCharacterNonPlayer> It(World); It; ++It)
    {
        AHDCharacterNonPlayer* Enemy = *It;
        if (IsValid(Enemy) == false)
        {
            continue;
        }

        FHDSentryCandidate& Candidate = Candidates.AddDefaulted_GetRef();
        // 래그돌 중에는 캡슐(액터 위치)이 제자리에 남는다. 날아간 몸(골반)을 그대로 겨눈다
        const bool bRagdoll = Enemy->GetMesh()->IsSimulatingPhysics();
        Candidate.Location = Enemy->GetBodyLocation();
        Candidate.bAlive = Enemy->IsDead() == false;

        const FVector AimPoint = bRagdoll ? Candidate.Location : Candidate.Location + AimOffset;
        AimPoints.Add(AimPoint);

        // 죽었거나 사거리 밖이면 SelectTarget 이 어차피 뺀다. 선 검사는 남은 후보만 한다
        if (Candidate.bAlive == false || FVector::DistSquared(Origin, Candidate.Location) > RangeSquared)
        {
            continue;
        }

        // 캐릭터 캡슐·메시는 Visibility 를 무시하므로(DefaultEngine.ini HDCapsule·HDPlayer 프로필) 막는 것은 지형·건물이다
        const FVector Direction = (AimPoint - Origin).GetSafeNormal2D();
        Candidate.bVisible = World->LineTraceTestByChannel(GetMuzzleLocation(Direction), AimPoint, ECC_Visibility, TraceParams) == false;
    }

    const int32 TargetIndex = HDSentryRules::SelectTarget(Origin, TurretRange, Candidates);
    if (Candidates.IsValidIndex(TargetIndex) == false)
    {
        return false;
    }

    // 총열을 표적 쪽(요)으로 돌리고 총열 끝에서 표적을 향해 쏜다
    const FVector AimPoint = AimPoints[TargetIndex];
    const FVector Direction = (AimPoint - Origin).GetSafeNormal2D();
    BarrelMesh->SetWorldRotation(FRotator(BarrelPitch, Direction.Rotation().Yaw, 0.f));

    const FVector MuzzleLocation = GetMuzzleLocation(Direction);
    const FTransform SpawnTransform((AimPoint - MuzzleLocation).Rotation(), MuzzleLocation);
    AHDProjectileBase* Projectile = World->SpawnActorDeferred<AHDProjectileBase>(
        ProjectileClass,
        SpawnTransform,
        this,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (Projectile == nullptr)
    {
        return false;
    }

    UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);

    // 총성: 최대 거리 0 = 듣는 쪽 HearingRange 로 거른다(UHDCombatComponent::SpawnProjectile 과 같은 인자).
    // 적 AI 는 플레이어가 낸 소리만 받으므로(AHDEnemyAIController::OnTargetPerceptionUpdated) 주체는 던진 플레이어(Instigator). 없으면 포탑 자신
    AActor* NoiseInstigator = GetInstigator() ? static_cast<AActor*>(GetInstigator()) : static_cast<AActor*>(this);
    UAISense_Hearing::ReportNoiseEvent(this, Origin, 1.f, NoiseInstigator, 0.f, NAME_None);
    return true;
}

FVector AHDSentryTurret::GetMuzzleLocation(const FVector& Direction) const
{
    using namespace HDSentryTurretPrivate;

    return BarrelMesh->GetComponentLocation() + Direction * BarrelHalfLength;
}

void AHDSentryTurret::FinishTurret()
{
    bFinished = true;
    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] sentry finished shots=%d"), ShotCount);
    Destroy();
}
