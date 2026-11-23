// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "UI/HDShipUIStyle.h"
#include "UI/HDShipUILayout.h"

class AHUD;
class UCanvas;
class UFont;
class UTexture2D;

// 글자 가로 맞춤. 세로는 늘 Pos.Y 가 글자 줄의 가운데다
enum class EHDShipTextAlign : uint8
{
    Left,
    Center,
    Right
};

// 함선 화면 캔버스 도구(UObject 아님). 한 프레임 동안 만들어 쓴다.
// 사각형·테두리·글자·키 상자·글자 타일·히트박스를 그리고, 그린 글자 상자를 겹침 검사용으로 기록한다.
// 모든 좌표는 캔버스 좌표(UCanvas::ClipX·ClipY 와 같은 단위)이고, 그릴 때 미끄러짐(Offset)을 더한다
class FHDShipUICanvas
{
public:
    // Alpha = 틀 투명도, ContentAlpha = 본문 투명도(SetContentLayer(true) 동안 곱한다). bHitBoxes 가 거짓이면 히트박스를 달지 않는다(닫는 중)
                        FHDShipUICanvas(AHUD* InHUD, UCanvas* InCanvas, UFont* InFont, const FHDShipUIFrame& InFrame, float InAlpha, float InContentAlpha, const FVector2D& InOffset, bool bInHitBoxes);

    const FHDShipUIFrame& GetFrame() const { return Frame; }
    float               GetScale() const { return Frame.Scale; }
    // 설계 길이 → 캔버스 길이
    float               Px(float DesignLength) const { return DesignLength * Frame.Scale; }

    // 참이면 본문 투명도를 곱한다
    void                SetContentLayer(bool bContent) { bContentLayer = bContent; }
    // 다음에 그리는 글자가 있어야 할 영역(겹침 검사의 잘림). 비우려면 FBox2D(ForceInit)
    void                SetRegion(const FBox2D& InRegion) { Region = InRegion; }

    // 화면 전체(미끄러짐 없음)
    void                FillScreen(const FLinearColor& Color);
    void                Fill(const FBox2D& Rect, const FLinearColor& Color);
    // 안쪽으로 두께 max(1, round(설계 두께 × 배율))
    void                Border(const FBox2D& Rect, float DesignThickness, const FLinearColor& Color);

    // 캔버스 단위 글자 크기(폭, 줄 높이)
    FVector2D           MeasureSize(const FString& Text, int32 FontPx, bool bBold) const;
    float               Measure(const FString& Text, int32 FontPx, bool bBold) const { return MeasureSize(Text, FontPx, bBold).X; }

    // FitText 로 MaxWidth 에 맞춰 그린다(0 이하면 맞추지 않음). Pos.X 는 Align 기준점, Pos.Y 는 줄 가운데. 그린 상자를 돌려주고 기록한다
    FBox2D              Text(const FString& InText, EHDShipFont Font, const FVector2D& Pos, EHDShipTextAlign Align, const FLinearColor& Color, float MaxWidth);
    // WrapText 결과를 Rect 위에서부터 줄마다 그린다(Rect 아래로 넘치는 줄은 그리지 않는다). 쓴 높이를 돌려준다
    float               Paragraph(const FString& InText, EHDShipFont Font, const FBox2D& Rect, const FLinearColor& Color, int32 MaxLines);
    // 테두리 상자 + KeyCap 글꼴. Pos.X 는 왼쪽, Pos.Y 는 가운데. 상자를 돌려준다
    FBox2D              KeyCap(const FString& Key, const FVector2D& Pos);
    // 채운 사각형 + 테두리 + 가운데 약칭(맞춤)
    void                LetterTile(const FString& Abbrev, const FBox2D& Rect, const FLinearColor& Tint, bool bDim);
    // 아이콘 표(HDShipIcons::Resolve)의 텍스처를 비율을 지켜 가운데 그린다. 텍스처가 없거나 못 읽으면 LetterTile.
    // Badge 가 있으면 오른쪽 아래 Small 글꼴. bDim 이면 투명도 × 0.35
    void                Icon(FName Id, const FBox2D& Rect, bool bDim);
    // 텍스처 경로 → 텍스처(못 읽으면 nullptr). HUD 가 캐시를 붙인다. 없으면 Icon 은 늘 글자 타일
    void                SetTextureLoader(TFunction<UTexture2D*(const FString&)> InLoader) { TextureLoader = MoveTemp(InLoader); }
    void                HitBox(const FBox2D& Rect, FName Name);

    const TArray<HDShipUILayout::FHDDrawnText>& GetDrawnTexts() const { return DrawnTexts; }
    void                ClearDrawnTexts() { DrawnTexts.Reset(); }
    // 기록한 글자 상자와 틀 영역을 얇은 선으로(hd.ShipUI.Debug 1)
    void                DrawDebug();

    // 마지막으로 그린 캔버스의 DPI 배율(로그용)
    float               GetDPIScale() const;
    bool                IsUsingFallbackFont() const { return Font == nullptr; }

private:
    FSlateFontInfo      MakeFontInfo(int32 FontPx, bool bBold) const;
    float               GetLayerAlpha() const { return bContentLayer ? Alpha * ContentAlpha : Alpha; }
    void                DrawTile(const FVector2D& Min, const FVector2D& Size, const FLinearColor& Color);

private:
    AHUD*               HUD             = nullptr;
    UCanvas*            Canvas          = nullptr;
    UFont*              Font            = nullptr;
    FHDShipUIFrame      Frame;
    float               Alpha           = 1.f;
    float               ContentAlpha    = 1.f;
    FVector2D           Offset          = FVector2D::ZeroVector;
    bool                bHitBoxes       = true;
    bool                bContentLayer   = false;
    FBox2D              Region          = FBox2D(ForceInit);
    TArray<HDShipUILayout::FHDDrawnText> DrawnTexts;
    TFunction<UTexture2D*(const FString&)> TextureLoader;
};
