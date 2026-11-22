// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Projectile/HDProjectileImpactGrenade.h"

AHDProjectileImpactGrenade::AHDProjectileImpactGrenade(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 파편 수류탄(350·500)보다 조금 작다. 나머지(메시·피해·넉백·관통)는 부모 값
    ExplodeDamageRange      = 300.f;
    ExplodeKnockBackRange   = 450.f;
}

void AHDProjectileImpactGrenade::OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 던진 사람(Owner·Instigator)은 이동에서 무시하지만, 닿더라도 터지지 않는다
    const bool bHitThrower = OtherActor != nullptr && (OtherActor == GetOwner() || OtherActor == GetInstigator());
    if (bHitThrower)
    {
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDGrenade] impact on %s"), *GetNameSafe(OtherActor));
    Explode();
}
