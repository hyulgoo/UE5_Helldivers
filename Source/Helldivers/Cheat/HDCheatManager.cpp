// Fill out your copyright notice in the Description page of Project Settings.

#include "Cheat/HDCheatManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayAbilityHelper.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Attribute/HDWeaponAttributeSet.h"
#include "Character/NPC/HDCharacterNonPlayer.h"
#include "Character/NPC/HDEnemyCharger.h"
#include "Character/NPC/HDEnemyShooter.h"
#include "Game/HDSpawnDirectorSubsystem.h"
#include "Game/HDGameMode.h"
#include "Game/HDGameState.h"
#include "Mission/HDExtractionZone.h"
#include "Mission/HDObjectiveFacility.h"
#include "Component/HDStratagemComponent.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "Player/HDGASPlayerState.h"
#include "AbilitySystem/GameplayEffect/HDGE_StratagemCooldown.h"
#include "UI/HDHUD.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Weapon/HDSupportWeapon.h"
#include "Weapon/Projectile/HDProjectileBase.h"
#include "Controller/HDPlayerController.h"
#include "Controller/HDDirectKeyRules.h"

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

void UHDCheatManager::HDStratagems()
{
    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    const UHDStratagemComponent* StratagemComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UHDStratagemComponent>() : nullptr;
    const FHDStratagemLoadout* Loadout = StratagemComponent ? StratagemComponent->GetLoadout() : nullptr;
    if (Loadout == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDStratagems] No stratagem loadout"));
        return;
    }

    for (int32 SlotIndex = 0; SlotIndex < Loadout->Num(); ++SlotIndex)
    {
        const FHDStratagemSlot* Slot = Loadout->GetSlot(SlotIndex);
        UE_LOG(LogTemp, Display, TEXT("[HDStratagems] %d %s cooldown=%.1f uses=%s available=%d"),
            SlotIndex + 1,
            *Slot->StratagemName.ToString(),
            StratagemComponent->GetSlotCooldownRemaining(SlotIndex),
            *Slot->GetUsesText(),
            StratagemComponent->IsSlotAvailable(SlotIndex) ? 1 : 0);
    }
}

void UHDCheatManager::HDStratagemReset()
{
    APlayerController* PlayerController = GetPlayerController();
    AHDGASPlayerState* GASPlayerState = PlayerController ? PlayerController->GetPlayerState<AHDGASPlayerState>() : nullptr;
    if (GASPlayerState == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDStratagemReset] No HDGASPlayerState"));
        return;
    }

    GASPlayerState->GetStratagemLoadout().ResetUses();
    UHDGE_StratagemCooldown::ClearAll(GASPlayerState->GetAbilitySystemComponent());
    UE_LOG(LogTemp, Display, TEXT("[HDStratagemReset] cooldowns cleared, uses reset"));
}

void UHDCheatManager::HDStratagemLoadout(const FString& Slot1, const FString& Slot2, const FString& Slot3, const FString& Slot4)
{
    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UHDStratagemComponent* StratagemComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UHDStratagemComponent>() : nullptr;
    if (StratagemComponent == nullptr || StratagemComponent->GetLoadout() == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDStratagemLoadout] No stratagem component or loadout"));
        return;
    }

    TArray<FName> LoadoutNames;
    for (const FString* SlotName : { &Slot1, &Slot2, &Slot3, &Slot4 })
    {
        if (SlotName->IsEmpty() == false)
        {
            LoadoutNames.Add(FName(**SlotName));
        }
    }

    StratagemComponent->ReinitializeLoadout(LoadoutNames);

    AHDHUD* HUD = PlayerController->GetHUD<AHDHUD>();
    if (HUD)
    {
        HUD->RebuildStratagemList();
    }

    const FHDStratagemLoadout* Loadout = StratagemComponent->GetLoadout();
    FString SlotNames;
    for (int32 SlotIndex = 0; SlotIndex < Loadout->Num(); ++SlotIndex)
    {
        if (SlotNames.IsEmpty() == false)
        {
            SlotNames += TEXT(", ");
        }
        SlotNames += Loadout->GetSlot(SlotIndex)->StratagemName.ToString();
    }

    UE_LOG(LogTemp, Display, TEXT("[HDStratagemLoadout] %d slots: %s"), Loadout->Num(), *SlotNames);
}

