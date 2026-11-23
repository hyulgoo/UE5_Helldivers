// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HDShipInterior.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

// 걸어 다니는 함선 방. 엔진 기본 도형과 점광으로 BeginPlay 에서 짓는다(바닥·천장·벽·창 난간·단말기 셋·헬포드 베이).
// 방 기준 좌표는 HDShipHub(바닥 윗면 가운데가 원점, +X 가 창 쪽)와 같다. AHDGameMode::EnterShip 이 스폰하고 출격 때 지운다
UCLASS()
class HELLDIVERS_API AHDShipInterior : public AActor
{
	GENERATED_BODY()

public:
    AHDShipInterior(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 월드 기준 함선 속 헬다이버 스폰 위치·방향(HDShipHub::GetCrewSpawnLocation·GetCrewSpawnYaw)
    FTransform                              GetCrewSpawnTransform() const;
    // 월드 위치 → 방 기준(액터 변환의 역)
    FVector                                 ToLocal(const FVector& WorldLocation) const;
    // HDShipHub::GetTerminals()[Index] 의 월드 위치. 범위 밖이면 액터 위치
    FVector                                 GetTerminalWorldLocation(int32 Index) const;

    // 작전 테이블 지도: 함선 아래에서 맵을 내려다본 정사영 1회 캡처. 설정 bWarTableMapCapture 가 거짓이면 캡처하지 않고 처음 한 번 schematic 로그
    void                                    RequestMapCapture();
    // 캡처한 다음 프레임부터 non-null
    UTextureRenderTarget2D*                 GetMapTexture() const;
    // 지도 중심(방 위치 − ShipInteriorOffset)의 XY 와 반 범위(HDWarTableMap::ComputeHalfExtent)
    FVector2D                               GetMapCenterXY() const { return MapCenterXY; }
    float                                   GetMapHalfExtent() const { return MapHalfExtent; }

protected:
    virtual void                            BeginPlay() override;

private:
    // 조각 하나: 가운데 Location(방 기준), 크기 SizeCm(기본 도형 100 단위라 / 100 = 스케일), 색, 충돌(BlockAll / NoCollision)
    UStaticMeshComponent*                   AddPiece(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& SizeCm, const FLinearColor& Color, bool bCollision);
    void                                    AddLight(const FVector& Location);
    // 설정이 참일 때만 캡처 컴포넌트와 렌더 타깃을 만든다
    void                                    CreateMapCapture();

private:
    UPROPERTY()
    TObjectPtr<USceneComponent>             Root;

    UPROPERTY()
    TObjectPtr<UStaticMesh>                 CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh>                 CylinderMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh>                 SphereMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh>                 ConeMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface>          BasicShapeMaterial;

    // BeginPlay 가 만든 조각(로그의 pieces 수)
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Pieces;

    UPROPERTY()
    TObjectPtr<USceneCaptureComponent2D>    MapCapture;

    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D>      MapTarget;

    FVector2D                               MapCenterXY         = FVector2D::ZeroVector;
    float                                   MapHalfExtent       = 0.f;
    // 마지막 CaptureScene 을 부른 GFrameCounter. 0 이면 아직 없음
    uint64                                  MapCaptureFrame     = 0;
    bool                                    bMapSchematicLogged = false;
};
