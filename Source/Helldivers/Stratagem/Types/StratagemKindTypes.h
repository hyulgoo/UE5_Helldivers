#pragma once

UENUM(BlueprintType)
enum class EHDStratagemKind : uint8
{
    // 투사체(궤도·이글. bIsEagle 로 나눈다). JSON 행에 Kind 를 적지 않으면 이것이다
    Projectile,
    Resupply,
    Sentry,
    Count,
};
