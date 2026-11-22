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
#include "Character/NPC/HDEnemyHeavy.h"
#include "Game/HDSpawnDirectorSubsystem.h"
#include "Game/HDGameMode.h"
#include "Game/HDGameState.h"
#include "Mission/HDExtractionZone.h"
#include "Mission/HDObjectiveFacility.h"
#include "Mission/HDSamplePickup.h"
#include "Component/HDStratagemComponent.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "Player/HDGASPlayerState.h"
#include "AbilitySystem/GameplayEffect/HDGE_StratagemCooldown.h"
#include "UI/HDHUD.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Weapon/HDSupportWeapon.h"
#include "Weapon/Projectile/HDProjectileBase.h"
#include "Weapon/Projectile/HDProjectileGrenade.h"
#include "Controller/HDPlayerController.h"
#include "Controller/HDDirectKeyRules.h"
#include "Progression/HDProfileSubsystem.h"
#include "Progression/HDProgressionRules.h"
#include "Progression/HDProgressionSettings.h"
#include "Progression/HDDifficulty.h"
#include "Progression/HDMissionReward.h"
#include "Progression/HDSampleRules.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Progression/HDUpgradeRules.h"
#include "Progression/HDArmoryRules.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Stratagem/HDStratagemSettings.h"
#include "Ship/HDShipHubRules.h"
#include "Ship/HDShipInterior.h"

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
            { TEXT("Heavy"), AHDEnemyHeavy::StaticClass() },
        };
    }

    // From 에서 가장 가까운 살아 있는 적. 없으면 nullptr
    AHDCharacterNonPlayer* FindNearestLivingEnemy(UWorld* World, const FVector& From)
    {
        AHDCharacterNonPlayer* NearestEnemy = nullptr;
        double NearestDistanceSquared = TNumericLimits<double>::Max();
        for (TActorIterator<AHDCharacterNonPlayer> It(World); It; ++It)
        {
            AHDCharacterNonPlayer* Enemy = *It;
            if (IsValid(Enemy) == false || Enemy->IsDead())
            {
                continue;
            }

            const double DistanceSquared = FVector::DistSquared(From, Enemy->GetActorLocation());
            if (DistanceSquared < NearestDistanceSquared)
            {
                NearestEnemy = Enemy;
                NearestDistanceSquared = DistanceSquared;
            }
        }

        return NearestEnemy;
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

    const FString Status = FString::Printf(TEXT("Phase=%s Objectives=%d/%d Reinforcements=%d Extraction=%.1f InZone=%d Called=%d Ship=%d"),
        *StaticEnum<EHDMissionPhase>()->GetNameStringByValue(static_cast<int64>(HDGameState->GetMissionPhase())),
        HDGameState->GetCompletedObjectives(),
        HDGameState->GetRequiredObjectives(),
        HDGameState->GetReinforcementsLeft(),
        HDGameState->GetExtractionRemaining(),
        HDGameState->IsPlayerInExtractionZone() ? 1 : 0,
        HDGameState->IsExtractionCalled() ? 1 : 0,
        HDGameState->IsInShip() ? 1 : 0);

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

void UHDCheatManager::HDCallExtraction()
{
    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    if (HDGameMode == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDCallExtraction] No HDGameMode in this world"));
        return;
    }

    HDGameMode->RequestExtractionCall();
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
        UE_LOG(LogTemp, Display, TEXT("[HDStratagems] %d %s cooldown=%.1f uses=%s available=%d maxCooldown=%.1f delay=%.1f rearm=%.0f"),
            SlotIndex + 1,
            *Slot->StratagemName.ToString(),
            StratagemComponent->GetSlotCooldownRemaining(SlotIndex),
            *Slot->GetUsesText(),
            StratagemComponent->IsSlotAvailable(SlotIndex) ? 1 : 0,
            Slot->Cooldown,
            Slot->ActivationDelay,
            Slot->RearmTime);
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

void UHDCheatManager::HDDive(const FString& Command)
{
    APlayerController* PlayerController = GetPlayerController();
    AHDCharacterPlayer* HDPlayer = PlayerController ? Cast<AHDCharacterPlayer>(PlayerController->GetPawn()) : nullptr;
    if (HDPlayer == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDDive] No player character"));
        return;
    }

    if (Command.Equals(TEXT("status"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Display, TEXT("[HDDive] status %s"), *HDPlayer->GetDiveStatusText());
        return;
    }

    HDPlayer->Dive();
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
    AHDCharacterNonPlayer* NearestEnemy = HDCheatManager::FindNearestLivingEnemy(World, EyeLocation);
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

void UHDCheatManager::HDExplodeAtEnemy(float Offset)
{
    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UWorld* World = GetWorld();
    if (PlayerPawn == nullptr || World == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDExplode] no player pawn"));
        return;
    }

    // 살아 있는 가장 가까운 적(HDShootEnemy 와 같은 고르기)
    const AHDCharacterNonPlayer* NearestEnemy = HDCheatManager::FindNearestLivingEnemy(World, PlayerPawn->GetPawnViewLocation());
    if (NearestEnemy == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDExplode] no enemy"));
        return;
    }

    // 래그돌 중에는 메시만 날아가고 캡슐(액터 위치)은 제자리에 남는다
    const USkeletalMeshComponent* EnemyMesh = NearestEnemy->GetMesh();
    const bool bRagdoll = EnemyMesh && EnemyMesh->IsSimulatingPhysics();
    const FVector BodyLocation = NearestEnemy->GetBodyLocation();

    // 터지는 자리 = 몸 위치에서 플레이어 쪽 수평 방향으로 Offset
    const FVector ToPlayer = (PlayerPawn->GetActorLocation() - BodyLocation).GetSafeNormal2D();
    const FVector ExplodeLocation = BodyLocation + ToPlayer * Offset;

    // 소유자의 ASC 로 피해 GE 를 건다(AHDCharacterPlayer 의 수류탄 투척과 같다)
    AHDProjectileGrenade* Grenade = World->SpawnActorDeferred<AHDProjectileGrenade>(AHDProjectileGrenade::StaticClass(), FTransform(ExplodeLocation), PlayerPawn, PlayerPawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (Grenade == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDExplode] failed to spawn grenade"));
        return;
    }

    Grenade->FinishSpawning(FTransform(ExplodeLocation));

    UE_LOG(LogTemp, Display, TEXT("[HDExplode] target=%s body=%.0f capsule=%.0f ragdoll=%d"),
        *NearestEnemy->GetName(),
        FVector::Dist(ExplodeLocation, BodyLocation),
        FVector::Dist(ExplodeLocation, NearestEnemy->GetActorLocation()),
        bRagdoll ? 1 : 0);

    Grenade->Explode();
}

