// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interface/HDCharacterRagdollInterface.h"
#include "Interface/HDDeadInterface.h"
#include "Character/CharacterTypes/HDCharacterStateTypes.h"
#include "Define/HDDefine.h"
#include "Define/HDGameplayTag.h"
#include "HDCharacterBase.generated.h"

class UHDAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
struct FHDCharacterAttributeData;

UCLASS()
class HELLDIVERS_API AHDCharacterBase : public ACharacter, public IAbilitySystemInterface, public IHDCharacterRagdollInterface, public IHDDeadInterface
{
    GENERATED_BODY()

public:
    AHDCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 몸이 실제로 있는 자리. 래그돌(메시 물리 시뮬레이션) 중에는 메시만 날아가고 캡슐은 제자리에 남으므로
    // pelvis 본 위치를, 그 밖에는 액터 위치를 돌려준다. 폭발 거리는 이 값으로 잰다
    FVector                                 GetBodyLocation() const;

protected:
    virtual UAbilitySystemComponent*        GetAbilitySystemComponent() const override final;
    void                                    SetAbilitySystemComponent(AActor* OwnerActor, UAbilitySystemComponent* ASC);
    void                                    ClearAbilitySystemComponent();

    template < class T >
    T*                                      GetAbilitySystemComponent() const { return Cast<T>(GetAbilitySystemComponent()); }

    virtual void                            SetDead() override;

    virtual void                            InitializeAttributeValues();

    virtual void				            SetRagdoll(const bool bRagdoll, const FVector& Impulse = FVector::ZeroVector) override;
    virtual const float			            GetRagdollPysicsLinearVelocity() const override;

private:
    FHDCharacterAttributeData*              GetAttributeStatDataByArmorType(const EHDArmorType NewArmorType);
    void                                    LoadDefaultMontage();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
    TObjectPtr<UAnimMontage>                DeadMontage;
    
    float                                   DeadEventDelayTime = 10.f;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayAbility>>	Abilities;

private:
	UPROPERTY()
	TObjectPtr<UHDAbilitySystemComponent>	AbilitySystemComponent;

    UPROPERTY(EditAnywhere)
    EHDArmorType                            ArmorType = EHDArmorType::Medium;

	UPROPERTY(EditAnywhere)
    TObjectPtr<UDataTable>					ArmorTypeStatusDataTable;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UGameplayEffect>            InitAttributeStatEffect;
};
