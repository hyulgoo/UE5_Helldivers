// Fill out your copyright notice in the Description page of Project Settings.

#include "Cheat/HDCheatManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "Character/NPC/HDEnemyCharger.h"
#include "Character/NPC/HDEnemyShooter.h"
#include "Game/HDSpawnDirectorSubsystem.h"
#include "Game/HDGameMode.h"
#include "Game/HDGameState.h"
#include "Mission/HDExtractionZone.h"
#include "Mission/HDObjectiveFacility.h"

namespace HDCheatManager
{
    struct FEnemyTypeEntry
    {
        const TCHAR*                        Name;
        TSubclassOf<AHDCharacterNonPlayer>  Class;
    };

    TArray<FEnemyTypeEntry> GetEnemyTypes()
    {
        return {
            { TEXT("Charger"), AHDEnemyCharger::StaticClass() },
            { TEXT("Shooter"), AHDEnemyShooter::StaticClass() },
        };
    }
}

void UHDCheatManager::HDDamageSelf(float Amount)
{
    APlayerController* PlayerController = GetPlayerController();
    APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Pawn);
    if (ASC == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDDamageSelf] No player pawn or ability system component"));
        return;
    }

    const float OldCurrentHealth = ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute());
    ASC->ApplyModToAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute(), EGameplayModOp::AddBase, -Amount);
    const float NewCurrentHealth = ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute());

    UE_LOG(LogTemp, Display, TEXT("[HDDamageSelf] CurrentHealth %.1f -> %.1f"), OldCurrentHealth, NewCurrentHealth);
}

void UHDCheatManager::HDSpawnEnemy(const FString& Type, int32 Count)
{
    using namespace HDCheatManager;

    const TArray<FEnemyTypeEntry> EnemyTypes = GetEnemyTypes();
    const FEnemyTypeEntry* FoundType = EnemyTypes.FindByPredicate([&Type](const FEnemyTypeEntry& Entry) { return Type.Equals(Entry.Name, ESearchCase::IgnoreCase); });
    if (FoundType == nullptr)
    {
        FString KnownNames;
        for (const FEnemyTypeEntry& Entry : EnemyTypes)
        {
            if (KnownNames.IsEmpty() == false)
            {
                KnownNames += TEXT(", ");
            }
            KnownNames += Entry.Name;
        }

        UE_LOG(LogTemp, Display, TEXT("[HDSpawnEnemy] Unknown enemy type '%s'. Known types: %s"), *Type, *KnownNames);
        return;
    }

    if (Count < 1)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSpawnEnemy] Count must be 1 or more (got %d)"), Count);
        return;
    }

    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UWorld* World = GetWorld();
    if (PlayerPawn == nullptr || World == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSpawnEnemy] No player pawn"));
        return;
    }

    // 앞 = 카메라가 보는 수평 방향(컨트롤 회전 Yaw)
    const FVector PlayerLocation = PlayerPawn->GetActorLocation();
    const FVector Forward = FRotator(0.f, PlayerController->GetControlRotation().Yaw, 0.f).Vector();
    const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
    const FVector FrontCenter = PlayerLocation + Forward * 1500.f;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDSpawnEnemy), false, PlayerPawn);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    int32 SpawnedCount = 0;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        // 가운데를 기준으로 좌우 250 간격
        const float SideOffset = (static_cast<float>(Index) - static_cast<float>(Count - 1) * 0.5f) * 250.f;
        const FVector Column = FrontCenter + Right * SideOffset;

        FHitResult GroundHit;
        const FVector TraceStart(Column.X, Column.Y, PlayerLocation.Z + 3000.f);
        const FVector TraceEnd(Column.X, Column.Y, PlayerLocation.Z - 10000.f);
        if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) == false)
        {
            UE_LOG(LogTemp, Display, TEXT("[HDSpawnEnemy] No ground at %s"), *Column.ToString());
            continue;
        }

        const FVector SpawnLocation = GroundHit.ImpactPoint + FVector(0.f, 0.f, 100.f);
        const FRotator SpawnRotation(0.f, (PlayerLocation - SpawnLocation).Rotation().Yaw, 0.f);

        if (World->SpawnActor<AHDCharacterNonPlayer>(FoundType->Class, SpawnLocation, SpawnRotation, SpawnParams))
        {
            ++SpawnedCount;
        }
    }

    UE_LOG(LogTemp, Display, TEXT("[HDSpawnEnemy] Spawned %d/%d %s"), SpawnedCount, Count, FoundType->Name);
}