void UHDCheatManager::HDProfile()
{
    const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(GetPlayerController());
    if (ProfileSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDProfile] No profile subsystem"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDProfile] slot=%s file=%d %s"),
        *ProfileSubsystem->GetSlotName(),
        ProfileSubsystem->HasSaveFile() ? 1 : 0,
        *HDProgression::Describe(ProfileSubsystem->GetProfile()));
}

void UHDCheatManager::HDGiveResources(int32 Requisition, int32 Common, int32 Rare, int32 Super)
{
    UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(GetPlayerController());
    if (ProfileSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDProfile] No profile subsystem"));
        return;
    }

    FHDSampleCounts Samples;
    Samples.Common = Common;
    Samples.Rare = Rare;
    Samples.Super = Super;

    ProfileSubsystem->ModifyProfile([Requisition, &Samples](FHDProfileData& Data)
    {
        HDProgression::AddResources(Data, Requisition, Samples);
    });
}

void UHDCheatManager::HDResetProfile()
{
    UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(GetPlayerController());
    if (ProfileSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDProfile] No profile subsystem"));
        return;
    }

    ProfileSubsystem->ResetProfile();
}

void UHDCheatManager::HDDeploy()
{
    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    if (HDGameMode == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] No HDGameMode in this world"));
        return;
    }

    HDGameMode->RequestDeploy(GetPlayerController());
}

