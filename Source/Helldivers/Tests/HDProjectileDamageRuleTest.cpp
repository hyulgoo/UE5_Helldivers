// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Weapon/Projectile/HDProjectileBase.h"
#include "Character/Player/HDCharacterPlayer.h"
#include "Character/NPC/HDEnemyShooter.h"
#include "Character/NPC/HDEnemyCharger.h"
#include "Mission/HDObjectiveFacility.h"
#include "Stratagem/BattleShip/HDBattleShip.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDProjectileDamageRuleTest, "Helldivers.Combat.ProjectileDamageRule", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDProjectileDamageRuleTest::RunTest(const FString& Parameters)
{
    // 월드 없이 CDO 로 판정만 본다
    const AActor* Shooter = GetDefault<AHDEnemyShooter>();
    const AActor* Charger = GetDefault<AHDEnemyCharger>();
    const AActor* Player = GetDefault<AHDCharacterPlayer>();
    const AActor* Facility = GetDefault<AHDObjectiveFacility>();
    const AActor* BattleShip = GetDefault<AHDBattleShip>();

    TestFalse(TEXT("Enemy -> facility: no damage"), AHDProjectileBase::CanApplyImpactDamage(Shooter, Facility));
    TestTrue(TEXT("Enemy -> enemy: damage"), AHDProjectileBase::CanApplyImpactDamage(Shooter, Charger));
    TestTrue(TEXT("Enemy -> player: damage"), AHDProjectileBase::CanApplyImpactDamage(Shooter, Player));
    TestTrue(TEXT("Player -> facility: damage"), AHDProjectileBase::CanApplyImpactDamage(Player, Facility));
    TestTrue(TEXT("Player -> enemy: damage"), AHDProjectileBase::CanApplyImpactDamage(Player, Shooter));
    TestTrue(TEXT("Battleship -> facility: damage"), AHDProjectileBase::CanApplyImpactDamage(BattleShip, Facility));
    TestFalse(TEXT("No owner: no damage"), AHDProjectileBase::CanApplyImpactDamage(nullptr, Facility));
    TestFalse(TEXT("No target: no damage"), AHDProjectileBase::CanApplyImpactDamage(Player, nullptr));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
