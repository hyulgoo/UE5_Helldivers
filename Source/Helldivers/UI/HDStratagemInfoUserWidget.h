// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HDStratagemInfoUserWidget.generated.h"

class UImage;
class UTextBlock;
class UHorizontalBox;
class UHDCommandIcon;
enum class EHDCommandInput : uint8;

UCLASS()
class HELLDIVERS_API UHDStratagemInfoUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// IconTexture 가 null 이면 아이콘 자리를 비우고 이름만 보인다
	void								InitializeStratagemInfoWidget(UTexture2D* IconTexture, const TArray<EHDCommandInput>& CommandList, const TMap<EHDCommandInput, UTexture2D*> CommandIconList, const FName InStratagemName);
	void								ActiveCommandIconByNum(const int32 InputNum);
	void								SetAllWidgetCapacity(const float Opacity);
	FName								GetStratagemName() const;
	// 이름 글자를 "이름 + Status" 로 바꾼다. 마지막으로 쓴 Status 와 같으면 다시 쓰지 않는다
	void								SetStatusText(const FString& Status);

protected:
    UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage>					Img_StratagemIcon;

    UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock>				TB_StratagemName;
	
    UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox>			HB_StratagemCommand;
	
    UPROPERTY(EditAnywhere)
	TSubclassOf<UHDCommandIcon>			CommandIconWidgetClass;
	
    UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UHDCommandIcon>>	CommandIconWidgetList;

	FName								StratagemName;
	FString								LastStatus;
};
