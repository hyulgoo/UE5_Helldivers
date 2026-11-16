// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/HDCharacterBase.h"
#include "HDCharacterNonPlayer.generated.h"

UCLASS()
class HELLDIVERS_API AHDCharacterNonPlayer : public AHDCharacterBase
{
	GENERATED_BODY()
	
public:
    AHDCharacterNonPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
    virtual void    BeginPlay() override final;
    virtual void    SetDead() override final;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHDAbilitySystemComponent>   NPCAbilitySystemComponent;
};