void UHDCheatManager::HDShip()
{
    UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    const AHDPlayerController* PlayerController = Cast<AHDPlayerController>(GetOuterAPlayerController());
    if (HDGameState == nullptr || PlayerController == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] No HDGameState or HDPlayerController"));
        return;
    }

    TArray<FString> Labels;
    for (const FHDShipMenuItem& Item : PlayerController->BuildShipMenuItems())
    {
        Labels.Add(Item.Label);
    }

    const FHDShipMenu& ShipMenu = PlayerController->GetShipMenu();
    UE_LOG(LogTemp, Display, TEXT("[HDShip] inShip=%d active=%d screen=%s cursor=%d items=%s"),
        HDGameState->IsInShip() ? 1 : 0,
        PlayerController->IsShipMenuActive() ? 1 : 0,
        HDShipMenu::GetScreenName(ShipMenu.GetScreen()),
        ShipMenu.GetCursor(),
        *FString::Join(Labels, TEXT("|")));
}

void UHDCheatManager::HDSetDifficulty(int32 Level)
{
    UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(GetPlayerController());
    if (ProfileSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDProfile] No profile subsystem"));
        return;
    }

    // 자르기는 ModifyProfile 의 정리(Sanitize)가 한다
    ProfileSubsystem->ModifyProfile([Level](FHDProfileData& Data)
    {
        Data.Difficulty = Level;
    });

    const TArray<FHDDifficultyLevel>& DifficultyTable = GetDefault<UHDProgressionSettings>()->DifficultyLevels;
    const int32 NewLevel = ProfileSubsystem->GetProfile().Difficulty;
    UE_LOG(LogTemp, Display, TEXT("[HDShip] difficulty %d %s (next deploy)"), NewLevel, *HDDifficulty::GetLevel(DifficultyTable, NewLevel).Label);
}

void UHDCheatManager::HDAbandon()
{
    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    if (HDGameMode == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDMission] No HDGameMode in this world"));
        return;
    }

    HDGameMode->RequestAbandon();
}

void UHDCheatManager::HDReturnToShip()
{
    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    if (HDGameMode == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] No HDGameMode in this world"));
        return;
    }

    HDGameMode->ReturnToShip();
}

void UHDCheatManager::HDMissionResult()
{
    UWorld* World = GetWorld();
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    if (HDGameState == nullptr || HDGameState->HasMissionResult() == false)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDMission] no result"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDMission] result %s"), *HDProgression::DescribeResult(HDGameState->GetMissionResult()));
}

void UHDCheatManager::HDSamples()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSample] No world"));
        return;
    }

    const APlayerController* PlayerController = GetPlayerController();
    const AHDGASPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AHDGASPlayerState>() : nullptr;
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;

    FHDSampleCounts WorldSamples;
    int32 BundleCount = 0;
    double NearestDistance = -1.0;
    for (TActorIterator<AHDSamplePickup> It(World); It; ++It)
    {
        const AHDSamplePickup* Sample = *It;
        if (IsValid(Sample) == false)
        {
            continue;
        }

        if (Sample->IsDroppedBundle())
        {
            ++BundleCount;
        }
        else
        {
            WorldSamples.Common += Sample->GetContents().Common;
            WorldSamples.Rare   += Sample->GetContents().Rare;
            WorldSamples.Super  += Sample->GetContents().Super;
        }

        if (PlayerPawn)
        {
            const double Distance = FVector::Distance(PlayerPawn->GetActorLocation(), Sample->GetActorLocation());
            if (NearestDistance < 0.0 || Distance < NearestDistance)
            {
                NearestDistance = Distance;
            }
        }
    }

    const FString CarriedText = PlayerState ? HDSamples::Describe(PlayerState->GetCarriedSamples()) : FString(TEXT("none"));
    UE_LOG(LogTemp, Display, TEXT("[HDSample] carried=%s world common=%d rare=%d super=%d bundles=%d nearest=%dm"),
        *CarriedText,
        WorldSamples.Common,
        WorldSamples.Rare,
        WorldSamples.Super,
        BundleCount,
        NearestDistance < 0.0 ? -1 : FMath::RoundToInt(NearestDistance / 100.0));
}

