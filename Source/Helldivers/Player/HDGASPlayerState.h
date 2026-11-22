// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "Progression/HDProfileTypes.h"
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

    // 이번 미션에서 주운 샘플. 증원돼도 남고, 사망하면 게임모드가 묶음으로 꺼낸다
    const FHDSampleCounts&              GetCarriedSamples() const { return CarriedSamples; }
    void                                AddCarriedSamples(const FHDSampleCounts& Samples);   // 종류별로 0 미만이 되지 않게
    // 지닌 것을 돌려주고 비운다
    FHDSampleCounts                     TakeCarriedSamples();

protected:
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

private:
    FHDStratagemLoadout                 StratagemLoadout;

    // 플레이어스테이트는 맵을 다시 열면 새로 생기므로 미션마다 0 에서 시작한다
    FHDSampleCounts                     CarriedSamples;
};
