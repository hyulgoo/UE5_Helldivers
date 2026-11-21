// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HDWeapon.h"
#include "HDSecondaryWeapon.generated.h"

// 보조무기(권총). 강하할 때마다 집어넣은 채로 지급한다. 무기 BP 를 새로 만들지 않고 기존 권총 메시·몽타주·탄 BP 를 생성자에서 잡는다
UCLASS()
class HELLDIVERS_API AHDSecondaryWeapon : public AHDWeapon
{
    GENERATED_BODY()

public:
    AHDSecondaryWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual EHDWeaponSlot               GetWeaponSlot() const override { return EHDWeaponSlot::Secondary; }

    // SK_AM_Pistol 에는 노티파이가 하나도 없어 발사·재장전 이벤트를 캐릭터가 C++ 로 보낸다
    virtual bool                        SendsMontageEventsFromCode() const override { return true; }
};
