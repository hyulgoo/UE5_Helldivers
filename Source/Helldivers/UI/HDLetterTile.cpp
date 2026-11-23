// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HDLetterTile.h"
#include "UI/HDShipUIStyle.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

namespace HDLetterTile
{
    namespace
    {
        constexpr int32     GlyphWidth      = 5;
        constexpr int32     GlyphHeight     = 7;
        constexpr int32     GlyphSpacing    = 1;
        constexpr float     BackgroundShade = 0.35f;

        struct FGlyph
        {
            TCHAR           Char;
            const char*     Rows[GlyphHeight];
        };

        const FGlyph Glyphs[] = {
            { 'A', { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
            { 'B', { "####.", "#...#", "#...#", "####.", "#...#", "#...#", "####." } },
            { 'C', { ".###.", "#...#", "#....", "#....", "#....", "#...#", ".###." } },
            { 'D', { "####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####." } },
            { 'E', { "#####", "#....", "#....", "####.", "#....", "#....", "#####" } },
            { 'F', { "#####", "#....", "#....", "####.", "#....", "#....", "#...." } },
            { 'G', { ".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####" } },
            { 'H', { "#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
            { 'I', { ".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###." } },
            { 'J', { "..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.." } },
            { 'K', { "#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#" } },
            { 'L', { "#....", "#....", "#....", "#....", "#....", "#....", "#####" } },
            { 'M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" } },
            { 'N', { "#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#" } },
            { 'O', { ".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
            { 'P', { "####.", "#...#", "#...#", "####.", "#....", "#....", "#...." } },
            { 'Q', { ".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#" } },
            { 'R', { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" } },
            { 'S', { ".####", "#....", "#....", ".###.", "....#", "....#", "####." } },
            { 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
            { 'U', { "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
            { 'V', { "#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.." } },
            { 'W', { "#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#." } },
            { 'X', { "#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#" } },
            { 'Y', { "#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.." } },
            { 'Z', { "#####", "....#", "...#.", "..#..", ".#...", "#....", "#####" } },
            { '0', { ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." } },
            { '1', { "..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###." } },
            { '2', { ".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####" } },
            { '3', { "#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###." } },
            { '4', { "...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#." } },
            { '5', { "#####", "#....", "####.", "....#", "....#", "#...#", ".###." } },
            { '6', { "..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###." } },
            { '7', { "#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..." } },
            { '8', { ".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###." } },
            { '9', { ".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.." } },
            { '-', { ".....", ".....", ".....", "#####", ".....", ".....", "....." } },
        };

        const FGlyph* FindGlyph(TCHAR Char)
        {
            const TCHAR Upper = FChar::ToUpper(Char);
            for (const FGlyph& Glyph : Glyphs)
            {
                if (Glyph.Char == Upper)
                {
                    return &Glyph;
                }
            }
            return nullptr;
        }
    }

    bool HasGlyph(TCHAR Char)
    {
        return FindGlyph(Char) != nullptr;
    }

    TArray<FColor> Rasterize(const FString& Text, int32 Width, int32 Height, FColor Background, FColor Foreground, FColor Border)
    {
        TArray<FColor> Pixels;
        if (Width <= 0 || Height <= 0)
        {
            return Pixels;
        }

        Pixels.Init(Background, Width * Height);

        // 바깥 1px 테두리
        for (int32 X = 0; X < Width; ++X)
        {
            Pixels[X] = Border;
            Pixels[(Height - 1) * Width + X] = Border;
        }
        for (int32 Y = 0; Y < Height; ++Y)
        {
            Pixels[Y * Width] = Border;
            Pixels[Y * Width + Width - 1] = Border;
        }

        const int32 CharCount = Text.Len();
        if (CharCount == 0)
        {
            return Pixels;
        }

        // 글자 칸 단위 크기(모르는 글자도 한 칸을 차지하고 비워 둔다)
        const int32 Margin = FMath::Max(2, FMath::Min(Width, Height) / 8);
        const int32 AvailableWidth = Width - Margin * 2;
        const int32 AvailableHeight = Height - Margin * 2;
        const int32 CellsWide = CharCount * GlyphWidth + (CharCount - 1) * GlyphSpacing;
        if (AvailableWidth <= 0 || AvailableHeight <= 0)
        {
            return Pixels;
        }

        const int32 Scale = FMath::Min(AvailableWidth / CellsWide, AvailableHeight / GlyphHeight);
        if (Scale < 1)
        {
            return Pixels;
        }

        const int32 OriginX = Margin + (AvailableWidth - CellsWide * Scale) / 2;
        const int32 OriginY = Margin + (AvailableHeight - GlyphHeight * Scale) / 2;

        for (int32 CharIndex = 0; CharIndex < CharCount; ++CharIndex)
        {
            const FGlyph* Glyph = FindGlyph(Text[CharIndex]);
            if (Glyph == nullptr)
            {
                continue;
            }

            const int32 GlyphX = OriginX + CharIndex * (GlyphWidth + GlyphSpacing) * Scale;
            for (int32 Row = 0; Row < GlyphHeight; ++Row)
            {
                for (int32 Column = 0; Column < GlyphWidth; ++Column)
                {
                    if (Glyph->Rows[Row][Column] != '#')
                    {
                        continue;
                    }

                    for (int32 DY = 0; DY < Scale; ++DY)
                    {
                        for (int32 DX = 0; DX < Scale; ++DX)
                        {
                            const int32 X = GlyphX + Column * Scale + DX;
                            const int32 Y = OriginY + Row * Scale + DY;
                            Pixels[Y * Width + X] = Foreground;
                        }
                    }
                }
            }
        }

        return Pixels;
    }

    UTexture2D* CreateTexture(UObject* Outer, const FString& Text, int32 Width, int32 Height, const FLinearColor& Tint)
    {
        if (Width <= 0 || Height <= 0)
        {
            return nullptr;
        }

        const FLinearColor BackgroundColor(Tint.R * BackgroundShade, Tint.G * BackgroundShade, Tint.B * BackgroundShade, 1.f);
        const TArray<FColor> Pixels = Rasterize(Text, Width, Height, BackgroundColor.ToFColor(true), HDShipUIStyle::Text.ToFColor(true), Tint.ToFColor(true));

        UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
        if (Texture == nullptr || Texture->GetPlatformData() == nullptr || Texture->GetPlatformData()->Mips.Num() == 0)
        {
            return nullptr;
        }

        if (Outer)
        {
            Texture->Rename(nullptr, Outer, REN_DontCreateRedirectors | REN_NonTransactional);
        }

        FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
        void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
        FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
        Mip.BulkData.Unlock();

        Texture->Filter = TF_Nearest;
        Texture->SRGB = true;
        Texture->UpdateResource();
        return Texture;
    }
}
