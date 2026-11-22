// Fill out your copyright notice in the Description page of Project Settings.

#include "Progression/HDProfileSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/HDProfileSaveGame.h"
#include "Progression/HDProgressionRules.h"
#include "Progression/HDProgressionSettings.h"

UHDProfileSubsystem* UHDProfileSubsystem::Get(const UObject* WorldContextObject)
{
    if (WorldContextObject == nullptr)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
    return GameInstance ? GameInstance->GetSubsystem<UHDProfileSubsystem>() : nullptr;
}

void UHDProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const FString SlotName = GetSlotName();
    const bool bExists = UGameplayStatics::DoesSaveGameExist(SlotName, 0);

    Profile = MakeNewProfile();
    if (bExists)
    {
        const UHDProfileSaveGame* SaveGame = Cast<UHDProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
        if (SaveGame)
        {
            Profile = SaveGame->Data;
            HDProgression::Sanitize(Profile);
        }
        else
        {
            // 파일은 지우지 않는다. 처음 바뀔 때 새 프로필로 덮어쓴다
            UE_LOG(LogTemp, Warning, TEXT("[HDProfile] load failed slot=%s"), *SlotName);
        }
    }

    UE_LOG(LogTemp, Display, TEXT("[HDProfile] loaded slot=%s exists=%d %s"), *SlotName, bExists ? 1 : 0, *HDProgression::Describe(Profile));
}

FString UHDProfileSubsystem::GetSlotName() const
{
    const UHDProgressionSettings* Settings = GetDefault<UHDProgressionSettings>();
    FString SlotName = Settings->SaveSlotName;
    if (Settings->bSeparateEditorSlot && GIsEditor)
    {
        SlotName += TEXT("_PIE");
    }
    return SlotName;
}

bool UHDProfileSubsystem::HasSaveFile() const
{
    return UGameplayStatics::DoesSaveGameExist(GetSlotName(), 0);
}

bool UHDProfileSubsystem::ModifyProfile(TFunctionRef<void(FHDProfileData&)> Change)
{
    Change(Profile);
    HDProgression::Sanitize(Profile);
    return SaveProfile();
}

void UHDProfileSubsystem::ResetProfile()
{
    const FString SlotName = GetSlotName();
    UGameplayStatics::DeleteGameInSlot(SlotName, 0);
    Profile = MakeNewProfile();

    UE_LOG(LogTemp, Display, TEXT("[HDProfile] reset slot=%s"), *SlotName);
}

bool UHDProfileSubsystem::HasShownTitle() const
{
    return bTitleShown;
}

void UHDProfileSubsystem::MarkTitleShown()
{
    bTitleShown = true;
}

FHDProfileData UHDProfileSubsystem::MakeNewProfile() const
{
    FHDProfileData NewProfile;
    NewProfile.Requisition = GetDefault<UHDProgressionSettings>()->StartingRequisition;
    HDProgression::Sanitize(NewProfile);
    return NewProfile;
}

bool UHDProfileSubsystem::SaveProfile()
{
    const FString SlotName = GetSlotName();

    UHDProfileSaveGame* SaveGame = NewObject<UHDProfileSaveGame>(this);
    SaveGame->Data = Profile;
    if (UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0) == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HDProfile] save failed slot=%s"), *SlotName);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("[HDProfile] saved slot=%s %s"), *SlotName, *HDProgression::Describe(Profile));
    return true;
}
