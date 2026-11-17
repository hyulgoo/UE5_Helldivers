// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Mission/HDMissionObjective.h"
#include "HDObjectiveFacility.generated.h"

class UAbilitySystemComponent;
class UStaticMeshComponent;

// 파괴 목표 시설. 400x400x600cm 회색 상자에 GAS 체력(UHDHealthAttributeSet)을 둔다.
// 체력 0 -> Trigger.Dead 태그(UHDHealthAttributeSet) -> 폭발 큐, 숨김, 목표 완료, 3초 뒤 제거
UCLASS()
class HELLDIVERS_API AHDObjectiveFacility : public AHDMissionObjective, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
    AHDObjectiveFacility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual UAbilitySystemComponent*        GetAbilitySystemComponent() const override;

    // FACILITY <체력%>%
    virtual FString                         GetMarkerLabel() const override;

    // 0~100
    float                                   GetHealthPercent() const;

protected:
    virtual void                            BeginPlay() override;

private:
    void                                    OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
    void                                    HandleDestroyed();

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent>        FacilityMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UAbilitySystemComponent>     AbilitySystemComponent;
};
