// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Progression/HDProfileTypes.h"
#include "HDSamplePickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UStaticMesh;

// 맵에 놓인 샘플 하나, 또는 사망 때 떨군 묶음. 모양은 엔진 기본 도형(일반 구·희귀 원뿔·최상급 원기둥·묶음 큰 구).
// 살아 있는 플레이어 캐릭터가 줍기 구(HDTrigger)에 들어오면 플레이어스테이트 소지에 더하고 사라진다
UCLASS()
class HELLDIVERS_API AHDSamplePickup : public AActor
{
	GENERATED_BODY()

public:
    AHDSamplePickup(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // FinishSpawning 전에 부른다. 맵에 놓는 샘플 하나
    void                                InitializeSample(EHDSampleType Type);
    // FinishSpawning 전에 부른다. 사망 때 떨군 묶음(지닌 것 전부)
    void                                InitializeBundle(const FHDSampleCounts& InContents);
    const FHDSampleCounts&              GetContents() const { return Contents; }
    bool                                IsDroppedBundle() const { return bDroppedBundle; }
    // 표식 글자: "SAMPLE" / 묶음 "SAMPLES"
    FString                             GetMarkerLabel() const;

private:
    // 메시·크기를 바꾸고 줍기 반지름을 설정 값(SamplePickupRadius)으로
    void                                ApplyShape(UStaticMesh* Mesh, const FVector& Scale);

    UFUNCTION()
    void                                OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent>        PickupSphere;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent>    SampleMesh;

    // 세 종류 도형과 묶음용 구(생성자에서 HDSamples::GetMeshPath 로 찾는다)
    UPROPERTY()
    TObjectPtr<UStaticMesh>             CommonMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh>             RareMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh>             SuperMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh>             BundleMesh;

    FHDSampleCounts                     Contents;
    bool                                bDroppedBundle = false;
};
