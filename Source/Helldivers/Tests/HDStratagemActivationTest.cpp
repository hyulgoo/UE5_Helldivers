// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemOrbital.h"
#include "AbilitySystem/GameAbility/HDGA_StratagemEagle.h"
#include "GameData/HDStratagemEffectData.h"
#include "Stratagem/HDStratagem.h"
#include "Stratagem/HDStratagemPattern.h"
#include "Stratagem/BattleShip/HDBattleShip.h"
#include "Define/HDGameplayTag.h"

namespace HDStratagemActivationTest
{
    // 아래 세 데이터는 DT_StratagemEffectData 의 세 행과 같은 값이다(계획 「현황 7-②」).
    // 값 초기화(= FHDStratagemEffectData())로 기본값이 없는 비트 필드도 0 으로 시작한다
    FHDStratagemEffectData MakePrecisionStrike()
    {
        FHDStratagemEffectData Data = FHDStratagemEffectData();
        Data.bIsEagle                   = false;
        Data.bUseRandomRange            = false;
        Data.StratagemProjectileType    = EHDStratagemProjectile::Bomb;
        Data.ProjectileDropLocation     = { FVector2D(0.f, 0.f) };
        return Data;
    }

    FHDStratagemEffectData MakeGatlingBarrage()
    {
        FHDStratagemEffectData Data = FHDStratagemEffectData();
        Data.bIsEagle                       = false;
        Data.OrbitalDuration                = 15.f;
        Data.bUseRandomRange                = true;
        Data.RandomPositionRange            = 500.f;
        Data.SpecifyProjectileSpawnCount    = 150;
        Data.StratagemProjectileType        = EHDStratagemProjectile::Bullet;
        return Data;
    }

    FHDStratagemEffectData MakeEagleAirstrike()
    {
        FHDStratagemEffectData Data = FHDStratagemEffectData();
        Data.bIsEagle                   = true;
        Data.EagleAirStrikeDirection    = EHDEagleAirStrikeDirection::Right;
        Data.bUseRandomRange            = false;
        Data.StratagemProjectileType    = EHDStratagemProjectile::Bomb;
        Data.ProjectileDropLocation     = { FVector2D(500.f, 0.f), FVector2D(-2.f, 0.f), FVector2D(-500.f, 0.f) };
        return Data;
    }

    // Config/HDStratagems.json 의 Resupply 행과 같은 종류(브리프 14). 투사체 열은 쓰지 않는다
    FHDStratagemEffectData MakeResupply()
    {
        FHDStratagemEffectData Data = FHDStratagemEffectData();
        Data.Kind                       = EHDStratagemKind::Resupply;
        Data.bIsEagle                   = false;
        Data.EffectLifetime             = 60.f;
        Data.EffectRange                = 250.f;
        return Data;
    }

    // 활성 인스턴스 하나 = (스트라타젬 이름, 표적 위치)
    using FActiveInstance = TPair<FName, FVector>;

    TArray<FActiveInstance> GetActiveInstances(const UAbilitySystemComponent* ASC, const TSubclassOf<UGameplayAbility> AbilityClass)
    {
        TArray<FActiveInstance> ActiveInstances;
        const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(AbilityClass);
        if (Spec == nullptr)
        {
            return ActiveInstances;
        }

        for (const UGameplayAbility* Instance : Spec->GetAbilityInstances())
        {
            const UHDGA_StratagemBase* StratagemAbility = Cast<UHDGA_StratagemBase>(Instance);
            if (StratagemAbility == nullptr || StratagemAbility->IsActive() == false)
            {
                continue;
            }

            ActiveInstances.Add(FActiveInstance(StratagemAbility->GetStratagemName(), StratagemAbility->GetTargetTransform().GetLocation()));
        }

        return ActiveInstances;
    }

    AHDStratagem* SpawnBeacon(UWorld* World, const FName StratagemName, const float ActiveDelay)
    {
        AHDStratagem* Beacon = World->SpawnActor<AHDStratagem>();
        if (Beacon)
        {
            Beacon->SetStratagemInfo(StratagemName, ActiveDelay);
        }

        return Beacon;
    }

