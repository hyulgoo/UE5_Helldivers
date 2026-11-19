// Fill out your copyright notice in the Description page of Project Settings.

#include "HDCharacterNonPlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Component/HDAbilitySystemComponent.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "AI/HDEnemyAIController.h"
#include "Collision/HDCollision.h"
#include "Define/HDSocketNames.h"

AHDCharacterNonPlayer::AHDCharacterNonPlayer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NPCAbilitySystemComponent = CreateDefaultSubobject<UHDAbilitySystemComponent>(TEXT("HDAbilitySystem"));

    // AI
    AIControllerClass = AHDEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Movement: 컨트롤러 포커스 방향으로 돈다. 부모의 45 는 적에게 너무 느리다
    UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
    CharacterMovementComponent->bOrientRotationToMovement = false;
    CharacterMovementComponent->bUseControllerDesiredRotation = true;
    CharacterMovementComponent->RotationRate = FRotator(0.f, 540.f, 0.f);

    // Weapon: 메시는 자식이 정한다
    EnemyWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EnemyWeaponMesh"));
    EnemyWeaponMesh->SetupAttachment(GetMesh(), HDSOCKETNAME_RIGRHTHAND);
    EnemyWeaponMesh->SetCollisionProfileName(HDCOLLISION_PROFILE_NOCOLLISION);
}

void AHDCharacterNonPlayer::BeginPlay()
{
    Super::BeginPlay();

    SetAbilitySystemComponent(this, NPCAbilitySystemComponent);
}

void AHDCharacterNonPlayer::SetDead()
{
    CONDITION_CHECK_WITHOUT_LOG(bIsDead == false);
    bIsDead = true;

    Super::SetDead();

	const float RandomDeadEventDelayTime = FMath::FRandRange(DeadEventDelayTime, DeadEventDelayTime * 1.5f);
    FTimerHandle DeadTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(DeadTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() { Destroy(); } ), RandomDeadEventDelayTime, false);

    AHDEnemyAIController* EnemyAIController = Cast<AHDEnemyAIController>(GetController());
    if (EnemyAIController)
    {
        EnemyAIController->OnControlledPawnDead();
    }

    OnEnemyDied.Broadcast(this);
}

void AHDCharacterNonPlayer::InitializeAttributeValues()
{
    // DataTable·GE 경로는 쓰지 않는다. BPGE_InitAttributeNPC 는 MaxHealth 만 정해 CurrentHealth 가 0 으로 남는다
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    NULL_CHECK(ASC);

    // 최대 체력을 먼저 정해야 현재 체력이 0 으로 잘리지 않는다(UHDHealthAttributeSet::PreAttributeChange)
    ASC->SetNumericAttributeBase(UHDHealthAttributeSet::GetMaxHealthAttribute(), Stats.MaxHealth);
    ASC->SetNumericAttributeBase(UHDHealthAttributeSet::GetCurrentHealthAttribute(), Stats.MaxHealth);

    GetCharacterMovement()->MaxWalkSpeed = Stats.WalkSpeed;
}
