// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "HDMovementStateComponent.generated.h"

UCLASS()
class HELLDIVERS_API UHDMovementStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	const EHDCharacterStanceState	GetStanceState() const;
	void							SetStanceState(const EHDCharacterStanceState NewState, const bool bForced = false);
	void							RestoreStanceState();

    const EHDCharacterMovementState GetMovementState() const;
    void                            SetMovementState(const EHDCharacterMovementState NewState);
    // 다음에 달리기가 끝날 때 애니메이션을 블렌딩 없이 바로 걷기로 바꾼다(사격으로 달리기가 끊길 때 UHDGA_Fire 가 부른다)
    void                            RequestInstantSprintExit();
    // 마지막 달리기 종료가 블렌딩 없이 끝났는지. UHDAnimInstance 가 달리기 → 걷기 블렌드 시간을 고른다
    bool                            IsLastSprintExitInstant() const;

	void							SetSpringArmDefaultZOffset(const float ZOffset);
	void							ChangeCameraZOffsetByCharacterMovementState(const EHDCharacterStanceState State);

private:
	EHDCharacterStanceState		    StanceState         = EHDCharacterStanceState::Idle;
	EHDCharacterStanceState		    PrevStanceState     = EHDCharacterStanceState::Idle;
	EHDCharacterMovementState       MovementState       = EHDCharacterMovementState::Idle;
    uint8                           bPendingInstantSprintExit : 1   = false;
    uint8                           bLastSprintExitInstant : 1      = false;
	
	float					        SpringArmZOffset    = 0.f;	
};
