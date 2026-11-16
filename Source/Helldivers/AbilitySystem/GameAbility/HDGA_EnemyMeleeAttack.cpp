// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_EnemyMeleeAttack.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "AI/HDEnemyAIController.h"

UHDGA_EnemyMeleeAttack::UHDGA_EnemyMeleeAttack(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectClassRef(TEXT("/Game/Helldivers/Blueprint/GameAbility/Effect/BPGE_Damage"));
    if (DamageEffectClassRef.Succeeded())
    {
        DamageEffectClass = DamageEffectClassRef.Class;
    }
}

void UHDGA_EnemyMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
    const AHDCharacterNonPlayer* Enemy = Cast<AHDCharacterNonPlayer>(ActorInfo->AvatarActor.Get());
    if (Enemy == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    UAbilityTask_WaitDelay* WaitWindupTask = UAbilityTask_WaitDelay::WaitDelay(this, Enemy->GetEnemyStats().AttackWindup);
    WaitWindupTask->OnFinish.AddDynamic(this, &UHDGA_EnemyMeleeAttack::OnWindupFinished);
    WaitWindupTask->ReadyForActivation();
}

void UHDGA_EnemyMeleeAttack::OnWindupFinished()
{
    // 예비 동작 중에 죽었으면 돌진·판정을 하지 않는다
    AHDCharacterNonPlayer* Enemy = Cast<AHDCharacterNonPlayer>(GetAvatarActorFromActorInfo());
    if (Enemy && Enemy->IsDead() == false)
    {
        // 돌진
        const FVector Forward = Enemy->GetActorForwardVector();
        Enemy->LaunchCharacter(Forward * 500.f + FVector::UpVector * 120.f, true, false);

        ApplyMeleeHit(Enemy);
    }

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = false;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHDGA_EnemyMeleeAttack::ApplyMeleeHit(AHDCharacterNonPlayer* Enemy)
{
    UWorld* World = GetWorld();
    NULL_CHECK(World);

    const FHDEnemyStats& Stats = Enemy->GetEnemyStats();
    const FVector Center = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * (Stats.AttackRange * 0.5f);
    const float Radius = Stats.AttackRange * 0.6f;

    if (AHDEnemyAIController::IsDebugEnabled())
    {
        DrawDebugSphere(World, Center, Radius, 16, FColor::Red, false, 0.5f);
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDEnemyMeleeAttack), false, Enemy);
    World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), QueryParams);

    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
    NULL_CHECK(SourceASC);
    NULL_CHECK(DamageEffectClass);

    // 캡슐과 메시가 함께 겹치므로 액터당 한 번만 때린다
    TSet<AActor*> HitActors;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AHDCharacterPlayer* Player = Cast<AHDCharacterPlayer>(Overlap.GetActor());
        if (Player == nullptr || HitActors.Contains(Player))
        {
            continue;
        }
        HitActors.Add(Player);

        UAbilitySystemComponent* TargetASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Player);
        if (TargetASC == nullptr)
        {
            continue;
        }

        // 투사체와 같은 태그·부호(HDProjectileBase.cpp ApplyDamageGameEffect)
        const FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass);
        if (DamageSpecHandle.IsValid() == false)
        {
            continue;
        }

        DamageSpecHandle.Data->SetSetByCallerMagnitude(Tag_Data_Damage_Projectile, -Stats.AttackDamage);
        SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
    }
}