void UHDCheatManager::HDKillEnemies()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    int32 KilledCount = 0;
    for (TActorIterator<AHDCharacterNonPlayer> It(World); It; ++It)
    {
        AHDCharacterNonPlayer* Enemy = *It;
        if (IsValid(Enemy) == false || Enemy->IsDead())
        {
            continue;
        }

        UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Enemy);
        if (ASC == nullptr)
        {
            continue;
        }

        ASC->ApplyModToAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute(), EGameplayModOp::AddBase, -9999.f);
        ++KilledCount;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDKillEnemies] Applied -9999 CurrentHealth to %d enemies"), KilledCount);
}

void UHDCheatManager::HDDirector(const FString& Command)
{
    UWorld* World = GetWorld();
    UHDSpawnDirectorSubsystem* Director = World ? World->GetSubsystem<UHDSpawnDirectorSubsystem>() : nullptr;
    if (Director == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDDirector] No spawn director in this world"));
        return;
    }

    if (Command.Equals(TEXT("start"), ESearchCase::IgnoreCase))
    {
        Director->StartDirector();
    }
    else if (Command.Equals(TEXT("stop"), ESearchCase::IgnoreCase))
    {
        Director->StopDirector();
    }
    else if (Command.Equals(TEXT("status"), ESearchCase::IgnoreCase))
    {
        const FString Status = Director->DescribeStatus();
        UE_LOG(LogTemp, Display, TEXT("[HDDirector] %s"), *Status);

        APlayerController* PlayerController = GetPlayerController();
        if (PlayerController)
        {
            PlayerController->ClientMessage(Status);
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("[HDDirector] Unknown command '%s'. Use start, stop or status"), *Command);
    }
}

void UHDCheatManager::HDBreach()
{
    UWorld* World = GetWorld();
    UHDSpawnDirectorSubsystem* Director = World ? World->GetSubsystem<UHDSpawnDirectorSubsystem>() : nullptr;
    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (Director == nullptr || PlayerPawn == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDBreach] No spawn director or player pawn"));
        return;
    }

    Director->ForceBreach(PlayerPawn);
}

void UHDCheatManager::HDMission()
{
    UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    if (HDGameState == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDMission] No HDGameState in this world"));
        return;
    }

    const FString Status = FString::Printf(TEXT("Phase=%s Objectives=%d/%d Reinforcements=%d Extraction=%.1f InZone=%d"),
        *StaticEnum<EHDMissionPhase>()->GetNameStringByValue(static_cast<int64>(HDGameState->GetMissionPhase())),
        HDGameState->GetCompletedObjectives(),
        HDGameState->GetRequiredObjectives(),
        HDGameState->GetReinforcementsLeft(),
        HDGameState->GetExtractionRemaining(),
        HDGameState->IsPlayerInExtractionZone() ? 1 : 0);

    UE_LOG(LogTemp, Display, TEXT("[HDMission] %s"), *Status);

    APlayerController* PlayerController = GetPlayerController();
    if (PlayerController)
    {
        PlayerController->ClientMessage(Status);
    }
}

