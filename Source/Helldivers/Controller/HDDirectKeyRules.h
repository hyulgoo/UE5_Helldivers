// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "Weapon/HDWeaponSlots.h"

// 치트 HDSimulateKeys 가 한 프레임에 보내는 원시 키 이벤트 하나
struct FHDSimulatedKeyEvent
{
    FKey            Key;
    EInputEvent     Event = IE_Pressed;
};

// AHDPlayerController::InputKey 가 받는 원시 키 규칙. 월드와 무관한 순수 함수라 자동 테스트(Helldivers.Input.DirectKeys)로 고정한다
namespace HDDirectKey
{
    // 화살표 Up/Down/Left/Right 의 IE_Pressed 만 참. IE_Repeat·IE_Released·다른 키는 거짓
    HELLDIVERS_API bool     TryGetCommand(const FKey& Key, const EInputEvent Event, EHDCommandInput& OutCommand);

    // 숫자 키 One~Four 의 IE_Pressed 만 참(Primary·Secondary·Support·Grenade). 숫자 패드·IE_Repeat·IE_Released·다른 키는 거짓
    HELLDIVERS_API bool     TryGetWeaponSlot(const FKey& Key, const EInputEvent Event, EHDWeaponSlot& OutSlot);

    // 스팀 키 Q 의 IE_Pressed 만 참. IE_Repeat·IE_Released·다른 키는 거짓
    HELLDIVERS_API bool     IsStimKey(const FKey& Key, const EInputEvent Event);

    // 급강하 키 LeftAlt 의 IE_Pressed 만 참. IE_Repeat·IE_Released·RightAlt·다른 키는 거짓
    HELLDIVERS_API bool     IsDiveKey(const FKey& Key, const EInputEvent Event);

    // "LeftControl+ | Up+ Right+ | Up- Right-" → 프레임 배열.
    // '|' 로 프레임을 나누고 프레임 안은 공백으로 나눈 토큰이다. 빈 프레임은 한 틱 기다림이다.
    // 토큰은 <키 이름>+(누름) 또는 <키 이름>-(뗌). 잘못된 토큰·잘못된 키 이름·토큰이 하나도 없으면 OutError 를 채우고 거짓
    HELLDIVERS_API bool     ParseSimulatedKeys(const FString& Text, TArray<TArray<FHDSimulatedKeyEvent>>& OutFrames, FString& OutError);
}
