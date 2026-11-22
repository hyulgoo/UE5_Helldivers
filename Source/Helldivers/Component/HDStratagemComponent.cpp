// Fill out your copyright notice in the Description page of Project Settings.

#include "HDStratagemComponent.h"
#include "Define/HDDefine.h"
#include "Define/HDSocketNames.h"
#include "GameData/HDStratagemData.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Stratagem/HDStratagem.h"
#include "Stratagem/HDStratagemCatalog.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "Stratagem/HDStratagemSettings.h"
#include "AbilitySystem/GameplayEffect/HDGE_StratagemCooldown.h"
#include "Player/HDGASPlayerState.h"
#include "Progression/HDProfileSubsystem.h"
#include "Progression/HDUpgradeCatalog.h"
#include "Progression/HDUpgradeRules.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

UHDStratagemComponent::UHDStratagemComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FObjectFinder<UDataTable> StratagemDataListRef(TEXT("/Script/Engine.DataTable'/Game/Helldivers/GameData/DT_StratagemData.DT_StratagemData'"));
    if (StratagemDataListRef.Succeeded())
    {
        AvaliableStratagemDataTable = StratagemDataListRef.Object;
    }
}

UDataTable* UHDStratagemComponent::GetAvaliableStratagemDataTable() const
{
    return AvaliableStratagemDataTable;
}

const TArray<FName>& UHDStratagemComponent::GetCommandMatchStratagemNameList() const
{
    return CommandMatchStratagemNameList;
}

const int32 UHDStratagemComponent::GetCurrentInputNum() const
{
    return CurrentInputCommandList.Num();
}

const TArray<EHDCommandInput>& UHDStratagemComponent::GetCurrentInputCommands() const
{
    return CurrentInputCommandList;
}

void UHDStratagemComponent::AddStratagemCommand(const EHDCommandInput NewInput)
{
    CurrentInputCommandList.Add(NewInput);
    CommandMatchStratagemNameList.Empty();

    // 로드아웃 칸만 본다. 대기 중이거나 횟수를 다 쓴 칸은 후보에서 빠진다
    const FHDStratagemLoadout* Loadout = GetLoadout();
    NULL_CHECK(Loadout);

    TArray<bool> bAvailable;
    bAvailable.Reserve(Loadout->Num());
    for (int32 SlotIndex = 0; SlotIndex < Loadout->Num(); ++SlotIndex)
    {
        bAvailable.Add(IsSlotAvailable(SlotIndex));
    }

    const FHDStratagemCommandMatch Match = Loadout->MatchCommands(CurrentInputCommandList, bAvailable);
    for (const int32 SlotIndex : Match.MatchedSlots)
    {
        CommandMatchStratagemNameList.Add(Loadout->GetSlot(SlotIndex)->StratagemName);
    }

    // 선택은 ClearCommand 까지 남는다. 다 맞힌 뒤 더 누르면 후보 목록만 빈다
    const FHDStratagemSlot* SelectedSlot = Loadout->GetSlot(Match.SelectedSlot);
    if (SelectedSlot)
    {
        SelectedSlotIndex = Match.SelectedSlot;
        SelectedStratagemName = SelectedSlot->StratagemName;
        SelecteddStratagemActiveDelay = SelectedSlot->ActivationDelay;
    }
}

UInputAction* UHDStratagemComponent::GetCommandInputAction() const
{
    return CommandInputAction;
}

const bool UHDStratagemComponent::IsSelectedStratagemExist() const
{
    return (SelectedStratagemName.IsNone() == false);
}

