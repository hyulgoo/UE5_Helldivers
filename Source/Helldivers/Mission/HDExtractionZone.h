// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HDExtractionZone.generated.h"

class USphereComponent;
class UNiagaraSystem;
class UNiagaraComponent;

// 추출 지점. 하늘로 솟은 빛기둥(NS_PointLaser)과 겹침 구(HDTrigger).
// 플레이어 캐릭터가 들어오고 나갈 때 AHDGameMode::NotifyPlayerInExtractionZone 을 부른다
UCLASS()
class HELLDIVERS_API AHDExtractionZone : public AActor
{
	GENERATED_BODY()

public:
    AHDExtractionZone(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
    virtual void                        BeginPlay() override;

private:
    UFUNCTION()
    void                                OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void                                OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void                                NotifyGameMode(AActor* OtherActor, bool bInZone) const;
    void                                SpawnPointLaser();

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent>        ZoneSphere;

    UPROPERTY()
    TObjectPtr<UNiagaraSystem>          PointLaserNiagara;

    UPROPERTY()
    TObjectPtr<UNiagaraComponent>       PointLaserComponent;
};
