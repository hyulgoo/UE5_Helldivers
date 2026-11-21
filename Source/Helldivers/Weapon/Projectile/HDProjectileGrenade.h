// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HDProjectileBase.h"
#include "GameplayTagContainer.h"
#include "HDProjectileGrenade.generated.h"

class USoundCue;

// 손으로 던지는 시한 신관 수류탄. 튕기다가 FuseTime 뒤 소유자 ASC 로 폭발한다(던진 사람도 다친다)
UCLASS()
class HELLDIVERS_API AHDProjectileGrenade : public AHDProjectileBase
{
	GENERATED_BODY()

public:
    AHDProjectileGrenade(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // SpawnActorDeferred 와 FinishSpawning 사이에 부른다(월드 기준 속도)
    void                    SetThrowVelocity(const FVector& InVelocity);

    // 치트 HDExplodeAtEnemy 가 신관을 기다리지 않고 부른다
    void                    Explode();

protected:
	virtual void	        BeginPlay() override final;

    // 닿아도 터지지 않고 튕긴다(부모는 처음 닿을 때 피해를 주고 지운다)
    virtual void            OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	UFUNCTION()
	void			        OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
    UPROPERTY(EditAnywhere)
    FGameplayTag            BounceSoundTag;

    UPROPERTY(EditDefaultsOnly)
    float                   FuseTime    = 3.f;

    FTimerHandle            FuseTimerHandle;
};
