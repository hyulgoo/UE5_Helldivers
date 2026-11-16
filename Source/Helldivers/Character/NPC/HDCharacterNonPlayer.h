// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/HDCharacterBase.h"
#include "Character/NPC/HDEnemyTypes.h"
#include "HDCharacterNonPlayer.generated.h"

class AHDCharacterNonPlayer;
class USkeletalMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHDEnemyDied, AHDCharacterNonPlayer*);

UCLASS()
class HELLDIVERS_API AHDCharacterNonPlayer : public AHDCharacterBase
{
	GENERATED_BODY()

public:
    AHDCharacterNonPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    const FHDEnemyStats&                    GetEnemyStats() const { return Stats; }
    bool                                    IsDead() const { return bIsDead; }

protected:
    virtual void                            BeginPlay() override final;
    virtual void                            SetDead() override final;
    virtual void                            InitializeAttributeValues() override;

public:
    FOnHDEnemyDied                          OnEnemyDied;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Enemy")
    FHDEnemyStats                           Stats;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent>      EnemyWeaponMesh;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHDAbilitySystemComponent>   NPCAbilitySystemComponent;

    bool                                    bIsDead = false;
};
