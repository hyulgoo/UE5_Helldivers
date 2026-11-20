// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameData/HDWeaponData.h"

class UAbilitySystemComponent;

enum class EHDWeaponSlot : uint8
{
    Primary,
    Support,
};

struct FHDWeaponPickupResult
{
    EHDWeaponSlot           Slot        = EHDWeaponSlot::Primary;
    // 그 칸에 무기가 있었다(옛 무기를 바닥에 버린다)
    bool                    bReplaced   = false;
};

// 주무기·지원 무기 두 칸의 교체 규칙. 무기 액터는 UHDCombatComponent 가 들고, 이 클래스는 어느 칸이 찼고 어느 칸을 쓰는지만 정한다.
// 월드와 무관한 순수 로직이라 자동 테스트(Helldivers.Combat.WeaponSlots)로 고정한다
class HELLDIVERS_API FHDWeaponSlots
{
public:
    // 지원 무기는 Support 칸, 아니면 Primary 칸에 넣고 그 칸을 쓴다
    FHDWeaponPickupResult   Pickup(const bool bIsSupportWeapon);
    // 다른 칸에 무기가 있으면 쓰는 칸을 바꾸고 참
    bool                    Swap();
    bool                    HasWeapon(const EHDWeaponSlot Slot) const;
    EHDWeaponSlot           GetActiveSlot() const { return ActiveSlot; }
    void                    Reset();

private:
    bool                    bHasPrimary = false;
    bool                    bHasSupport = false;
    EHDWeaponSlot           ActiveSlot  = EHDWeaponSlot::Primary;
};

namespace HDWeaponAmmo
{
    // ASC 의 무기 Attribute(Ammo·MaxAmmo·Capacity·MaxCapacity) 지금 값. ASC 가 없으면 모두 0
    HELLDIVERS_API FHDWeaponAmmoData ReadFromAttributes(const UAbilitySystemComponent* ASC);
}

// 무기 몽타주에 노티파이가 없는 무기(SendsMontageEventsFromCode)를 위해 캐릭터가 C++ 로 보내는 이벤트의 시점 계산
namespace HDWeaponMontageEvent
{
    // 재장전 이벤트가 몸 몽타주보다 이만큼 먼저 오게 한다(초)
    constexpr float ReloadEventMargin = 0.1f;

    // 재장전 이벤트를 보낼 때까지의 시간(초). 0 이면 다음 틱에 보낸다.
    // 무기 몽타주 재장전 섹션 길이가 기본이다. 몸 재장전 몽타주(Reload GA 의 대기 태스크가 재생, 끝나면 태스크도 끝난다)가 더 짧으면
    // 그 길이 - ReloadEventMargin 으로 줄인다. 음수는 0. 섹션 길이가 0 이하면 0, 몸 길이가 0 이하면 섹션 길이 그대로
    HELLDIVERS_API float    GetReloadEventDelay(const float WeaponSectionLength, const float BodyMontageLength);

    // 직전 코드 발사 시각(LastFireTime, 아직 없으면 음수)에서 FireDelay 이상 지났으면 참. Now - LastFireTime == FireDelay 도 참
    HELLDIVERS_API bool     IsFireReady(const double LastFireTime, const double Now, const float FireDelay);
}