void UHDStratagemComponent::HoldStratagem(USkeletalMeshComponent* MeshComponent, const FVector& ThrowDirection)
{
    NULL_CHECK(StratagemClass);
    CONDITION_CHECK_WITHOUT_LOG(IsSelectedStratagemExist());

    UWorld* World = GetWorld();
    VALID_CHECK(World);

    const USkeletalMeshSocket* RightHandSocket = MeshComponent->GetSocketByName(HDSOCKETNAME_RIGRHTHAND);
    NULL_CHECK(RightHandSocket);

	const FTransform SocketTransform = RightHandSocket->GetSocketTransform(MeshComponent);
	FRotator TargetRotation = (ThrowDirection - SocketTransform.GetLocation()).Rotation();
	TargetRotation.Roll = 0;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());

	// 든 채 다시 고르면 앞의 것을 버린다. 그대로 두면 포인터만 덮여 앞 액터가 손에 붙은 채 남는다
	if (IsValid(Stratagem))
	{
		UE_LOG(LogTemp, Display, TEXT("[HDStratagem] replace held"));
		Stratagem->Destroy();
		Stratagem = nullptr;
	}

	Stratagem = World->SpawnActor<AHDStratagem>(StratagemClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
	NULL_CHECK(Stratagem);
    
    RightHandSocket->AttachActor(Stratagem, MeshComponent);
    Stratagem->SetStratagemInfo(SelectedStratagemName, SelecteddStratagemActiveDelay);
    HeldSlotIndex = SelectedSlotIndex;
}

void UHDStratagemComponent::ThrowFinished()
{
    VALID_CHECK(Stratagem);

    const FRotator& BaseAim = GetOwner<APawn>()->GetBaseAimRotation();
    const FVector ThrowDirection = BaseAim.Vector().GetSafeNormal();
    Stratagem->AddImpulseToStratagem(ThrowDirection);
    Stratagem = nullptr;
    // 재사용 대기는 손에서 떠날 때 시작한다
    CommitHeldSlot();
}

void UHDStratagemComponent::CancelStratagem()
{
    // 던지지 않고 버리면(죽음 등) 횟수·대기를 쓰지 않는다
    HeldSlotIndex = INDEX_NONE;
    NULL_CHECK_WITHOUT_LOG(Stratagem);

    Stratagem->Destroy();
    Stratagem = nullptr;
}

void UHDStratagemComponent::ClearCommand()
{
    // HeldSlotIndex 는 지우지 않는다(TryHoldStratagem 이 HoldStratagem 바로 뒤에 부른다)
    SelectedStratagemName = FName();
    SelectedSlotIndex = INDEX_NONE;
    CommandMatchStratagemNameList.Empty();
    CurrentInputCommandList.Empty();
}

void UHDStratagemComponent::BindLoadout(AHDGASPlayerState* InPlayerState)
{
    NULL_CHECK(InPlayerState);
    LoadoutOwner = InPlayerState;

    // 다시 강하하면 이미 채워져 있으므로 남은 횟수를 그대로 둔다
    if (InPlayerState->GetStratagemLoadout().IsInitialized())
    {
        return;
    }

    ReinitializeLoadout(GetDefault<UHDStratagemSettings>()->Loadout);
}

void UHDStratagemComponent::ReinitializeLoadout(const TArray<FName>& LoadoutNames)
{
    FHDStratagemLoadout* LoadoutPtr = GetMutableLoadout();
    NULL_CHECK(LoadoutPtr);

    FHDStratagemLoadout& Loadout = *LoadoutPtr;
    Loadout.Initialize(LoadoutNames, BuildCatalog());

    // 칸 태그(Cooldown.Stratagem.SlotN)는 칸 번호에 붙어 있어 이름이 바뀐 칸에 앞 대기가 남지 않게 모두 지운다
    UHDGE_StratagemCooldown::ClearAll(GetOwnerAbilitySystemComponent());
    SelectedSlotIndex = INDEX_NONE;
    HeldSlotIndex = INDEX_NONE;

    FString SlotNames;
    for (int32 SlotIndex = 0; SlotIndex < Loadout.Num(); ++SlotIndex)
    {
        if (SlotNames.IsEmpty() == false)
        {
            SlotNames += TEXT(", ");
        }
        SlotNames += Loadout.GetSlot(SlotIndex)->StratagemName.ToString();
    }

    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] loadout %d slots: %s"), Loadout.Num(), *SlotNames);
}

const FHDStratagemLoadout* UHDStratagemComponent::GetLoadout() const
{
    return GetMutableLoadout();
}

