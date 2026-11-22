// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGameMode.h"
#include "AI/HDEnemyAIController.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Game/HDGameState.h"
#include "Game/HDMissionSettings.h"
#include "Game/HDSpawnDirectorSettings.h"
#include "Game/HDSpawnDirectorSubsystem.h"
#include "Game/HDMissionPlacement.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "AbilitySystemComponent.h"
#include "Mission/HDExtractionZone.h"
#include "Mission/HDMissionObjective.h"
#include "Mission/HDObjectiveFacility.h"
#include "Controller/HDPlayerController.h"
#include "Progression/HDProfileSubsystem.h"
#include "Progression/HDProgressionSettings.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

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

namespace HDReinforce
{
    // 지면 찾기: 시체 Z + 낙하 높이 + 5000 에서 아래 50000 까지
    constexpr float GroundTraceExtraUp = 5000.f;
    constexpr float GroundTraceLength = 50000.f;

    // 남겨 둔 시체가 사라지기까지(초)
    constexpr float CorpseLifeSpan = 60.f;
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

void AHDGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // 로그인(첫 헬포드)이 StartPlay 보다 먼저라 여기서 정한다
    bInShip = GetDefault<UHDProgressionSettings>()->bStartInShip;
}

void AHDGameMode::StartPlay()
{
	Super::StartPlay();

    if (bInShip)
    {
        EnterShip();
    }
    else
    {
        BeginMission();
    }
}

bool AHDGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
    // 엔진은 로그인 때 이 값이 참일 때만 기본 폰을 스폰한다. 출격은 RequestDeploy 가 RestartPlayer 로 직접 부른다
    if (bInShip)
    {
        return false;
    }

    return Super::PlayerCanRestart_Implementation(Player);
}

bool AHDGameMode::RequestDeploy(APlayerController* PC)
{
    if (bInShip == false)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] deploy refused (not in ship)"));
        return false;
    }

    NULL_CHECK_WITH_RETURNTYPE(PC, false);

    UE_LOG(LogTemp, Display, TEXT("[HDShip] deploy"));

    bInShip = false;

    AHDGameState* HDGameState = GetGameState<AHDGameState>();
    if (HDGameState)
    {
        HDGameState->SetInShip(false);
    }

    UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
    if (ProfileSubsystem)
    {
        ProfileSubsystem->MarkTitleShown();
    }

    AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PC);
    if (HDPlayerController)
    {
        HDPlayerController->ExitShipMenu();
    }

    BeginMission();

    // 엔진 RestartPlayer 는 PlayerCanRestart 를 보지 않는다. 빙의가 시점을 헬포드로 옮긴다
    if (PC->GetPawn() == nullptr)
    {
        RestartPlayer(PC);
    }

    if (ShipCamera)
    {
        ShipCamera->Destroy();
        ShipCamera = nullptr;
    }

    return true;
}

bool AHDGameMode::IsInShip() const
{
    return bInShip;
}

void AHDGameMode::BeginMission()
{
    const UHDMissionSettings* Settings = GetDefault<UHDMissionSettings>();

    // 난이도 = 프로필의 단계(서브시스템이 없으면 새 프로필의 값)를 표 안으로 자른 행. 출격 뒤에 프로필을 바꿔도 이 미션은 그대로다
    const TArray<FHDDifficultyLevel>& DifficultyTable = GetDefault<UHDProgressionSettings>()->DifficultyLevels;
    const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
    const int32 ProfileDifficulty = ProfileSubsystem ? ProfileSubsystem->GetProfile().Difficulty : FHDProfileData().Difficulty;
    MissionDifficultyLevel = HDDifficulty::ClampLevel(ProfileDifficulty, DifficultyTable.Num());
    MissionDifficulty = HDDifficulty::GetLevel(DifficultyTable, MissionDifficultyLevel);

    FHDMissionConfig Config = HDDifficulty::ApplyToConfig(Settings->MakeConfig(), MissionDifficulty);

    // 목표 배치를 미션 로직보다 먼저 한다. 필요 수는 실제로 있는 목표 수를 넘지 않게(최소 1)
    const int32 ObjectiveCount = PlaceObjectives(*Settings, Config.RequiredObjectives);
    Config.RequiredObjectives = FMath::Max(1, FMath::Min(ObjectiveCount, Config.RequiredObjectives));

    MissionFlow = MakeUnique<FHDMissionFlow>(Config);
    SyncGameState();

    UE_LOG(LogTemp, Display, TEXT("[HDMission] start objectives=%d reinforcements=%d difficulty=%d %s"),
        MissionFlow->GetRequiredObjectives(),
        MissionFlow->GetReinforcementsLeft(),
        MissionDifficultyLevel,
        *MissionDifficulty.Label);
}

