// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_EnemyRangedAttack.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/World.h"
#include "AI/HDEnemyAIController.h"
#include "Character/NPC/HDEnemyShooter.h"
#include "Weapon/Projectile/HDProjectileBase.h"
#include "Game/HDSpawnDirectorSettings.h"

UHDGA_EnemyRangedAttack::UHDGA_EnemyRangedAttack(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    static ConstructorHelpers::FClassFinder<AHDProjectileBase> ProjectileClassRef(TEXT("/Game/Helldivers/Blueprint/Projectile/BPP_Bullet_Light"));
    if (ProjectileClassRef.Succeeded())
    {
        ProjectileClass = ProjectileClassRef.Class;
    }
}

void UHDGA_EnemyRangedAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = true;

    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    // NULL_CHECK 는 return 만 해서 어빌리티가 끝나지 않으므로 직접 끝낸다
    const AHDEnemyShooter* Shooter = Cast<AHDEnemyShooter>(ActorInfo->AvatarActor.Get());
    if (Shooter == nullptr || ProjectileClass == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    ShotsFired = 0;
    WaitNextShot(Shooter->GetEnemyStats().AttackWindup);
}

void UHDGA_EnemyRangedAttack::WaitNextShot(const float DelaySeconds)
{
    // 어빌리티 태스크라 EndAbility 때 함께 정리된다
    UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, DelaySeconds);
    WaitTask->OnFinish.AddDynamic(this, &UHDGA_EnemyRangedAttack::OnShotDelayFinished);
    WaitTask->ReadyForActivation();
}

void UHDGA_EnemyRangedAttack::OnShotDelayFinished()
{
    // 점사 중에 죽었으면 더 쏘지 않는다
    AHDEnemyShooter* Shooter = Cast<AHDEnemyShooter>(GetAvatarActorFromActorInfo());
    if (Shooter && Shooter->IsDead() == false)
    {
        FireOneShot(*Shooter);
        ++ShotsFired;

        if (ShotsFired < Shooter->GetEnemyStats().BurstCount)
        {
            WaitNextShot(Shooter->GetEnemyStats().BurstInterval);
            return;
        }
    }

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = false;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHDGA_EnemyRangedAttack::FireOneShot(AHDEnemyShooter& Shooter)
{
    UWorld* World = GetWorld();
    NULL_CHECK(World);

    const AHDEnemyAIController* EnemyAIController = Cast<AHDEnemyAIController>(Shooter.GetController());
    const AActor* Target = EnemyAIController ? EnemyAIController->GetTargetActor() : nullptr;
    NULL_CHECK_WITHOUT_LOG(Target);

    const FVector MuzzleLocation = Shooter.GetMuzzleLocation();
    const FVector AimLocation = Target->GetActorLocation() + FVector(0.f, 0.f, TargetChestHeight);
    const FVector AimDirection = (AimLocation - MuzzleLocation).GetSafeNormal();
    const FVector ShotDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(Shooter.GetEnemyStats().AimSpreadDegrees));

    // 소유자가 적이므로 AHDProjectileBase::OnBoxHit 이 적 ASC 를 원천으로 피해를 준다
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = &Shooter;
    SpawnParams.Instigator = &Shooter;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AHDProjectileBase* Projectile = World->SpawnActor<AHDProjectileBase>(ProjectileClass, MuzzleLocation, ShotDirection.Rotation(), SpawnParams);
    NULL_CHECK(Projectile);

    // 총알 BP 는 플레이어 소총과 같으므로 피해만 원거리형 값으로 바꾼다(Project Settings > HD Spawn Director > Enemy)
    Projectile->SetImpactDamage(GetDefault<UHDSpawnDirectorSettings>()->ShooterShotDamage);
}
