#pragma once

#include "AbilitySystemInterface.h"
#include "GameplayTagStack.h"
#include "ModularPlayerState.h"
#include "ActorComponent/ModularAbilitySystemComponent.h"
#include "Player/ModularExperiencePlayerState.h"

#include "ModularAbilityPlayerState.generated.h"

UCLASS(Config = "Game")
class MODULARGAMEPLAYABILITIES_API AModularAbilityPlayerState : public AModularExperiencePlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AModularAbilityPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface
	
	static const FName NAME_ModularAbilityReady;
	
	UFUNCTION(BlueprintCallable, Category = "ModularAbility|PlayerState")
	UModularAbilitySystemComponent* GetModularAbilitySystemComponent() const { return ModularAbilitySystemComponent; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ModularAbilitySystemComponent; };
	
	void SetPawnData(const UModularPawnData* InPawnData) override;

private:
	// The ability system component sub-object used by player characters.
	UPROPERTY(VisibleAnywhere, Category = "ModularAbility|PlayerState")
	TObjectPtr<UModularAbilitySystemComponent> ModularAbilitySystemComponent;
};