bool UHDStratagemComponent::IsSlotAvailable(const int32 SlotIndex) const
{
    const FHDStratagemLoadout* Loadout = GetLoadout();
    return Loadout
        && Loadout->HasUsesLeft(SlotIndex)
        && UHDGE_StratagemCooldown::IsSlotOnCooldown(GetOwnerAbilitySystemComponent(), SlotIndex) == false;
}

float UHDStratagemComponent::GetSlotCooldownRemaining(const int32 SlotIndex) const
{
    return UHDGE_StratagemCooldown::GetSlotRemaining(GetOwnerAbilitySystemComponent(), SlotIndex);
}

TArray<FHDStratagemEntry> UHDStratagemComponent::BuildCatalog() const
{
    // 카탈로그 문제는 GetProjectCatalog 가 처음 읽을 때 Warning 으로 남긴다
    const FHDStratagemCatalog& Catalog = FHDStratagemCatalog::GetProjectCatalog();

    // 함선 업그레이드는 칸을 만들 때(첫 빙의 BindLoadout, 치트 HDStratagemLoadout)만 넣는다. 그래서 미션 중에는 바뀌지 않는다
    const UHDProfileSubsystem* ProfileSubsystem = UHDProfileSubsystem::Get(this);
    const FHDUpgradeModifiers Upgrades = ProfileSubsystem
        ? HDUpgrade::ComputeModifiers(ProfileSubsystem->GetProfile(), FHDUpgradeCatalog::GetProjectCatalog())
        : FHDUpgradeModifiers();

    TArray<FHDStratagemEntry> Entries;
    for (const FName& StratagemName : Catalog.GetNames())
    {
        const FHDStratagemDefinition* Definition = Catalog.Find(StratagemName);
        if (Definition == nullptr)
        {
            continue;
        }

        FHDStratagemEntry& Entry = Entries.AddDefaulted_GetRef();
        Entry.Name = StratagemName;
        Entry.CommandSequence = Definition->CommandSequence;
        Entry.ActivationDelay = Definition->ActivationDelay;
        Entry.Cooldown = Definition->Cooldown;
        Entry.MaxUses = Definition->MaxUses;
        Entry.RearmTime = Definition->RearmTime;
        HDUpgrade::ApplyToEntry(Entry, *Definition, Upgrades);
    }

    return Entries;
}

FHDStratagemLoadout* UHDStratagemComponent::GetMutableLoadout() const
{
    AHDGASPlayerState* PlayerState = LoadoutOwner.Get();
    return PlayerState ? &PlayerState->GetStratagemLoadout() : nullptr;
}

UAbilitySystemComponent* UHDStratagemComponent::GetOwnerAbilitySystemComponent() const
{
    // 캐릭터는 플레이어스테이트 ASC 를 돌려준다
    return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

void UHDStratagemComponent::CommitHeldSlot()
{
    const int32 SlotIndex = HeldSlotIndex;
    HeldSlotIndex = INDEX_NONE;

    FHDStratagemLoadout* Loadout = GetMutableLoadout();
    const FHDStratagemSlot* Slot = Loadout ? Loadout->GetSlot(SlotIndex) : nullptr;
    NULL_CHECK_WITHOUT_LOG(Slot);

    // 마지막 한 번이면 ConsumeUse 가 횟수를 되돌리고 재무장 대기(RearmTime)를 돌려준다. 거짓이면 대기를 걸지 않는다
    float CooldownSeconds = 0.f;
    if (Loadout->ConsumeUse(SlotIndex, &CooldownSeconds))
    {
        UHDGE_StratagemCooldown::ApplyToSlot(GetOwnerAbilitySystemComponent(), SlotIndex, CooldownSeconds);
    }

    UE_LOG(LogTemp, Display, TEXT("[HDStratagem] thrown %s slot=%d uses=%s cooldown=%.1f%s"), *Slot->StratagemName.ToString(), SlotIndex + 1, *Slot->GetUsesText(), CooldownSeconds, Slot->bRearming ? TEXT(" rearm") : TEXT(""));
}
