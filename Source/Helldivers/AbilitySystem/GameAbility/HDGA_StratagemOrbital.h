// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemBase.h"
#include "GameData/HDStratagemEffectData.h"
#include "HDGA_StratagemOrbital.generated.h"

// 궤도 포격(Event.Stratagem.Orbital). 전함 위치에서 표적 둘레로 투사체를 OrbitalDuration 동안 나눠 쏜다
UCLASS()
class HELLDIVERS_API UHDGA_StratagemOrbital : public UHDGA_StratagemBase
{
	GENERATED_BODY()

public:
	UHDGA_StratagemOrbital(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void			ExecuteStratagem(const FHDStratagemEffectData& EffectData) override;

private:
	// 간격이 0 이면 남은 투사체를 한 번에 쏜다
	UFUNCTION()
	void					FireShots();

	void					SpawnProjectile(const int32 Index);

private:
	FHDStratagemEffectData	OrbitalData		= FHDStratagemEffectData();
	FRandomStream			RandomStream;
	int32					ShotIndex		= 0;
	int32					ShotCount		= 0;
	float					ShotInterval	= 0.f;
};
