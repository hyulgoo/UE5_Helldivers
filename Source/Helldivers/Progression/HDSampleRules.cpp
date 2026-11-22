// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDSampleRules.h"

namespace HDSamples
{
    bool TryParseType(const FString& Text, EHDSampleType& OutType)
    {
        const FString Trimmed = Text.TrimStartAndEnd();
        for (const EHDSampleType Type : { EHDSampleType::Common, EHDSampleType::Rare, EHDSampleType::Super })
        {
            if (Trimmed.Equals(GetTypeName(Type), ESearchCase::IgnoreCase))
            {
                OutType = Type;
                return true;
            }
        }
        return false;
    }

    const TCHAR* GetTypeName(EHDSampleType Type)
    {
        switch (Type)
        {
        case EHDSampleType::Common: return TEXT("Common");
        case EHDSampleType::Rare:   return TEXT("Rare");
        case EHDSampleType::Super:  return TEXT("Super");
        default:                    return TEXT("None");
        }
    }

    const TCHAR* GetMeshPath(EHDSampleType Type)
    {
        // 색 대신 모양으로 가른다(엔진 기본 도형)
        switch (Type)
        {
        case EHDSampleType::Rare:   return TEXT("/Engine/BasicShapes/Cone.Cone");
        case EHDSampleType::Super:  return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
        default:                    return TEXT("/Engine/BasicShapes/Sphere.Sphere");
        }
    }

    TArray<FHDSamplePlacement> MakePlacementPlan(const FHDSampleCounts& Counts, const FVector2D& CommonRing, const FVector2D& RareRing, const FVector2D& SuperRing)
    {
        const TPair<EHDSampleType, FVector2D> Rows[] =
        {
            { EHDSampleType::Common,    CommonRing },
            { EHDSampleType::Rare,      RareRing },
            { EHDSampleType::Super,     SuperRing },
        };

        TArray<FHDSamplePlacement> Plan;
        for (const TPair<EHDSampleType, FVector2D>& Row : Rows)
        {
            const int32 Count = Counts.Get(Row.Key);
            if (Count <= 0)
            {
                continue;
            }

            FHDSamplePlacement Placement;
            Placement.Type      = Row.Key;
            Placement.Count     = Count;
            Placement.MinRadius = Row.Value.X;
            Placement.MaxRadius = Row.Value.Y;
            Plan.Add(Placement);
        }
        return Plan;
    }

    FString FormatCarried(const FHDSampleCounts& Counts)
    {
        return FString::Printf(TEXT("SAMPLES  C %d  R %d  S %d"), Counts.Common, Counts.Rare, Counts.Super);
    }

    FString Describe(const FHDSampleCounts& Counts)
    {
        return FString::Printf(TEXT("%d/%d/%d"), Counts.Common, Counts.Rare, Counts.Super);
    }
}
