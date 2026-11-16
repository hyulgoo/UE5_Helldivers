// Fill out your copyright notice in the Description page of Project Settings.

#include "Cheat/HDCheatManager.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Attribute/HDHealthAttributeSet.h"

void UHDCheatManager::HDDamageSelf(float Amount)
{
    APlayerController* PlayerController = GetPlayerController();
    APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Pawn);
    if (ASC == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDDamageSelf] No player pawn or ability system component"));
        return;
    }

    const float OldCurrentHealth = ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute());
    ASC->ApplyModToAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute(), EGameplayModOp::AddBase, -Amount);
    const float NewCurrentHealth = ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute());

    UE_LOG(LogTemp, Display, TEXT("[HDDamageSelf] CurrentHealth %.1f -> %.1f"), OldCurrentHealth, NewCurrentHealth);
}
