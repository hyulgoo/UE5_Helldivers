// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HDWeapon.h"
#include "HDSupportWeapon.generated.h"

// 지원 무기(대물 저격총). 무기 BP 를 새로 만들지 않고 기존 저격총 메시·몽타주·탄 BP 를 생성자에서 잡는다
UCLASS()
class HELLDIVERS_API AHDSupportWeapon : public AHDWeapon
{
    GENERATED_BODY()

public:
    AHDSupportWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual bool                        IsSupportWeapon() const override { return true; }

    // SK_AM_SniperRifle 에는 노티파이가 하나도 없어(SK_AM_Rifle 과 달리) 발사·재장전 이벤트를 캐릭터가 C++ 로 보낸다(브리프 16 개발 1회차 「결과」)
    virtual bool                        SendsMontageEventsFromCode() const override { return true; }
};
