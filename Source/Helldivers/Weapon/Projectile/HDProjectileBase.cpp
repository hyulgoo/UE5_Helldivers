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
#include "Weapon/HDArmorRules.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/HDHUD.h"

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

int32 AHDProjectileBase::GetEffectiveArmorPenetration() const
{
    constexpr int32 AutoBulletPenetration   = 2;
    constexpr int32 AutoExplodePenetration  = 4;

    if (ArmorPenetration > 0)
    {
        return ArmorPenetration;
    }

    return ImpactType == EImpactType::Explode ? AutoExplodePenetration : AutoBulletPenetration;
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
        bool bDamaged = false;
		if (TargetASC)
		{
            // 장갑 표는 적만 갖는다. 그 밖(플레이어·시설·포탑)은 기본 표라 피해가 ImpactDamage 그대로다
            const AHDCharacterNonPlayer* TargetEnemy = Cast<AHDCharacterNonPlayer>(OtherActor);
            const FHDArmorProfile ArmorProfile = TargetEnemy ? TargetEnemy->GetArmorProfile() : FHDArmorProfile();

            // 투사체는 속도 방향으로 돈다(bRotationFollowsVelocity). 정면이면 1, 비스듬할수록 0 에 가깝다
            const float CosIncidence = FVector::DotProduct(-GetActorForwardVector(), Hit.ImpactNormal);
            const FHDHitResolution Resolution = HDArmor::ResolveHit(ArmorProfile, Hit.BoneName, HDArmor::ApplyIncidence(GetEffectiveArmorPenetration(), CosIncidence), ImpactDamage);

            // 도탄이면 피해 GE 를 걸지 않는다(상태 효과도 ApplyDamageGameEffect 안에 있어 함께 빠진다)
            bDamaged = Resolution.Outcome != EHDHitOutcome::Ricochet;
            if (bDamaged)
            {
                ApplyDamageGameEffect(OwnerASC, TargetASC, Resolution.Damage);
            }

            if (TargetEnemy)
            {
                UE_LOG(LogTemp, Display, TEXT("[HDArmor] hit %s bone=%s zone=%s pen=%d armor=%d -> %s damage=%.1f health=%.0f"),
                    *TargetEnemy->GetName(),
                    *Hit.BoneName.ToString(),
                    HDArmor::GetZoneName(Resolution.Zone),
                    Resolution.Penetration,
                    Resolution.Armor,
                    HDArmor::GetOutcomeName(Resolution.Outcome),
                    Resolution.Damage,
                    TargetASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute()));

                // 약점: 머리이고 그 표의 머리 배율이 1 보다 클 때
                NotifyOwnerHud(Resolution.Outcome, Resolution.Zone == EHDHitZone::Head && ArmorProfile.HeadDamageMultiplier > 1.f);
            }
		}

        FGameplayAbilityHelper::ExcuteGameplayCue(bDamaged ? ImpactHitCueTag : ImpactBlocklCueTag, Hit.ImpactPoint, Hit.ImpactNormal, OwnerASC);
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

    // 히트마커는 한 번의 폭발에서 가장 좋은 결과(Full > Reduced > Ricochet) 하나만 알린다
    bool bHitEnemy = false;
    EHDHitOutcome BestOutcome = EHDHitOutcome::Ricochet;

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

		float Distance = FVector::Distance(HitLocation, Target->GetActorLocation());

		// 시설은 400x400x600 상자라 중심(지면 위 3m)까지 재면 벽 바로 옆 폭발도 반경을 넘는다. 시설만 가장 가까운 표면까지 잰다
		if (Target->IsA<AHDObjectiveFacility>())
		{
			const UPrimitiveComponent* HitComponent = Result.GetComponent();
			FVector ClosestPoint = FVector::ZeroVector;
			const float SurfaceDistance = HitComponent ? HitComponent->GetClosestPointOnCollision(HitLocation, ClosestPoint) : -1.f;
			if (SurfaceDistance >= 0.f)
			{
				Distance = SurfaceDistance;
			}
		}

		if (Distance <= ExplodeDamageRange)
		{
			const float Damage = ImpactDamage * (1.f - (Distance / ExplodeDamageRange));

            // 폭발은 몸통 장갑으로 판정한다. 도탄이면 피해 없이 아래 넉백만 간다
            const AHDCharacterNonPlayer* TargetEnemy = Cast<AHDCharacterNonPlayer>(Target);
            const FHDHitResolution Resolution = HDArmor::ResolveExplosion(TargetEnemy ? TargetEnemy->GetArmorProfile() : FHDArmorProfile(), GetEffectiveArmorPenetration(), Damage);
            if (Resolution.Outcome != EHDHitOutcome::Ricochet)
            {
                ApplyDamageGameEffect(SourceASC, TargetASC, Resolution.Damage);
            }

            if (TargetEnemy)
            {
                UE_LOG(LogTemp, Display, TEXT("[HDArmor] explosion %s pen=%d armor=%d -> %s damage=%.1f"),
                    *TargetEnemy->GetName(),
                    Resolution.Penetration,
                    Resolution.Armor,
                    HDArmor::GetOutcomeName(Resolution.Outcome),
                    Resolution.Damage);

                // 열거형 순서가 Full, Reduced, Ricochet 이라 작은 값이 더 좋은 결과다
                if (bHitEnemy == false || Resolution.Outcome < BestOutcome)
                {
                    BestOutcome = Resolution.Outcome;
                }
                bHitEnemy = true;
            }

			UE_LOG(LogTemp, Error, TEXT("Distance : %f, ExplodeDamageRange : %f, ImpactDamage : %f"), Damage, ExplodeDamageRange, ImpactDamage);
		}

        FGameplayAbilityHelper::SendGameplayEventToTarget(KnockbackTag, this, TargetASC, KnockbackImpulse * (1.f - (Distance / ExplodeKnockBackRange)));
	}

    if (bHitEnemy)
    {
        NotifyOwnerHud(BestOutcome, false);
    }
}

void AHDProjectileBase::NotifyOwnerHud(EHDHitOutcome Outcome, bool bWeakPoint) const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    AHDHUD* HUD = PlayerController ? PlayerController->GetHUD<AHDHUD>() : nullptr;
    if (HUD)
    {
        HUD->NotifyHitFeedback(Outcome, bWeakPoint);
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
