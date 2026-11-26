// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "HDGC_Base.generated.h"

class UParticleSystem;
class UNiagaraSystem;

UCLASS()
class HELLDIVERS_API UHDGC_Base : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
	
public:
	virtual bool						OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override final;

private:
	void								PlayNiagaraSystem(AActor* Target, const FGameplayCueParameters& Parameters) const;
	void								PlayParticleSystem(AActor* Target, const FGameplayCueParameters& Parameters) const;
	void								PlaySound(AActor* Target, const FGameplayCueParameters& Parameters) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue|Pacticle")
	uint8								bUseNiagara : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue|Pacticle", meta = (EditCondition = "!bUseNiagara"))
	TArray<TObjectPtr<UParticleSystem>>	ParticleSystems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue|Pacticle", meta = (EditCondition = "bUseNiagara"))
	TArray<TObjectPtr<UNiagaraSystem>>	NiagaraSystems;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue|Pacticle")
	float								ParticleScale = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue|Pacticle")
	// 0 이면 소리가 나지 않는다. BP 에서 바꾸지 않은 큐는 1 로 재생한다
	float								SoundVolume = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue|Sound")
	TArray<TObjectPtr<USoundBase>>		EffectSounds;
};