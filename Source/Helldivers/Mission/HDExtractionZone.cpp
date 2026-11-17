// Fill out your copyright notice in the Description page of Project Settings.

#include "Mission/HDExtractionZone.h"
#include "Components/SphereComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Collision/HDCollision.h"
#include "Define/HDDefine.h"
#include "Game/HDGameMode.h"
#include "Game/HDMissionSettings.h"

namespace HDExtractionZonePrivate
{
    // AHDStratagem::SpawnPointLaser 와 같은 빔 매개변수. 보이는 시간은 사실상 끝나지 않게 둔다
    constexpr float PointLaserHeight = 80000.f;
    constexpr float PointLaserVisibleTime = 99999.f;
}

AHDExtractionZone::AHDExtractionZone(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;

    // 플레이어 무기 줍기 구(AHDWeapon::AreaSphere)와 같은 프로필: Pawn 채널과 겹친다. 반지름은 BeginPlay 에서 설정값으로
    ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneSphere"));
    SetRootComponent(ZoneSphere);
    ZoneSphere->SetCollisionProfileName(HDCOLLISION_PROFILE_TRIGGER);
    ZoneSphere->SetGenerateOverlapEvents(true);
    ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &AHDExtractionZone::OnZoneBeginOverlap);
    ZoneSphere->OnComponentEndOverlap.AddDynamic(this, &AHDExtractionZone::OnZoneEndOverlap);

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PointLaserRef(TEXT("/Game/Helldivers/Particle/NS_PointLaser.NS_PointLaser"));
    if (PointLaserRef.Succeeded())
    {
        PointLaserNiagara = PointLaserRef.Object;
    }
}

void AHDExtractionZone::BeginPlay()
{
    Super::BeginPlay();

    ZoneSphere->SetSphereRadius(GetDefault<UHDMissionSettings>()->ExtractionZoneRadius);

    SpawnPointLaser();
}

void AHDExtractionZone::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    NotifyGameMode(OtherActor, true);
}

void AHDExtractionZone::OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    NotifyGameMode(OtherActor, false);
}

void AHDExtractionZone::NotifyGameMode(AActor* OtherActor, bool bInZone) const
{
    CONDITION_CHECK_WITHOUT_LOG(Cast<AHDCharacterPlayer>(OtherActor) != nullptr);

    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    NULL_CHECK(HDGameMode);

    HDGameMode->NotifyPlayerInExtractionZone(bInZone);
}

void AHDExtractionZone::SpawnPointLaser()
{
    using namespace HDExtractionZonePrivate;

    NULL_CHECK(PointLaserNiagara);

    PointLaserComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PointLaserNiagara, GetActorLocation());
    NULL_CHECK(PointLaserComponent);

    PointLaserComponent->SetVectorParameter(TEXT("Beam Start"), FVector::ZeroVector);
    PointLaserComponent->SetVectorParameter(TEXT("Beam End"),   FVector::UpVector * PointLaserHeight);
    PointLaserComponent->SetFloatParameter(TEXT("VisibleTime"), PointLaserVisibleTime);
}
