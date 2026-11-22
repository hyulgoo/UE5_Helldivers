// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Progression/HDProfileTypes.h"
#include "Progression/HDMissionReward.h"
#include "HDProfileSubsystem.generated.h"

// 게임 인스턴스와 함께 산다(맵을 다시 열어도 남는다). 프로필을 읽고, 바꿀 때마다 저장한다. 규칙은 HDProgression, 슬롯은 UHDProgressionSettings
UCLASS()
class HELLDIVERS_API UHDProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    // 월드 문맥에서 찾는다. 게임 인스턴스가 없으면(자동 테스트) null
    static UHDProfileSubsystem*     Get(const UObject* WorldContextObject);

    virtual void                    Initialize(FSubsystemCollectionBase& Collection) override;

    const FHDProfileData&           GetProfile() const { return Profile; }
    FString                         GetSlotName() const;
    bool                            HasSaveFile() const;

    // Change → Sanitize → 저장. 저장에 성공하면 참
    bool                            ModifyProfile(TFunctionRef<void(FHDProfileData&)> Change);

    // 파일을 지우고 새 프로필로(치트). 저장하지 않는다
    void                            ResetProfile();

    // 타이틀을 이미 보였는가. 저장하지 않고 게임 인스턴스가 살아 있는 동안만 기억한다(복귀 뒤에는 타이틀 없이 Bridge)
    bool                            HasShownTitle() const;
    void                            MarkTitleShown();

    // 마지막 미션 결과(함선 Bridge 의 LAST MISSION 줄). 저장하지 않고 게임 인스턴스가 살아 있는 동안만 기억한다
    void                            SetLastResult(const FHDMissionResultSummary& Summary);
    // 없으면 null
    const FHDMissionResultSummary*  GetLastResult() const;

private:
    // StartingRequisition 반영
    FHDProfileData                  MakeNewProfile() const;
    bool                            SaveProfile();

private:
    FHDProfileData                  Profile;

    bool                            bTitleShown = false;

    // 저장하지 않는다(SetLastResult)
    TOptional<FHDMissionResultSummary> LastResult;
};
