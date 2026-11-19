// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemBase.h"
#include "HDGA_StratagemEagle.generated.h"

// 이글 공습(Event.Stratagem.Eagle). 발동마다 AHDEagleFighter 를 새로 띄운다. 전투기는 복귀가 끝나면 스스로 사라진다
UCLASS()
class HELLDIVERS_API UHDGA_StratagemEagle : public UHDGA_StratagemBase
{
	GENERATED_BODY()

public:
	UHDGA_StratagemEagle(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void	ExecuteStratagem(const FHDStratagemEffectData& EffectData) override;
};
