// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipUIStyle.h"

float HDShipUIStyle::ComputeScale(const FVector2D& ViewSize)
{
    const float Scale = static_cast<float>(FMath::Min(ViewSize.X / DesignWidth, ViewSize.Y / DesignHeight));
    return FMath::Clamp(Scale, MinScale, MaxScale);
}

int32 HDShipUIStyle::GetDesignFontPx(EHDShipFont Font)
{
    switch (Font)
    {
    case EHDShipFont::Title:        return 34;
    case EHDShipFont::Header:       return 24;
    case EHDShipFont::Body:         return 22;
    case EHDShipFont::Small:        return 18;
    case EHDShipFont::KeyCap:       return 18;
    case EHDShipFont::Countdown:    return 180;
    default:                        return 22;
    }
}

int32 HDShipUIStyle::GetFontPx(EHDShipFont Font, float Scale)
{
    return FMath::Max(FMath::RoundToInt(GetDesignFontPx(Font) * Scale), MinFontPx);
}

bool HDShipUIStyle::IsBold(EHDShipFont Font)
{
    return Font == EHDShipFont::Title || Font == EHDShipFont::Header || Font == EHDShipFont::KeyCap || Font == EHDShipFont::Countdown;
}
