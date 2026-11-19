// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemTestPawn.h"
#include "Stratagem/HDStratagemLoadout.h"
#include "AbilitySystem/GameplayEffect/HDGE_StratagemCooldown.h"
#include "Define/HDGameplayTag.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDStratagemLoadoutTest, "Helldivers.Stratagem.Loadout", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace HDStratagemLoadoutTest
{
    const EHDCommandInput   Up      = EHDCommandInput::Up;
    const EHDCommandInput   Down    = EHDCommandInput::Down;
    const EHDCommandInput   Left    = EHDCommandInput::Left;
    const EHDCommandInput   Right   = EHDCommandInput::Right;

    const FName             Precision(TEXT("OrbitalPrecisionStrike"));
    const FName             Eagle(TEXT("EagleAirstrike"));
    const FName             Gatling(TEXT("OrbitalGatlingBarrage"));

    FHDStratagemEntry MakeEntry(const FName Name, const TArray<EHDCommandInput>& CommandSequence, const float ActivationDelay, const float Cooldown, const int32 MaxUses)
    {
        FHDStratagemEntry Entry;
        Entry.Name = Name;
        Entry.CommandSequence = CommandSequence;
        Entry.ActivationDelay = ActivationDelay;
        Entry.Cooldown = Cooldown;
        Entry.MaxUses = MaxUses;
        return Entry;
    }

    // 프로젝트 JSON 을 읽지 않는다(사용자가 값을 바꿔도 깨지지 않게). 대기 0 경우를 보려고 개틀링만 대기 0
    TArray<FHDStratagemEntry> MakeCatalog()
    {
        return {
            MakeEntry(Eagle,        { Up, Right, Down, Right },     3.f, 10.f, 2),
            MakeEntry(Precision,    { Up, Right, Right },           2.f, 30.f, 0),
            MakeEntry(Gatling,      { Right, Down, Left, Up, Up },  1.f, 0.f,  0),
        };
    }
}

