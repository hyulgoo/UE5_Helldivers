// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_StratagemSupportWeapon.h"
#include "Engine/World.h"
#include "GameData/HDStratagemEffectData.h"
#include "Weapon/HDWeapon.h"

namespace HDGA_StratagemSupportWeaponPrivate
{
    // 바닥에 묻히지 않게 표적 지면에서 조금 띄운다
    const FVector WeaponSpawnOffset(0.f, 0.f, 30.f);
}

UHDGA_StratagemSupportWeapon::UHDGA_StratagemSupportWeapon(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetActivationEventTag(Tag_Event_Stratagem_SupportWeapon);
}

void UHDGA_StratagemSupportWeapon::ExecuteStratagem(const FHDStratagemEffectData& EffectData)
{
    using namespace HDGA_StratagemSupportWeaponPrivate;

    UWorld* World = GetWorld();
    if (World == nullptr || EffectData.SupportWeaponClass == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    // 소유자 없음: 바닥에 놓인 무기다. 새로 스폰한 무기는 줍기 구가 켜져 있다(AHDWeapon 생성자)
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 표적 회전은 던진 방향이라 쓰지 않는다
    const FTransform SpawnTransform(GetTargetTransform().GetLocation() + WeaponSpawnOffset);
    AHDWeapon* Weapon = World->SpawnActor<AHDWeapon>(EffectData.SupportWeaponClass, SpawnTransform, SpawnParams);
    if (Weapon == nullptr)
    {
        FinishStratagem(true);
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] support weapon dropped %s"), *EffectData.SupportWeaponClass->GetName());

    FinishStratagem(false);
}