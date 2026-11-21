// Fill out your copyright notice in the Description page of Project Settings.

#include "HDGA_ThrowGrenade.h"
#include "Animation/AnimMontage.h"
#include "UObject/ConstructorHelpers.h"
#include "AbilitySystem/AbilityTask/HDAT_PlayMontageAndWaitForEvent.h"
#include "Character/Player/HDCharacterPlayer.h"

UHDGA_ThrowGrenade::UHDGA_ThrowGrenade(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 좌클릭(DA_Input 의 Trigger.Attack)이 사격·스트라타젬 투척 GA 와 함께 이 GA 도 시도한다(UHDAbilitySystemComponent::AbilityInputTagTriggered)
    SetAssetTags(FGameplayTagContainer(Tag_Trigger_Attack));

    ActivationRequiredTags.AddTag(Tag_Character_State_GrenadeReady);

    ActivationBlockedTags.AddTag(Tag_Block_Grenade);
    ActivationBlockedTags.AddTag(Tag_Block_All);
    ActivationBlockedTags.AddTag(Tag_Character_Action_HoldStratagem);
    ActivationBlockedTags.AddTag(Tag_Character_Action_Dead);

    ActivationOwnedTags.AddTag(Tag_Block_Fire);
    ActivationOwnedTags.AddTag(Tag_Block_Reload);
    ActivationOwnedTags.AddTag(Tag_Block_Stratagem);
    ActivationOwnedTags.AddTag(Tag_Block_Grenade);

    static ConstructorHelpers::FObjectFinder<UAnimMontage> ThrowMontageRef(TEXT("/Game/Helldivers/Animation/AM_Throw.AM_Throw"));
    if (ThrowMontageRef.Succeeded())
    {
        ThrowMontage = ThrowMontageRef.Object;
    }
}

void UHDGA_ThrowGrenade::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = true;

    // NULL_CHECK 는 return 만 해서 어빌리티가 끝나지 않으므로 직접 끝낸다
    if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false || ThrowMontage == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }

    // AM_Throw 는 0.427초에 DetachStratagem 을 한 번 보낸다. 끝까지 재생되면 OnCompleted 로 끝낸다. 끊기면 OnInterrupted 로 끝낸다.
    // OnlyTriggerOnce 를 켜면 이벤트 직후 태스크가 지워져 OnCompleted 가 오지 않으므로 끈다.
    // 노티파이(Send Gameplay Event to Actor)는 Payload->EventTag 를 채우지 않아 OnlyMatchExact 경로는 이벤트를 버린다. 태그 컨테이너 경로를 쓴다
    UHDAT_PlayMontageAndWaitForEvent* PlayMontageAndWaitEventTask = UHDAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitEvent(
        this,
        ThrowMontage,
        FGameplayTagContainer(Tag_Character_Action_DetachStratagem),
        1.f,
        NAME_None,
        true,
        false,
        false
    );

    PlayMontageAndWaitEventTask->OnEventReceived.AddDynamic(this, &UHDGA_ThrowGrenade::OnDetach);
    PlayMontageAndWaitEventTask->OnCompleted.AddDynamic(this, &UHDGA_ThrowGrenade::OnMontageFinished);
    PlayMontageAndWaitEventTask->OnInterrupted.AddDynamic(this, &UHDGA_ThrowGrenade::OnMontageInterrupted);
    PlayMontageAndWaitEventTask->ReadyForActivation();
}

void UHDGA_ThrowGrenade::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    const bool bWasActive = IsActive();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (bWasActive == false)
    {
        return;
    }

    AHDCharacterPlayer* Player = ActorInfo ? Cast<AHDCharacterPlayer>(ActorInfo->AvatarActor.Get()) : nullptr;
    if (Player)
    {
        Player->OnGrenadeThrowFinished();
    }
}

void UHDGA_ThrowGrenade::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
}

void UHDGA_ThrowGrenade::OnDetach(FGameplayEventData Payload)
{
    AHDCharacterPlayer* Player = Cast<AHDCharacterPlayer>(GetAvatarActorFromActorInfo());
    NULL_CHECK(Player);

    Player->ThrowGrenade();
}

void UHDGA_ThrowGrenade::OnMontageFinished(FGameplayEventData Payload)
{
    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = false;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHDGA_ThrowGrenade::OnMontageInterrupted(FGameplayEventData Payload)
{
    UE_LOG(LogTemp, Display, TEXT("[HDGrenade] throw interrupted"));

    const bool bReplicateEndAbility = true;
    const bool bWasCancelled = true;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}
