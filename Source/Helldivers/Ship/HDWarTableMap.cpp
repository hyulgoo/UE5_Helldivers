// Fill out your copyright notice in the Description page of Project Settings.

#include "Ship/HDWarTableMap.h"

float HDWarTableMap::ComputeHalfExtent(float ObjectiveRadiusMax)
{
    return FMath::Max(ObjectiveRadiusMax * HalfExtentScale, MinHalfExtent);
}

FVector2D HDWarTableMap::WorldToMap(const FVector2D& WorldXY, const FVector2D& CenterXY, float HalfExtent, const FBox2D& MapRect)
{
    // 화면 Y 는 아래로 자라므로 월드 +X(지도 위)는 화면 Y 를 줄인다
    const FVector2D MapCenter = MapRect.GetCenter();
    const double PixelsPerUnit = MapRect.GetSize().X * 0.5 / FMath::Max(HalfExtent, KINDA_SMALL_NUMBER);
    const FVector2D Delta = WorldXY - CenterXY;
    return FVector2D(MapCenter.X + Delta.Y * PixelsPerUnit, MapCenter.Y - Delta.X * PixelsPerUnit);
}

TArray<FVector2D> HDWarTableMap::MakeRing(const FVector2D& Center, float Radius, int32 Segments)
{
    TArray<FVector2D> Points;
    if (Segments <= 0)
    {
        return Points;
    }

    Points.Reserve(Segments);
    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const double Angle = UE_DOUBLE_TWO_PI * Index / Segments;
        Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
    }
    return Points;
}

FVector HDWarTableMap::GetCaptureLocation(const FVector& CenterWorld, float ShipZ)
{
    return FVector(CenterWorld.X, CenterWorld.Y, ShipZ - CaptureBelowShip);
}
