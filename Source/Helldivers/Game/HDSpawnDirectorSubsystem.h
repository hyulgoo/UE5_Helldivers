// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HDSpawnDirectorSubsystem.generated.h"

class AHDCharacterNonPlayer;
class UHDSpawnDirectorSettings;

// 순찰조 주기 스폰 + 경보가 이어지면 증원 웨이브(bug breach). 수 계산은 HDSpawnRules, 수치는 UHDSpawnDirectorSettings
UCLASS()
class HELLDIVERS_API UHDSpawnDirectorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
    void                        StartDirector();
    void                        StopDirector();
    bool                        IsRunning() const { return bRunning; }
    void                        SetIntensity(float NewIntensity);

    // 적이 처음 대상을 알아챘을 때 AI 컨트롤러가 부른다
    void                        NotifyEnemyAlerted(AHDCharacterNonPlayer* Enemy, AActor* Target);

    // 쿨다운·경보와 상관없이 Target 주변에 웨이브를 바로 부른다
    void                        ForceBreach(AActor* Target);

    int32                       GetAliveEnemyCount() const;
    FString                     DescribeStatus() const;

    virtual void                Tick(float DeltaTime) override;
    virtual TStatId             GetStatId() const override;

protected:
    virtual bool                DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
    struct FAlertEntry
    {
        TWeakObjectPtr<AHDCharacterNonPlayer>   Enemy;
        TWeakObjectPtr<AActor>                  Target;
        double                                  StartTime = 0.0;
    };

    void                        TickPatrol(const UHDSpawnDirectorSettings& Settings, const float DeltaTime);
    void                        TickBreach(const UHDSpawnDirectorSettings& Settings);
    void                        SpawnBreachWave(AActor* Target, const FString& Reason);
    void                        TickDebug(const float DeltaTime);

    // Count 명(ShooterCount 만큼 원거리형)을 Center 주변 링에 스폰하고 실제 스폰 수를 돌려준다. ForceTarget 이 있으면 웨이브로 달려오게 한다
    int32                       SpawnGroup(const FVector& Center, const int32 Count, const float RingMin, const float RingMax, AActor* ForceTarget);
    bool                        FindSpawnPoint(const FVector& Center, const float RingMin, const float RingMax, FVector& OutLocation) const;

    APawn*                      GetPlayerPawn() const;
    double                      GetNow() const;

private:
    TArray<FAlertEntry>         Alerts;

    bool                        bRunning = false;
    float                       Intensity = 1.f;
    float                       PatrolElapsed = 0.f;
    float                       DebugElapsed = 0.f;

    // 처음에는 아주 과거로 둬서 첫 웨이브가 쿨다운에 막히지 않게 한다
    double                      LastBreachTime = -1.0e9;

    static constexpr int32      SpawnPointAttempts = 8;
    static constexpr int32      AvoidCameraAttempts = 5;
    static constexpr float      AvoidCameraDot = 0.5f;
    static constexpr float      GroundTraceUp = 5000.f;
    static constexpr float      GroundTraceDown = 20000.f;
    static constexpr float      DebugSphereDuration = 5.f;
    static constexpr float      DebugMessageInterval = 1.f;
};
