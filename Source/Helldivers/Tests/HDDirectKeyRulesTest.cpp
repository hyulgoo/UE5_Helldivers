// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Controller/HDDirectKeyRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHDDirectKeyRulesTest, "Helldivers.Input.DirectKeys", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHDDirectKeyRulesTest::RunTest(const FString& Parameters)
{
    // 화살표 → 명령
    {
        struct FArrowCase
        {
            const TCHAR*        Message;
            FKey                Key;
            EHDCommandInput     Expected;
        };

        const FArrowCase ArrowCases[] =
        {
            { TEXT("Command: Up pressed"),    EKeys::Up,    EHDCommandInput::Up },
            { TEXT("Command: Down pressed"),  EKeys::Down,  EHDCommandInput::Down },
            { TEXT("Command: Left pressed"),  EKeys::Left,  EHDCommandInput::Left },
            { TEXT("Command: Right pressed"), EKeys::Right, EHDCommandInput::Right },
        };

        for (const FArrowCase& ArrowCase : ArrowCases)
        {
            EHDCommandInput Command = EHDCommandInput::Count;
            const bool bResult = HDDirectKey::TryGetCommand(ArrowCase.Key, IE_Pressed, Command);
            TestTrue(ArrowCase.Message, bResult && Command == ArrowCase.Expected);
        }
    }

    {
        EHDCommandInput Command = EHDCommandInput::Count;
        TestFalse(TEXT("Command: repeat ignored"), HDDirectKey::TryGetCommand(EKeys::Up, IE_Repeat, Command));
        TestFalse(TEXT("Command: release ignored"), HDDirectKey::TryGetCommand(EKeys::Up, IE_Released, Command));
        TestFalse(TEXT("Command: other key ignored"), HDDirectKey::TryGetCommand(EKeys::W, IE_Pressed, Command));
    }

    // 치트 문자열
    {
        TArray<TArray<FHDSimulatedKeyEvent>> Frames;
        FString Error;
        const bool bParsed = HDDirectKey::ParseSimulatedKeys(TEXT("LeftControl+ | Up+ Right+ | Up- Right- | LeftControl-"), Frames, Error);
        TestTrue(TEXT("Parse: frames"), bParsed && Frames.Num() == 4);

        const bool bOrdered = Frames.Num() == 4 && Frames[1].Num() == 2
            && Frames[1][0].Key == EKeys::Up && Frames[1][0].Event == IE_Pressed
            && Frames[1][1].Key == EKeys::Right && Frames[1][1].Event == IE_Pressed;
        TestTrue(TEXT("Parse: order in frame"), bOrdered);
    }

    {
        TArray<TArray<FHDSimulatedKeyEvent>> Frames;
        FString Error;
        const bool bParsed = HDDirectKey::ParseSimulatedKeys(TEXT("Up+ | | Up-"), Frames, Error);
        TestTrue(TEXT("Parse: wait frame"), bParsed && Frames.Num() == 3 && Frames[1].IsEmpty()
            && Frames[2].Num() == 1 && Frames[2][0].Event == IE_Released);
    }

    {
        TArray<TArray<FHDSimulatedKeyEvent>> Frames;
        FString Error;
        const bool bParsed = HDDirectKey::ParseSimulatedKeys(TEXT("Up*"), Frames, Error);
        TestTrue(TEXT("Parse: bad suffix"), bParsed == false && Error.IsEmpty() == false);
    }

    {
        TArray<TArray<FHDSimulatedKeyEvent>> Frames;
        FString Error;
        TestFalse(TEXT("Parse: bad key"), HDDirectKey::ParseSimulatedKeys(TEXT("NoSuchKey+"), Frames, Error));
    }

    {
        TArray<TArray<FHDSimulatedKeyEvent>> Frames;
        FString Error;
        const bool bEmptyParsed = HDDirectKey::ParseSimulatedKeys(TEXT(""), Frames, Error);
        const bool bBlankParsed = HDDirectKey::ParseSimulatedKeys(TEXT(" | "), Frames, Error);
        TestTrue(TEXT("Parse: empty"), bEmptyParsed == false && bBlankParsed == false);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
