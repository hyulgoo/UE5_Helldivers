// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 함선 화면 글꼴. 설계 px 는 34·24·22·18·18·180 이고 Bold 는 Title·Header·KeyCap·Countdown
enum class EHDShipFont : uint8
{
    Title,
    Header,
    Body,
    Small,
    KeyCap,
    Countdown
};

// 함선 화면(단말기·함선 메뉴·Bridge)의 색 토큰과 설계 크기. 색 값은 이 파일 밖에 쓰지 않는다.
// 설계 단위는 1920×1080 이고, 화면 배율(ComputeScale)을 곱해 16:9 안전 영역 안에 그린다
namespace HDShipUIStyle
{
    // 색 토큰
    const FLinearColor  Backdrop        (0.01f, 0.015f, 0.025f, 0.82f);
    const FLinearColor  Panel           (0.04f, 0.05f, 0.07f, 0.92f);
    const FLinearColor  Card            (0.07f, 0.085f, 0.11f, 0.95f);
    const FLinearColor  Line            (0.25f, 0.28f, 0.33f, 1.f);
    const FLinearColor  Text            (0.92f, 0.93f, 0.95f, 1.f);
    const FLinearColor  TextDim         (0.55f, 0.58f, 0.63f, 1.f);
    const FLinearColor  TextDisabled    (0.35f, 0.37f, 0.40f, 1.f);
    const FLinearColor  Accent          (1.f, 0.85f, 0.2f, 1.f);
    const FLinearColor  AccentFill      (1.f, 0.85f, 0.2f, 0.14f);
    const FLinearColor  Good            (0.35f, 0.85f, 0.45f, 1.f);
    const FLinearColor  Warn            (0.95f, 0.45f, 0.2f, 1.f);
    const FLinearColor  Requisition     (0.85f, 0.85f, 0.90f, 1.f);
    const FLinearColor  SampleCommon    (0.40f, 0.78f, 0.70f, 1.f);
    const FLinearColor  SampleRare      (0.40f, 0.60f, 0.95f, 1.f);
    const FLinearColor  SampleSuper     (0.90f, 0.40f, 0.55f, 1.f);

    // 설계 크기(1920×1080 단위)
    constexpr float     DesignWidth     = 1920.f;
    constexpr float     DesignHeight    = 1080.f;
    constexpr float     MinScale        = 0.5f;
    constexpr float     MaxScale        = 2.f;
    constexpr int32     MinFontPx       = 12;
    constexpr float     Margin          = 48.f;
    constexpr float     TopBarHeight    = 88.f;
    constexpr float     BottomBarHeight = 64.f;
    constexpr float     Gap             = 16.f;
    constexpr float     DetailWidth     = 520.f;
    constexpr float     RowWidth        = 640.f;
    constexpr float     RowHeight       = 56.f;
    constexpr float     RowGap          = 12.f;
    constexpr float     BorderThin      = 1.f;
    constexpr float     BorderSelected  = 3.f;
    // 열 때 틀이 아래에서 올라오는 거리
    constexpr float     OpenSlide       = 16.f;

    // clamp(min(가로 / 1920, 세로 / 1080), 0.5, 2)
    HELLDIVERS_API float    ComputeScale(const FVector2D& ViewSize);
    // 설계 px
    HELLDIVERS_API int32    GetDesignFontPx(EHDShipFont Font);
    // max(round(설계 px × Scale), 12)
    HELLDIVERS_API int32    GetFontPx(EHDShipFont Font, float Scale);
    HELLDIVERS_API bool     IsBold(EHDShipFont Font);
}
