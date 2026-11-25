// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagContainer.h"
#include "Components/TimelineComponent.h"
#include "HDHellpod.generated.h"

class UInputComponent;
class UInputAction;
class UInputMappingContext;
class AHDCharacterPlayer;
class UBoxComponent;
class USpringArmComponent;
class UCameraComponent;
struct FInputActionValue;

UCLASS()
class HELLDIVERS_API AHDHellpod : public APawn
{
	GENERATED_BODY()

public:
    AHDHellpod(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 착지하면 스폰하는 캐릭터 클래스(BP 값 BP_CharacterPlayer). 함선 속 헬다이버도 이 클래스로 스폰한다(AHDGameMode::EnterShipInterior)
    TSubclassOf<AHDCharacterPlayer>     GetCharacterClass() const { return CharacterClass; }

protected:
	virtual void						BeginPlay() override;
	virtual void						Tick(float DeltaTime) override;
	virtual void						SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void								OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	void								SpawnCharacter();

	UFUNCTION()
	void								OnSpawnTimelineUpdate(const float Value);

	UFUNCTION()
	void								SpawnCharacterEnd();
	void								SetSpawnTimeline();

	void								MoveHellpod(const FInputActionValue& Value);
	void								RotateHellpodByCurrentImpulse(const float DeltaTime);
	void								ShakeCamera(const float Scale);

	// 착지 연출: 카메라를 강하 시점에 고정 → 포드 뒤 넓은 시점으로 내려앉기 → 캡 사출·캐릭터 상승 → 캐릭터 카메라로 블렌드 → 조작 반환
	void								FreezeLandingCamera();
	void								ComputeRevealCamera();
	void								TickLanding(const float DeltaTime);
	void								UpdateLandingCamera();
	void								EjectCap();
	void								ReturnControl();
	float								GetCameraShakeWeight() const;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod")
	TObjectPtr<UStaticMeshComponent>	HellpodMesh;

	// 포드 꼭대기 해치. 착지 뒤 CapEjectDelay 에 떼어 물리로 튕겨 낸다
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod")
	TObjectPtr<UStaticMeshComponent>	HellpodCap;

    UPROPERTY(EditDefaultsOnly, Category = "Hellpod")
	TObjectPtr<UBoxComponent>			CollisionBox;
	
	// Input
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Input")
	TObjectPtr<UInputAction>			MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Input")
	TObjectPtr<UInputMappingContext>	InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Input")
    float								MaxMoveSpeed;
	
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Input")
    float								FallSpeed;

	FVector2D							CurrentInput                = FVector2D::ZeroVector;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Input")
	float								MaxPitchAngle;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Input")
	float								MaxRollAngle;

	FRotator							MeshDefaultRelativeRotation = FRotator::ZeroRotator;
	uint8								bIsLanded : 1               = false;

	// Camera
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Camera")
	TObjectPtr<USpringArmComponent>		CameraBoom;
	
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Camera")
	TObjectPtr<UCameraComponent>		FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Hellpod|Camera")
	TObjectPtr<UCameraShakeSourceComponent> CameraShakeSource;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Camera")
	TSubclassOf<UCameraShakeBase>		FallCameraShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Camera")
	float								CameraShakeScaleWhenFalling;
	
	// Spawn
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Spawn")
    TSubclassOf<AHDCharacterPlayer>		CharacterClass;
	
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Spawn")
	float								SpawnTime;
	
	UPROPERTY();
	TObjectPtr<AHDCharacterPlayer>		SpawnedCharacter;

    FTimeline							SpawnCharacterTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Spawn")
	TObjectPtr<UCurveFloat>				SpawnCurveFloat;

	float								RiseStartZ                  = 0.f;
	float								RiseEndZ                    = 0.f;

	//Effect
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Effect")
	FGameplayTag						ImpactTag;

	// 캡이 튕겨 나갈 때 낼 큐(소리·연기). 비어 있으면 건너뛴다
    UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Effect")
	FGameplayTag						CapEjectTag;

	// Landing - 충격 흔들림(카메라에 직접 더한다)
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ImpactShakeTime             = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ImpactShakeAngle            = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ImpactShakeOffset           = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ImpactShakeFrequency        = 18.f;

	// 충격 순간 카메라가 아래로 덜컥 내려갔다가 흔들림과 함께 돌아오는 거리(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ImpactDip                   = 25.f;

	// 캡이 튕길 때 다시 주는 작은 흔들림의 세기(충격 대비 비율)와 길이
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapEjectShakeScale          = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapEjectShakeTime           = 0.3f;

	// Landing - 강하 시점에서 포드 뒤 넓은 시점으로 내려앉기
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								RevealTime                  = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								RevealDistance              = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								RevealPitch                 = -15.f;

	// 카메라가 바라보는 점: 포드 루트에서 위로 이만큼(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								RevealFocusHeight           = 110.f;

	// 숄더 뷰처럼 오른쪽으로 비키는 거리(cm). 캐릭터 카메라의 SocketOffset.Y 와 같은 쪽
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								RevealSideOffset            = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CameraProbeMargin           = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CameraMinDistance           = 150.f;

	// Landing - 캐릭터 카메라로 넘어가기
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ViewBlendDelay              = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ViewBlendTime               = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ViewBlendExp                = 2.f;

	// 캐릭터 카메라가 처음 볼 피치. 요는 강하 때 보던 방향을 이어 받는다
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								ArrivalPitch                = -10.f;

	// Landing - 캡 사출. 캐릭터 상승도 이때 시작한다
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapEjectDelay               = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapEjectUpSpeed             = 900.f;

	// 카메라 반대쪽(앞)으로 날아가게 주는 수평 속도. 플레이어가 날아가는 캡을 보게 된다
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapEjectSideSpeed           = 450.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapEjectSpreadDeg           = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapEjectSpinDeg             = 900.f;

	// 원판이라 모서리로 서서 비탈을 멀리 굴러가므로 감쇠로 금방 눕힌다
	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapLinearDamping            = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Hellpod|Landing")
	float								CapAngularDamping           = 3.f;

	TWeakObjectPtr<APlayerController>	LandingController;
	FTransform							LandingCameraStart;
	FVector								RevealLocation              = FVector::ZeroVector;
	FRotator							RevealRotation              = FRotator::ZeroRotator;
	float								LandingElapsed              = 0.f;
	float								ControlReturnAt             = 0.f;
	uint8								bLandingActive : 1          = false;
	uint8								bCapEjected : 1             = false;
	uint8								bViewBlendStarted : 1       = false;
};
