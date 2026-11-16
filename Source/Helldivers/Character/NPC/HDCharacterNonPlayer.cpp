// Fill out your copyright notice in the Description page of Project Settings.

#include "HDCharacterNonPlayer.h"
#include "Component/HDAbilitySystemComponent.h"

AHDCharacterNonPlayer::AHDCharacterNonPlayer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NPCAbilitySystemComponent = CreateDefaultSubobject<UHDAbilitySystemComponent>(TEXT("HDAbilitySystem"));
}

void AHDCharacterNonPlayer::BeginPlay()
{
    Super::BeginPlay();

    SetAbilitySystemComponent(this, NPCAbilitySystemComponent);
}

void AHDCharacterNonPlayer::SetDead()
{
    Super::SetDead();

	const float RandomDeadEventDelayTime = FMath::FRandRange(DeadEventDelayTime, DeadEventDelayTime * 1.5f);
    FTimerHandle DeadTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(DeadTimerHandle, FTimerDelegate::CreateLambda([this]() { Destroy(); } ), RandomDeadEventDelayTime, false);
}