void AHDGameMode::EnterShip()
{
    UWorld* World = GetWorld();
    NULL_CHECK(World);

    // 바라볼 곳 = 첫 PlayerStart(없으면 원점)
    FVector LookAtLocation = FVector::ZeroVector;
    TActorIterator<APlayerStart> PlayerStartIt(World);
    if (PlayerStartIt)
    {
        LookAtLocation = PlayerStartIt->GetActorLocation();
    }

    const FVector CameraLocation = LookAtLocation + GetDefault<UHDProgressionSettings>()->ShipCameraOffset;
    const FRotator CameraRotation = (LookAtLocation - CameraLocation).Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ShipCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraLocation, CameraRotation, SpawnParams);
    NULL_CHECK(ShipCamera);

    // 화면 비율이 달라도 검은 띠를 넣지 않는다
    ShipCamera->GetCameraComponent()->SetConstraintAspectRatio(false);

    const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
    const bool bShowTitle = ProfileSubsystem == nullptr || ProfileSubsystem->HasShownTitle() == false;

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();
        if (PlayerController == nullptr)
        {
            continue;
        }

        PlayerController->SetViewTarget(ShipCamera);

        AHDPlayerController* HDPlayerController = Cast<AHDPlayerController>(PlayerController);
        if (HDPlayerController)
        {
            HDPlayerController->EnterShipMenu(bShowTitle);
        }
    }

    AHDGameState* HDGameState = GetGameState<AHDGameState>();
    if (HDGameState)
    {
        HDGameState->SetInShip(true);
    }

    UE_LOG(LogTemp, Display, TEXT("[HDShip] enter title=%d camera=%s"), bShowTitle ? 1 : 0, *CameraLocation.ToString());
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

    // 증원 대기 남은 시간. 타이머가 없으면 GetTimerRemaining 이 -1 을 돌려준다
    AHDGameState* HDGameState = GetGameState<AHDGameState>();
    if (HDGameState)
    {
        HDGameState->SetRespawnRemaining(GetWorldTimerManager().GetTimerRemaining(ReinforceTimerHandle));
    }
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

    // 남은 증원이 0 이면 로직이 이미 Fail 로 바꿨다
    if (MissionFlow.IsValid() == false || MissionFlow->IsFinished() || MissionFlow->GetReinforcementsLeft() < 1)
    {
        return;
    }

    APlayerController* DeadPlayerController = Cast<APlayerController>(DeadController);
    NULL_CHECK(DeadPlayerController);

    const float RespawnDelay = GetDefault<UHDMissionSettings>()->RespawnDelay;
    const TWeakObjectPtr<APlayerController> WeakPlayerController(DeadPlayerController);
    GetWorldTimerManager().SetTimer(ReinforceTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, WeakPlayerController]()
    {
        Reinforce(WeakPlayerController.Get());
    }), RespawnDelay, false);

    AHDGameState* HDGameState = GetGameState<AHDGameState>();
    if (HDGameState)
    {
        HDGameState->SetRespawnRemaining(RespawnDelay);
    }
}

EHDExtractionCallResult AHDGameMode::RequestExtractionCall()
{
    EHDExtractionCallResult Result = EHDExtractionCallResult::NotExtraction;
    ApplyMissionChange([&Result](FHDMissionFlow& Flow) { Result = Flow.CallExtraction(); });

    if (Result != EHDExtractionCallResult::Called)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDMission] extraction call refused (%s)"), HDMissionRules::GetCallResultName(Result));
        return Result;
    }

    // 호출이 필요한 설정이면 강도는 추출 단계가 될 때가 아니라 여기서 오른다. 난이도 강도를 곱한다(NORMAL 은 그대로)
    const float Intensity = HDDifficulty::GetExtractionIntensity(GetDefault<UHDSpawnDirectorSettings>()->ExtractionIntensity, MissionDifficulty);
    UWorld* World = GetWorld();
    UHDSpawnDirectorSubsystem* Director = World ? World->GetSubsystem<UHDSpawnDirectorSubsystem>() : nullptr;
    if (Director)
    {
        Director->SetIntensity(Intensity);
    }

    UE_LOG(LogTemp, Display, TEXT("[HDMission] extraction called intensity=%.1f"), Intensity);
    return Result;
}

const FHDMissionFlow* AHDGameMode::GetMissionFlow() const
{
    return MissionFlow.Get();
}

FHDMissionFlow* AHDGameMode::GetMissionFlowForDebug()
{
    return MissionFlow.Get();
}

