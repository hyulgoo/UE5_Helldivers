// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "HDProjectileBase.generated.h"

class UProjectileMovementComponent;
class UBoxComponent;
class AHDCasing;
class UNiagaraSystem;
class UNiagaraComponent;
class UGameplayEffect;
class UAbilitySystemComponent;
enum class EImpactType : uint8;
enum class EStatusEffect : uint8;

UCLASS()
class HELLDIVERS_API AHDProjectileBase : public AActor
{
	GENERATED_BODY()

public:	
    AHDProjectileBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 충돌 피해를 줄 수 있는가. 적이 쏜 탄은 시설만 다치게 하지 않는다(적끼리는 피해를 준다)
    static bool                                 CanApplyImpactDamage(const AActor* OwnerActor, const AActor* TargetActor);

    // 같은 투사체 BP 를 쓰되 쏘는 쪽에 따라 충돌 피해만 바꿀 때(예: 원거리형 적)
    void                                        SetImpactDamage(const float NewImpactDamage) { ImpactDamage = NewImpactDamage; }

protected:
	virtual void	                            BeginPlay() override;
	
	UFUNCTION()
	virtual void	                            OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    // HitLocation 중심 반경 안의 Pawn(쏜 쪽 포함)에 거리 비례 피해와 넉백. 시한 신관 수류탄(AHDProjectileGrenade)도 쓴다
    void                                        ApplyExplode(UAbilitySystemComponent* SourceAbiltySystemComponent, const FVector& HitLocation);

private:
    void                                        InitializeBeginPlay();
    void			                            StartDestroyTimer();
    void                                        SpawnTrailSystem();

    void                                        ApplyDamageGameEffect(UAbilitySystemComponent* SourceAbiltySystemComponent, UAbilitySystemComponent* TargetAbiltySystemComponent, const float InterpImpactDamage);

protected:
	UPROPERTY(EditAnywhere, Category = "Info|Default")
	TObjectPtr<UStaticMeshComponent>			ProjectileMesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent>	ProjectileMovement;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent>					CollisionBox;

    UPROPERTY(EditDefaultsOnly, Category = "Info|Default")
    FGameplayTag                                ProjectileTag;

    UPROPERTY(EditDefaultsOnly, Category = "Info|Default")
    float                                       InitSpeed;
    
    UPROPERTY(EditDefaultsOnly, Category = "Info|Default")
    EImpactType                                 ImpactType;

    UPROPERTY(EditDefaultsOnly, Category = "Info|Default")
    TSubclassOf<UGameplayEffect>                DamageGameEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Info|Default")
    float                                       ImpactDamage;

    UPROPERTY(EditDefaultsOnly, Category = "Info|Default")
    FGameplayTag                                ImpactBlocklCueTag;

    UPROPERTY(EditDefaultsOnly, Category = "Info|Default")
    FGameplayTag                                ImpactHitCueTag;

    UPROPERTY(EditDefaultsOnly, Category = "Info|Status")
    EStatusEffect                               StatusEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Info|Status")
    TSubclassOf<UGameplayEffect>                StatusGameEffect;
        
    UPROPERTY(EditDefaultsOnly, Category = "Info|Status")
    float                                       DotDamage;
    
    UPROPERTY(EditDefaultsOnly, Category = "Info|Status")
    float                                       StatusDuration;
            
    UPROPERTY(EditDefaultsOnly, Category = "Info|Explode")
    float                                       ExplodeDamageRange;
    
    UPROPERTY(EditDefaultsOnly, Category = "Info|Explode")
    float                                       ExplodeKnockBackRange;
    
    UPROPERTY(EditDefaultsOnly, Category = "Info|Knockback")
    FGameplayTag                                KnockbackTag;
    
    UPROPERTY(EditDefaultsOnly, Category = "Info|Knockback")
    float                                       KnockbackImpulse;
    	
private:        
    UPROPERTY(EditAnywhere, Category = "Info|Default")
    TObjectPtr<UParticleSystem>                 Tracer;

	UPROPERTY()
    TObjectPtr<UParticleSystemComponent>        TracerComponent;

    UPROPERTY(EditAnywhere, Category = "Info|Default")
    TObjectPtr<UNiagaraSystem>                  TrailSystem;

    UPROPERTY()
    TObjectPtr<UNiagaraComponent>               TrailSystemComponent;

	UPROPERTY(EditAnywhere, Category = "Info|Default")
	float										DestroyTime = 10.f;

	FTimerHandle								DestroyTimer = FTimerHandle();
};
