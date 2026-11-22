// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HDSecondaryWeapon.h"
#include "HDMachinePistol.generated.h"

// 보조무기 기관 권총(브리프 9). 권총(AHDSecondaryWeapon)의 메시·몽타주·탄을 그대로 쓰고 수치만 바꾼다
UCLASS()
class HELLDIVERS_API AHDMachinePistol : public AHDSecondaryWeapon
{
    GENERATED_BODY()

public:
    AHDMachinePistol(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
