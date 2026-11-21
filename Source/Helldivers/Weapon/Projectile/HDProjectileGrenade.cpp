// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Projectile/HDProjectileGrenade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Weapon/WeaponTypes.h"

AHDProjectileGrenade::AHDProjectileGrenade(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Mesh"));
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> GrenadeMeshRef(TEXT("/Game/Asset/MilitaryWeapSilver/Weapons/GrenadeLauncherA_Ammo.GrenadeLauncherA_Ammo"));
    if (GrenadeMeshRef.Succeeded())
    {
        ProjectileMesh->SetStaticMesh(GrenadeMeshRef.Object);
    }

    CollisionBox->SetBoxExtent(FVector(6.f));

    ProjectileMovement->SetIsReplicated(true);
    ProjectileMovement->bShouldBounce           = true;
    ProjectileMovement->Bounciness              = 0.3f;
    ProjectileMovement->Friction                = 0.5f;
    ProjectileMovement->ProjectileGravityScale  = 1.f;
    ProjectileMovement->InitialSpeed            = 0.f;
    ProjectileMovement->MaxSpeed                = 0.f;

    ImpactType              = EImpactType::Explode;
    ImpactDamage            = 80.f;
    ExplodeDamageRange      = 350.f;
    ExplodeKnockBackRange   = 500.f;
    KnockbackTag            = Tag_Data_Knockback_Ragdoll;
    KnockbackImpulse        = 6000.f;
    ImpactHitCueTag         = Tag_GameplayCue_Explode_BombLight;

    static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectClassRef(TEXT("/Game/Helldivers/Blueprint/GameAbility/Effect/BPGE_Damage"));
    if (DamageEffectClassRef.Succeeded())
    {
        DamageGameEffect = DamageEffectClassRef.Class;
    }
}

void AHDProjectileGrenade::SetThrowVelocity(const FVector& InVelocity)
{
    ProjectileMovement->Velocity = InVelocity;
}

void AHDProjectileGrenade::BeginPlay()
{
    // 부모 BeginPlay(중력 0·직선 속도·10초 소멸)를 건너뛴다
    AActor::BeginPlay();

    CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
    ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AHDProjectileGrenade::OnBounce);

    // 먼저 Destroy 되면 EndPlay 의 ClearAllTimersForObject 가 이 타이머도 지우도록 this 에 묶는다
    const FTimerDelegate FuseDelegate = FTimerDelegate::CreateWeakLambda(this, [this]() { Explode(); });
    GetWorldTimerManager().SetTimer(FuseTimerHandle, FuseDelegate, FuseTime, false);
}

void AHDProjectileGrenade::OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 시한 신관이라 닿아도 아무것도 하지 않는다
}

void AHDProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
    if (BounceSoundTag.IsValid() == false)
    {
        return;
    }

    FGameplayAbilityHelper::ExcuteGameplayCue(BounceSoundTag, GetActorLocation(), FVector::ZeroVector, GetOwner());
}

void AHDProjectileGrenade::Explode()
{
    UAbilitySystemComponent* OwnerASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(GetOwner());
    if (OwnerASC == nullptr)
    {
        // 던진 사람이 먼저 죽어 시체의 ASC 가 비었다
        UE_LOG(LogTemp, Warning, TEXT("[HDGrenade] no owner ASC, fizzle"));
        Destroy();
        return;
    }

    const FVector ExplodeLocation = GetActorLocation();
    ApplyExplode(OwnerASC, ExplodeLocation);
    FGameplayAbilityHelper::ExcuteGameplayCue(ImpactHitCueTag, ExplodeLocation, FVector::UpVector, OwnerASC);
    UE_LOG(LogTemp, Display, TEXT("[HDGrenade] exploded at %s"), *ExplodeLocation.ToCompactString());
    Destroy();
}
