// Fill out your copyright notice in the Description page of Project Settings.

#include "Mission/HDObjectiveFacility.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "Game/HDMissionSettings.h"

AHDObjectiveFacility::AHDObjectiveFacility(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 엔진 Cube(100cm) x 4x4x6 = 400x400x600cm. 피벗이 가운데라 지면 + 300 에 두면 바닥이 지면에 닿는다
    FacilityMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacilityMesh"));
    SetRootComponent(FacilityMesh);
    FacilityMesh->SetRelativeScale3D(FVector(4.f, 4.f, 6.f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshRef(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMeshRef.Succeeded())
    {
        FacilityMesh->SetStaticMesh(CubeMeshRef.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialRef(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterialRef.Succeeded())
    {
        FacilityMesh->SetMaterial(0, BasicMaterialRef.Object);
    }

    // Pawn 형 + 모든 채널 Block: 총알(HDProjectile 채널)이 맞고, 폭발 스윕(ApplyExplode 의 ECC_Pawn)에 들고, 시야선(Visibility)을 막는다
    FacilityMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    FacilityMesh->SetCollisionObjectType(ECC_Pawn);
    FacilityMesh->SetCollisionResponseToAllChannels(ECR_Block);

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
}

UAbilitySystemComponent* AHDObjectiveFacility::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

FString AHDObjectiveFacility::GetMarkerLabel() const
{
    return FString::Printf(TEXT("FACILITY %d%%"), FMath::RoundToInt(GetHealthPercent()));
}

float AHDObjectiveFacility::GetHealthPercent() const
{
    NULL_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(AbilitySystemComponent, 0.f);

    const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute());
    if (MaxHealth <= 0.f)
    {
        return 0.f;
    }

    const float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute());
    return FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f) * 100.f;
}

void AHDObjectiveFacility::BeginPlay()
{
    Super::BeginPlay();

    NULL_CHECK(AbilitySystemComponent);

    AbilitySystemComponent->InitAbilityActorInfo(this, this);
    AbilitySystemComponent->AddSet<UHDHealthAttributeSet>();

    // 최대 체력을 먼저 정해야 현재 체력이 0 으로 잘리지 않는다(UHDHealthAttributeSet::PreAttributeChange)
    const float FacilityHealth = GetDefault<UHDMissionSettings>()->FacilityHealth;
    AbilitySystemComponent->SetNumericAttributeBase(UHDHealthAttributeSet::GetMaxHealthAttribute(), FacilityHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UHDHealthAttributeSet::GetCurrentHealthAttribute(), FacilityHealth);

    // 체력 0 이면 UHDHealthAttributeSet::PostAttributeChange 가 Trigger.Dead 를 붙인다
    AbilitySystemComponent->RegisterGameplayTagEvent(Tag_Trigger_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AHDObjectiveFacility::OnDeadTagChanged);
}

void AHDObjectiveFacility::OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (NewCount > 0)
    {
        HandleDestroyed();
    }
}

void AHDObjectiveFacility::HandleDestroyed()
{
    CONDITION_CHECK_WITHOUT_LOG(IsCompleted() == false);

    FGameplayAbilityHelper::ExcuteGameplayCue(Tag_GameplayCue_Explode_BombHeavy, GetActorLocation(), FVector::UpVector, AbilitySystemComponent);

    FacilityMesh->SetHiddenInGame(true);
    FacilityMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    CompleteObjective();
    SetLifeSpan(3.f);

    UE_LOG(LogTemp, Display, TEXT("[HDMission] facility %s destroyed"), *GetName());
}
