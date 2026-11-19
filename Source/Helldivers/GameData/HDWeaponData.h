#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "HDWeaponData.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EHDCrosshair : uint8
{
    Center,
    Left,
    Right,
    Top,
    Bottom,
    Count
};
 
 USTRUCT(BlueprintType)
 struct FHDWeaponErgoData : public FTableRowBase
 {
     GENERATED_BODY()
 
 public:
     UPROPERTY(EditDefaultsOnly)
     float ZoomedFOV = 0.f;
     
     UPROPERTY(EditDefaultsOnly)
     float ZoomInterpSpeed = 0.f;
     
     UPROPERTY(EditDefaultsOnly)
     float ErgonomicFactor = 0.f;
 };
 
 USTRUCT(BlueprintType)
 struct FHDWeaponAmmoData : public FTableRowBase
 {
     GENERATED_BODY()
 
 public:
     UPROPERTY(EditDefaultsOnly)
     int32 Ammo = 0;
     
     UPROPERTY(EditDefaultsOnly)
     int32 MaxAmmo = 0;
     
     UPROPERTY(EditDefaultsOnly)
     int32 Capacity = 0;
 
     UPROPERTY(EditDefaultsOnly)
     int32 MaxCapacity = 0;
 };
 
 USTRUCT(BlueprintType)
struct FHDWeaponCrosshairPair
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly)
    EHDCrosshair            Type = EHDCrosshair::Center;

    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UTexture2D>  Texture;
};

 USTRUCT(BlueprintType)
 struct FHDWeaponUIData : public FTableRowBase
 {
     GENERATED_BODY()
 
 public:
     UPROPERTY(EditDefaultsOnly)
     TObjectPtr<UTexture2D>		       WeaponIconImage;
 
     UPROPERTY(EditDefaultsOnly)
     TArray<FHDWeaponCrosshairPair>    CrosshairTextureList;
 };
