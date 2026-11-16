// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "HDCheatManager.generated.h"

UCLASS()
class HELLDIVERS_API UHDCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
    UFUNCTION(Exec)
    void        HDDamageSelf(float Amount = 50.f);
};
