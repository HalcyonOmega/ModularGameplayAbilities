// Copyright 2021-2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "MGAGameFeatureTypes.generated.h"

class UInputAction;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct FMGAGameFeatureAbilityMapping
{
	GENERATED_BODY()

	/** Type of ability to grant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	TSoftClassPtr<UGameplayAbility> AbilityType;

	/** Level to grant the ability at */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	int32 Level = 1;

	// Tag used to process input for the ability.
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
	
	/* @TODO: Relook in future if functionality needed/desired. */
	/* Input action to bind the ability to, if any (can be left unset) */
	/*
	 *UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	 *TSoftObjectPtr<UInputAction> InputAction;
	*/

	/** The enhanced input action event to use for ability activation */
	/*
	 *UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability, meta=(EditCondition = "InputAction != nullptr", EditConditionHides))
	 *EMGAAbilityTriggerEvent TriggerEvent = EMGAAbilityTriggerEvent::Started;
	*/
	/** Default constructor */
	FMGAGameFeatureAbilityMapping() = default;
};

USTRUCT(BlueprintType)
struct FMGAGameFeatureEffectMapping
{
	GENERATED_BODY()

	/** Gameplay Effect to apply */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Effect)
	TSoftClassPtr<UGameplayEffect> EffectType;

	/** Level for the Gameplay Effect to apply */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Effect)
	float Level = 1.f;

	/** Default constructor */
	FMGAGameFeatureEffectMapping() = default;
};

USTRUCT(BlueprintType)
struct FMGAGameFeatureAttributeSetMapping
{
	GENERATED_BODY()

	/** Attribute Set to grant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Attribute)
	TSoftClassPtr<UAttributeSet> AttributeSet;

	/** Data table referent to initialize the attributes with, if any (can be left unset) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Attribute, meta = (RequiredAssetDataTags = "RowStructure=/Script/GameplayAbilities.AttributeMetaData"))
	TSoftObjectPtr<UDataTable> InitializationData;

	/** Default constructor */
	FMGAGameFeatureAttributeSetMapping() = default;
};