void AHDGameMode::DebugSetReinforcementsLeft(int32 Count)
{
    ApplyMissionChange([Count](FHDMissionFlow& Flow) { Flow.DebugSetReinforcementsLeft(Count); });
}

void AHDGameMode::DebugSetMissionTimeLimit(float Seconds)
{
    ApplyMissionChange([Seconds](FHDMissionFlow& Flow) { Flow.DebugSetMissionTimeLimit(Seconds); });
}

int32 AHDGameMode::PlaceObjectives(const UHDMissionSettings& Settings, int32 RequiredCount)
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
    const int32 MissingCount = RequiredCount - PlacedCount;
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

void AHDGameMode::SpawnExtractionZone(const UHDMissionSettings& Settings)
{
    using namespace HDPlaceObjective;

    UWorld* World = GetWorld();
    NULL_CHECK(World);

    // 중심 = 플레이어 폰(없으면 첫 PlayerStart, 그것도 없으면 원점)
    FVector Center = FVector::ZeroVector;
    APlayerController* PlayerController = World->GetFirstPlayerController();
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (PlayerPawn)
    {
        Center = PlayerPawn->GetActorLocation();
    }
    else
    {
        TActorIterator<APlayerStart> PlayerStartIt(World);
        if (PlayerStartIt)
        {
            Center = PlayerStartIt->GetActorLocation();
        }
    }

    // 목표 배치와 같은 선 검사. 지면을 못 찾으면 같은 링에서 다시 뽑는다
    FRandomStream Stream(FMath::Rand());
    FVector GroundLocation = FVector::ZeroVector;
    bool bFoundGround = false;
    for (int32 Attempt = 0; Attempt <= MaxGroundRetries && bFoundGround == false; ++Attempt)
    {
        const TArray<FVector2D> Offsets = HDMissionPlacement::ComputeRingOffsets(1, Settings.ExtractionDistanceMin, Settings.ExtractionDistanceMax, 0.f, Stream);
        bFoundGround = FindGround(World, Center, Offsets[0], GroundLocation);
    }

    if (bFoundGround == false)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDMission] no ground for the extraction zone around %s"), *Center.ToString());
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const AHDExtractionZone* ExtractionZone = World->SpawnActor<AHDExtractionZone>(AHDExtractionZone::StaticClass(), GroundLocation, FRotator::ZeroRotator, SpawnParams);
    NULL_CHECK(ExtractionZone);

    UE_LOG(LogTemp, Display, TEXT("[HDMission] extraction zone spawned %.0fm"), FVector::Distance(Center, GroundLocation) / 100.f);
}

void AHDGameMode::StopForMissionEnd()
{
    UWorld* World = GetWorld();
    NULL_CHECK(World);

    // 증원을 기다리던 중에 끝나면 예약을 지운다. 이후 Tick 은 GetTimerRemaining 이 -1 이라 RespawnRemaining 이 -1 로 남는다
    GetWorldTimerManager().ClearTimer(ReinforceTimerHandle);
    AHDGameState* HDGameState = GetGameState<AHDGameState>();
    if (HDGameState)
    {
        HDGameState->SetRespawnRemaining(-1.f);
    }

    for (TActorIterator<AHDEnemyAIController> It(World); It; ++It)
    {
        It->StopForMissionEnd();
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();
        if (PlayerController == nullptr)
        {
            continue;
        }

        // 이동·시점·능력 입력은 컨트롤러 InputComponent 에 묶여 있어(AHDPlayerController::SetupInputComponent) 컨트롤러 자신도 막는다
        PlayerController->DisableInput(PlayerController);

        // 스트라타젬 명령은 캐릭터 InputComponent 에 묶여 있다(AHDCharacterPlayer::SetupPlayerInputComponent)
        APawn* PlayerPawn = PlayerController->GetPawn();
        if (PlayerPawn)
        {
            PlayerPawn->DisableInput(PlayerController);
        }
    }
}

