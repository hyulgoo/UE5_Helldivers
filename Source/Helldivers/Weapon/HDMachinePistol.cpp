// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HDMachinePistol.h"

AHDMachinePistol::AHDMachinePistol(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 메시·몽타주·탄·탄피는 부모(권총) 값. 0.08초 간격 자동, 탄창 24발, 예비 탄창 4개(권총은 0.25·단발·15·4)
    FireDelay   = 0.08f;
    bIsAutoFire = true;

    AmmoData.Ammo           = 24;
    AmmoData.MaxAmmo        = 24;
    AmmoData.Capacity       = 4;
    AmmoData.MaxCapacity    = 4;

    ArmorPenetration = 1;
}
