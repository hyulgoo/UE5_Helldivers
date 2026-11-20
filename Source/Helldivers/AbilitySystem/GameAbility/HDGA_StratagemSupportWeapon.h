// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemBase.h"
#include "HDGA_StratagemSupportWeapon.generated.h"

// 지원 무기 투하(Event.Stratagem.SupportWeapon). 발동마다 표적 위치에 행의 SupportWeaponClass 무기를 하나 놓는다. 플레이어가 주우면 지원 무기 칸에 들어간다
UCLASS()
class HELLDIVERS_API UHDGA_StratagemSupportWeapon : public UHDGA_StratagemBase
{
	GENERATED_BODY()

public:
	UHDGA_StratagemSupportWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void	ExecuteStratagem(const FHDStratagemEffectData& EffectData) override;
};