bool FHDStratagemLoadoutTest::RunTest(const FString& Parameters)
{
    using namespace HDStratagemLoadoutTest;

    // 1. 칸 만들기
    {
        TArray<FHDStratagemEntry> Catalog = MakeCatalog();

        FHDStratagemLoadout Loadout;
        TestFalse(TEXT("Init: not initialized before Initialize"), Loadout.IsInitialized());

        Loadout.Initialize({ Precision, FName(TEXT("Unknown")), Eagle, NAME_None, Precision, Gatling }, Catalog);
        TestTrue(TEXT("Init: initialized"), Loadout.IsInitialized());
        TestEqual(TEXT("Init: unknown, None, duplicate skipped -> 3 slots"), Loadout.Num(), 3);
        TestTrue(TEXT("Init: slot 0 is Precision"), Loadout.GetSlot(0) && Loadout.GetSlot(0)->StratagemName == Precision);
        TestTrue(TEXT("Init: slot 1 is Eagle"), Loadout.GetSlot(1) && Loadout.GetSlot(1)->StratagemName == Eagle);
        TestTrue(TEXT("Init: slot 2 is Gatling"), Loadout.GetSlot(2) && Loadout.GetSlot(2)->StratagemName == Gatling);

        const FHDStratagemSlot* EagleSlot = Loadout.GetSlot(1);
        if (TestNotNull(TEXT("Init: Eagle slot"), EagleSlot))
        {
            TestEqual(TEXT("Init: Eagle cooldown from catalog"), EagleSlot->Cooldown, 10.f);
            TestEqual(TEXT("Init: Eagle uses from catalog"), EagleSlot->MaxUses, 2);
            TestEqual(TEXT("Init: Eagle delay from catalog"), EagleSlot->ActivationDelay, 3.f);
        }

        const FHDStratagemSlot* GatlingSlot = Loadout.GetSlot(2);
        if (TestNotNull(TEXT("Init: Gatling slot"), GatlingSlot))
        {
            TestEqual(TEXT("Init: zero cooldown -> 0"), GatlingSlot->Cooldown, 0.f);
            TestEqual(TEXT("Init: zero uses -> unlimited"), GatlingSlot->MaxUses, 0);
        }

        const FName Neg(TEXT("Neg"));
        Catalog.Add(MakeEntry(Neg, { Down }, 0.f, -5.f, -1));
        Loadout.Initialize({ Neg }, Catalog);
        const FHDStratagemSlot* NegSlot = Loadout.GetSlot(0);
        TestTrue(TEXT("Init: negative cooldown clamped to 0"), NegSlot && NegSlot->Cooldown == 0.f);
        TestTrue(TEXT("Init: negative uses clamped to unlimited"), NegSlot && NegSlot->MaxUses == 0);

        const FName A(TEXT("A"));
        const FName B(TEXT("B"));
        Catalog.Add(MakeEntry(A, { Down }, 0.f, 0.f, 0));
        Catalog.Add(MakeEntry(B, { Left }, 0.f, 0.f, 0));
        Loadout.Initialize({ Precision, Eagle, Gatling, A, B }, Catalog);
        TestEqual(TEXT("Init: more than 4 -> 4 slots"), Loadout.Num(), 4);
        TestEqual(TEXT("Init: 5th name dropped"), Loadout.FindSlot(B), (int32)INDEX_NONE);
    }

    // 2. 커맨드 대조
    {
        FHDStratagemLoadout Loadout;
        Loadout.Initialize({ Precision, Eagle, Gatling }, MakeCatalog());
        const TArray<bool> AllAvailable = { true, true, true };

        const FHDStratagemCommandMatch MatchUp = Loadout.MatchCommands({ Up }, AllAvailable);
        TestTrue(TEXT("Match Up: Precision and Eagle"), MatchUp.MatchedSlots == TArray<int32>({ 0, 1 }));
        TestEqual(TEXT("Match Up: nothing selected"), MatchUp.SelectedSlot, (int32)INDEX_NONE);

        const FHDStratagemCommandMatch MatchPrecision = Loadout.MatchCommands({ Up, Right, Right }, AllAvailable);
        TestEqual(TEXT("Match Up Right Right: selects Precision"), MatchPrecision.SelectedSlot, 0);
        TestTrue(TEXT("Match Up Right Right: only Precision left"), MatchPrecision.MatchedSlots == TArray<int32>({ 0 }));

        const FHDStratagemCommandMatch MatchEagle = Loadout.MatchCommands({ Up, Right, Down, Right }, AllAvailable);
        TestEqual(TEXT("Match Eagle full: selects Eagle"), MatchEagle.SelectedSlot, 1);

        const FHDStratagemCommandMatch MatchLeft = Loadout.MatchCommands({ Left }, AllAvailable);
        TestEqual(TEXT("Match Left: no candidates"), MatchLeft.MatchedSlots.Num(), 0);

        const FHDStratagemCommandMatch MatchEmpty = Loadout.MatchCommands({}, AllAvailable);
        TestEqual(TEXT("Match empty input: no candidates"), MatchEmpty.MatchedSlots.Num(), 0);

        // 이글 칸(1)이 대기 중·다 씀
        const FHDStratagemCommandMatch MatchEagleUnavailable = Loadout.MatchCommands({ Up, Right, Down, Right }, { true, false, true });
        TestEqual(TEXT("Match Eagle unavailable: nothing selected"), MatchEagleUnavailable.SelectedSlot, (int32)INDEX_NONE);
        TestEqual(TEXT("Match Eagle unavailable: no candidates"), MatchEagleUnavailable.MatchedSlots.Num(), 0);

        // → 는 개틀링(칸 2)만 맞는데 가용 배열이 칸 0 까지뿐
        const FHDStratagemCommandMatch MatchShort = Loadout.MatchCommands({ Right }, { true });
        TestEqual(TEXT("Match with short availability: missing slots are unavailable"), MatchShort.MatchedSlots.Num(), 0);
    }

    // 3. 사용 횟수
    {
        FHDStratagemLoadout Loadout;
        Loadout.Initialize({ Precision, Eagle }, MakeCatalog());
        const int32 EagleIndex = 1;

        TestTrue(TEXT("Uses: Eagle 1st"), Loadout.ConsumeUse(EagleIndex));
        TestTrue(TEXT("Uses: Eagle 2nd"), Loadout.ConsumeUse(EagleIndex));
        TestFalse(TEXT("Uses: Eagle 3rd refused"), Loadout.ConsumeUse(EagleIndex));
        TestFalse(TEXT("Uses: Eagle has no uses left"), Loadout.HasUsesLeft(EagleIndex));
        const FHDStratagemSlot* EagleSlot = Loadout.GetSlot(EagleIndex);
        if (TestNotNull(TEXT("Uses: Eagle slot"), EagleSlot))
        {
            TestEqual(TEXT("Uses: Eagle stays at 0"), EagleSlot->UsesLeft, 0);
        }

        for (int32 Count = 0; Count < 5; ++Count)
        {
            Loadout.ConsumeUse(0);
        }
        TestTrue(TEXT("Uses: unlimited Precision still has uses"), Loadout.HasUsesLeft(0));
        TestFalse(TEXT("Uses: slot out of range"), Loadout.ConsumeUse(7));

        Loadout.ResetUses();
        const FHDStratagemSlot* ResetEagleSlot = Loadout.GetSlot(EagleIndex);
        TestTrue(TEXT("Uses: reset Eagle to 2"), ResetEagleSlot && ResetEagleSlot->UsesLeft == 2);
    }

    // 4. 재사용 대기 GE (엔진 GameplayEffectTests.cpp 와 같은 월드 준비)
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(World);

        FURL URL;
        World->InitializeActorsForPlay(URL);
        World->BeginPlay();

        AAbilitySystemTestPawn* Pawn = World->SpawnActor<AAbilitySystemTestPawn>();
        UAbilitySystemComponent* ASC = Pawn ? Pawn->GetAbilitySystemComponent() : nullptr;
        if (TestNotNull(TEXT("Cooldown: ASC"), ASC))
        {
            ASC->InitAbilityActorInfo(Pawn, Pawn);

            UHDGE_StratagemCooldown::ApplyToSlot(ASC, 1, 30.f);
            TestTrue(TEXT("Cooldown: slot 2 tag present"), ASC->HasMatchingGameplayTag(Tag_Cooldown_Stratagem_Slot2));
            TestFalse(TEXT("Cooldown: slot 1 tag absent"), ASC->HasMatchingGameplayTag(Tag_Cooldown_Stratagem_Slot1));
            TestTrue(TEXT("Cooldown: slot index 1 on cooldown"), UHDGE_StratagemCooldown::IsSlotOnCooldown(ASC, 1));
            TestEqual(TEXT("Cooldown: slot index 1 remaining 30"), UHDGE_StratagemCooldown::GetSlotRemaining(ASC, 1), 30.f, 0.01f);
            TestEqual(TEXT("Cooldown: slot index 0 remaining 0"), UHDGE_StratagemCooldown::GetSlotRemaining(ASC, 0), 0.f);

            UHDGE_StratagemCooldown::ApplyToSlot(ASC, 0, 0.f);
            TestFalse(TEXT("Cooldown: 0 seconds adds nothing"), UHDGE_StratagemCooldown::IsSlotOnCooldown(ASC, 0));

            UHDGE_StratagemCooldown::ApplyToSlot(ASC, 4, 30.f);
            TestFalse(TEXT("Cooldown: slot index 4 out of range"), UHDGE_StratagemCooldown::GetSlotTag(4).IsValid());

            UHDGE_StratagemCooldown::ClearAll(ASC);
            bool bAnyOnCooldown = false;
            for (int32 SlotIndex = 0; SlotIndex < FHDStratagemLoadout::MaxSlots; ++SlotIndex)
            {
                bAnyOnCooldown |= UHDGE_StratagemCooldown::IsSlotOnCooldown(ASC, SlotIndex);
            }
            TestFalse(TEXT("Cooldown: cleared"), bAnyOnCooldown);
        }

        World->EndPlay(EEndPlayReason::Quit);
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
