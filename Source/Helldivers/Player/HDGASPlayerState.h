// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "HDGASPlayerState.generated.h"

class UAbilitySystemComponent;

UCLASS()
class HELLDIVERS_API AHDGASPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
    AHDGASPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual UAbilitySystemComponent*    GetAbilitySystemComponent() const override final;

    template < class T >
    T*                                  GetAbilitySystemComponent() const { return Cast<T>(GetAbilitySystemComponent()); }

    // 처음 빙의한 캐릭터의 UHDStratagemComponent 가 채운다. 다시 강하해도 남은 횟수가 그대로다
    FHDStratagemLoadout&                GetStratagemLoadout() { return StratagemLoadout; }
    const FHDStratagemLoadout&          GetStratagemLoadout() const { return StratagemLoadout; }

protected:
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

private:
    FHDStratagemLoadout                 StratagemLoadout;
};
