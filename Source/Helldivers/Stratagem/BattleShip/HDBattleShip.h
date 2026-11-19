// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "HDBattleShip.generated.h"

class AHDEagleFighter;
class AHDProjectileBase;
class AHDStratagem;
class UGameplayAbility;
class USphereComponent;
struct FHDStratagemEffectData;
enum class EHDStratagemProjectile : uint8;

UCLASS()
class HELLDIVERS_API AHDBattleShip : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
    AHDBattleShip(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual UAbilitySystemComponent*    GetAbilitySystemComponent() const override;

    // 표적에 떨어진 스트라타젬의 효과 GA 를 GameplayEvent 로 깨운다(이벤트 태그는 HDStratagemPattern::GetActivationEventTag)
	void							    ActivateStratagem(const AHDStratagem* Beacon, const FTransform& TargetTransform);

    // 전함 GA 를 깨우는 이벤트 데이터. OptionalObject = Beacon, TargetData[0] = 표적 변환
    static FGameplayEventData           MakeActivationPayload(const FGameplayTag& EventTag, const AHDStratagem* Beacon, const FTransform& TargetTransform);

	const FHDStratagemEffectData*	    FindStratagemEffectData(const FName StratagemName) const;
    TSubclassOf<AHDProjectileBase>      GetProjectileClass(const EHDStratagemProjectile ProjectileType) const;
    TSubclassOf<AHDEagleFighter>        GetEagleFighterClass() const { return EagleFighterClass; }

protected:
    // 스트라타젬 효과 GA 를 준다. 스폰 중에 불리므로 BeginPlay 가 없는 월드(자동 테스트)에서도 GA 가 있다
    virtual void                        PostInitializeComponents() override;
	virtual void					    BeginPlay() override final;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "BattleShip")
    TObjectPtr<UStaticMeshComponent>    BattleShipMesh;

    UPROPERTY(EditDefaultsOnly, Category = "BattleShip")
	TObjectPtr<USphereComponent>		CollisionSphere;

    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    // 전함 ASC 에 주는 스트라타젬 효과 GA(궤도·이글)
    UPROPERTY()
    TArray<TSubclassOf<UGameplayAbility>> StratagemAbilities;

	UPROPERTY(EditAnywhere, Category = "Stratagem|Eagle")
    TSubclassOf<AHDEagleFighter>		EagleFighterClass;

    UPROPERTY(EditAnywhere, Category = "Stratagem|Projectile")
    TSubclassOf<AHDProjectileBase>		ProjectileBombClass;

    UPROPERTY(EditAnywhere, Category = "Stratagem|Projectile")
    TSubclassOf<AHDProjectileBase>		ProjectileBulletClass;

    // StratagemEffectData
    UPROPERTY(EditAnywhere, Category = "Stratagem|Data")
    TObjectPtr<UDataTable>				StratagemEffectDataTable;
};
