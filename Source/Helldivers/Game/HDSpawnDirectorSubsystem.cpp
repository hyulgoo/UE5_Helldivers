// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/HDSpawnDirectorSubsystem.h"
#include "Game/HDSpawnDirectorRules.h"
#include "Game/HDSpawnDirectorSettings.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "AI/HDEnemyAIController.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "Character/NPC/HDEnemyCharger.h"
#include "Character/NPC/HDEnemyShooter.h"
#include "Define/HDDefine.h"

static TAutoConsoleVariable<int32> CVarHDDirectorDebug(
    TEXT("hd.Director.Debug"),
    0,
    TEXT("1: draw spawn director spawn points and show alive enemy count on screen"),
    ECVF_Cheat);

namespace HDSpawnDirectorPrivate
{
    // 화면 메시지를 쌓지 않고 덮어쓰는 키
    constexpr uint64 DebugMessageKey = 0x48444449524543ull;

    // 내비 투영 범위(브리프 5-2 ④)
    const FVector NavProjectExtent(500.f, 500.f, 1000.f);
}

void UHDSpawnDirectorSubsystem::StartDirector()
{
    bRunning = true;
    PatrolElapsed = 0.f;
    Alerts.Reset();

    UE_LOG(LogTemp, Display, TEXT("[HDDirector] start"));
}

void UHDSpawnDirectorSubsystem::StopDirector()
{
    bRunning = false;
    Alerts.Reset();

    UE_LOG(LogTemp, Display, TEXT("[HDDirector] stop"));
}

void UHDSpawnDirectorSubsystem::SetIntensity(float NewIntensity)
{
    Intensity = NewIntensity;
}

void UHDSpawnDirectorSubsystem::NotifyEnemyAlerted(AHDCharacterNonPlayer* Enemy, AActor* Target)
{
    // 꺼져 있을 때 받은 경보는 들고 있지 않는다(켜는 순간 옛 경보로 웨이브가 오지 않게)
    CONDITION_CHECK_WITHOUT_LOG(bRunning);
    NULL_CHECK_WITHOUT_LOG(Enemy);
    NULL_CHECK_WITHOUT_LOG(Target);

    const bool bAlreadyAlerted = Alerts.ContainsByPredicate([Enemy](const FAlertEntry& Entry) { return Entry.Enemy.Get() == Enemy; });
    CONDITION_CHECK_WITHOUT_LOG(bAlreadyAlerted == false);

    FAlertEntry& Entry = Alerts.AddDefaulted_GetRef();
    Entry.Enemy = Enemy;
    Entry.Target = Target;
    Entry.StartTime = GetNow();
}

void UHDSpawnDirectorSubsystem::ForceBreach(AActor* Target)
{
    NULL_CHECK(Target);
    SpawnBreachWave(Target, TEXT("forced"));
}

int32 UHDSpawnDirectorSubsystem::GetAliveEnemyCount() const
{
    UWorld* World = GetWorld();
    NULL_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(World, 0);

    int32 AliveCount = 0;
    for (TActorIterator<AHDCharacterNonPlayer> It(World); It; ++It)
    {
        if (IsValid(*It) && It->IsDead() == false)
        {
            ++AliveCount;
        }
    }
    return AliveCount;
}

FString UHDSpawnDirectorSubsystem::DescribeStatus() const
{
    const UHDSpawnDirectorSettings* Settings = GetDefault<UHDSpawnDirectorSettings>();
    const float PatrolInterval = HDSpawnRules::ScaledInterval(Settings->PatrolInterval, Intensity);
    const FString LastBreachText = LastBreachTime < 0.0 ? FString(TEXT("never")) : FString::Printf(TEXT("%.1fs ago"), GetNow() - LastBreachTime);

    // 예: "Director running=1 intensity=1.0 alive=12/30 alerts=1 patrol=10.2/25.0s lastBreach=never"
    return FString::Printf(TEXT("Director running=%d intensity=%.1f alive=%d/%d alerts=%d patrol=%.1f/%.1fs lastBreach=%s"),
        bRunning ? 1 : 0,
        Intensity,
        GetAliveEnemyCount(),
        Settings->MaxAliveEnemies,
        Alerts.Num(),
        PatrolElapsed,
        PatrolInterval,
        *LastBreachText);
}

void UHDSpawnDirectorSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TickDebug(DeltaTime);

    CONDITION_CHECK_WITHOUT_LOG(bRunning);

    const UHDSpawnDirectorSettings* Settings = GetDefault<UHDSpawnDirectorSettings>();
    NULL_CHECK(Settings);

    TickPatrol(*Settings, DeltaTime);
    TickBreach(*Settings);
}

TStatId UHDSpawnDirectorSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UHDSpawnDirectorSubsystem, STATGROUP_Tickables);
}

