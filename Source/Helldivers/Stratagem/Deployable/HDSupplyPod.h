// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HDSupplyPod.generated.h"

class USphereComponent;
class UStaticMeshComponent;

// 보급 상자(UHDGA_StratagemResupply 가 표적에 놓는다). 엔진 기본 Cube 모양.
// 플레이어 캐릭터가 줍기 구(HDTrigger)에 들어와 AHDCharacterPlayer::Resupply 가 참이면 사라진다. 한 번만 주울 수 있다
UCLASS()
class HELLDIVERS_API AHDSupplyPod : public AActor
{
	GENERATED_BODY()

public:
    AHDSupplyPod(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // FinishSpawning 전에 부른다. 반지름이 0 보다 크면 줍기 구 반지름, 수명이 0 보다 크면 BeginPlay 에서 그 시간 뒤 사라진다
    void                                InitializePod(const float Lifetime, const float PickupRadius);

protected:
    virtual void                        BeginPlay() override;

private:
    UFUNCTION()
    void                                OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent>        PickupSphere;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent>    PodMesh;

    float                               PodLifetime = 0.f;
};
