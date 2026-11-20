// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "HDSentryTurret.generated.h"

class AHDProjectileBase;
class UAbilitySystemComponent;
class UStaticMeshComponent;

// 기관총 포탑(UHDGA_StratagemSentry 가 표적에 놓는다). 엔진 기본 Cube 받침 + Cylinder 총열.
// 사거리 안에서 살아 있고 보이는 가장 가까운 적(HDSentryRules::SelectTarget)에게 발사 간격마다 탄을 쏜다. 수명이나 최대 발사 수에 닿으면 사라진다.
// 탄의 피해 경로(AHDProjectileBase::OnBoxHit)가 소유자의 ASC 를 요구하므로 자기 ASC 를 둔다(시설 목표와 같은 방법)
UCLASS()
class HELLDIVERS_API AHDSentryTurret : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
    AHDSentryTurret(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual UAbilitySystemComponent*        GetAbilitySystemComponent() const override;

    // FinishSpawning 전에 부른다. Lifetime·MaxShots 가 0 이하면 그 조건으로는 끝나지 않는다
    void                                    InitializeTurret(const float Lifetime, const float Range, const float FireInterval, const int32 MaxShots, TSubclassOf<AHDProjectileBase> InProjectileClass);

    virtual void                            Tick(float DeltaSeconds) override;

protected:
    virtual void                            BeginPlay() override;

private:
    // 표적을 고르고 있으면 총열을 돌려 한 발 쏜다. 쏘았으면 참
    bool                                    TryFire();

    // 총열 가운데에서 Direction(수평) 쪽 총열 끝
    FVector                                 GetMuzzleLocation(const FVector& Direction) const;

    void                                    FinishTurret();

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent>        BaseMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent>        BarrelMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UAbilitySystemComponent>     AbilitySystemComponent;

    UPROPERTY()
    TSubclassOf<AHDProjectileBase>          ProjectileClass;

    float                                   TurretLifetime      = 0.f;
    float                                   TurretRange         = 0.f;
    float                                   TurretFireInterval  = 0.f;
    int32                                   TurretMaxShots      = 0;

    float                                   ElapsedTime         = 0.f;
    float                                   FireCooldown        = 0.f;
    int32                                   ShotCount           = 0;
    bool                                    bFinished           = false;
};
