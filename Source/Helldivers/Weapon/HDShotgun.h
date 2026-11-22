// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HDWeapon.h"
#include "HDShotgun.generated.h"

// 주무기 산탄총(브리프 9). 무기 BP 를 새로 만들지 않고 기존 산탄총 메시·몽타주·탄 BP 를 생성자에서 잡는다.
// 칸은 부모 기본(주무기)이고, 한 번에 PelletCount 개를 흩뿌린다
UCLASS()
class HELLDIVERS_API AHDShotgun : public AHDWeapon
{
    GENERATED_BODY()

public:
    AHDShotgun(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // SK_AM_Shotgun 에는 노티파이가 하나도 없어 발사·재장전 이벤트를 캐릭터가 C++ 로 보낸다
    virtual bool                        SendsMontageEventsFromCode() const override { return true; }
};
