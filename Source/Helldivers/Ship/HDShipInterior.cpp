// Fill out your copyright notice in the Description page of Project Settings.

#include "Ship/HDShipInterior.h"
#include "Ship/HDShipHubRules.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Define/HDDefine.h"

namespace HDShipInteriorLayout
{
    // 색(BasicShapeMaterial 의 벡터 매개변수 "Color")
    const FName         ColorParameterName(TEXT("Color"));
    const FLinearColor  FloorColor(0.04f, 0.04f, 0.05f);
    const FLinearColor  CeilingColor(0.08f, 0.08f, 0.09f);
    const FLinearColor  WallColor(0.18f, 0.19f, 0.21f);
    const FLinearColor  RailColor(0.1f, 0.1f, 0.1f);
    const FLinearColor  TableColor(0.22f, 0.22f, 0.24f);
    const FLinearColor  HologramColor(0.1f, 0.6f, 1.f);
    const FLinearColor  ConsoleColor(0.12f, 0.12f, 0.14f);
    const FLinearColor  ScreenColor(0.2f, 0.8f, 1.f);
    const FLinearColor  PodColor(0.5f, 0.5f, 0.52f);
    const FLinearColor  PodTipColor(0.9f, 0.7f, 0.1f);

    // 방 껍데기(바닥 윗면 Z 0, 안쪽 40×24m·높이 7m, 두께 20)
    const FVector       FloorLocation(0.f, 0.f, -10.f);
    const FVector       CeilingLocation(0.f, 0.f, 710.f);
    const FVector       FloorSize(4000.f, 2400.f, 20.f);
    const FVector       BackWallLocation(-2010.f, 0.f, 350.f);
    const FVector       BackWallSize(20.f, 2400.f, 700.f);
    const FVector       SideWallLocation(0.f, 1210.f, 350.f);
    const FVector       SideWallSize(4000.f, 20.f, 700.f);
    // 앞쪽(+X)은 난간만 보이고 하늘이 보인다. 그 위는 보이지 않는 막이(떨어지지 않게)
    const FVector       RailLocation(2010.f, 0.f, 50.f);
    const FVector       RailSize(20.f, 2400.f, 100.f);
    const FVector       WindowBlockerLocation(2010.f, 0.f, 400.f);
    const FVector       WindowBlockerSize(20.f, 2400.f, 600.f);

    // 작전 테이블(단말기 위치 기준)
    constexpr float     TableZ = 50.f;
    const FVector       TableSize(300.f, 300.f, 100.f);
    constexpr float     HologramZ = 220.f;
    const FVector       HologramSize(160.f, 160.f, 160.f);

    // 함선 관리·무기고 콘솔(단말기 위치 기준, 앞면 = +X 를 LocalYaw 로 돌린다)
    constexpr float     ConsoleBodyZ = 60.f;
    const FVector       ConsoleBodySize(60.f, 120.f, 120.f);
    const FVector       ScreenOffset(25.f, 0.f, 160.f);
    const FVector       ScreenSize(10.f, 100.f, 70.f);

    // 헬포드 베이(장식)
    const FVector       PodLocation(1500.f, 0.f, 150.f);
    const FVector       PodSize(200.f, 200.f, 300.f);
    constexpr float     PodTipZ = 350.f;
    const FVector       PodTipSize(200.f, 200.f, 100.f);

    // 점광 4개: (±LightX, ±LightY, LightZ)
    constexpr float     LightX = 1000.f;
    constexpr float     LightY = 600.f;
    constexpr float     LightZ = 600.f;
    constexpr float     LightIntensity = 5000.f;
    constexpr float     LightAttenuationRadius = 2500.f;
}

