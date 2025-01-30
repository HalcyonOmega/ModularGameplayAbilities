// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "Abilities/MGAAbilitySet.h"
#include "Abilities/GameplayAbility.h"
#include "Runtime/Launch/Resources/Version.h"
#include "MGAGameFeatureAction_AddAbilities.generated.h"

struct FComponentRequestHandle;
struct FMGAGameFeatureAbilityMapping;
struct FMGAGameFeatureAttributeSetMapping;
struct FMGAGameFeatureEffectMapping;

/** Data holder for a list of Abilities / Attributes / Effects to grant to actors of the specified class from a Game Feature Action */
USTRUCT()
struct FMGAGameFeatureAbilitiesEntry
{
	GENERATED_BODY()

	/** The base actor class to add to */
	UPROPERTY(EditAnywhere, Category="Abilities")
	TSoftClassPtr<AActor> ActorClass;

	/** List of abilities to grant to actors of the specified class */
	UPROPERTY(EditAnywhere, Category="Abilities")
	TArray<FMGAGameFeatureAbilityMapping> GrantedAbilities;
	
	/** List of gameplay effects to grant to actors of the specified class */
	UPROPERTY(EditAnywhere, Category="Attributes")
	TArray<FMGAGameFeatureEffectMapping> GrantedEffects;
	
	/** List of attribute sets to grant to actors of the specified class */
	UPROPERTY(EditAnywhere, Category="Attributes")
	TArray<FMGAGameFeatureAttributeSetMapping> GrantedAttributes;
	
	/** List of Gameplay Ability Sets to actors of the specified class */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Abilities")
	TArray<TSoftObjectPtr<UMGAAbilitySet>> GrantedAbilitySets;

	/** Default constructor */
	FMGAGameFeatureAbilitiesEntry() = default;
};

/**
 * GameFeatureAction responsible for granting abilities (and attributes, effects or ability sets) to actors of a specified type.
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Add Abilities (GAS Companion)"))
class UMGAGameFeatureAction_AddAbilities : public UGameFeatureAction
{
	GENERATED_BODY()
	
public:
	
	struct FActorExtensions
	{
		TArray<FGameplayAbilitySpecHandle> Abilities;
		TArray<FActiveGameplayEffectHandle> EffectHandles;
		TArray<UAttributeSet*> Attributes;
		TArray<FDelegateHandle> InputBindingDelegateHandles;
		TArray<FMGAAbilitySetHandle> AbilitySetHandles;
	};

	//~ Begin UGameFeatureAction interface
	virtual void OnGameFeatureActivating() override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
#if WITH_EDITORONLY_DATA
	virtual void AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData) override;
#endif
	//~ End UGameFeatureAction interface

	//~ Begin UObject interface
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~ End UObject interface

	/** List of Ability to grant to actors of the specified class */
	UPROPERTY(EditAnywhere, Category="Abilities", meta=(TitleProperty="ActorClass", ShowOnlyInnerProperties))
	TArray<FMGAGameFeatureAbilitiesEntry> AbilitiesList;

	void Reset();
	void HandleActorExtension(AActor* Actor, FName EventName, int32 EntryIndex);

	void AddActorAbilities(AActor* Actor, const FMGAGameFeatureAbilitiesEntry& AbilitiesEntry);
	void RemoveActorAbilities(const AActor* Actor);

private:

	FDelegateHandle GameInstanceStartHandle;

	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObjectsInContainer
	TMap<AActor*, FActorExtensions> ActiveExtensions;

	TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;

	virtual void AddToWorld(const FWorldContext& WorldContext);
	void HandleGameInstanceStart(UGameInstance* GameInstance);
};