bool UHDSpawnDirectorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UHDSpawnDirectorSubsystem::TickPatrol(const UHDSpawnDirectorSettings& Settings, const float DeltaTime)
{
    PatrolElapsed += DeltaTime;
    CONDITION_CHECK_WITHOUT_LOG(PatrolElapsed >= HDSpawnRules::ScaledInterval(Settings.PatrolInterval, Intensity));
    PatrolElapsed = 0.f;

    APawn* PlayerPawn = GetPlayerPawn();
    NULL_CHECK_WITHOUT_LOG(PlayerPawn);

    // 순찰조는 대상을 정해 주지 않는다(스스로 인지한다)
    const int32 Requested = HDSpawnRules::ScaledCount(Settings.PatrolGroupSize, Intensity);
    const int32 Count = HDSpawnRules::ClampSpawnCount(Requested, GetAliveEnemyCount(), Settings.MaxAliveEnemies);
    const int32 SpawnedCount = SpawnGroup(PlayerPawn->GetActorLocation(), Count, Settings.PatrolRingMin, Settings.PatrolRingMax, nullptr);

    UE_LOG(LogTemp, Display, TEXT("[HDDirector] patrol x%d"), SpawnedCount);
}

void UHDSpawnDirectorSubsystem::TickBreach(const UHDSpawnDirectorSettings& Settings)
{
    // 죽었거나 사라진 적(또는 대상)의 경보는 지운다
    Alerts.RemoveAll([](const FAlertEntry& Entry)
    {
        return Entry.Enemy.IsValid() == false || Entry.Enemy->IsDead() || Entry.Target.IsValid() == false;
    });
    CONDITION_CHECK_WITHOUT_LOG(Alerts.IsEmpty() == false);

    const FAlertEntry* Oldest = &Alerts[0];
    for (const FAlertEntry& Entry : Alerts)
    {
        if (Entry.StartTime < Oldest->StartTime)
        {
            Oldest = &Entry;
        }
    }

    const double Now = GetNow();
    const float AlertedSeconds = static_cast<float>(Now - Oldest->StartTime);
    const float SecondsSinceLastBreach = static_cast<float>(Now - LastBreachTime);
    CONDITION_CHECK_WITHOUT_LOG(HDSpawnRules::ShouldBreach(AlertedSeconds, Settings.BreachDelayAfterAlert, SecondsSinceLastBreach, Settings.BreachCooldown));

    AActor* Target = Oldest->Target.Get();
    const FString Reason = FString::Printf(TEXT("alerted by %s"), *GetNameSafe(Oldest->Enemy.Get()));
    SpawnBreachWave(Target, Reason);
}

void UHDSpawnDirectorSubsystem::SpawnBreachWave(AActor* Target, const FString& Reason)
{
    NULL_CHECK(Target);

    const UHDSpawnDirectorSettings* Settings = GetDefault<UHDSpawnDirectorSettings>();
    NULL_CHECK(Settings);

    const int32 Requested = HDSpawnRules::ScaledCount(Settings->BreachWaveSize, Intensity);
    const int32 Count = HDSpawnRules::ClampSpawnCount(Requested, GetAliveEnemyCount(), Settings->MaxAliveEnemies);
    const int32 SpawnedCount = SpawnGroup(Target->GetActorLocation(), Count, Settings->BreachRingMin, Settings->BreachRingMax, Target);

    Alerts.Reset();
    LastBreachTime = GetNow();

    UE_LOG(LogTemp, Display, TEXT("[HDDirector] breach x%d (%s)"), SpawnedCount, *Reason);
}

void UHDSpawnDirectorSubsystem::TickDebug(const float DeltaTime)
{
    CONDITION_CHECK_WITHOUT_LOG(CVarHDDirectorDebug.GetValueOnGameThread() == 1);

    DebugElapsed += DeltaTime;
    CONDITION_CHECK_WITHOUT_LOG(DebugElapsed >= DebugMessageInterval);
    DebugElapsed = 0.f;

    NULL_CHECK_WITHOUT_LOG(GEngine);
    const FString Message = FString::Printf(TEXT("Director alive=%d running=%d"), GetAliveEnemyCount(), bRunning ? 1 : 0);
    GEngine->AddOnScreenDebugMessage(HDSpawnDirectorPrivate::DebugMessageKey, DebugMessageInterval + 0.1f, FColor::Cyan, Message);
}