AHDShipInterior::AHDShipInterior(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshRef(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMeshRef.Succeeded())
    {
        CubeMesh = CubeMeshRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshRef(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMeshRef.Succeeded())
    {
        CylinderMesh = CylinderMeshRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshRef(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMeshRef.Succeeded())
    {
        SphereMesh = SphereMeshRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshRef(TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (ConeMeshRef.Succeeded())
    {
        ConeMesh = ConeMeshRef.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialRef(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterialRef.Succeeded())
    {
        BasicShapeMaterial = BasicMaterialRef.Object;
    }
}

FTransform AHDShipInterior::GetCrewSpawnTransform() const
{
    const FTransform& ActorTransform = GetActorTransform();
    const FVector WorldLocation = ActorTransform.TransformPosition(HDShipHub::GetCrewSpawnLocation());
    const FRotator WorldRotation(0.f, GetActorRotation().Yaw + HDShipHub::GetCrewSpawnYaw(), 0.f);
    return FTransform(WorldRotation, WorldLocation);
}

FVector AHDShipInterior::ToLocal(const FVector& WorldLocation) const
{
    return GetActorTransform().InverseTransformPosition(WorldLocation);
}

FVector AHDShipInterior::GetTerminalWorldLocation(int32 Index) const
{
    const TArray<FHDShipTerminalInfo>& Terminals = HDShipHub::GetTerminals();
    if (Terminals.IsValidIndex(Index) == false)
    {
        return GetActorLocation();
    }

    return GetActorTransform().TransformPosition(Terminals[Index].LocalLocation);
}

void AHDShipInterior::BeginPlay()
{
    using namespace HDShipInteriorLayout;

    Super::BeginPlay();

    // 방 껍데기
    AddPiece(CubeMesh, FloorLocation, FRotator::ZeroRotator, FloorSize, FloorColor, true);
    AddPiece(CubeMesh, CeilingLocation, FRotator::ZeroRotator, FloorSize, CeilingColor, true);
    AddPiece(CubeMesh, BackWallLocation, FRotator::ZeroRotator, BackWallSize, WallColor, true);
    AddPiece(CubeMesh, SideWallLocation, FRotator::ZeroRotator, SideWallSize, WallColor, true);
    AddPiece(CubeMesh, FVector(SideWallLocation.X, -SideWallLocation.Y, SideWallLocation.Z), FRotator::ZeroRotator, SideWallSize, WallColor, true);
    AddPiece(CubeMesh, RailLocation, FRotator::ZeroRotator, RailSize, RailColor, true);

    UStaticMeshComponent* WindowBlocker = AddPiece(CubeMesh, WindowBlockerLocation, FRotator::ZeroRotator, WindowBlockerSize, RailColor, true);
    if (WindowBlocker)
    {
        WindowBlocker->SetVisibility(false);
    }

    // 단말기
    for (const FHDShipTerminalInfo& Terminal : HDShipHub::GetTerminals())
    {
        const FVector Base = Terminal.LocalLocation;
        if (Terminal.Terminal == EHDShipTerminal::WarTable)
        {
            AddPiece(CubeMesh, Base + FVector(0.f, 0.f, TableZ), FRotator::ZeroRotator, TableSize, TableColor, true);
            AddPiece(SphereMesh, Base + FVector(0.f, 0.f, HologramZ), FRotator::ZeroRotator, HologramSize, HologramColor, false);
            continue;
        }

        const FRotator Facing(0.f, Terminal.LocalYaw, 0.f);
        AddPiece(CubeMesh, Base + FVector(0.f, 0.f, ConsoleBodyZ), Facing, ConsoleBodySize, ConsoleColor, true);
        AddPiece(CubeMesh, Base + Facing.RotateVector(ScreenOffset), Facing, ScreenSize, ScreenColor, true);
    }

    // 헬포드 베이(장식, 상호작용 없음)
    AddPiece(CylinderMesh, PodLocation, FRotator::ZeroRotator, PodSize, PodColor, true);
    AddPiece(ConeMesh, FVector(PodLocation.X, PodLocation.Y, PodTipZ), FRotator::ZeroRotator, PodTipSize, PodTipColor, true);

    for (const float SignX : { -1.f, 1.f })
    {
        for (const float SignY : { -1.f, 1.f })
        {
            AddLight(FVector(SignX * LightX, SignY * LightY, LightZ));
        }
    }

    UE_LOG(LogTemp, Display, TEXT("[HDShip] interior built at %s pieces=%d terminals=%d labels=0"), *GetActorLocation().ToString(), Pieces.Num(), HDShipHub::GetTerminals().Num());
}

UStaticMeshComponent* AHDShipInterior::AddPiece(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& SizeCm, const FLinearColor& Color, bool bCollision)
{
    NULL_CHECK_WITH_RETURNTYPE(Mesh, nullptr);
    NULL_CHECK_WITH_RETURNTYPE(Root, nullptr);

    UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
    NULL_CHECK_WITH_RETURNTYPE(Piece, nullptr);

    // 기본 도형은 100 단위이고 가운데가 원점이라 크기(cm) / 100 = 스케일
    Piece->SetupAttachment(Root);
    Piece->SetStaticMesh(Mesh);
    Piece->SetRelativeLocationAndRotation(Location, Rotation);
    Piece->SetRelativeScale3D(SizeCm / 100.f);
    Piece->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);

    if (BasicShapeMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
        if (Material)
        {
            Material->SetVectorParameterValue(HDShipInteriorLayout::ColorParameterName, Color);
            Piece->SetMaterial(0, Material);
        }
    }

    Piece->RegisterComponent();
    Pieces.Add(Piece);
    return Piece;
}

void AHDShipInterior::AddLight(const FVector& Location)
{
    using namespace HDShipInteriorLayout;

    NULL_CHECK(Root);

    UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
    NULL_CHECK(Light);

    Light->SetupAttachment(Root);
    Light->SetRelativeLocation(Location);
    Light->SetIntensity(LightIntensity);
    Light->SetAttenuationRadius(LightAttenuationRadius);
    Light->SetCastShadows(false);
    Light->RegisterComponent();
}
