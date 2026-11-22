// Fill out your copyright notice in the Description page of Project Settings.

#include "Mission/HDSamplePickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Player/HDGASPlayerState.h"
#include "Collision/HDCollision.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "Progression/HDProgressionSettings.h"
#include "Progression/HDSampleRules.h"

namespace HDSamplePickupPrivate
{
    // Initialize* 전의 줍기 반지름(cm). Initialize* 가 설정 SamplePickupRadius 로 바꾼다
    constexpr float DefaultPickupRadius = 200.f;

    // 엔진 기본 도형(100cm, 피벗 가운데)에 곱하는 크기
    const FVector   CommonScale(0.4f);
    const FVector   RareScale(0.5f);
    const FVector   SuperScale(0.5f, 0.5f, 0.6f);
    const FVector   BundleScale(0.7f);
}

AHDSamplePickup::AHDSamplePickup(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    using namespace HDSamplePickupPrivate;

    PrimaryActorTick.bCanEverTick = false;

    // 보급 상자(AHDSupplyPod)·추출 지점과 같은 프로필: Pawn 채널과 겹친다
    PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    SetRootComponent(PickupSphere);
    PickupSphere->SetCollisionProfileName(HDCOLLISION_PROFILE_TRIGGER);
    PickupSphere->SetGenerateOverlapEvents(true);
    PickupSphere->SetSphereRadius(DefaultPickupRadius);
    PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AHDSamplePickup::OnPickupBeginOverlap);

    SampleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SampleMesh"));
    SampleMesh->SetupAttachment(RootComponent);
    SampleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CommonMeshRef(HDSamples::GetMeshPath(EHDSampleType::Common));
    if (CommonMeshRef.Succeeded())
    {
        CommonMesh = CommonMeshRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> RareMeshRef(HDSamples::GetMeshPath(EHDSampleType::Rare));
    if (RareMeshRef.Succeeded())
    {
        RareMesh = RareMeshRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SuperMeshRef(HDSamples::GetMeshPath(EHDSampleType::Super));
    if (SuperMeshRef.Succeeded())
    {
        SuperMesh = SuperMeshRef.Object;
    }

    // 묶음은 일반과 같은 구를 더 크게
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BundleMeshRef(HDSamples::GetMeshPath(EHDSampleType::Common));
    if (BundleMeshRef.Succeeded())
    {
        BundleMesh = BundleMeshRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialRef(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterialRef.Succeeded())
    {
        SampleMesh->SetMaterial(0, BasicMaterialRef.Object);
    }

    // Initialize* 를 부르지 않아도 보이게 일반 모양으로 둔다
    SampleMesh->SetStaticMesh(CommonMesh);
    SampleMesh->SetRelativeScale3D(CommonScale);
}

void AHDSamplePickup::InitializeSample(EHDSampleType Type)
{
    using namespace HDSamplePickupPrivate;

    Contents = FHDSampleCounts();
    Contents.Add(Type, 1);
    bDroppedBundle = false;

    switch (Type)
    {
    case EHDSampleType::Rare:   ApplyShape(RareMesh, RareScale);        break;
    case EHDSampleType::Super:  ApplyShape(SuperMesh, SuperScale);      break;
    default:                    ApplyShape(CommonMesh, CommonScale);    break;
    }
}

void AHDSamplePickup::InitializeBundle(const FHDSampleCounts& InContents)
{
    using namespace HDSamplePickupPrivate;

    Contents = InContents;
    bDroppedBundle = true;

    ApplyShape(BundleMesh, BundleScale);
}

FString AHDSamplePickup::GetMarkerLabel() const
{
    return bDroppedBundle ? TEXT("SAMPLES") : TEXT("SAMPLE");
}

void AHDSamplePickup::ApplyShape(UStaticMesh* Mesh, const FVector& Scale)
{
    SampleMesh->SetStaticMesh(Mesh);
    SampleMesh->SetRelativeScale3D(Scale);

    const float PickupRadius = GetDefault<UHDProgressionSettings>()->SamplePickupRadius;
    if (PickupRadius > 0.f)
    {
        PickupSphere->SetSphereRadius(PickupRadius);
    }
}

void AHDSamplePickup::OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 한 캐릭터의 여러 컴포넌트가 같은 갱신에서 겹쳐도 한 번만 줍는다
    CONDITION_CHECK_WITHOUT_LOG(IsActorBeingDestroyed() == false);

    AHDCharacterPlayer* Player = Cast<AHDCharacterPlayer>(OtherActor);
    CONDITION_CHECK_WITHOUT_LOG(Player != nullptr);

    // 죽은 몸(묶음을 떨군 그 시체 포함)은 줍지 않는다
    const UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Player);
    if (ASC && ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead))
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSample] pickup refused (dead)"));
        return;
    }

    // 증원 뒤 빙의가 풀린 시체 등은 플레이어스테이트가 없다
    AHDGASPlayerState* PlayerState = Player->GetPlayerState<AHDGASPlayerState>();
    NULL_CHECK_WITHOUT_LOG(PlayerState);

    PlayerState->AddCarriedSamples(Contents);

    UE_LOG(LogTemp, Display, TEXT("[HDSample] picked up %s bundle=%d carried=%s"),
        *HDSamples::Describe(Contents),
        bDroppedBundle ? 1 : 0,
        *HDSamples::Describe(PlayerState->GetCarriedSamples()));

    Destroy();
}
