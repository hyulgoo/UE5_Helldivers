// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemBase.h"
#include "HDGA_StratagemResupply.generated.h"

// 보급(Event.Stratagem.Resupply). 발동마다 표적 위치에 AHDSupplyPod 를 하나 놓는다. 상자는 플레이어가 주우면 사라진다
UCLASS()
class HELLDIVERS_API UHDGA_StratagemResupply : public UHDGA_StratagemBase
{
	GENERATED_BODY()

public:
	UHDGA_StratagemResupply(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void	ExecuteStratagem(const FHDStratagemEffectData& EffectData) override;
};
