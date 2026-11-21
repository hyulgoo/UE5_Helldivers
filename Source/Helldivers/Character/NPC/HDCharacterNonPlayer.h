// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/HDCharacterBase.h"
#include "Character/NPC/HDEnemyTypes.h"
#include "Weapon/HDArmorRules.h"
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

    // 부위별 장갑 등급과 피해 배율. 투사체가 맞을 때 읽는다(HDArmor::ResolveHit)
    const FHDArmorProfile&                  GetArmorProfile() const { return ArmorProfile; }

protected:
    virtual void                            BeginPlay() override final;
    virtual void                            SetDead() override final;
    virtual void                            InitializeAttributeValues() override;

public:
    FOnHDEnemyDied                          OnEnemyDied;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Enemy")
    FHDEnemyStats                           Stats;

    // 기본: 장갑 0, 머리만 2배(생성자). 자식이 생성자에서 바꾼다
    UPROPERTY(EditDefaultsOnly, Category = "Enemy")
    FHDArmorProfile                         ArmorProfile;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent>      EnemyWeaponMesh;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHDAbilitySystemComponent>   NPCAbilitySystemComponent;

    bool                                    bIsDead = false;
};
