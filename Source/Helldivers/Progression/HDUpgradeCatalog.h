// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Progression/HDUpgradeTypes.h"

// 업그레이드 텍스트 카탈로그(Config/HDUpgrades.json, 언리얼 DataTable JSON 형식)를 읽은 결과.
// FHDStratagemCatalog 와 같은 모양이다. 월드와 무관한 순수 C++ 라 자동 테스트(Helldivers.Progression.Upgrades)로 고정한다
class HELLDIVERS_API FHDUpgradeCatalog
{
public:
    // JSON 글자에서 만든다. 문법 오류·구조체에 없는 열·틀린 열거값·중복 이름·이름 없는 행은 Problems 에 남고, 읽힌 행만 담긴다
    static FHDUpgradeCatalog            FromJsonString(const FString& JsonText);

    // <프로젝트>/Config/HDUpgrades.json
    static FString                      GetProjectFilePath();

    // 프로젝트 파일을 읽은 카탈로그. 파일 수정 시각이 바뀌면 다시 읽는다(에디터를 끄지 않고 다음 PIE 에 반영)
    static const FHDUpgradeCatalog&     GetProjectCatalog();

    int32                               Num() const { return Names.Num(); }
    // 파일의 행 순서
    const TArray<FName>&                GetNames() const { return Names; }
    const FHDUpgradeDefinition*         Find(const FName Name) const;
    const TArray<FString>&              GetProblems() const { return Problems; }

private:
    TArray<FName>                       Names;
    TArray<FHDUpgradeDefinition>        Definitions;
    TArray<FString>                     Problems;
};
