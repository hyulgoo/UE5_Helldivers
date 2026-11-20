// Fill out your copyright notice in the Description page of Project Settings.

#include "Stratagem/Deployable/HDSupplyPod.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Collision/HDCollision.h"
#include "Define/HDDefine.h"

namespace HDSupplyPodPrivate
{
    // 플레이어 캡슐과 겹치는 기본 줍기 반지름(cm). JSON 의 EffectRange 가 있으면 그 값
    constexpr float DefaultPickupRadius = 250.f;
}

AHDSupplyPod::AHDSupplyPod(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    using namespace HDSupplyPodPrivate;

    PrimaryActorTick.bCanEverTick = false;

    // 추출 지점(AHDExtractionZone::ZoneSphere)과 같은 프로필: Pawn 채널과 겹친다
    PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    SetRootComponent(PickupSphere);
    PickupSphere->SetCollisionProfileName(HDCOLLISION_PROFILE_TRIGGER);
    PickupSphere->SetGenerateOverlapEvents(true);
    PickupSphere->SetSphereRadius(DefaultPickupRadius);
    PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AHDSupplyPod::OnPickupBeginOverlap);

    // 엔진 Cube(100cm) x 0.6x0.6x0.9 = 60x60x90cm. 피벗이 가운데라 GA 가 지면 + 45 에 놓는다(시설 목표와 같은 방법)
    PodMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PodMesh"));
    PodMesh->SetupAttachment(RootComponent);
    PodMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.9f));
    PodMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshRef(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMeshRef.Succeeded())
    {
        PodMesh->SetStaticMesh(CubeMeshRef.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialRef(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterialRef.Succeeded())
    {
        PodMesh->SetMaterial(0, BasicMaterialRef.Object);
    }
}

void AHDSupplyPod::InitializePod(const float Lifetime, const float PickupRadius)
{
    if (PickupRadius > 0.f)
    {
        PickupSphere->SetSphereRadius(PickupRadius);
    }

    PodLifetime = Lifetime;
}

void AHDSupplyPod::BeginPlay()
{
    Super::BeginPlay();

    if (PodLifetime > 0.f)
    {
        SetLifeSpan(PodLifetime);
    }
}

void AHDSupplyPod::OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AHDCharacterPlayer* Player = Cast<AHDCharacterPlayer>(OtherActor);
    CONDITION_CHECK_WITHOUT_LOG(Player != nullptr);

    // 죽어 있는 등으로 채우지 못하면 상자를 남긴다
    const bool bResupplied = Player->Resupply();
    CONDITION_CHECK_WITHOUT_LOG(bResupplied);

    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] supply pod picked up"));
    Destroy();
}