void UHDCheatManager::HDGiveSupportWeapon()
{
    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UWorld* World = GetWorld();
    if (PlayerPawn == nullptr || World == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDGiveSupportWeapon] No player pawn"));
        return;
    }

    // 앞 = 카메라가 보는 수평 방향(HDSpawnEnemy 와 같다)
    const float Yaw = PlayerController->GetControlRotation().Yaw;
    const FVector PlayerLocation = PlayerPawn->GetActorLocation();
    const FVector Column = PlayerLocation + FRotator(0.f, Yaw, 0.f).Vector() * 300.f;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HDGiveSupportWeapon), false, PlayerPawn);
    FHitResult GroundHit;
    const FVector TraceStart(Column.X, Column.Y, PlayerLocation.Z + 3000.f);
    const FVector TraceEnd(Column.X, Column.Y, PlayerLocation.Z - 10000.f);
    if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) == false)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDGiveSupportWeapon] No ground at %s"), *Column.ToString());
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const AHDSupportWeapon* SupportWeapon = World->SpawnActor<AHDSupportWeapon>(AHDSupportWeapon::StaticClass(), GroundHit.ImpactPoint, FRotator(0.f, Yaw, 0.f), SpawnParams);
    if (SupportWeapon == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDGiveSupportWeapon] Failed to spawn"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDGiveSupportWeapon] spawned at %s"), *SupportWeapon->GetActorLocation().ToString());
}

void UHDCheatManager::HDSelectWeapon(int32 SlotNumber)
{
    if (SlotNumber < 1 || SlotNumber > 4)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSelectWeapon] use 1-4"));
        return;
    }

    APlayerController* PlayerController = GetPlayerController();
    AHDCharacterPlayer* PlayerCharacter = PlayerController ? Cast<AHDCharacterPlayer>(PlayerController->GetPawn()) : nullptr;
    if (PlayerCharacter == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSelectWeapon] No player character"));
        return;
    }

    // 1~4 는 EHDWeaponSlot 의 Primary~Grenade 순서다
    PlayerCharacter->SelectWeaponSlot(static_cast<EHDWeaponSlot>(SlotNumber - 1));
}

void UHDCheatManager::HDSetGrenades(int32 Count)
{
    APlayerController* PlayerController = GetPlayerController();
    APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Pawn);
    if (ASC == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDGrenade] No player pawn or ability system component"));
        return;
    }

    const int32 MaxGrenade = FMath::RoundToInt32(ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxGrenadeAttribute()));
    const int32 NewCount = FMath::Clamp(Count, 0, MaxGrenade);
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetGrenadeAttribute(), static_cast<float>(NewCount));

    UE_LOG(LogTemp, Display, TEXT("[HDGrenade] set %d/%d"), NewCount, MaxGrenade);
}

void UHDCheatManager::HDSetStims(int32 Count)
{
    APlayerController* PlayerController = GetPlayerController();
    APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UAbilitySystemComponent* ASC = FGameplayAbilityHelper::GetAbilitySystemComponentFromActor(Pawn);
    if (ASC == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDStim] No player pawn or ability system component"));
        return;
    }

    const int32 MaxStim = FMath::RoundToInt32(ASC->GetNumericAttribute(UHDWeaponAttributeSet::GetMaxStimAttribute()));
    const int32 NewCount = FMath::Clamp(Count, 0, MaxStim);
    ASC->SetNumericAttributeBase(UHDWeaponAttributeSet::GetStimAttribute(), static_cast<float>(NewCount));

    UE_LOG(LogTemp, Display, TEXT("[HDStim] set %d/%d"), NewCount, MaxStim);
}

void UHDCheatManager::HDUseStim()
{
    APlayerController* PlayerController = GetPlayerController();
    AHDCharacterPlayer* HDPlayer = PlayerController ? Cast<AHDCharacterPlayer>(PlayerController->GetPawn()) : nullptr;
    if (HDPlayer == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDStim] No player character"));
        return;
    }

    HDPlayer->UseStim();
}