void AHDGameMode::Reinforce(APlayerController* PC)
{
    using namespace HDReinforce;

    NULL_CHECK(PC);

    bool bConsumed = false;
    ApplyMissionChange([&bConsumed](FHDMissionFlow& Flow) { bConsumed = Flow.TryConsumeReinforcement(); });
    if (bConsumed == false)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDMission] reinforce skipped"));
        return;
    }

    // PlayerState ASC 는 캐릭터가 바뀌어도 남는다. 사망 태그를 지우고, 활성 BPGA_Dead 를 끝내 Character.State.IsDead 도 뺀다
    UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(PC->PlayerState);
    NULL_CHECK(ASC);

    ASC->SetLooseGameplayTagCount(Tag_Character_Action_Dead, 0);
    ASC->SetLooseGameplayTagCount(Tag_Trigger_Dead, 0);
    ASC->CancelAllAbilities();

    // 시체는 남겨 두었다가 지운다
    APawn* Corpse = PC->GetPawn();
    NULL_CHECK(Corpse);

    PC->UnPossess();
    Corpse->SetLifeSpan(CorpseLifeSpan);

    // 위치: 시체에서 무작위 방향 ReinforceOffsetMin~Max, 그 XY 의 지면 + ReinforceDropHeight
    const UHDMissionSettings* Settings = GetDefault<UHDMissionSettings>();
    const FVector CorpseLocation = Corpse->GetActorLocation();
    const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
    const float Distance = FMath::FRandRange(Settings->ReinforceOffsetMin, Settings->ReinforceOffsetMax);

    FVector DropLocation(CorpseLocation.X + FMath::Cos(Angle) * Distance, CorpseLocation.Y + FMath::Sin(Angle) * Distance, CorpseLocation.Z + Settings->ReinforceDropHeight);

    UWorld* World = GetWorld();
    NULL_CHECK(World);

    const FVector TraceStart(DropLocation.X, DropLocation.Y, CorpseLocation.Z + Settings->ReinforceDropHeight + GroundTraceExtraUp);
    const FVector TraceEnd(TraceStart.X, TraceStart.Y, TraceStart.Z - GroundTraceLength);

    FHitResult GroundHit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDReinforce), false);
    QueryParams.AddIgnoredActor(Corpse);
    if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
    {
        DropLocation.Z = GroundHit.ImpactPoint.Z + Settings->ReinforceDropHeight;
    }

    // 헬포드 = 게임모드의 기본 폰. 착지하면 캐릭터를 스폰·빙의하고 HandlePossessedPawnChanged 가 NotifyPlayerRespawned 를 부른다
    UClass* HellpodClass = GetDefaultPawnClassForController(PC);
    NULL_CHECK(HellpodClass);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APawn* Hellpod = World->SpawnActor<APawn>(HellpodClass, DropLocation, FRotator::ZeroRotator, SpawnParams);
    NULL_CHECK(Hellpod);

    PC->Possess(Hellpod);

    AHDGameState* HDGameState = GetGameState<AHDGameState>();
    if (HDGameState)
    {
        HDGameState->SetRespawnRemaining(-1.f);
    }

    UE_LOG(LogTemp, Display, TEXT("[HDMission] reinforce left=%d"), MissionFlow->GetReinforcementsLeft());
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
        MissionFlow->IsPlayerInExtractionZone(),
        MissionFlow->IsExtractionCalled());
}

void AHDGameMode::HandlePhaseChanged(EHDMissionPhase OldPhase, EHDMissionPhase NewPhase)
{
    UE_LOG(LogTemp, Display, TEXT("[HDMission] phase %s -> %s"), *UEnum::GetValueAsString(OldPhase), *UEnum::GetValueAsString(NewPhase));

    UWorld* World = GetWorld();
    UHDSpawnDirectorSubsystem* Director = World ? World->GetSubsystem<UHDSpawnDirectorSubsystem>() : nullptr;

    if (NewPhase == EHDMissionPhase::InProgress)
    {
        if (Director)
        {
            // 난이도 행의 강도·중장갑 비율로 켠다(NORMAL = 설정 값과 같은 1.0·0.1)
            Director->SetIntensity(MissionDifficulty.SpawnIntensity);
            Director->SetHeavyRatioOverride(MissionDifficulty.HeavyRatio);
            Director->StartDirector();
        }
    }
    else if (NewPhase == EHDMissionPhase::Extraction)
    {
        SpawnExtractionZone(*GetDefault<UHDMissionSettings>());

        // 설정이 호출 불필요일 때만 여기서 강도를 올린다. 그 설정이면 추출 단계가 되는 순간 IsExtractionCalled 가 참이다.
        // 호출이 필요하면 아직 거짓이고, 강도는 RequestExtractionCall 이 부를 때 올린다
        if (Director && MissionFlow.IsValid() && MissionFlow->IsExtractionCalled())
        {
            Director->SetIntensity(HDDifficulty::GetExtractionIntensity(GetDefault<UHDSpawnDirectorSettings>()->ExtractionIntensity, MissionDifficulty));
        }
    }
    else if (NewPhase == EHDMissionPhase::Success || NewPhase == EHDMissionPhase::Fail)
    {
        if (Director)
        {
            Director->StopDirector();
        }

        StopForMissionEnd();

        UE_LOG(LogTemp, Display, TEXT("[HDMission] mission %s"), NewPhase == EHDMissionPhase::Success ? TEXT("Success") : TEXT("Fail"));
    }
}
