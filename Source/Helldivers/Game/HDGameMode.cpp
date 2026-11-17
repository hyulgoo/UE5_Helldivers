// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGameMode.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Game/HDGameState.h"
#include "Game/HDMissionSettings.h"
#include "Game/HDSpawnDirectorSubsystem.h"
#include "Game/HDMissionPlacement.h"
#include "Define/HDDefine.h"
#include "Mission/HDMissionObjective.h"
#include "Mission/HDObjectiveFacility.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace HDPlaceObjective
{
    // 지면 찾기: 중심 Z + 20000 에서 아래 50000 까지
    constexpr float GroundTraceUp = 20000.f;
    constexpr float GroundTraceLength = 50000.f;

    // 시설(높이 600, 피벗 가운데)을 지면 + 300 에 둔다
    constexpr float FacilityHeightAboveGround = 300.f;

    // 지면을 못 찾았을 때 같은 링에서 다시 뽑는 최대 횟수
    constexpr int32 MaxGroundRetries = 5;

    bool FindGround(UWorld* World, const FVector& Center, const FVector2D& Offset, FVector& OutGroundLocation)
    {
        const FVector TraceStart(Center.X + Offset.X, Center.Y + Offset.Y, Center.Z + GroundTraceUp);
        const FVector TraceEnd(TraceStart.X, TraceStart.Y, TraceStart.Z - GroundTraceLength);

        FHitResult GroundHit;
        const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDPlaceObjective), false);
        if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) == false)
        {
            return false;
        }

        OutGroundLocation = GroundHit.ImpactPoint;
        return true;
    }
}

AHDGameMode::AHDGameMode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerClassRef(TEXT("/Script/Helldivers.HDPlayerController"));
    if (PlayerControllerClassRef.Class)
    {
        PlayerControllerClass = PlayerControllerClassRef.Class;
    }

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AHDGameMode::StartPlay()
{
	Super::StartPlay();

    const UHDMissionSettings* Settings = GetDefault<UHDMissionSettings>();

    // 목표 배치를 미션 로직보다 먼저 한다. 필요 수는 실제로 있는 목표 수를 넘지 않게(최소 1)
    const int32 ObjectiveCount = PlaceObjectives(*Settings);
    FHDMissionConfig Config = Settings->MakeConfig();
    Config.RequiredObjectives = FMath::Max(1, FMath::Min(ObjectiveCount, Config.RequiredObjectives));

    MissionFlow = MakeUnique<FHDMissionFlow>(Config);
    SyncGameState();

    UE_LOG(LogTemp, Display, TEXT("[HDMission] start objectives=%d reinforcements=%d"), MissionFlow->GetRequiredObjectives(), MissionFlow->GetReinforcementsLeft());
}

void AHDGameMode::PostLogin(APlayerController* NewPlayer)
{
    // Super 안의 RestartPlayer 가 첫 빙의를 하므로 그 전에 묶는다
    if (NewPlayer)
    {
        NewPlayer->OnPossessedPawnChanged.AddUniqueDynamic(this, &AHDGameMode::HandlePossessedPawnChanged);
    }

    Super::PostLogin(NewPlayer);
}

void AHDGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ApplyMissionChange([DeltaSeconds](FHDMissionFlow& Flow) { Flow.Tick(DeltaSeconds); });
}

void AHDGameMode::NotifyObjectiveCompleted(AActor* Objective)
{
    ApplyMissionChange([](FHDMissionFlow& Flow) { Flow.NotifyObjectiveCompleted(); });
}

void AHDGameMode::NotifyPlayerInExtractionZone(bool bInZone)
{
    ApplyMissionChange([bInZone](FHDMissionFlow& Flow) { Flow.SetPlayerInExtractionZone(bInZone); });
}

void AHDGameMode::NotifyPlayerDied(AController* DeadController)
{
    ApplyMissionChange([](FHDMissionFlow& Flow) { Flow.NotifyPlayerDied(); });
}

const FHDMissionFlow* AHDGameMode::GetMissionFlow() const
{
    return MissionFlow.Get();
}

FHDMissionFlow* AHDGameMode::GetMissionFlowForDebug()
{
    return MissionFlow.Get();
}

