#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Stratagem/Types/StratagemKindTypes.h"
#include "Stratagem/Types/StratagemProjectileTypes.h"
#include "Stratagem/Types/EagleAirStrikeDirectionTypes.h"
#include "HDStratagemEffectData.generated.h"

class AHDWeapon;

USTRUCT(BlueprintType)
struct FHDStratagemEffectData : public FTableRowBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kind")
    EHDStratagemKind            Kind                        = EHDStratagemKind::Projectile;

    // 보급 상자·포탑이 남아 있는 시간(초). 0 = 보급: 줍기 전까지 / 포탑: 발사 수까지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployable")
    float                       EffectLifetime              = 0.f;

    // 보급: 줍는 반지름 / 포탑: 사거리(cm)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployable")
    float                       EffectRange                 = 0.f;

    // 포탑 발사 간격(초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployable")
    float                       EffectInterval              = 0.f;

    // 포탑 최대 발사 수. 0 = 수명까지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deployable")
    int32                       EffectMaxCount              = 0;

    // 지원 무기 투하로 떨어뜨리는 무기 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SupportWeapon")
    TSubclassOf<AHDWeapon>      SupportWeaponClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eagle")
    uint8                       bIsEagle : 1;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eagle")
    EHDEagleAirStrikeDirection  EagleAirStrikeDirection = EHDEagleAirStrikeDirection::Front;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbital")
    float                       OrbitalDuration = 0.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RandomLocation")
    uint8                       bUseRandomRange: 1;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RandomLocation")
    float                       RandomPositionRange = 0.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecifyProjectile")
    uint8                       SpecifyProjectileSpawnCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecifyProjectile")
    float                       SpecifyProjectileSpawnDelay = 0.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiple")
    uint8                       bMultipleSpawn: 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiple")
    TArray<FVector2D>           MultiSpawnDropLocation;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    EHDStratagemProjectile      StratagemProjectileType = EHDStratagemProjectile::Bullet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    TArray<FVector2D>           ProjectileDropLocation;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    float                       ProjectileAttackRange = 0.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    float                       ProjectileDamage = 0.f;
};