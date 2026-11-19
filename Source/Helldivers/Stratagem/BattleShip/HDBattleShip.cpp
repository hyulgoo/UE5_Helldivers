// Fill out your copyright notice in the Description page of Project Settings.

#include "HDBattleShip.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/SphereComponent.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemOrbital.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemEagle.h"
#include "Stratagem/HDStratagem.h"
#include "Stratagem/HDStratagemPattern.h"
#include "Stratagem/Types/StratagemProjectileTypes.h"
#include "GameData/HDStratagemEffectData.h"
#include "Game/HDGameState.h"
#include "Define/HDDefine.h"

AHDBattleShip::AHDBattleShip(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    SetRootComponent(CollisionSphere);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BattleShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BattleShipMesh"));
    BattleShipMesh->SetupAttachment(RootComponent);
    BattleShipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    StratagemAbilities.Add(UHDGA_StratagemOrbital::StaticClass());
    StratagemAbilities.Add(UHDGA_StratagemEagle::StaticClass());
}

void AHDBattleShip::ActivateStratagem(const AHDStratagem* Beacon, const FTransform& TargetTransform)
{
    NULL_CHECK(Beacon);
    NULL_CHECK(AbilitySystemComponent);

    const FHDStratagemEffectData* StratagemEffectData = FindStratagemEffectData(Beacon->GetStratagemName());
    NULL_CHECK(StratagemEffectData);

    const FGameplayEventData Payload = MakeActivationPayload(HDStratagemPattern::GetActivationEventTag(*StratagemEffectData), Beacon, TargetTransform);
    const int32 ActivatedCount = AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
    // at = 표적 XY(PIE 판정용, D12)
    UE_LOG(LogTemp, Display, TEXT("[BattleShip] %s -> %s activated=%d at=(%.0f, %.0f)"), *Beacon->GetStratagemName().ToString(), *Payload.EventTag.ToString(), ActivatedCount,
        TargetTransform.GetLocation().X, TargetTransform.GetLocation().Y);
}

FGameplayEventData AHDBattleShip::MakeActivationPayload(const FGameplayTag& EventTag, const AHDStratagem* Beacon, const FTransform& TargetTransform)
{
    // 이름·발동 지연은 던진 스트라타젬에서, 표적 변환은 TargetData 에서 GA 가 읽는다
    FGameplayAbilityTargetData_LocationInfo* LocationData = new FGameplayAbilityTargetData_LocationInfo();
    LocationData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
    LocationData->TargetLocation.LiteralTransform = TargetTransform;

    FGameplayEventData Payload;
    Payload.EventTag = EventTag;
    Payload.Instigator = Beacon ? Beacon->GetInstigator() : nullptr;
    Payload.OptionalObject = Beacon;
    Payload.TargetData.Add(LocationData);
    return Payload;
}

UAbilitySystemComponent* AHDBattleShip::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AHDBattleShip::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // 전함 ASC 는 자기 자신이 주인·아바타다
    AbilitySystemComponent->InitAbilityActorInfo(this, this);
    for (const TSubclassOf<UGameplayAbility>& AbilityClass : StratagemAbilities)
    {
        if (AbilityClass == nullptr || AbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass))
        {
            continue;
        }

        AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass));
    }
}

void AHDBattleShip::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    VALID_CHECK(World);

    AHDGameState* GameState = Cast<AHDGameState>(World->GetGameState());
    VALID_CHECK(GameState);

    GameState->SetBattleShip(this);
}

const FHDStratagemEffectData* AHDBattleShip::FindStratagemEffectData(const FName StratagemName) const
{
    NULL_CHECK_WITH_RETURNTYPE(StratagemEffectDataTable, nullptr);

    const FHDStratagemEffectData* StratagemData = StratagemEffectDataTable->FindRow<FHDStratagemEffectData>(StratagemName, TEXT("Lookup"));
    NULL_CHECK_WITH_RETURNTYPE(StratagemData, nullptr);

    return StratagemData;
}

TSubclassOf<AHDProjectileBase> AHDBattleShip::GetProjectileClass(const EHDStratagemProjectile ProjectileType) const
{
    return ProjectileType == EHDStratagemProjectile::Bullet ? ProjectileBulletClass : ProjectileBombClass;
}
