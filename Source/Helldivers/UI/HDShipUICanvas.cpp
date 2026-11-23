// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDShipUICanvas.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineFontServices.h"
#include "Fonts/FontMeasure.h"
#include "GameFramework/HUD.h"
#include "Engine/Texture2D.h"
#include "UI/HDShipIcons.h"
#include "Stratagem/HDStratagemCatalog.h"

namespace HDShipUICanvasPrivate
{
    // Slate 글꼴 크기는 96 DPI 기준 포인트라 px × 0.75
    constexpr float     SlateSizePerPx  = 0.75f;
    const FName         BoldTypeface(TEXT("Bold"));
    const FName         RegularTypeface(TEXT("Regular"));

    // 키 상자: 높이 32, 글자 양옆 여백 10(설계 단위)
    constexpr float     KeyCapHeight    = 32.f;
    constexpr float     KeyCapPadding   = 10.f;
    // 글자 타일 안쪽 여백
    constexpr float     TilePadding     = 3.f;
    // 타일 채움은 색의 이 비율 투명도
    constexpr float     TileFillOpacity = 0.22f;
    constexpr float     TileDimOpacity  = 0.45f;
    // 문단 줄 사이(줄 높이 배율)
    constexpr float     LineSpacing     = 1.15f;
    // 흐린 아이콘 투명도 배율
    constexpr float     IconDimOpacity  = 0.35f;
}

FHDShipUICanvas::FHDShipUICanvas(AHUD* InHUD, UCanvas* InCanvas, UFont* InFont, const FHDShipUIFrame& InFrame, float InAlpha, float InContentAlpha, const FVector2D& InOffset, bool bInHitBoxes)
    : HUD(InHUD)
    , Canvas(InCanvas)
    , Font(InFont)
    , Frame(InFrame)
    , Alpha(FMath::Clamp(InAlpha, 0.f, 1.f))
    , ContentAlpha(FMath::Clamp(InContentAlpha, 0.f, 1.f))
    , Offset(InOffset)
    , bHitBoxes(bInHitBoxes)
{
}

FSlateFontInfo FHDShipUICanvas::MakeFontInfo(int32 FontPx, bool bBold) const
{
    using namespace HDShipUICanvasPrivate;

    if (Font == nullptr)
    {
        // Roboto 를 못 읽었으면 엔진 중간 글꼴(크기 고정)
        UFont* Fallback = GEngine ? GEngine->GetMediumFont() : nullptr;
        return Fallback ? Fallback->GetLegacySlateFontInfo() : FSlateFontInfo();
    }

    return FSlateFontInfo(Font, static_cast<float>(FMath::RoundToInt(FontPx * SlateSizePerPx)), bBold ? BoldTypeface : RegularTypeface);
}

float FHDShipUICanvas::GetDPIScale() const
{
    return Canvas ? Canvas->GetDPIScale() : 1.f;
}

void FHDShipUICanvas::DrawTile(const FVector2D& Min, const FVector2D& Size, const FLinearColor& Color)
{
    if (Canvas == nullptr || Size.X <= 0.0 || Size.Y <= 0.0)
    {
        return;
    }

    FLinearColor DrawColor = Color;
    DrawColor.A *= GetLayerAlpha();
    if (DrawColor.A <= 0.f)
    {
        return;
    }

    FCanvasTileItem Tile(Min, Size, DrawColor);
    Tile.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Tile);
}

void FHDShipUICanvas::FillScreen(const FLinearColor& Color)
{
    DrawTile(Frame.View.Min, Frame.View.GetSize(), Color);
}

void FHDShipUICanvas::Fill(const FBox2D& Rect, const FLinearColor& Color)
{
    DrawTile(Rect.Min + Offset, Rect.GetSize(), Color);
}

void FHDShipUICanvas::Border(const FBox2D& Rect, float DesignThickness, const FLinearColor& Color)
{
    // 네 변을 안쪽으로 채운 띠로 그린다(FCanvasBoxItem 은 선을 모서리 가운데에 걸쳐 그려 바깥으로 삐져나온다)
    const FVector2D Min     = Rect.Min + Offset;
    const FVector2D Size    = Rect.GetSize();
    const float Thickness   = FMath::Min(static_cast<float>(FMath::Max(1, FMath::RoundToInt(Px(DesignThickness)))), static_cast<float>(FMath::Min(Size.X, Size.Y) * 0.5));

    DrawTile(Min, FVector2D(Size.X, Thickness), Color);
    DrawTile(FVector2D(Min.X, Min.Y + Size.Y - Thickness), FVector2D(Size.X, Thickness), Color);
    DrawTile(FVector2D(Min.X, Min.Y + Thickness), FVector2D(Thickness, Size.Y - Thickness * 2.f), Color);
    DrawTile(FVector2D(Min.X + Size.X - Thickness, Min.Y + Thickness), FVector2D(Thickness, Size.Y - Thickness * 2.f), Color);
}

