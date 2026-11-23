// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UObject;
class UTexture2D;

// 코드 안 5×7 비트맵 글꼴로 약칭 타일 픽셀을 만든다. UMG 위젯이 UTexture2D 를 받기 때문에 런타임 텍스처로 올린다.
// 픽셀 함수는 순수라 자동 테스트(Helldivers.FrontEnd.ShipIcons)로 고정한다
namespace HDLetterTile
{
    // Width×Height, 왼쪽 위부터 행 순서. 바깥 1px 테두리, 나머지 배경, 글자 픽셀은 Foreground.
    // 5×7 글꼴(A~Z, 0~9, '-', 소문자는 대문자로). 글자 사이 1칸. 여백 = max(2, min(W, H) / 8).
    // 정수 배율 = 여백 안에 들어가는 가장 큰 값. 1 도 안 들어가면 글자를 그리지 않는다. 가운데 정렬(나머지는 버림)
    HELLDIVERS_API TArray<FColor>   Rasterize(const FString& Text, int32 Width, int32 Height, FColor Background, FColor Foreground, FColor Border);
    HELLDIVERS_API bool             HasGlyph(TCHAR Char);

    // 런타임 전용(테스트 안 함): CreateTransient(PF_B8G8R8A8) → 밉 0 에 픽셀 복사 → Filter = TF_Nearest, SRGB = true → UpdateResource.
    // 배경 = Tint × 0.35(알파 1), 글자 = Text 토큰, 테두리 = Tint
    HELLDIVERS_API UTexture2D*      CreateTexture(UObject* Outer, const FString& Text, int32 Width, int32 Height, const FLinearColor& Tint);
}
