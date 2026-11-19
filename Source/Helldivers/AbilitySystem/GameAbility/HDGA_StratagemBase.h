// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameAbility/HDGA_Base.h"
#include "HDGA_StratagemBase.generated.h"

class AHDBattleShip;
struct FHDStratagemEffectData;

// 전함 ASC 에서 GameplayEvent 로 발동하는 스트라타젬 효과의 공통 부분.
// 발동마다 인스턴스를 새로 만들고(InstancedPerExecution) 이름·표적을 인스턴스에 담으므로, 연속으로 던져도 앞 호출을 덮어쓰지 않는다.
// 이벤트: OptionalObject = 던진 AHDStratagem(이름·발동 지연), TargetData[0] = 표적 변환(AHDBattleShip::MakeActivationPayload)
UCLASS(Abstract)
class HELLDIVERS_API UHDGA_StratagemBase : public UHDGA_Base
{
	GENERATED_BODY()

public:
	UHDGA_StratagemBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void		ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	FName				GetStratagemName() const { return StratagemName; }
	const FTransform&	GetTargetTransform() const { return TargetTransform; }

protected:
	// 하위 클래스 생성자에서 한 번 부른다. 이 태그의 GameplayEvent 로 발동한다
	void				SetActivationEventTag(const FGameplayTag& EventTag);

	// 발동 지연이 끝나면 불린다. 하위 클래스는 효과를 시작하고 다 끝나면 FinishStratagem 을 부른다
	virtual void		ExecuteStratagem(const FHDStratagemEffectData& EffectData);

	void				FinishStratagem(const bool bWasCancelled);
	AHDBattleShip*		GetBattleShip() const;

private:
	UFUNCTION()
	void				OnActivationDelayFinished();

private:
	FName				StratagemName	= NAME_None;
	FTransform			TargetTransform	= FTransform::Identity;
};