void UHDCheatManager::HDTeleportToSample(const FString& Type)
{
    UWorld* World = GetWorld();
    APlayerController* PlayerController = GetPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (World == nullptr || PlayerPawn == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSample] No player pawn"));
        return;
    }

    // 빈 글자 = 아무 샘플, bundle = 떨군 묶음, 그 밖은 한 종류의 낱개
    const FString Filter = Type.TrimStartAndEnd();
    const bool bAny = Filter.IsEmpty();
    const bool bBundle = Filter.Equals(TEXT("bundle"), ESearchCase::IgnoreCase);
    EHDSampleType SampleType = EHDSampleType::Count;
    if (bAny == false && bBundle == false && HDSamples::TryParseType(Filter, SampleType) == false)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSample] unknown sample type %s (common|rare|super|bundle)"), *Filter);
        return;
    }

    const FVector From = PlayerPawn->GetActorLocation();
    const AHDSamplePickup* Nearest = nullptr;
    double NearestDistance = TNumericLimits<double>::Max();
    for (TActorIterator<AHDSamplePickup> It(World); It; ++It)
    {
        const AHDSamplePickup* Sample = *It;
        if (IsValid(Sample) == false)
        {
            continue;
        }

        const bool bMatches = bAny
            || (bBundle && Sample->IsDroppedBundle())
            || (bBundle == false && Sample->IsDroppedBundle() == false && Sample->GetContents().Get(SampleType) > 0);
        if (bMatches == false)
        {
            continue;
        }

        const double Distance = FVector::Distance(From, Sample->GetActorLocation());
        if (Distance < NearestDistance)
        {
            NearestDistance = Distance;
            Nearest = Sample;
        }
    }

    const FString FilterName = bAny ? FString(TEXT("any")) : Filter.ToLower();
    if (Nearest == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSample] no sample (%s)"), *FilterName);
        return;
    }

    // HDTeleportToExtraction 과 같은 방식: 위 200
    const FVector Destination = Nearest->GetActorLocation() + FVector(0.f, 0.f, 200.f);
    const bool bTeleported = PlayerPawn->TeleportTo(Destination, PlayerPawn->GetActorRotation());
    UE_LOG(LogTemp, Display, TEXT("[HDSample] teleport to %s %dm%s"),
        *FilterName,
        FMath::RoundToInt(NearestDistance / 100.0),
        bTeleported ? TEXT("") : TEXT(" (failed)"));
}

void UHDCheatManager::HDGiveSamples(int32 Common, int32 Rare, int32 Super)
{
    APlayerController* PlayerController = GetPlayerController();
    AHDGASPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AHDGASPlayerState>() : nullptr;
    if (PlayerState == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDSample] No player state"));
        return;
    }

    FHDSampleCounts Samples;
    Samples.Common = Common;
    Samples.Rare = Rare;
    Samples.Super = Super;
    PlayerState->AddCarriedSamples(Samples);

    UE_LOG(LogTemp, Display, TEXT("[HDSample] carried=%s"), *HDSamples::Describe(PlayerState->GetCarriedSamples()));
}

void UHDCheatManager::HDUpgrades()
{
    const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(GetPlayerController());
    if (ProfileSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDUpgrade] No profile subsystem"));
        return;
    }

    const FHDProfileData& Profile = ProfileSubsystem->GetProfile();
    const FHDUpgradeCatalog& Catalog = FHDUpgradeCatalog::GetProjectCatalog();
    for (const FName& UpgradeName : Catalog.GetNames())
    {
        const FHDUpgradeDefinition* Definition = Catalog.Find(UpgradeName);
        if (Definition == nullptr)
        {
            continue;
        }

        const int32 LevelCount = Definition->Levels.Num();
        const int32 Level = FMath::Clamp(HDUpgrade::GetLevel(Profile, UpgradeName), 0, LevelCount);
        const FHDUpgradeLevel* NextLevel = HDUpgrade::GetNextLevel(*Definition, Level);
        UE_LOG(LogTemp, Display, TEXT("[HDUpgrade] %s level=%d/%d next=%s"),
            *UpgradeName.ToString(),
            Level,
            LevelCount,
            NextLevel ? *HDUpgrade::FormatCost(NextLevel->ToCost()) : TEXT("max"));
    }

    UE_LOG(LogTemp, Display, TEXT("[HDUpgrade] active %s"), *HDUpgrade::DescribeModifiers(HDUpgrade::ComputeModifiers(Profile, Catalog)));
}

