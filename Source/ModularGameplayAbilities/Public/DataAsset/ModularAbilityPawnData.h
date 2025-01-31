// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DataAsset/IAbilityPawnDataInterface.h"
#include "DataAsset/ModularPawnData.h"

#include "ModularAbilityPawnData.generated.h"

class APawn;
class UModularAbilitySet;
class UModularAbilityTagRelationshipMapping;
class UModalCameraMode;
class UModularInputConfig;
class UObject;


/**
 * UModularAbilityPawnData
 *
 *	Non-mutable data asset that contains properties used to define a pawn.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "ModularAbilityPawnData", ShortTooltip = "Data asset used to define a Pawn with Abilities."))
class MODULARGAMEPLAYABILITIES_API UModularAbilityPawnData : public UModularPawnData, public IAbilityPawnDataInterface
{
	GENERATED_BODY()

public:

	UModularAbilityPawnData(const FObjectInitializer& ObjectInitializer);

public:
	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Core|Abilities")
	TArray<TObjectPtr<UModularAbilitySet>> AbilitySets;

	// What mapping of ability tags to use for actions taking by this pawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Core|Abilities")
	TObjectPtr<UModularAbilityTagRelationshipMapping> TagRelationshipMapping;

	// Default camera mode used by player controlled pawns.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Core|Camera")
	TSubclassOf<UModalCameraMode> DefaultCameraMode;

public:
	// Begin IAbilityPawnDataInterface
	virtual TArray<UModularAbilitySet*> GetAbilitySet() const override;
	virtual UModularAbilityTagRelationshipMapping* GetTagRelationshipMapping() const override;
	// End IAbilityPawnDataInterface
};