void UHDCheatManager::HDCompleteObjective()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    // 착지 전(Deploy)·종료 뒤에는 목표 완료가 무시되므로 시설도 부수지 않는다(부수면 목표 수는 그대로인 채 시설만 사라진다)
    const AHDGameState* HDGameState = World->GetGameState<AHDGameState>();
    if (HDGameState && HDGameState->GetMissionPhase() != EHDMissionPhase::InProgress)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDCompleteObjective] Ignored: Phase=%s (only InProgress)"),
            *StaticEnum<EHDMissionPhase>()->GetNameStringByValue(static_cast<int64>(HDGameState->GetMissionPhase())));
        return;
    }

    // 기준 = 플레이어 폰 위치(폰이 없으면 시점 위치)
    FVector ReferenceLocation = FVector::ZeroVector;
    APlayerController* PlayerController = GetPlayerController();
    if (PlayerController)
    {
        if (const APawn* PlayerPawn = PlayerController->GetPawn())
        {
            ReferenceLocation = PlayerPawn->GetActorLocation();
        }
        else
        {
            FRotator ViewRotation;
            PlayerController->GetPlayerViewPoint(ReferenceLocation, ViewRotation);
        }
    }

    // 가장 가까운 미완료 시설
    AHDObjectiveFacility* NearestFacility = nullptr;
    double NearestDistanceSquared = TNumericLimits<double>::Max();
    for (TActorIterator<AHDObjectiveFacility> It(World); It; ++It)
    {
        AHDObjectiveFacility* Facility = *It;
        if (IsValid(Facility) == false || Facility->IsCompleted())
        {
            continue;
        }

        const double DistanceSquared = FVector::DistSquared(ReferenceLocation, Facility->GetActorLocation());
        if (DistanceSquared < NearestDistanceSquared)
        {
            NearestFacility = Facility;
            NearestDistanceSquared = DistanceSquared;
        }
    }

    UAbilitySystemComponent* FacilityASC = NearestFacility ? NearestFacility->GetAbilitySystemComponent() : nullptr;
    if (FacilityASC)
    {
        // 파괴 경로를 그대로 탄다: 체력 0 -> Trigger.Dead -> 폭발·목표 완료
        FacilityASC->ApplyModToAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute(), EGameplayModOp::AddBase, -99999.f);
        UE_LOG(LogTemp, Display, TEXT("[HDCompleteObjective] Applied -99999 CurrentHealth to %s (%.0fm)"), *NearestFacility->GetName(), FMath::Sqrt(NearestDistanceSquared) / 100.0);
        return;
    }

    AHDGameMode* HDGameMode = World->GetAuthGameMode<AHDGameMode>();
    if (HDGameMode == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDCompleteObjective] No facility and no HDGameMode in this world"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDCompleteObjective] No facility left. Counting one objective as completed"));
    HDGameMode->NotifyObjectiveCompleted(nullptr);
}

void UHDCheatManager::HDTeleportToExtraction()
{
    UWorld* World = GetWorld();
    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (World == nullptr || PlayerPawn == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDTeleportToExtraction] No player pawn"));
        return;
    }

    TActorIterator<AHDExtractionZone> ZoneIt(World);
    if (ZoneIt)
    {
        const FVector Destination = ZoneIt->GetActorLocation() + FVector(0.f, 0.f, 200.f);
        const bool bTeleported = PlayerPawn->TeleportTo(Destination, PlayerPawn->GetActorRotation());
        UE_LOG(LogTemp, Display, TEXT("[HDTeleportToExtraction] %s to %s"), bTeleported ? TEXT("Teleported") : TEXT("Failed to teleport"), *Destination.ToString());
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDTeleportToExtraction] No extraction zone in this world"));
}

void UHDCheatManager::HDSetReinforcements(int32 Count)
{
    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    if (HDGameMode == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSetReinforcements] No HDGameMode in this world"));
        return;
    }

    HDGameMode->DebugSetReinforcementsLeft(Count);

    const AHDGameState* HDGameState = World->GetGameState<AHDGameState>();
    UE_LOG(LogTemp, Display, TEXT("[HDSetReinforcements] Reinforcements=%d"), HDGameState ? HDGameState->GetReinforcementsLeft() : -1);
}

void UHDCheatManager::HDSetTimeLimit(float Seconds)
{
    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    const FHDMissionFlow* MissionFlow = HDGameMode ? HDGameMode->GetMissionFlow() : nullptr;
    if (MissionFlow == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSetTimeLimit] No HDGameMode or mission flow in this world"));
        return;
    }

    HDGameMode->DebugSetMissionTimeLimit(Seconds);
    UE_LOG(LogTemp, Display, TEXT("[HDSetTimeLimit] MissionTimeLimit=%.1f Elapsed=%.1f"), MissionFlow->GetMissionTimeLimit(), MissionFlow->GetElapsedSeconds());
}