void UHDCheatManager::HDBuyUpgrade(const FString& Name)
{
    UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(GetPlayerController());
    if (ProfileSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDUpgrade] No profile subsystem"));
        return;
    }

    // 메뉴(AHDPlayerController::ExecuteShipMenuAction 의 Select)와 같은 경로·같은 로그. 거부도 저장 경로를 탄다(프로필은 그대로)
    const FHDUpgradeCatalog& Catalog = FHDUpgradeCatalog::GetProjectCatalog();
    const FName UpgradeName(*Name);
    const int32 OldRequisition = ProfileSubsystem->GetProfile().Requisition;
    EHDUpgradeBuyResult Result = EHDUpgradeBuyResult::Unknown;
    ProfileSubsystem->ModifyProfile([&Result, &Catalog, UpgradeName](FHDProfileData& Data)
    {
        Result = HDUpgrade::TryBuy(Data, UpgradeName, Catalog);
    });

    const FHDProfileData& Profile = ProfileSubsystem->GetProfile();
    UE_LOG(LogTemp, Display, TEXT("[HDUpgrade] buy %s -> %d (%s) requisition %d -> %d"),
        *UpgradeName.ToString(),
        HDUpgrade::GetLevel(Profile, UpgradeName),
        HDUpgrade::GetBuyResultName(Result),
        OldRequisition,
        Profile.Requisition);
}

void UHDCheatManager::HDArmory()
{
    const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(GetPlayerController());
    if (ProfileSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDArmory] No profile subsystem"));
        return;
    }

    // 이 멤버 함수와 이름이 같은 규칙 네임스페이스는 :: 로 부른다
    const FHDProfileData& Profile = ProfileSubsystem->GetProfile();
    const FHDStratagemCatalog& Catalog = FHDStratagemCatalog::GetProjectCatalog();
    for (const FName& StratagemName : Catalog.GetNames())
    {
        const FHDStratagemDefinition* Definition = Catalog.Find(StratagemName);
        if (Definition == nullptr)
        {
            continue;
        }

        UE_LOG(LogTemp, Display, TEXT("[HDArmory] %s cost=%d unlocked=%d"),
            *StratagemName.ToString(),
            Definition->UnlockCost,
            ::HDArmory::IsUnlocked(Profile, Catalog, StratagemName) ? 1 : 0);
    }

    // 다음 출격의 칸(UHDStratagemComponent::BindLoadout 과 같은 정리)
    const TArray<FName> Loadout = ::HDArmory::ValidateLoadout(Profile.Loadout, Profile, Catalog, GetDefault<UHDStratagemSettings>()->Loadout);
    TArray<FString> LoadoutNames;
    for (const FName& StratagemName : Loadout)
    {
        LoadoutNames.Add(StratagemName.ToString());
    }

    UE_LOG(LogTemp, Display, TEXT("[HDArmory] loadout %s"), *FString::Join(LoadoutNames, TEXT(",")));
}

void UHDCheatManager::HDUnlockStratagem(const FString& Name)
{
    UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(GetPlayerController());
    if (ProfileSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDArmory] No profile subsystem"));
        return;
    }

    // 메뉴(AHDPlayerController::ExecuteShipMenuAction 의 Armory Select)와 같은 경로·같은 로그. 거부도 저장 경로를 탄다(프로필은 그대로)
    const FHDStratagemCatalog& Catalog = FHDStratagemCatalog::GetProjectCatalog();
    const FName StratagemName(*Name);
    const int32 OldRequisition = ProfileSubsystem->GetProfile().Requisition;
    EHDUnlockResult Result = EHDUnlockResult::Unknown;
    ProfileSubsystem->ModifyProfile([&Result, &Catalog, StratagemName](FHDProfileData& Data)
    {
        Result = ::HDArmory::TryUnlock(Data, Catalog, StratagemName);
    });

    UE_LOG(LogTemp, Display, TEXT("[HDArmory] unlock %s (%s) requisition %d -> %d"),
        *StratagemName.ToString(),
        ::HDArmory::GetUnlockResultName(Result),
        OldRequisition,
        ProfileSubsystem->GetProfile().Requisition);
}

