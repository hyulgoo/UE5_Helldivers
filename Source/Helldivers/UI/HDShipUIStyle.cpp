// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipUIStyle.h"

float HDShipUIStyle::ComputeScale(const FVector2D& ViewSize)
{
    // 검증 먼저(1-1): 일부러 가로만 본다
    return static_cast<float>(ViewSize.X) / DesignWidth;
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
