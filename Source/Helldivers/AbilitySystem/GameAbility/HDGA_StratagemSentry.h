// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemBase.h"
#include "HDGA_StratagemSentry.generated.h"

// 기관총 포탑(Event.Stratagem.Sentry). 발동마다 표적 위치에 AHDSentryTurret 를 하나 놓는다. 포탑은 수명이나 최대 발사 수에 닿으면 사라진다
UCLASS()
class HELLDIVERS_API UHDGA_StratagemSentry : public UHDGA_StratagemBase
{
	GENERATED_BODY()

public:
	UHDGA_StratagemSentry(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void	ExecuteStratagem(const FHDStratagemEffectData& EffectData) override;
};
