// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayTagsManager.h"
#include "Define/HDGameplayTag.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDGameplayTagTest, "Helldivers.Tags.NativeTagsRegistered", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDGameplayTagTest::RunTest(const FString& Parameters)
{
    // HDGameplayTag.h 에 선언된 상수를 빠짐없이 넣는다
    const FNativeGameplayTag* NativeTags[] =
    {
        // Charaacter Action
        &Tag_Character_Action_StratagemInputMode,
        &Tag_Character_Action_HoldStratagem,
        &Tag_Character_Action_DetachStratagem,
        &Tag_Character_Action_ThrowEnd,
        &Tag_Character_Action_Dead,
        &Tag_Character_State_IsDead,

        // Block
        &Tag_Block_All,
        &Tag_Block_Move,
        &Tag_Block_Fire,
        &Tag_Block_Reload,
        &Tag_Block_Grenade,
        &Tag_Block_Stratagem,
        &Tag_Block_Minimap,

        // Input
        &Tag_Input,
        &Tag_Trigger_Aiming,
        &Tag_Trigger_Shoulder,
        &Tag_Trigger_Sprint,
        &Tag_Trigger_Crouch,
        &Tag_Trigger_Prone,
        &Tag_Trigger_Attack,
        &Tag_Trigger_Reload,
        &Tag_Trigger_RegenStamina,
        &Tag_Trigger_StratagemInputMode,
        &Tag_Trigger_ThrowStratagem,
        &Tag_Trigger_Dead,

        // Event
        &Tag_Event_StratagemHUD,
        &Tag_Event_StratagemHUD_Active,
        &Tag_Event_StratagemHUD_Deactive,
        &Tag_Event_StratagemHUD_AddCommand,
        &Tag_Event_PlayerHUD,
        &Tag_Event_PlayerHUD_CurrentAmmoChange,
        &Tag_Event_PlayerHUD_MaxAmmoChange,
        &Tag_Event_PlayerHUD_CurrentCapacityChange,
        &Tag_Event_PlayerHUD_MaxCapacityChange,
        &Tag_Event_PlayerHUD_Initialize,
        &Tag_Event_PlayerHUD_EquipWeapon,
        &Tag_Event_Spawn_Projectile,
        &Tag_Event_Reload,

        // Stratagem
        &Tag_Event_Stratagem_Orbital,
        &Tag_Event_Stratagem_Eagle,
        &Tag_Event_Stratagem_Resupply,
        &Tag_Event_Stratagem_Sentry,

        // Cooldown
        &Tag_Cooldown_Stratagem_Slot1,
        &Tag_Cooldown_Stratagem_Slot2,
        &Tag_Cooldown_Stratagem_Slot3,
        &Tag_Cooldown_Stratagem_Slot4,

        // Data
        &Tag_Data_Damage_Projectile,
        &Tag_Data_DotDamage_TickDamage,
        &Tag_Data_Projectile_StatusEffectDuration,
        &Tag_Data_Attribute,
        &Tag_Data_Attribute_Ammo,
        &Tag_Data_Attribute_MaxAmmo,
        &Tag_Data_Attribute_Capacity,
        &Tag_Data_Attribute_MaxCapacity,
        &Tag_Data_Knockback_Hit,
        &Tag_Data_Knockback_Ragdoll,

        // Stamina
        &Tag_Cost_Stamina,
        &Tag_Regen_Stamina,

        // GameplayCue
        &Tag_GameplayCue_Explode_BombHeavy,
    };

    const UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();
    for (const FNativeGameplayTag* NativeTag : NativeTags)
    {
        const FName TagName = NativeTag->GetTag().GetTagName();
        const FString TagString = TagName.ToString();

        const bool bEndsWithWhitespace = TagString.IsEmpty() == false && FChar::IsWhitespace(TagString[TagString.Len() - 1]);
        TestFalse(FString::Printf(TEXT("Native tag '%s' ends with whitespace"), *TagString), bEndsWithWhitespace);
        TestTrue(FString::Printf(TEXT("Native tag '%s' is registered in the tag tree"), *TagString), TagsManager.RequestGameplayTag(TagName, false).IsValid());
    }

    // BPGA_Dead 의 트리거가 기다리는 이름
    TestEqual(TEXT("Tag_Trigger_Dead name"), Tag_Trigger_Dead.GetTag().GetTagName(), FName(TEXT("Trigger.Dead")));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