FVector2D FHDShipUICanvas::MeasureSize(const FString& InText, int32 FontPx, bool bBold) const
{
    if (InText.IsEmpty() || FEngineFontServices::IsInitialized() == false)
    {
        return FVector2D::ZeroVector;
    }

    const TSharedPtr<FSlateFontMeasure> FontMeasure = FEngineFontServices::Get().GetFontMeasure();
    if (FontMeasure.IsValid() == false)
    {
        return FVector2D::ZeroVector;
    }

    // FCanvasTextItem 은 글리프를 캔버스 DPI 배율로 그리므로 같은 배율로 재고 캔버스 단위로 되돌린다
    const float DPIScale = FMath::Max(GetDPIScale(), KINDA_SMALL_NUMBER);
    const FVector2D Size = FontMeasure->Measure(InText, MakeFontInfo(FontPx, bBold), DPIScale);
    return Size / DPIScale;
}

FBox2D FHDShipUICanvas::Text(const FString& InText, EHDShipFont InFont, const FVector2D& Pos, EHDShipTextAlign Align, const FLinearColor& Color, float MaxWidth)
{
    const bool bBold = HDShipUIStyle::IsBold(InFont);
    const int32 FontPx = HDShipUIStyle::GetFontPx(InFont, Frame.Scale);

    HDShipUILayout::FHDFitResult Fit;
    Fit.Text    = InText;
    Fit.FontPx  = FontPx;
    if (MaxWidth > 0.f)
    {
        Fit = HDShipUILayout::FitText(InText, MaxWidth, FontPx, HDShipUIStyle::MinFontPx,
            [this, bBold](const FString& Candidate, int32 CandidatePx) { return Measure(Candidate, CandidatePx, bBold); });
    }

    if (Fit.Text.IsEmpty())
    {
        return FBox2D(ForceInit);
    }

    const FVector2D Size = MeasureSize(Fit.Text, Fit.FontPx, bBold);
    FVector2D Min(Pos.X, Pos.Y - Size.Y * 0.5);
    if (Align == EHDShipTextAlign::Center)
    {
        Min.X -= Size.X * 0.5;
    }
    else if (Align == EHDShipTextAlign::Right)
    {
        Min.X -= Size.X;
    }
    Min += Offset;

    FLinearColor DrawColor = Color;
    DrawColor.A *= GetLayerAlpha();
    if (Canvas && DrawColor.A > 0.f)
    {
        FCanvasTextItem TextItem(Min, FText::FromString(Fit.Text), MakeFontInfo(Fit.FontPx, bBold), DrawColor);
        Canvas->DrawItem(TextItem);
    }

    HDShipUILayout::FHDDrawnText& Drawn = DrawnTexts.AddDefaulted_GetRef();
    Drawn.Rect          = FBox2D(Min, Min + Size);
    Drawn.Region        = Region.bIsValid ? FBox2D(Region.Min + Offset, Region.Max + Offset) : Region;
    Drawn.Text          = Fit.Text;
    Drawn.bTruncated    = Fit.bTruncated;
    return Drawn.Rect;
}

float FHDShipUICanvas::Paragraph(const FString& InText, EHDShipFont InFont, const FBox2D& Rect, const FLinearColor& Color, int32 MaxLines)
{
    const bool bBold = HDShipUIStyle::IsBold(InFont);
    const int32 FontPx = HDShipUIStyle::GetFontPx(InFont, Frame.Scale);
    const float Width = static_cast<float>(Rect.GetSize().X);

    const TArray<FString> Lines = HDShipUILayout::WrapText(InText, Width, FontPx, MaxLines,
        [this, bBold](const FString& Candidate, int32 CandidatePx) { return Measure(Candidate, CandidatePx, bBold); });

    const float LineHeight = static_cast<float>(FMath::Max(MeasureSize(TEXT("Ag"), FontPx, bBold).Y, 1.0)) * HDShipUICanvasPrivate::LineSpacing;
    float Used = 0.f;
    for (const FString& Line : Lines)
    {
        if (Rect.Min.Y + Used + LineHeight > Rect.Max.Y + 0.5f)
        {
            break;
        }

        Text(Line, InFont, FVector2D(Rect.Min.X, Rect.Min.Y + Used + LineHeight * 0.5f), EHDShipTextAlign::Left, Color, Width);
        Used += LineHeight;
    }
    return Used;
}

FBox2D FHDShipUICanvas::KeyCap(const FString& Key, const FVector2D& Pos)
{
    using namespace HDShipUICanvasPrivate;

    const int32 FontPx  = HDShipUIStyle::GetFontPx(EHDShipFont::KeyCap, Frame.Scale);
    const float Width   = Measure(Key, FontPx, true) + Px(KeyCapPadding) * 2.f;
    const float Height  = Px(KeyCapHeight);
    const FBox2D Box(FVector2D(Pos.X, Pos.Y - Height * 0.5f), FVector2D(Pos.X + Width, Pos.Y + Height * 0.5f));

    Fill(Box, HDShipUIStyle::Card);
    Border(Box, HDShipUIStyle::BorderThin, HDShipUIStyle::Text);
    Text(Key, EHDShipFont::KeyCap, FVector2D(Box.GetCenter().X, Pos.Y), EHDShipTextAlign::Center, HDShipUIStyle::Text, 0.f);
    return Box;
}