void UHDCheatManager::HDShipHub()
{
    UWorld* World = GetWorld();
    const AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    const AHDGameState* HDGameState = World ? World->GetGameState<AHDGameState>() : nullptr;
    const AHDPlayerController* PlayerController = Cast<AHDPlayerController>(GetOuterAPlayerController());
    if (HDGameMode == nullptr || HDGameState == nullptr || PlayerController == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShipHub] No HDGameMode, HDGameState or HDPlayerController"));
        return;
    }

    // 이 멤버 함수와 이름이 같은 규칙 네임스페이스는 :: 로 부른다
    const AHDShipInterior* ShipInterior = HDGameMode->GetShipInterior();
    const APawn* PlayerPawn = PlayerController->GetPawn();
    const FHDShipMenu& ShipMenu = PlayerController->GetShipMenu();
    const float LaunchRemaining = HDGameState->GetLaunchRemaining();

    const FString MenuText      = PlayerController->IsShipMenuActive() ? FString(HDShipMenu::GetScreenName(ShipMenu.GetScreen())) : FString(TEXT("none"));
    const FString LaunchText    = LaunchRemaining >= 0.f ? FString::Printf(TEXT("%.1f"), LaunchRemaining) : FString(TEXT("-1"));
    const FString PawnLocalText = (ShipInterior && PlayerPawn) ? ShipInterior->ToLocal(PlayerPawn->GetActorLocation()).ToString() : FString(TEXT("none"));

    UE_LOG(LogTemp, Display, TEXT("[HDShipHub] interior=%d crew=%d menu=%s root=%s launch=%s pawnLocal=%s"),
        ShipInterior ? 1 : 0,
        PlayerController->IsInShipInterior() ? 1 : 0,
        *MenuText,
        HDShipMenu::GetScreenName(ShipMenu.GetRootScreen()),
        *LaunchText,
        *PawnLocalText);

    const TArray<FHDShipTerminalInfo>& Terminals = ::HDShipHub::GetTerminals();
    for (int32 Index = 0; Index < Terminals.Num(); ++Index)
    {
        const FVector WorldLocation = ShipInterior ? ShipInterior->GetTerminalWorldLocation(Index) : FVector::ZeroVector;
        const FString WorldText     = ShipInterior ? WorldLocation.ToString() : FString(TEXT("none"));
        const float Distance        = (ShipInterior && PlayerPawn) ? FVector::Dist2D(PlayerPawn->GetActorLocation(), WorldLocation) : -1.f;

        UE_LOG(LogTemp, Display, TEXT("[HDShipHub] terminal %d %s world=%s dist=%.0f"),
            Index,
            *Terminals[Index].Id.ToString(),
            *WorldText,
            Distance);
    }
}

void UHDCheatManager::HDTerminal(const FString& Id)
{
    AHDPlayerController* PlayerController = Cast<AHDPlayerController>(GetOuterAPlayerController());
    if (PlayerController == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] No HDPlayerController"));
        return;
    }

    const FName TerminalId(*Id);
    if (TerminalId == FName(TEXT("System")))
    {
        PlayerController->OpenShipTerminal(EHDShipScreen::System, TerminalId);
        return;
    }

    const FHDShipTerminalInfo* Terminal = ::HDShipHub::FindTerminal(TerminalId);
    if (Terminal == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] terminal unknown %s"), *Id);
        return;
    }

    // 거리와 상관없이 연다(E 와 같은 OpenShipTerminal)
    PlayerController->OpenShipTerminal(Terminal->RootScreen, Terminal->Id);
}

void UHDCheatManager::HDLaunch()
{
    UWorld* World = GetWorld();
    AHDGameMode* HDGameMode = World ? World->GetAuthGameMode<AHDGameMode>() : nullptr;
    if (HDGameMode == nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("[HDShip] No HDGameMode in this world"));
        return;
    }

    HDGameMode->RequestLaunch(GetPlayerController());
}
