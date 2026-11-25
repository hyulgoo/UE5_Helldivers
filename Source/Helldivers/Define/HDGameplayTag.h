
#pragma once

#include "NativeGameplayTags.h"

// Charaacter Action
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_Action_StratagemInputMode);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_Action_HoldStratagem);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_Action_DetachStratagem);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_Action_ThrowEnd);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_Action_Dead);
// BPGA_Dead 의 ActivationOwnedTags 가 쓰는 이름. C++ 은 쓰지 않고 등록만 한다(없으면 에셋 태그가 무효)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_State_IsDead);
// 4 로 수류탄을 든 상태(루즈 태그, 네이티브 전용). 투척 GA 의 필요 태그, 사격·재장전 GA 는 이 태그가 있으면 켜지지 않는다
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_State_GrenadeReady);
// 스팀으로 회복하는 중(UHDGA_Stim 의 ActivationOwnedTags, 네이티브 전용). 있는 동안 스팀을 또 쓰지 못한다
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_State_Stimmed);
// 급강하로 몸을 던져 공중에 있는 중(루즈 태그, 네이티브 전용). AHDCharacterPlayer::Dive 가 세우고 착지·사망·래그돌이 내린다
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Character_State_Diving);

// Block
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Block_All);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Block_Move);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Block_Fire);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Block_Reload);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Block_Grenade);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Block_Stratagem);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Block_Minimap);
// 사격 중(UHDGA_Fire)에는 달리기를 새로 시작하지 않는다(UHDGA_MovementTrigger::CanActivateAbility)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Block_Sprint);

// Input
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Input);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_Aiming);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_Shoulder);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_Sprint);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_Crouch);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_Prone);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_Attack);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_Reload);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_RegenStamina);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_StratagemInputMode);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_ThrowStratagem);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Trigger_Dead);

// Event
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_StratagemHUD);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_StratagemHUD_Active);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_StratagemHUD_Deactive);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_StratagemHUD_AddCommand);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_PlayerHUD);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_PlayerHUD_CurrentAmmoChange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_PlayerHUD_MaxAmmoChange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_PlayerHUD_CurrentCapacityChange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_PlayerHUD_MaxCapacityChange);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_PlayerHUD_Initialize);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_PlayerHUD_EquipWeapon);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_Spawn_Projectile);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_Reload);

// Stratagem: 전함 ASC 의 스트라타젬 GA 를 깨우는 이벤트
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_Stratagem_Orbital);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_Stratagem_Eagle);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_Stratagem_Resupply);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_Stratagem_Sentry);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Event_Stratagem_SupportWeapon);

// Cooldown: 로드아웃 칸(1~4)의 재사용 대기(UHDGE_StratagemCooldown 이 Spec 에 붙인다)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Cooldown_Stratagem_Slot1);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Cooldown_Stratagem_Slot2);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Cooldown_Stratagem_Slot3);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Cooldown_Stratagem_Slot4);

// Data
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Damage_Projectile);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_DotDamage_TickDamage);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Projectile_StatusEffectDuration);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Attribute);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Attribute_Ammo);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Attribute_MaxAmmo);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Attribute_Capacity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Attribute_MaxCapacity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Knockback_Hit);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Data_Knockback_Ragdoll);

// Stamina
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Cost_Stamina);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Regen_Stamina);

// GameplayCue
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_GameplayCue_Explode_BombHeavy);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_GameplayCue_Explode_BombLight);
