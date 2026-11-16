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

    // 플레이어 앞 1500 에 Count 마리를 좌우 250 간격으로 스폰한다. Type 은 대소문자 무시(Charger)
    UFUNCTION(Exec)
    void        HDSpawnEnemy(const FString& Type = TEXT("Charger"), int32 Count = 1);

    // 살아 있는 적 전부에 CurrentHealth -9999 (사망 경로를 그대로 탄다)
    UFUNCTION(Exec)
    void        HDKillEnemies();

    // 스폰 디렉터: start / stop / status
    UFUNCTION(Exec)
    void        HDDirector(const FString& Command);

    // 플레이어를 대상으로 증원 웨이브를 바로 부른다
    UFUNCTION(Exec)
    void        HDBreach();
};
