// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemTestPawn.h"
#include "Attribute/HDHealthAttributeSet.h"
#include "Define/HDGameplayTag.h"

namespace HDHealthAttributeSetTest
{
    // 엔진 GameplayEffectTests.cpp 의 AddModifier 와 같은 방식
    void AddModifier(UGameplayEffect* Effect, const FGameplayAttribute& Attribute, const EGameplayModOp::Type ModifierOp, const float Magnitude)
    {
        const int32 Index = Effect->Modifiers.Num();
        Effect->Modifiers.SetNum(Index + 1);

        FGameplayModifierInfo& Info = Effect->Modifiers[Index];
        Info.ModifierMagnitude = FScalableFloat(Magnitude);
        Info.ModifierOp = ModifierOp;
        Info.Attribute = Attribute;
    }

    UGameplayEffect* MakeInstantEffect()
    {
        UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
        Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
        return Effect;
    }

    // BPGE_InitAttributePlayer 와 같은 꼴: 두 모디파이어 모두 Override, 실제 에셋 순서는 CurrentHealth -> MaxHealth
    UGameplayEffect* MakeInitHealthEffect(const bool bCurrentHealthFirst, const float Health)
    {
        UGameplayEffect* Effect = MakeInstantEffect();
        if (bCurrentHealthFirst)
        {
            AddModifier(Effect, UHDHealthAttributeSet::GetCurrentHealthAttribute(), EGameplayModOp::Override, Health);
            AddModifier(Effect, UHDHealthAttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Override, Health);
        }
        else
        {
            AddModifier(Effect, UHDHealthAttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Override, Health);
            AddModifier(Effect, UHDHealthAttributeSet::GetCurrentHealthAttribute(), EGameplayModOp::Override, Health);
        }

        return Effect;
    }

    UGameplayEffect* MakeSingleModEffect(const FGameplayAttribute& Attribute, const EGameplayModOp::Type ModifierOp, const float Magnitude)
    {
        UGameplayEffect* Effect = MakeInstantEffect();
        AddModifier(Effect, Attribute, ModifierOp, Magnitude);
        return Effect;
    }

    void ApplyToSelf(UAbilitySystemComponent* ASC, const UGameplayEffect* Effect)
    {
        ASC->ApplyGameplayEffectToSelf(Effect, 1.f, ASC->MakeEffectContext());
    }

    UAbilitySystemComponent* SpawnHealthPawn(UWorld* World)
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
        ASC->AddSet<UHDHealthAttributeSet>();
        return ASC;
    }

    float GetCurrentHealth(const UAbilitySystemComponent* ASC)
    {
        return ASC->GetNumericAttribute(UHDHealthAttributeSet::GetCurrentHealthAttribute());
    }

    float GetMaxHealth(const UAbilitySystemComponent* ASC)
    {
        return ASC->GetNumericAttribute(UHDHealthAttributeSet::GetMaxHealthAttribute());
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDHealthAttributeSetTest, "Helldivers.Attributes.Health", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDHealthAttributeSetTest::RunTest(const FString& Parameters)
{
    using namespace HDHealthAttributeSetTest;

    // 엔진 GameplayEffectTests.cpp 와 같은 월드 준비
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    WorldContext.SetCurrentWorld(World);

    FURL URL;
    World->InitializeActorsForPlay(URL);
    World->BeginPlay();

    // 초기화(현재 -> 최대 순서): 실제 BPGE_InitAttributePlayer 순서
    {
        UAbilitySystemComponent* ASC = SpawnHealthPawn(World);
        if (TestNotNull(TEXT("Init (Current -> Max): ASC"), ASC))
        {
            ApplyToSelf(ASC, MakeInitHealthEffect(true, 100.f));

            TestEqual(TEXT("Init (Current -> Max): CurrentHealth"), GetCurrentHealth(ASC), 100.f);
            TestEqual(TEXT("Init (Current -> Max): MaxHealth"), GetMaxHealth(ASC), 100.f);
        }
    }

    // 초기화(최대 -> 현재 순서) 다음 최대 감소·증가를 같은 폰에서 이어서 본다
    {
        UAbilitySystemComponent* ASC = SpawnHealthPawn(World);
        if (TestNotNull(TEXT("Init (Max -> Current): ASC"), ASC))
        {
            ApplyToSelf(ASC, MakeInitHealthEffect(false, 100.f));

            TestEqual(TEXT("Init (Max -> Current): CurrentHealth"), GetCurrentHealth(ASC), 100.f);

            ApplyToSelf(ASC, MakeSingleModEffect(UHDHealthAttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Override, 60.f));

            TestEqual(TEXT("MaxHealth decreased to 60: CurrentHealth"), GetCurrentHealth(ASC), 60.f);

            const float CurrentHealthBeforeIncrease = GetCurrentHealth(ASC);
            ApplyToSelf(ASC, MakeSingleModEffect(UHDHealthAttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Override, 120.f));

            TestEqual(TEXT("MaxHealth increased to 120: CurrentHealth does not rise"), GetCurrentHealth(ASC), CurrentHealthBeforeIncrease);
        }
    }

    // 사망 태그: 초기화 뒤 CurrentHealth AddBase -999
    {
        UAbilitySystemComponent* ASC = SpawnHealthPawn(World);
        if (TestNotNull(TEXT("Dead: ASC"), ASC))
        {
            ApplyToSelf(ASC, MakeInitHealthEffect(true, 100.f));
            ApplyToSelf(ASC, MakeSingleModEffect(UHDHealthAttributeSet::GetCurrentHealthAttribute(), EGameplayModOp::AddBase, -999.f));

            TestEqual(TEXT("Dead: CurrentHealth"), GetCurrentHealth(ASC), 0.f);
            TestTrue(TEXT("Dead: ASC has Trigger.Dead"), ASC->HasMatchingGameplayTag(Tag_Trigger_Dead));
            TestTrue(TEXT("Dead: ASC has Character.State.Dead"), ASC->HasMatchingGameplayTag(Tag_Character_Action_Dead));
        }
    }

    World->EndPlay(EEndPlayReason::Quit);
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
