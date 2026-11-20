// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemTestPawn.h"
#include "Attribute/HDWeaponAttributeSet.h"
#include "Weapon/HDWeaponSlots.h"

namespace HDWeaponSlotsTest
{
    // 엔진 GameplayEffectTests.cpp 의 AddModifier 와 같은 방식(체력 테스트와 같다)
    void AddOverride(UGameplayEffect* Effect, const FGameplayAttribute& Attribute, const float Magnitude)
    {
        const int32 Index = Effect->Modifiers.Num();
        Effect->Modifiers.SetNum(Index + 1);

        FGameplayModifierInfo& Info = Effect->Modifiers[Index];
        Info.ModifierMagnitude  = FScalableFloat(Magnitude);
        Info.ModifierOp         = EGameplayModOp::Override;
        Info.Attribute          = Attribute;
    }

    UAbilitySystemComponent* SpawnWeaponPawn(UWorld* World)
    {
        AAbilitySystemTestPawn* Pawn = World->SpawnActor<AAbilitySystemTestPawn>();
        if (Pawn == nullptr)
        {
            return nullptr;
        }

        UAbilitySystemComponent* ASC = Pawn->GetAbilitySystemComponent();
        if (ASC == nullptr)
        {
            return nullptr;
        }

        ASC->InitAbilityActorInfo(Pawn, Pawn);
        ASC->AddSet<UHDWeaponAttributeSet>();
        return ASC;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDWeaponSlotsTest, "Helldivers.Combat.WeaponSlots", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDWeaponSlotsTest::RunTest(const FString& Parameters)
{
    using namespace HDWeaponSlotsTest;

    // 칸 규칙
    {
        FHDWeaponSlots Slots;

        const FHDWeaponPickupResult PrimaryResult = Slots.Pickup(false);
        TestTrue(TEXT("Pickup primary: slot Primary"), PrimaryResult.Slot == EHDWeaponSlot::Primary);
        TestFalse(TEXT("Pickup primary: not replaced"), PrimaryResult.bReplaced);
        TestTrue(TEXT("Pickup primary: active Primary"), Slots.GetActiveSlot() == EHDWeaponSlot::Primary);

        const FHDWeaponPickupResult SupportResult = Slots.Pickup(true);
        TestTrue(TEXT("Pickup support: slot Support"), SupportResult.Slot == EHDWeaponSlot::Support);
        TestTrue(TEXT("Pickup support: active Support"), Slots.GetActiveSlot() == EHDWeaponSlot::Support);
        TestFalse(TEXT("Pickup support: not replaced"), SupportResult.bReplaced);

        const FHDWeaponPickupResult SupportAgainResult = Slots.Pickup(true);
        TestTrue(TEXT("Pickup support again: replaced"), SupportAgainResult.bReplaced);

        const bool bSwappedToPrimary = Slots.Swap();
        TestTrue(TEXT("Swap: to Primary"), bSwappedToPrimary && Slots.GetActiveSlot() == EHDWeaponSlot::Primary);

        const bool bSwappedToSupport = Slots.Swap();
        TestTrue(TEXT("Swap: back to Support"), bSwappedToSupport && Slots.GetActiveSlot() == EHDWeaponSlot::Support);
    }

    {
        FHDWeaponSlots Slots;
        Slots.Pickup(false);

        const bool bSwapped = Slots.Swap();
        TestTrue(TEXT("Swap with one weapon: refused"), bSwapped == false && Slots.GetActiveSlot() == EHDWeaponSlot::Primary);
    }

    // 코드 이벤트 시점(노티파이 없는 무기 몽타주)
    TestEqual(TEXT("ReloadDelay: weapon section"), HDWeaponMontageEvent::GetReloadEventDelay(2.5f, 3.f), 2.5f, KINDA_SMALL_NUMBER);
    TestEqual(TEXT("ReloadDelay: clamped by body montage"), HDWeaponMontageEvent::GetReloadEventDelay(2.5f, 1.5f), 1.4f, KINDA_SMALL_NUMBER);
    TestEqual(TEXT("ReloadDelay: never negative"), HDWeaponMontageEvent::GetReloadEventDelay(2.5f, 0.05f), 0.f, KINDA_SMALL_NUMBER);
    TestEqual(TEXT("ReloadDelay: no section is next tick"), HDWeaponMontageEvent::GetReloadEventDelay(0.f, 3.f), 0.f, KINDA_SMALL_NUMBER);
    TestEqual(TEXT("ReloadDelay: no body montage"), HDWeaponMontageEvent::GetReloadEventDelay(2.5f, 0.f), 2.5f, KINDA_SMALL_NUMBER);

    TestTrue(TEXT("FireReady: first shot"), HDWeaponMontageEvent::IsFireReady(-1.0, 0.0, 1.f));
    TestFalse(TEXT("FireReady: too soon"), HDWeaponMontageEvent::IsFireReady(10.0, 10.5, 1.f));
    TestTrue(TEXT("FireReady: after delay"), HDWeaponMontageEvent::IsFireReady(10.0, 11.0, 1.f));

    // 탄약 읽기: 체력 테스트와 같은 월드 준비
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    WorldContext.SetCurrentWorld(World);

    FURL URL;
    World->InitializeActorsForPlay(URL);
    World->BeginPlay();

    {
        UAbilitySystemComponent* ASC = SpawnWeaponPawn(World);
        if (TestNotNull(TEXT("ReadAmmo: ASC"), ASC))
        {
            // 최대값 먼저: PreAttributeChange 가 현재값을 최대값으로 자른다(HDWeaponAttributeSet.cpp)
            UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
            Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
            AddOverride(Effect, UHDWeaponAttributeSet::GetMaxAmmoAttribute(), 45.f);
            AddOverride(Effect, UHDWeaponAttributeSet::GetAmmoAttribute(), 12.f);
            AddOverride(Effect, UHDWeaponAttributeSet::GetMaxCapacityAttribute(), 7.f);
            AddOverride(Effect, UHDWeaponAttributeSet::GetCapacityAttribute(), 3.f);
            ASC->ApplyGameplayEffectToSelf(Effect, 1.f, ASC->MakeEffectContext());

            const FHDWeaponAmmoData AmmoData = HDWeaponAmmo::ReadFromAttributes(ASC);
            TestEqual(TEXT("ReadAmmo: Ammo 12"), AmmoData.Ammo, 12);
            TestEqual(TEXT("ReadAmmo: MaxAmmo 45"), AmmoData.MaxAmmo, 45);
            TestEqual(TEXT("ReadAmmo: Capacity 3"), AmmoData.Capacity, 3);
            TestEqual(TEXT("ReadAmmo: MaxCapacity 7"), AmmoData.MaxCapacity, 7);
        }
    }

    {
        const FHDWeaponAmmoData AmmoData = HDWeaponAmmo::ReadFromAttributes(nullptr);
        TestTrue(TEXT("ReadAmmo: null ASC is zero"), AmmoData.Ammo == 0 && AmmoData.MaxAmmo == 0 && AmmoData.Capacity == 0 && AmmoData.MaxCapacity == 0);
    }

    World->EndPlay(EEndPlayReason::Quit);
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
