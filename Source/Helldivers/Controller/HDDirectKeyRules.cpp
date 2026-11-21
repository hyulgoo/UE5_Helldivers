// Fill out your copyright notice in the Description page of Project Settings.

#include "Controller/HDDirectKeyRules.h"

bool HDDirectKey::TryGetCommand(const FKey& Key, const EInputEvent Event, EHDCommandInput& OutCommand)
{
    if (Event != IE_Pressed)
    {
        return false;
    }

    if (Key == EKeys::Up)
    {
        OutCommand = EHDCommandInput::Up;
        return true;
    }

    if (Key == EKeys::Down)
    {
        OutCommand = EHDCommandInput::Down;
        return true;
    }

    if (Key == EKeys::Left)
    {
        OutCommand = EHDCommandInput::Left;
        return true;
    }

    if (Key == EKeys::Right)
    {
        OutCommand = EHDCommandInput::Right;
        return true;
    }

    return false;
}

bool HDDirectKey::TryGetWeaponSlot(const FKey& Key, const EInputEvent Event, EHDWeaponSlot& OutSlot)
{
    if (Event != IE_Pressed)
    {
        return false;
    }

    if (Key == EKeys::One)
    {
        OutSlot = EHDWeaponSlot::Primary;
        return true;
    }

    if (Key == EKeys::Two)
    {
        OutSlot = EHDWeaponSlot::Secondary;
        return true;
    }

    if (Key == EKeys::Three)
    {
        OutSlot = EHDWeaponSlot::Support;
        return true;
    }

    if (Key == EKeys::Four)
    {
        OutSlot = EHDWeaponSlot::Grenade;
        return true;
    }

    return false;
}

bool HDDirectKey::IsStimKey(const FKey& Key, const EInputEvent Event)
{
    return Event == IE_Pressed && Key == EKeys::Q;
}

bool HDDirectKey::ParseSimulatedKeys(const FString& Text, TArray<TArray<FHDSimulatedKeyEvent>>& OutFrames, FString& OutError)
{
    OutFrames.Reset();
    OutError.Reset();

    // 빈 프레임(| |)도 한 틱 기다림으로 남겨야 하므로 빈 칸을 지우지 않는다
    TArray<FString> FrameTexts;
    Text.ParseIntoArray(FrameTexts, TEXT("|"), false);

    int32 TokenCount = 0;
    for (const FString& FrameText : FrameTexts)
    {
        TArray<FString> Tokens;
        FrameText.ParseIntoArrayWS(Tokens);

        TArray<FHDSimulatedKeyEvent>& Frame = OutFrames.AddDefaulted_GetRef();
        for (const FString& Token : Tokens)
        {
            const TCHAR Suffix = Token[Token.Len() - 1];
            if (Suffix != TEXT('+') && Suffix != TEXT('-'))
            {
                OutError = FString::Printf(TEXT("token must end with + or -: '%s'"), *Token);
                OutFrames.Reset();
                return false;
            }

            const FString KeyName = Token.LeftChop(1);
            const FKey Key(*KeyName);
            if (KeyName.IsEmpty() || Key.IsValid() == false)
            {
                OutError = FString::Printf(TEXT("unknown key name: '%s'"), *Token);
                OutFrames.Reset();
                return false;
            }

            FHDSimulatedKeyEvent& KeyEvent = Frame.AddDefaulted_GetRef();
            KeyEvent.Key    = Key;
            KeyEvent.Event  = Suffix == TEXT('+') ? IE_Pressed : IE_Released;
            ++TokenCount;
        }
    }

    if (TokenCount == 0)
    {
        OutError = FString::Printf(TEXT("no key token: '%s'"), *Text);
        OutFrames.Reset();
        return false;
    }

    return true;
}
