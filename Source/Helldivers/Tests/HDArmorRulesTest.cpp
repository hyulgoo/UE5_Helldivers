// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Weapon/HDArmorRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDArmorRulesTest, "Helldivers.Combat.Armor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDArmorRulesTest::RunTest(const FString& Parameters)
{
    // 중장갑 표: 몸통 3·머리 1·팔다리 2, 머리 2배. 기본 표: 전부 0, 전부 1배
    FHDArmorProfile Heavy;
    Heavy.BodyArmor             = 3;
    Heavy.HeadArmor             = 1;
    Heavy.ArmArmor              = 2;
    Heavy.LegArmor              = 2;
    Heavy.HeadDamageMultiplier  = 2.f;

    const FHDArmorProfile Default;

    // 1. 본 이름 → 부위
    {
        struct FZoneCase
        {
            FName       BoneName;
            EHDHitZone  Expected;
        };

        const FZoneCase ZoneCases[] =
        {
            { TEXT("head"),         EHDHitZone::Head },
            { TEXT("neck_01"),      EHDHitZone::Head },
            { TEXT("neck_02"),      EHDHitZone::Head },
            { TEXT("Head"),         EHDHitZone::Head },
            { TEXT("upperarm_l"),   EHDHitZone::Arm },
            { TEXT("lowerarm_r"),   EHDHitZone::Arm },
            { TEXT("hand_l"),       EHDHitZone::Arm },
            { TEXT("clavicle_r"),   EHDHitZone::Arm },
            { TEXT("thigh_l"),      EHDHitZone::Leg },
            { TEXT("calf_r"),       EHDHitZone::Leg },
            { TEXT("foot_l"),       EHDHitZone::Leg },
            { TEXT("ball_r"),       EHDHitZone::Leg },
            { TEXT("spine_03"),     EHDHitZone::Body },
            { TEXT("pelvis"),       EHDHitZone::Body },
            { NAME_None,            EHDHitZone::Body },
            { TEXT("weapon_r"),     EHDHitZone::Body },
        };

        for (const FZoneCase& ZoneCase : ZoneCases)
        {
            const FString Message = FString::Printf(TEXT("Zone: %s"), *ZoneCase.BoneName.ToString());
            TestEqual(*Message, FString(HDArmor::GetZoneName(HDArmor::ZoneFromBone(ZoneCase.BoneName))), FString(HDArmor::GetZoneName(ZoneCase.Expected)));
        }
    }

    // 2. 관통 대 장갑
    {
        struct FOutcomeCase
        {
            int32           Penetration;
            int32           Armor;
            EHDHitOutcome   Expected;
        };

        const FOutcomeCase OutcomeCases[] =
        {
            { 2, 1, EHDHitOutcome::Full },
            { 1, 0, EHDHitOutcome::Full },
            { 4, 3, EHDHitOutcome::Full },
            { 2, 2, EHDHitOutcome::Reduced },
            { 2, 3, EHDHitOutcome::Ricochet },
        };

        for (const FOutcomeCase& OutcomeCase : OutcomeCases)
        {
            const FString Message = FString::Printf(TEXT("Outcome: (%d, %d)"), OutcomeCase.Penetration, OutcomeCase.Armor);
            TestEqual(*Message, FString(HDArmor::GetOutcomeName(HDArmor::ResolveOutcome(OutcomeCase.Penetration, OutcomeCase.Armor))), FString(HDArmor::GetOutcomeName(OutcomeCase.Expected)));
        }
    }

    // 3. 입사각
    {
        TestEqual(TEXT("Incidence: (2, 1.0)"), HDArmor::ApplyIncidence(2, 1.f), 2);
        TestEqual(TEXT("Incidence: (2, 0.5) boundary keeps"), HDArmor::ApplyIncidence(2, 0.5f), 2);
        TestEqual(TEXT("Incidence: (2, 0.49)"), HDArmor::ApplyIncidence(2, 0.49f), 1);
        TestEqual(TEXT("Incidence: (1, 0.1) not below 1"), HDArmor::ApplyIncidence(1, 0.1f), 1);
        TestEqual(TEXT("Incidence: (4, -0.2)"), HDArmor::ApplyIncidence(4, -0.2f), 3);
    }

    // 4. 한 발의 판정
    {
        struct FHitCase
        {
            const TCHAR*            Message;
            const FHDArmorProfile*  Profile;
            FName                   BoneName;
            int32                   Penetration;
            float                   BaseDamage;
            EHDHitZone              ExpectedZone;
            EHDHitOutcome           ExpectedOutcome;
            float                   ExpectedDamage;
        };

        const FHitCase HitCases[] =
        {
            { TEXT("Hit: heavy spine pen 2"),       &Heavy,   TEXT("spine_03"), 2, 50.f,  EHDHitZone::Body, EHDHitOutcome::Ricochet, 0.f },
            { TEXT("Hit: heavy head pen 2"),        &Heavy,   TEXT("head"),     2, 50.f,  EHDHitZone::Head, EHDHitOutcome::Full,     100.f },
            { TEXT("Hit: heavy head pen 1"),        &Heavy,   TEXT("head"),     1, 50.f,  EHDHitZone::Head, EHDHitOutcome::Reduced,  65.f },
            { TEXT("Hit: heavy thigh pen 2"),       &Heavy,   TEXT("thigh_l"),  2, 50.f,  EHDHitZone::Leg,  EHDHitOutcome::Reduced,  32.5f },
            { TEXT("Hit: heavy spine pen 3"),       &Heavy,   TEXT("spine_03"), 3, 50.f,  EHDHitZone::Body, EHDHitOutcome::Reduced,  32.5f },
            { TEXT("Hit: heavy spine pen 4"),       &Heavy,   TEXT("spine_03"), 4, 50.f,  EHDHitZone::Body, EHDHitOutcome::Full,     50.f },
            { TEXT("Hit: default spine pen 1"),     &Default, TEXT("spine_03"), 1, 50.f,  EHDHitZone::Body, EHDHitOutcome::Full,     50.f },
            { TEXT("Hit: default head pen 1"),      &Default, TEXT("head"),     1, 50.f,  EHDHitZone::Head, EHDHitOutcome::Full,     50.f },
            { TEXT("Hit: negative damage is zero"), &Default, TEXT("spine_03"), 2, -10.f, EHDHitZone::Body, EHDHitOutcome::Full,     0.f },
        };

        for (const FHitCase& HitCase : HitCases)
        {
            const FHDHitResolution Resolution = HDArmor::ResolveHit(*HitCase.Profile, HitCase.BoneName, HitCase.Penetration, HitCase.BaseDamage);
            TestEqual(*FString::Printf(TEXT("%s: zone"), HitCase.Message), FString(HDArmor::GetZoneName(Resolution.Zone)), FString(HDArmor::GetZoneName(HitCase.ExpectedZone)));
            TestEqual(*FString::Printf(TEXT("%s: outcome"), HitCase.Message), FString(HDArmor::GetOutcomeName(Resolution.Outcome)), FString(HDArmor::GetOutcomeName(HitCase.ExpectedOutcome)));
            TestEqual(*FString::Printf(TEXT("%s: damage"), HitCase.Message), Resolution.Damage, HitCase.ExpectedDamage, KINDA_SMALL_NUMBER);
        }
    }

    // 5. 폭발: 몸통 장갑, 부위 배율 없음
    {
        struct FExplosionCase
        {
            const TCHAR*            Message;
            const FHDArmorProfile*  Profile;
            int32                   Penetration;
            float                   BaseDamage;
            EHDHitOutcome           ExpectedOutcome;
            float                   ExpectedDamage;
        };

        const FExplosionCase ExplosionCases[] =
        {
            { TEXT("Explosion: heavy pen 3"),   &Heavy,   3, 200.f, EHDHitOutcome::Reduced,  130.f },
            { TEXT("Explosion: heavy pen 2"),   &Heavy,   2, 200.f, EHDHitOutcome::Ricochet, 0.f },
            { TEXT("Explosion: heavy pen 4"),   &Heavy,   4, 200.f, EHDHitOutcome::Full,     200.f },
            { TEXT("Explosion: default pen 1"), &Default, 1, 200.f, EHDHitOutcome::Full,     200.f },
        };

        for (const FExplosionCase& ExplosionCase : ExplosionCases)
        {
            const FHDHitResolution Resolution = HDArmor::ResolveExplosion(*ExplosionCase.Profile, ExplosionCase.Penetration, ExplosionCase.BaseDamage);
            TestEqual(*FString::Printf(TEXT("%s: outcome"), ExplosionCase.Message), FString(HDArmor::GetOutcomeName(Resolution.Outcome)), FString(HDArmor::GetOutcomeName(ExplosionCase.ExpectedOutcome)));
            TestEqual(*FString::Printf(TEXT("%s: damage"), ExplosionCase.Message), Resolution.Damage, ExplosionCase.ExpectedDamage, KINDA_SMALL_NUMBER);
        }
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
