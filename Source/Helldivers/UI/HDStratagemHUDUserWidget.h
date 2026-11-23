// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HDStratagemHUDUserWidget.generated.h"

class UImage;
class UVerticalBox;
class UHDStratagemInfoUserWidget;
class UHDStratagemComponent;
struct FGameplayEventData;
enum class EHDCommandInput : uint8;

enum class EHDStratagemWidgetMoveState
{
	NONE,
	APPEAR,
	DISAPPEAR,
};

UCLASS()
class HELLDIVERS_API UHDStratagemHUDUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    void                                            InitializeStratagemHUD(UHDStratagemComponent* InStratagemComponent);
    // 다시 강하한 새 폰의 컴포넌트로 포인터만 바꾼다. 로드아웃은 플레이어스테이트에 있어 그대로이므로 목록은 다시 만들지 않는다(로드아웃을 바꾸면 RebuildStratagemList)
    void                                            SetStratagemComponent(UHDStratagemComponent* InStratagemComponent);
    // 컴포넌트 로드아웃의 칸 순서대로 목록을 다시 만든다. 자식 위젯은 이름 없이 만든다(고정 이름이면 다시 만들 때 같은 이름의 위젯과 부딪친다)
    void                                            RebuildStratagemList();
	void											SetHUDActiveByCurrentInputMatchList(const TArray<FName>& MatchStratagemList, const int32 CurrentInputNum);
	void											SetAllWidgetOpacity(const float Opacity);
	void											WidgetAppear(const bool bAppear);
    void                                            OnStratagemEventReceived(const FGameplayEventData* Payload);

protected:
	// 맨 앞에서 칸 i 의 남은 대기·횟수를 위젯 i 의 이름 옆 글자로 맞춘다
	virtual void									NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override final;

private:
	// StratagemIconList 에 없는 칸의 아이콘: 아이콘 표(HDShipIcons)의 텍스처, 없으면 약칭 글자 타일(GeneratedIcons 에 캐시)
	UTexture2D*										ResolveFallbackIcon(FName StratagemName, FString& OutSource);

protected:	
    UPROPERTY()
    TObjectPtr<UHDStratagemComponent>				StratagemComponent;

    UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox>						VB_StratagemList;
	
    UPROPERTY(EditAnywhere)
	TMap<EHDCommandInput, UTexture2D*>				CommandIconList;
	
    UPROPERTY(EditAnywhere)
	TMap<FName, UTexture2D*>						StratagemIconList;
	
    UPROPERTY(EditAnywhere)
	TMap<uint8, UTexture2D*>						RemainImageList;
	
    UPROPERTY(EditAnywhere)
	TSubclassOf<UHDStratagemInfoUserWidget>			StratagemInfoWidgetClass;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UHDStratagemInfoUserWidget>>	StratagemInfoWidgetList;

	// StratagemIconList 에 없는 스트라타젬의 아이콘(읽은 텍스처 또는 만든 글자 타일)
	UPROPERTY()
	TMap<FName, TObjectPtr<UTexture2D>>				GeneratedIcons;
	
	int8											MaxCommandCount;

	float											WidgetAppearDistance    = 0.f;
	EHDStratagemWidgetMoveState						MoveState               = EHDStratagemWidgetMoveState::NONE;

};