int32 UHDSpawnDirectorSubsystem::SpawnGroup(const FVector& Center, const int32 Count, const float RingMin, const float RingMax, AActor* ForceTarget)
{
    UWorld* World = GetWorld();
    NULL_CHECK_WITH_RETURNTYPE(World, 0);

    const UHDSpawnDirectorSettings* Settings = GetDefault<UHDSpawnDirectorSettings>();
    const int32 ShooterCount = HDSpawnRules::ShooterCount(Count, Settings->ShooterRatio);
    const bool bDebug = CVarHDDirectorDebug.GetValueOnGameThread() == 1;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    int32 SpawnedCount = 0;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        // 앞의 ShooterCount 명은 원거리형, 나머지는 돌격형
        const TSubclassOf<AHDCharacterNonPlayer> EnemyClass = Index < ShooterCount ? TSubclassOf<AHDCharacterNonPlayer>(AHDEnemyShooter::StaticClass()) : TSubclassOf<AHDCharacterNonPlayer>(AHDEnemyCharger::StaticClass());

        FVector GroundLocation;
        if (FindSpawnPoint(Center, RingMin, RingMax, GroundLocation) == false)
        {
            UE_LOG(LogTemp, Display, TEXT("[HDDirector] no spawn point after %d attempts around %s, skipped one"), SpawnPointAttempts, *Center.ToString());
            continue;
        }

        // 캡슐 바닥이 지면에 오도록 반높이만큼 올린다
        const AHDCharacterNonPlayer* EnemyCDO = EnemyClass->GetDefaultObject<AHDCharacterNonPlayer>();
        const float HalfHeight = EnemyCDO && EnemyCDO->GetCapsuleComponent() ? EnemyCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 100.f;
        const FVector SpawnLocation = GroundLocation + FVector(0.f, 0.f, HalfHeight);
        const FRotator SpawnRotation(0.f, (Center - SpawnLocation).Rotation().Yaw, 0.f);

        AHDCharacterNonPlayer* Enemy = World->SpawnActor<AHDCharacterNonPlayer>(EnemyClass, SpawnLocation, SpawnRotation, SpawnParams);
        if (Enemy == nullptr)
        {
            continue;
        }
        ++SpawnedCount;

        if (bDebug)
        {
            DrawDebugSphere(World, GroundLocation, 60.f, 12, ForceTarget ? FColor::Red : FColor::Orange, false, DebugSphereDuration);
        }

        // 웨이브 적은 바로 대상에게 달려온다(디렉터에는 알리지 않는다)
        if (ForceTarget)
        {
            AHDEnemyAIController* EnemyAIController = Cast<AHDEnemyAIController>(Enemy->GetController());
            if (EnemyAIController)
            {
                EnemyAIController->ForceTarget(ForceTarget);
            }
        }
    }

    return SpawnedCount;
}

bool UHDSpawnDirectorSubsystem::FindSpawnPoint(const FVector& Center, const float RingMin, const float RingMax, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    NULL_CHECK_WITH_RETURNTYPE(World, false);

    // 카메라 정면 판정은 플레이어 시점 기준
    FVector ViewLocation = Center;
    FRotator ViewRotation = FRotator::ZeroRotator;
    const APlayerController* PlayerController = World->GetFirstPlayerController();
    const bool bHasView = PlayerController != nullptr;
    if (bHasView)
    {
        PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
    }
    const FVector ViewForward2D = ViewRotation.Vector().GetSafeNormal2D();

    UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    const bool bHasNavData = NavSystem && NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDDirectorSpawnPoint), false);
    if (PlayerController && PlayerController->GetPawn())
    {
        QueryParams.AddIgnoredActor(PlayerController->GetPawn());
    }

    for (int32 Attempt = 0; Attempt < SpawnPointAttempts; ++Attempt)
    {
        const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
        const float Radius = FMath::FRandRange(RingMin, RingMax);
        const FVector Candidate = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;

        // 처음 몇 번은 카메라 정면을 피한다
        if (bHasView && Attempt < AvoidCameraAttempts)
        {
            const FVector ToCandidate2D = (Candidate - ViewLocation).GetSafeNormal2D();
            if (FVector::DotProduct(ViewForward2D, ToCandidate2D) > AvoidCameraDot)
            {
                continue;
            }
        }

        FHitResult GroundHit;
        const FVector TraceStart = Candidate + FVector(0.f, 0.f, GroundTraceUp);
        const FVector TraceEnd = Candidate - FVector(0.f, 0.f, GroundTraceDown);
        if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) == false)
        {
            continue;
        }

        FVector GroundLocation = GroundHit.ImpactPoint;
        if (bHasNavData)
        {
            FNavLocation NavLocation;
            if (NavSystem->ProjectPointToNavigation(GroundLocation, NavLocation, HDSpawnDirectorPrivate::NavProjectExtent) == false)
            {
                continue;
            }
            GroundLocation = NavLocation.Location;
        }

        OutLocation = GroundLocation;
        return true;
    }

    return false;
}

APawn* UHDSpawnDirectorSubsystem::GetPlayerPawn() const
{
    UWorld* World = GetWorld();
    NULL_CHECK_WITH_RETURNTYPE_WITHOUT_LOG(World, nullptr);

    const APlayerController* PlayerController = World->GetFirstPlayerController();
    return PlayerController ? PlayerController->GetPawn() : nullptr;
}

double UHDSpawnDirectorSubsystem::GetNow() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0;
}
