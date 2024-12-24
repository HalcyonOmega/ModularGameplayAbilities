#pragma once

#include "AbilitySystemInterface.h"
#include "GameplayTagStack.h"
#include "ModularPlayerState.h"
#include "ActorComponent/ModularAbilitySystemComponent.h"

#include "ModularAbilityPlayerState.generated.h"

UCLASS(Config = "Game")
class MODULARGAMEPLAYABILITIES_API AModularAbilityPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AModularAbilityPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "ModularAbility|PlayerState")
	UModularAbilitySystemComponent* GetModularAbilitySystemComponent() const { return ModularAbilitySystemComponent; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ModularAbilitySystemComponent; };

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1).
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ModularAbility|Tags")
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1).
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ModularAbility|Tags")
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present).
	UFUNCTION(BlueprintCallable, Category = "ModularAbility|Tags")
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag.
	UFUNCTION(BlueprintCallable, Category = "ModularAbility|Tags")
	bool HasStatTag(FGameplayTag Tag) const;

private:
	// The ability system component sub-object used by player characters.
	UPROPERTY(VisibleAnywhere, Category = "ModularAbility|PlayerState")
	TObjectPtr<UModularAbilitySystemComponent> ModularAbilitySystemComponent;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;
};
