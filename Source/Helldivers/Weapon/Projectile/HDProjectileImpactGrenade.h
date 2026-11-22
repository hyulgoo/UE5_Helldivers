// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile/HDProjectileGrenade.h"
#include "HDProjectileImpactGrenade.generated.h"

// 충격 수류탄(브리프 9). 던진 사람이 아닌 것에 처음 닿으면 바로 터진다. 아무 데도 안 닿으면 부모의 3초 신관이 남는다
UCLASS()
class HELLDIVERS_API AHDProjectileImpactGrenade : public AHDProjectileGrenade
{
    GENERATED_BODY()

public:
    AHDProjectileImpactGrenade(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
    // 부모(시한 신관)는 닿아도 튕기기만 한다. 여기서는 던진 사람이 아니면 Explode 한다
    virtual void            OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};