    // AHDBattleShip::ActivateStratagem 이 보내는 것과 같은 이벤트를 보낸다. 반환값 = 발동한 어빌리티 수
    int32 SendActivationEvent(UAbilitySystemComponent* ASC, const FGameplayTag& EventTag, const AHDStratagem* Beacon, const FVector& TargetLocation)
    {
        const FGameplayEventData Payload = AHDBattleShip::MakeActivationPayload(EventTag, Beacon, FTransform(TargetLocation));
        return ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDStratagemActivationTest, "Helldivers.Stratagem.Activation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDStratagemActivationTest::RunTest(const FString& Parameters)
{
    using namespace HDStratagemActivationTest;

    // 1부: 순수 함수(HDStratagemPattern)
    {
        const FHDStratagemEffectData Precision = MakePrecisionStrike();
        FRandomStream Stream(1);

        TestTrue(TEXT("Precision: event Orbital"), HDStratagemPattern::GetActivationEventTag(Precision) == Tag_Event_Stratagem_Orbital.GetTag());
        TestEqual(TEXT("Precision: 1 projectile"), HDStratagemPattern::GetProjectileCount(Precision), 1);
        TestTrue(TEXT("Precision: offset 0 is (0,0)"), HDStratagemPattern::GetDropOffset(Precision, 0, Stream).Equals(FVector2D(0.f, 0.f)));
        TestEqual(TEXT("Precision: interval 0"), HDStratagemPattern::GetShotInterval(Precision), 0.f);
    }

    {
        const FHDStratagemEffectData Gatling = MakeGatlingBarrage();

        TestTrue(TEXT("Gatling: event Orbital"), HDStratagemPattern::GetActivationEventTag(Gatling) == Tag_Event_Stratagem_Orbital.GetTag());
        TestEqual(TEXT("Gatling: 150 projectiles from SpecifyProjectileSpawnCount"), HDStratagemPattern::GetProjectileCount(Gatling), 150);
        TestEqual(TEXT("Gatling: interval 15 / 150"), HDStratagemPattern::GetShotInterval(Gatling), 0.1f);

        bool bAllInsideRadius = true;
        TSet<FVector2D> Offsets;
        for (int32 Seed = 1; Seed <= 50; ++Seed)
        {
            FRandomStream Stream(Seed);
            const FVector2D Offset = HDStratagemPattern::GetDropOffset(Gatling, 0, Stream);
            bAllInsideRadius &= Offset.Size() <= Gatling.RandomPositionRange + KINDA_SMALL_NUMBER;
            Offsets.Add(Offset);
        }

        TestTrue(TEXT("Gatling: random offsets inside radius 500 (seeds 1-50)"), bAllInsideRadius);
        TestTrue(TEXT("Gatling: random offsets are not all the same (seeds 1-50)"), Offsets.Num() > 1);
    }

    {
        const FHDStratagemEffectData Eagle = MakeEagleAirstrike();
        FRandomStream Stream(1);

        TestTrue(TEXT("Eagle: event Eagle"), HDStratagemPattern::GetActivationEventTag(Eagle) == Tag_Event_Stratagem_Eagle.GetTag());
        TestEqual(TEXT("Eagle: 3 projectiles from ProjectileDropLocation"), HDStratagemPattern::GetProjectileCount(Eagle), 3);
        TestTrue(TEXT("Eagle: offset 1 is (-2,0)"), HDStratagemPattern::GetDropOffset(Eagle, 1, Stream).Equals(FVector2D(-2.f, 0.f)));
        TestTrue(TEXT("Eagle: offset 3 out of range is (0,0)"), HDStratagemPattern::GetDropOffset(Eagle, 3, Stream).Equals(FVector2D(0.f, 0.f)));
    }

    {
        FHDStratagemEffectData Resupply = MakeResupply();

        TestTrue(TEXT("Resupply: event Resupply"), HDStratagemPattern::GetActivationEventTag(Resupply) == Tag_Event_Stratagem_Resupply.GetTag());

        // 종류가 Projectile 이 아니면 bIsEagle 은 이벤트를 고르지 않는다
        Resupply.bIsEagle = true;
        TestTrue(TEXT("Resupply: bIsEagle ignored when kind is not Projectile"), HDStratagemPattern::GetActivationEventTag(Resupply) == Tag_Event_Stratagem_Resupply.GetTag());
    }

    {
        FHDStratagemEffectData EmptyDrops = FHDStratagemEffectData();
        EmptyDrops.SpecifyProjectileSpawnCount = 4;
        FRandomStream Stream(1);

        TestEqual(TEXT("Empty drops: 4 projectiles from SpecifyProjectileSpawnCount"), HDStratagemPattern::GetProjectileCount(EmptyDrops), 4);
        TestTrue(TEXT("Empty drops: offset is (0,0)"), HDStratagemPattern::GetDropOffset(EmptyDrops, 2, Stream).Equals(FVector2D(0.f, 0.f)));
    }

    // 2부: 전함 GA. 엔진 GameplayEffectTests.cpp 와 같은 월드 준비(게임모드가 없어 액터 BeginPlay 는 불리지 않는다)
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    WorldContext.SetCurrentWorld(World);

    FURL URL;
    World->InitializeActorsForPlay(URL);
    World->BeginPlay();

    {
        // C++ 클래스(BP 아님). 월드를 틱하지 않으므로 발동 지연 안에 있는 인스턴스는 활성으로 남는다
        AHDBattleShip* BattleShip = World->SpawnActor<AHDBattleShip>(FVector(0.f, 0.f, 80000.f), FRotator::ZeroRotator);
        UAbilitySystemComponent* ASC = BattleShip ? BattleShip->GetAbilitySystemComponent() : nullptr;
        if (TestNotNull(TEXT("BattleShip: ASC"), ASC))
        {
            TestNotNull(TEXT("BattleShip: orbital ability given"), ASC->FindAbilitySpecFromClass(UHDGA_StratagemOrbital::StaticClass()));
            TestNotNull(TEXT("BattleShip: eagle ability given"), ASC->FindAbilitySpecFromClass(UHDGA_StratagemEagle::StaticClass()));

            const FName NameA(TEXT("OrbitalPrecisionStrike"));
            const FName NameB(TEXT("OrbitalGatlingBarrage"));
            const FName NameEagle(TEXT("EagleAirstrike"));
            const FVector TargetA(1000.f, 0.f, 0.f);
            const FVector TargetB(0.f, 2000.f, 0.f);
            const FVector TargetEagle(-3000.f, 0.f, 0.f);

            // 앞 스트라타젬이 끝나기 전에 다른 곳에 다시 던진다(계획 「현황 7-③」)
            const int32 ActivatedA = SendActivationEvent(ASC, Tag_Event_Stratagem_Orbital, SpawnBeacon(World, NameA, 2.f), TargetA);
            const int32 ActivatedB = SendActivationEvent(ASC, Tag_Event_Stratagem_Orbital, SpawnBeacon(World, NameB, 1.f), TargetB);
            TestEqual(TEXT("Orbital A: activated 1"), ActivatedA, 1);
            TestEqual(TEXT("Orbital B: activated 1"), ActivatedB, 1);

            const TArray<FActiveInstance> OrbitalInstances = GetActiveInstances(ASC, UHDGA_StratagemOrbital::StaticClass());
            TestEqual(TEXT("Orbital: 2 active instances"), OrbitalInstances.Num(), 2);
            TestTrue(TEXT("Orbital: instance A keeps its name and target"), OrbitalInstances.Contains(FActiveInstance(NameA, TargetA)));
            TestTrue(TEXT("Orbital: instance B keeps its name and target"), OrbitalInstances.Contains(FActiveInstance(NameB, TargetB)));

            SendActivationEvent(ASC, Tag_Event_Stratagem_Eagle, SpawnBeacon(World, NameEagle, 3.f), TargetEagle);
            TestEqual(TEXT("Eagle: 1 active instance"), GetActiveInstances(ASC, UHDGA_StratagemEagle::StaticClass()).Num(), 1);
            TestEqual(TEXT("Eagle: orbital instances unchanged"), GetActiveInstances(ASC, UHDGA_StratagemOrbital::StaticClass()).Num(), 2);

            // 비컨이 없으면 바로 끝나고 다른 인스턴스는 그대로다
            SendActivationEvent(ASC, Tag_Event_Stratagem_Orbital, nullptr, TargetA);
            TestEqual(TEXT("No beacon: ends at once, orbital instances unchanged"), GetActiveInstances(ASC, UHDGA_StratagemOrbital::StaticClass()).Num(), 2);
        }
    }

    World->EndPlay(EEndPlayReason::Quit);
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