void UHDCheatManager::HDSimulateKeys(const FString& Sequence)
{
    TArray<TArray<FHDSimulatedKeyEvent>> Frames;
    FString Error;
    if (HDDirectKey::ParseSimulatedKeys(Sequence, Frames, Error) == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HDSimulateKeys] parse failed: %s"), *Error);
        return;
    }

    AHDPlayerController* PlayerController = Cast<AHDPlayerController>(GetOuterAPlayerController());
    if (PlayerController == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HDSimulateKeys] No HDPlayerController"));
        return;
    }

    PlayerController->SimulateKeyFrames(MoveTemp(Frames));
}

void UHDCheatManager::HDStopMontage()
{
    APlayerController* PlayerController = GetPlayerController();
    const ACharacter* PlayerCharacter = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
    UAnimInstance* AnimInstance = (PlayerCharacter && PlayerCharacter->GetMesh()) ? PlayerCharacter->GetMesh()->GetAnimInstance() : nullptr;
    if (AnimInstance == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDCheat] No player pawn or anim instance"));
        return;
    }

    const UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage();
    UE_LOG(LogTemp, Display, TEXT("[HDCheat] stop montage %s"), *GetNameSafe(CurrentMontage));

    // Montage_Stop 은 끊김(bInterrupted)으로 끝낸다
    AnimInstance->Montage_Stop(0.f);
}

void UHDCheatManager::HDShootEnemy(const FString& BoneName, int32 Penetration)
{
    constexpr float MuzzleForwardOffset = 150.f;

    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UWorld* World = GetWorld();
    if (PlayerPawn == nullptr || World == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDArmor] cheat: no player pawn"));
        return;
    }

    // 살아 있는 가장 가까운 적
    const FVector EyeLocation = PlayerPawn->GetPawnViewLocation();
    AHDCharacterNonPlayer* NearestEnemy = nullptr;
    double NearestDistanceSquared = TNumericLimits<double>::Max();
    for (TActorIterator<AHDCharacterNonPlayer> It(World); It; ++It)
    {
        AHDCharacterNonPlayer* Enemy = *It;
        if (IsValid(Enemy) == false || Enemy->IsDead())
        {
            continue;
        }

        const double DistanceSquared = FVector::DistSquared(EyeLocation, Enemy->GetActorLocation());
        if (DistanceSquared < NearestDistanceSquared)
        {
            NearestEnemy = Enemy;
            NearestDistanceSquared = DistanceSquared;
        }
    }

    if (NearestEnemy == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDArmor] cheat: no living enemy"));
        return;
    }

    const FName TargetBoneName(*BoneName);
    const USkeletalMeshComponent* EnemyMesh = NearestEnemy->GetMesh();
    if (EnemyMesh == nullptr || EnemyMesh->GetBoneIndex(TargetBoneName) == INDEX_NONE)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDArmor] cheat: %s has no bone '%s'"), *NearestEnemy->GetName(), *BoneName);
        return;
    }

    UClass* BulletClass = LoadClass<AHDProjectileBase>(nullptr, TEXT("/Game/Helldivers/Blueprint/Projectile/BPP_Bullet_Light.BPP_Bullet_Light_C"));
    if (BulletClass == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDArmor] cheat: failed to load BPP_Bullet_Light"));
        return;
    }

    const FVector ToBone = (EnemyMesh->GetBoneLocation(TargetBoneName) - EyeLocation).GetSafeNormal();
    const FVector SpawnLocation = EyeLocation + ToBone * MuzzleForwardOffset;

    // 소유자의 ASC 로 피해 GE 를 걸고, Instigator 를 넘겨야 탄이 쏜 캐릭터를 무시한다(AHDWeapon::SpawnProjectile 과 같다)
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = PlayerPawn;
    SpawnParams.Instigator = PlayerPawn;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AHDProjectileBase* Bullet = World->SpawnActor<AHDProjectileBase>(BulletClass, SpawnLocation, ToBone.Rotation(), SpawnParams);
    if (Bullet == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDArmor] cheat: failed to spawn bullet"));
        return;
    }

    if (Penetration > 0)
    {
        Bullet->SetArmorPenetration(Penetration);
    }

    UE_LOG(LogTemp, Display, TEXT("[HDArmor] cheat shot at %s bone=%s pen=%d"), *NearestEnemy->GetName(), *BoneName, Bullet->GetEffectiveArmorPenetration());
}
