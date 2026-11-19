#include "HDProjectileBase.h"  
#include "Components/BoxComponent.h"  
#include "NiagaraFunctionLibrary.h"  
#include "Kismet/GameplayStatics.h"  
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "Collision/HDCollision.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "Mission/HDObjectiveFacility.h"
#include "Weapon/WeaponTypes.h"

AHDProjectileBase::AHDProjectileBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) 
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionProfileName(HDCOLLISION_PROFILE_PROJECTILE);
	CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
	CollisionBox->OnComponentHit.AddDynamic(this, &AHDProjectileBase::OnBoxHit);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(RootComponent);
	ProjectileMovement->bRotationFollowsVelocity = true;
}

bool AHDProjectileBase::CanApplyImpactDamage(const AActor* OwnerActor, const AActor* TargetActor)
{
    if (OwnerActor == nullptr || TargetActor == nullptr)
    {
        return false;
    }

    // 쏜 쪽 자신은 다치지 않는다(총구가 캡슐과 겹쳐 생성될 때)
    if (OwnerActor == TargetActor)
    {
        return false;
    }

    // 적(돌격형·원거리형)이 쏜 탄은 시설만 다치게 하지 않는다. 적끼리는 피해를 준다
    if (OwnerActor->IsA<AHDCharacterNonPlayer>() && TargetActor->IsA<AHDObjectiveFacility>())
    {
        return false;
    }

    return true;
}

void AHDProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeBeginPlay();
	StartDestroyTimer();
	SpawnTrailSystem();
}

void AHDProjectileBase::OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	NULL_CHECK(OtherActor);

	AActor* OwnerActor = GetOwner();
	NULL_CHECK(OwnerActor);

	TScriptInterface<IAbilitySystemInterface> OwnerAbilitySystemInterface = OwnerActor;
	NULL_CHECK(OwnerAbilitySystemInterface);

	UAbilitySystemComponent* OwnerASC = OwnerAbilitySystemInterface->GetAbilitySystemComponent();
	NULL_CHECK(OwnerASC);

	if (ImpactType == EImpactType::Explode)
	{
		ApplyExplode(OwnerASC, Hit.ImpactPoint);
        FGameplayAbilityHelper::ExcuteGameplayCue(ImpactHitCueTag, Hit.ImpactPoint, Hit.ImpactNormal, OwnerASC);
	}
	else
	{
        // 적이 쏜 탄은 시설을 다치게 하지 않는다(막힘 큐와 Destroy 는 그대로)
        UAbilitySystemComponent* TargetASC = CanApplyImpactDamage(OwnerActor, OtherActor) ? FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(OtherActor) : nullptr;
		if (TargetASC)
		{
			ApplyDamageGameEffect(OwnerASC, TargetASC, ImpactDamage);
		}

        FGameplayAbilityHelper::ExcuteGameplayCue(TargetASC ? ImpactHitCueTag : ImpactBlocklCueTag, Hit.ImpactPoint, Hit.ImpactNormal, OwnerASC);
	}

	Destroy();
}

void AHDProjectileBase::InitializeBeginPlay()
{
	CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);

	ProjectileMovement->MaxSpeed = InitSpeed;
	ProjectileMovement->Velocity = GetActorForwardVector() * InitSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void AHDProjectileBase::StartDestroyTimer()
{
    // 맞아서 먼저 Destroy 되면 EndPlay 의 ClearAllTimersForObject 가 이 타이머도 지우도록 this 에 묶는다
    FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this]() { Destroy(); });
	GetWorldTimerManager().SetTimer(DestroyTimer, Delegate, DestroyTime, false);
}

void AHDProjectileBase::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, RootComponent, FName(),
			GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition, false);
	}

	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			GetActorScale(),
			EAttachLocation::KeepWorldPosition
		);
	}
}

void AHDProjectileBase::ApplyExplode(UAbilitySystemComponent* SourceASC, const FVector& HitLocation)
{
	VALID_CHECK(SourceASC);

	UWorld* World = GetWorld();
	VALID_CHECK(World);

	const FCollisionShape CollisionSphere = FCollisionShape::MakeSphere(ExplodeKnockBackRange);

	const FCollisionQueryParams Params(SCENE_QUERY_STAT(Explosion), false, this);
	const FCollisionObjectQueryParams ObjectParams(ECC_Pawn);

	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		HitLocation,
		HitLocation,
		FQuat::Identity,
		ObjectParams,
		CollisionSphere,
		Params
	);

	TSet<AActor*> HitActors;
	for (const FHitResult& Result : HitResults)
	{
		AActor* Target = Result.GetActor();
		if (Target == nullptr || HitActors.Contains(Target))
		{
			continue;
		}

		HitActors.Add(Target);

		UAbilitySystemComponent* TargetASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Target);
		if (TargetASC == nullptr)
		{
			continue;
		}

		const float Distance = FVector::Distance(HitLocation, Target->GetActorLocation());

		if (Distance <= ExplodeDamageRange)
		{
			const float Damage = ImpactDamage * (1.f - (Distance / ExplodeDamageRange));
			ApplyDamageGameEffect(SourceASC, TargetASC, Damage);
			UE_LOG(LogTemp, Error, TEXT("Distance : %f, ExplodeDamageRange : %f, ImpactDamage : %f"), Damage, ExplodeDamageRange, ImpactDamage);
		}

        FGameplayAbilityHelper::SendGameplayEventToTarget(KnockbackTag, this, TargetASC, KnockbackImpulse * (1.f - (Distance / ExplodeKnockBackRange)));
	}
}

void AHDProjectileBase::ApplyDamageGameEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const float InterpImpactDamage)
{
	NULL_CHECK(DamageGameEffect);
	VALID_CHECK(SourceASC);
	VALID_CHECK(TargetASC);

	const FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	const FGameplayEffectSpecHandle ImpactGameEffectSpecHandle = TargetASC->MakeOutgoingSpec(DamageGameEffect, 1.f, Context);
	CONDITION_CHECK(ImpactGameEffectSpecHandle.IsValid());

	ImpactGameEffectSpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Damage_Projectile, -InterpImpactDamage);
	SourceASC->ApplyGameplayEffectSpecToTarget(*ImpactGameEffectSpecHandle.Data.Get(), TargetASC);

	if (StatusEffect != EStatusEffect::None)
	{
		NULL_CHECK(StatusGameEffect);

		const FGameplayEffectSpecHandle StatusGameEffectSpecHandle = TargetASC->MakeOutgoingSpec(StatusGameEffect, 1.f, Context);
		CONDITION_CHECK(StatusGameEffectSpecHandle.IsValid());

		StatusGameEffectSpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_DotDamage_TickDamage, -DotDamage);
		StatusGameEffectSpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Projectile_StatusEffectDuration, StatusDuration);
		SourceASC->ApplyGameplayEffectSpecToTarget(*StatusGameEffectSpecHandle.Data.Get(), TargetASC);
	}
}