int32 AHDGameMode::PlaceObjectives(const UHDMissionSettings& Settings)
{
    using namespace HDPlaceObjective;

    UWorld* World = GetWorld();
    NULL_CHECK_WITH_RETURNTYPE(World, 0);

    int32 PlacedCount = 0;
    for (TActorIterator<AHDMissionObjective> It(World); It; ++It)
    {
        ++PlacedCount;
    }

    int32 SpawnedCount = 0;
    const int32 MissingCount = Settings.RequiredObjectives - PlacedCount;
    if (MissingCount > 0)
    {
        // 중심 = 맵의 첫 PlayerStart(없으면 원점)
        FVector Center = FVector::ZeroVector;
        TActorIterator<APlayerStart> PlayerStartIt(World);
        if (PlayerStartIt)
        {
            Center = PlayerStartIt->GetActorLocation();
        }

        FRandomStream Stream(FMath::Rand());
        const TArray<FVector2D> Offsets = HDMissionPlacement::ComputeRingOffsets(MissingCount, Settings.ObjectiveRadiusMin, Settings.ObjectiveRadiusMax, Settings.ObjectiveMinSeparation, Stream);

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        for (const FVector2D& Offset : Offsets)
        {
            FVector GroundLocation = FVector::ZeroVector;
            bool bFoundGround = FindGround(World, Center, Offset, GroundLocation);

            // 다시 뽑은 점은 다른 목표와의 간격을 보지 않는다
            for (int32 Retry = 0; Retry < MaxGroundRetries && bFoundGround == false; ++Retry)
            {
                const TArray<FVector2D> Redrawn = HDMissionPlacement::ComputeRingOffsets(1, Settings.ObjectiveRadiusMin, Settings.ObjectiveRadiusMax, 0.f, Stream);
                bFoundGround = FindGround(World, Center, Redrawn[0], GroundLocation);
            }

            if (bFoundGround == false)
            {
                UE_LOG(LogTemp, Display, TEXT("[HDMission] no ground for an objective around %s"), *Center.ToString());
                continue;
            }

            const FVector SpawnLocation = GroundLocation + FVector(0.f, 0.f, FacilityHeightAboveGround);
            if (World->SpawnActor<AHDObjectiveFacility>(AHDObjectiveFacility::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams))
            {
                ++SpawnedCount;
            }
        }
    }

    UE_LOG(LogTemp, Display, TEXT("[HDMission] objectives placed=%d spawned=%d"), PlacedCount, SpawnedCount);
    return PlacedCount + SpawnedCount;
}

void AHDGameMode::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    if (Cast<AHDCharacterPlayer>(NewPawn) == nullptr || MissionFlow.IsValid() == false)
    {
        return;
    }

    if (MissionFlow->GetPhase() == EHDMissionPhase::Deploy)
    {
        ApplyMissionChange([](FHDMissionFlow& Flow) { Flow.NotifyPlayerLanded(); });
    }
    else
    {
        ApplyMissionChange([](FHDMissionFlow& Flow) { Flow.NotifyPlayerRespawned(); });
    }
}

void AHDGameMode::ApplyMissionChange(TFunctionRef<void(FHDMissionFlow&)> Change)
{
    if (MissionFlow.IsValid() == false)
    {
        return;
    }

    const EHDMissionPhase OldPhase = MissionFlow->GetPhase();
    Change(*MissionFlow);
    SyncGameState();

    const EHDMissionPhase NewPhase = MissionFlow->GetPhase();
    if (OldPhase != NewPhase)
    {
        HandlePhaseChanged(OldPhase, NewPhase);
    }
}

void AHDGameMode::SyncGameState() const
{
    AHDGameState* HDGameState = GetGameState<AHDGameState>();
    if (HDGameState == nullptr || MissionFlow.IsValid() == false)
    {
        return;
    }

    HDGameState->SetMissionSnapshot(
        MissionFlow->GetPhase(),
        MissionFlow->GetCompletedObjectives(),
        MissionFlow->GetRequiredObjectives(),
        MissionFlow->GetReinforcementsLeft(),
        MissionFlow->GetExtractionRemaining(),
        MissionFlow->IsPlayerInExtractionZone());
}

void AHDGameMode::HandlePhaseChanged(EHDMissionPhase OldPhase, EHDMissionPhase NewPhase)
{
    UE_LOG(LogTemp, Display, TEXT("[HDMission] phase %s -> %s"), *UEnum::GetValueAsString(OldPhase), *UEnum::GetValueAsString(NewPhase));

    UWorld* World = GetWorld();
    UHDSpawnDirectorSubsystem* Director = World ? World->GetSubsystem<UHDSpawnDirectorSubsystem>() : nullptr;
    if (Director == nullptr)
    {
        return;
    }

    if (NewPhase == EHDMissionPhase::InProgress)
    {
        Director->StartDirector();
    }
    else if (NewPhase == EHDMissionPhase::Success || NewPhase == EHDMissionPhase::Fail)
    {
        Director->StopDirector();
    }
}