void FHDShipUICanvas::LetterTile(const FString& Abbrev, const FBox2D& Rect, const FLinearColor& Tint, bool bDim)
{
    using namespace HDShipUICanvasPrivate;

    const FLinearColor EdgeColor = bDim ? HDShipUIStyle::TextDisabled : Tint;
    Fill(Rect, EdgeColor.CopyWithNewOpacity(TileFillOpacity));
    Border(Rect, HDShipUIStyle::BorderThin, bDim ? EdgeColor.CopyWithNewOpacity(TileDimOpacity) : EdgeColor);

    // 정사각형이 아니어도 가운데. 안쪽 여백만큼 좁혀 맞춘다
    const float MaxWidth = FMath::Max(1.f, static_cast<float>(Rect.GetSize().X) - Px(TilePadding) * 2.f);
    Text(Abbrev, EHDShipFont::KeyCap, Rect.GetCenter(), EHDShipTextAlign::Center, bDim ? HDShipUIStyle::TextDisabled : HDShipUIStyle::Text, MaxWidth);
}

void FHDShipUICanvas::Icon(FName Id, const FBox2D& Rect, bool bDim)
{
    using namespace HDShipUICanvasPrivate;

    const FHDIconRef Ref = HDShipIcons::Resolve(Id, &FHDStratagemCatalog::GetProjectCatalog());
    UTexture2D* Texture = (Ref.TexturePath.IsEmpty() == false && TextureLoader) ? TextureLoader(Ref.TexturePath) : nullptr;
    if (Texture == nullptr || Canvas == nullptr)
    {
        LetterTile(Ref.Abbrev, Rect, Ref.Tint, bDim);
        return;
    }

    // 비율을 지켜 Rect 안 가운데
    const FVector2D RectSize = Rect.GetSize();
    const float TextureWidth = static_cast<float>(FMath::Max(1, Texture->GetSizeX()));
    const float TextureHeight = static_cast<float>(FMath::Max(1, Texture->GetSizeY()));
    const float Fit = FMath::Min(static_cast<float>(RectSize.X) / TextureWidth, static_cast<float>(RectSize.Y) / TextureHeight);
    const FVector2D DrawSize(TextureWidth * Fit, TextureHeight * Fit);
    const FVector2D DrawMin = Rect.GetCenter() - DrawSize * 0.5f;

    FLinearColor DrawColor = FLinearColor::White;
    DrawColor.A = GetLayerAlpha() * (bDim ? IconDimOpacity : 1.f);
    if (DrawColor.A > 0.f && DrawSize.X > 0.0 && DrawSize.Y > 0.0)
    {
        FCanvasTileItem Tile(DrawMin + Offset, Texture->GetResource(), DrawSize, DrawColor);
        Tile.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Tile);
    }

    if (Ref.Badge.IsEmpty() == false)
    {
        const FVector2D BadgePos(Rect.Max.X - Px(TilePadding), Rect.Max.Y - Px(TilePadding) - Px(9.f));
        Text(Ref.Badge, EHDShipFont::Small, BadgePos, EHDShipTextAlign::Right, bDim ? HDShipUIStyle::TextDisabled : HDShipUIStyle::Text, 0.f);
    }
}

void FHDShipUICanvas::HitBox(const FBox2D& Rect, FName Name)
{
    if (bHitBoxes == false || HUD == nullptr || Name.IsNone())
    {
        return;
    }

    HUD->AddHitBox(Rect.Min + Offset, Rect.GetSize(), Name, true);
}

void FHDShipUICanvas::DrawDebug()
{
    // 틀 영역(Good)과 기록한 글자 상자(Warn)를 1px 선으로. 투명도는 1
    const float SavedAlpha = Alpha;
    const bool bSavedContent = bContentLayer;
    const FVector2D SavedOffset = Offset;
    Alpha = 1.f;
    bContentLayer = false;
    Offset = FVector2D::ZeroVector;

    for (const FBox2D& Area : { Frame.Safe, Frame.TopBar, Frame.Content, Frame.BottomBar, Frame.Main, Frame.Detail })
    {
        Border(Area, 0.f, HDShipUIStyle::Good);
    }

    for (const HDShipUILayout::FHDDrawnText& Drawn : DrawnTexts)
    {
        Border(Drawn.Rect, 0.f, HDShipUIStyle::Warn);
    }

    Alpha = SavedAlpha;
    bContentLayer = bSavedContent;
    Offset = SavedOffset;
}
