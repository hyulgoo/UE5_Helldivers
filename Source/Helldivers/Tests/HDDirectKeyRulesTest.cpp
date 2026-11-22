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

    // 숫자 키 → 무기 칸
    {
        struct FSlotCase
        {
            const TCHAR*        Message;
            FKey                Key;
            EHDWeaponSlot       Expected;
        };

        const FSlotCase SlotCases[] =
        {
            { TEXT("Slot: One is Primary"),     EKeys::One,     EHDWeaponSlot::Primary },
            { TEXT("Slot: Two is Secondary"),   EKeys::Two,     EHDWeaponSlot::Secondary },
            { TEXT("Slot: Three is Support"),   EKeys::Three,   EHDWeaponSlot::Support },
            { TEXT("Slot: Four is Grenade"),    EKeys::Four,    EHDWeaponSlot::Grenade },
        };

        for (const FSlotCase& SlotCase : SlotCases)
        {
            // 기대값과 다른 값에서 시작해 실제로 채웠는지 본다
            EHDWeaponSlot Slot = EHDWeaponSlot::Grenade == SlotCase.Expected ? EHDWeaponSlot::Primary : EHDWeaponSlot::Grenade;
            const bool bResult = HDDirectKey::TryGetWeaponSlot(SlotCase.Key, IE_Pressed, Slot);
            TestTrue(SlotCase.Message, bResult && Slot == SlotCase.Expected);
        }
    }

    {
        EHDWeaponSlot Slot = EHDWeaponSlot::Primary;
        TestFalse(TEXT("Slot: repeat ignored"), HDDirectKey::TryGetWeaponSlot(EKeys::One, IE_Repeat, Slot));
        TestFalse(TEXT("Slot: Five ignored"), HDDirectKey::TryGetWeaponSlot(EKeys::Five, IE_Pressed, Slot));
        TestFalse(TEXT("Slot: numpad ignored"), HDDirectKey::TryGetWeaponSlot(EKeys::NumPadOne, IE_Pressed, Slot));
    }

    // Q → 스팀
    {
        TestTrue(TEXT("Stim: Q pressed"), HDDirectKey::IsStimKey(EKeys::Q, IE_Pressed));
        TestFalse(TEXT("Stim: Q repeat ignored"), HDDirectKey::IsStimKey(EKeys::Q, IE_Repeat));
        TestFalse(TEXT("Stim: Q release ignored"), HDDirectKey::IsStimKey(EKeys::Q, IE_Released));
        TestFalse(TEXT("Stim: V ignored"), HDDirectKey::IsStimKey(EKeys::V, IE_Pressed));
    }

    // LeftAlt → 급강하
    {
        TestTrue(TEXT("Dive: LeftAlt pressed"), HDDirectKey::IsDiveKey(EKeys::LeftAlt, IE_Pressed));
        TestFalse(TEXT("Dive: LeftAlt repeat ignored"), HDDirectKey::IsDiveKey(EKeys::LeftAlt, IE_Repeat));
        TestFalse(TEXT("Dive: LeftAlt release ignored"), HDDirectKey::IsDiveKey(EKeys::LeftAlt, IE_Released));
        TestFalse(TEXT("Dive: RightAlt ignored"), HDDirectKey::IsDiveKey(EKeys::RightAlt, IE_Pressed));
    }

    // Enter·P·Escape·BackSpace → 미션 메뉴 키(작전 포기·결과 화면)
    {
        struct FMissionMenuCase
        {
            const TCHAR*        Message;
            FKey                Key;
            EHDMissionMenuKey   Expected;
        };

        const FMissionMenuCase MissionMenuCases[] =
        {
            { TEXT("MissionMenu: Enter is Confirm"),        EKeys::Enter,       EHDMissionMenuKey::Confirm },
            { TEXT("MissionMenu: P is ToggleAbandon"),      EKeys::P,           EHDMissionMenuKey::ToggleAbandon },
            { TEXT("MissionMenu: Escape is ToggleAbandon"), EKeys::Escape,      EHDMissionMenuKey::ToggleAbandon },
            { TEXT("MissionMenu: BackSpace is Cancel"),     EKeys::BackSpace,   EHDMissionMenuKey::Cancel },
        };

        for (const FMissionMenuCase& MissionMenuCase : MissionMenuCases)
        {
            // 기대값과 다른 값에서 시작해 실제로 채웠는지 본다
            EHDMissionMenuKey MenuKey = EHDMissionMenuKey::Confirm == MissionMenuCase.Expected ? EHDMissionMenuKey::Cancel : EHDMissionMenuKey::Confirm;
            const bool bResult = HDDirectKey::TryGetMissionMenuKey(MissionMenuCase.Key, IE_Pressed, MenuKey);
            TestTrue(MissionMenuCase.Message, bResult && MenuKey == MissionMenuCase.Expected);
        }
    }

    {
        EHDMissionMenuKey MenuKey = EHDMissionMenuKey::Confirm;
        TestFalse(TEXT("MissionMenu: P release ignored"), HDDirectKey::TryGetMissionMenuKey(EKeys::P, IE_Released, MenuKey));
        TestFalse(TEXT("MissionMenu: Up ignored"), HDDirectKey::TryGetMissionMenuKey(EKeys::Up, IE_Pressed, MenuKey));
        TestFalse(TEXT("MissionMenu: Q ignored"), HDDirectKey::TryGetMissionMenuKey(EKeys::Q, IE_Pressed, MenuKey));
        TestFalse(TEXT("MissionMenu: LeftAlt ignored"), HDDirectKey::TryGetMissionMenuKey(EKeys::LeftAlt, IE_Pressed, MenuKey));
    }

    // 미션 메뉴 키 넷은 다른 원시 키 규칙(명령·무기 칸·스팀·급강하)에 걸리지 않는다
    {
        const FKey MissionMenuKeys[] = { EKeys::Enter, EKeys::P, EKeys::Escape, EKeys::BackSpace };
        for (const FKey& Key : MissionMenuKeys)
        {
            const FString KeyName = Key.GetFName().ToString();
            EHDCommandInput Command = EHDCommandInput::Count;
            EHDWeaponSlot Slot = EHDWeaponSlot::Primary;
            TestFalse(*FString::Printf(TEXT("MissionMenu: %s is not a command"), *KeyName), HDDirectKey::TryGetCommand(Key, IE_Pressed, Command));
            TestFalse(*FString::Printf(TEXT("MissionMenu: %s is not a weapon slot"), *KeyName), HDDirectKey::TryGetWeaponSlot(Key, IE_Pressed, Slot));
            TestFalse(*FString::Printf(TEXT("MissionMenu: %s is not stim"), *KeyName), HDDirectKey::IsStimKey(Key, IE_Pressed));
            TestFalse(*FString::Printf(TEXT("MissionMenu: %s is not dive"), *KeyName), HDDirectKey::IsDiveKey(Key, IE_Pressed));
        }
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
