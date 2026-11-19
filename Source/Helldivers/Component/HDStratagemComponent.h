// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HDStratagemComponent.generated.h"

class AHDStratagem;
class UInputAction;
class AHDGASPlayerState;
class FHDStratagemLoadout;
class UAbilitySystemComponent;
struct FHDStratagemEntry;
enum class EHDCommandInput : uint8;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HELLDIVERS_API UHDStratagemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHDStratagemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UDataTable*					GetAvaliableStratagemDataTable() const;
	const TArray<FName>&	    GetCommandMatchStratagemNameList() const;
	const int32				    GetCurrentInputNum() const;
	void						AddStratagemCommand(const EHDCommandInput NewInput);
    UInputAction*               GetCommandInputAction() const;    

	const bool					IsSelectedStratagemExist() const;
	void						HoldStratagem(USkeletalMeshComponent* MeshComponent, const FVector& ThrowDirection);
	void						ThrowFinished();
	void						CancelStratagem();
	void						ClearCommand();

public:
    // 빙의한 플레이어스테이트의 로드아웃을 쓴다. 아직 비어 있으면 Project Settings(HD Stratagem)의 Loadout 과 텍스트 카탈로그로 채운다
    void                        BindLoadout(AHDGASPlayerState* InPlayerState);
    // 바인딩된 플레이어스테이트의 로드아웃을 LoadoutNames 로 다시 만든다. 칸 번호가 바뀌므로 모든 칸의 재사용 대기를 지우고 남은 횟수도 처음 값이 된다
    void                        ReinitializeLoadout(const TArray<FName>& LoadoutNames);
    const FHDStratagemLoadout*  GetLoadout() const;
    // 남은 횟수가 있고 재사용 대기 중이 아닌가
    bool                        IsSlotAvailable(const int32 SlotIndex) const;
    float                       GetSlotCooldownRemaining(const int32 SlotIndex) const;
    // 텍스트 카탈로그(Config/HDStratagems.json) 행 -> 로드아웃 항목. 파일 행 순서
    TArray<FHDStratagemEntry>   BuildCatalog() const;

private:
    FHDStratagemLoadout*        GetMutableLoadout() const;
    UAbilitySystemComponent*    GetOwnerAbilitySystemComponent() const;
    // 들고 있던 칸을 쓴다: 남은 횟수 1 감소 + 재사용 대기 GE
    void                        CommitHeldSlot();

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AHDStratagem>	StratagemClass;
    
	UPROPERTY()
	TObjectPtr<AHDStratagem>	Stratagem;
    
	UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UInputAction>	CommandInputAction;

    FName						SelectedStratagemName;
    float						SelecteddStratagemActiveDelay   = 0.f;

	TArray<EHDCommandInput>		CurrentInputCommandList;
	TArray<FName>				CommandMatchStratagemNameList;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDataTable>		AvaliableStratagemDataTable;

    TWeakObjectPtr<AHDGASPlayerState>   LoadoutOwner;
    int32                       SelectedSlotIndex   = INDEX_NONE;
    int32                       HeldSlotIndex       = INDEX_NONE;
};
