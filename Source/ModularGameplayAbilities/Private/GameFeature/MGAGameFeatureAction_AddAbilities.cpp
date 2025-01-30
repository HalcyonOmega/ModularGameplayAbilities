// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GameFeature/MGAGameFeatureAction_AddAbilities.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFeaturesSubsystemSettings.h"
#include "ModularGameplayAbilitiesLogChannels.h"
#include "ActorComponent/ModularAbilitySystemComponent.h"
#include "Utilities/MGAUtilities.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/AssetManager.h"
#include "GameFeature/MGAGameFeatureTypes.h"
#include "Engine/Engine.h" // for FWorldContext
#include "Engine/GameInstance.h"
#include "Engine/World.h" // for FWorldDelegates::OnStartGameInstance
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "ModularGameplayAbilities"

void UMGAGameFeatureAction_AddAbilities::OnGameFeatureActivating()
{
	if (!ensureAlways(ActiveExtensions.Num() == 0) || !ensureAlways(ComponentRequests.Num() == 0))
	{
		Reset();
	}

	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UMGAGameFeatureAction_AddAbilities::HandleGameInstanceStart);

	check(ComponentRequests.Num() == 0);

	// Add to any worlds with associated game instances that have already been initialized
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		AddToWorld(WorldContext);
	}

	Super::OnGameFeatureActivating();
}

void UMGAGameFeatureAction_AddAbilities::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);

	Reset();
}

#if WITH_EDITORONLY_DATA
void UMGAGameFeatureAction_AddAbilities::AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData)
{
	if (!UAssetManager::IsInitialized())
	{
		return;
	}

	auto AddBundleAsset = [&AssetBundleData](const FSoftObjectPath& SoftObjectPath)
	{
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, SoftObjectPath.GetAssetPath());
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, SoftObjectPath.GetAssetPath());
	};

	for (const FMGAGameFeatureAbilitiesEntry& Entry : AbilitiesList)
	{
		for (const FMGAGameFeatureAbilityMapping& Ability : Entry.GrantedAbilities)
		{
			AddBundleAsset(Ability.AbilityType.ToSoftObjectPath());
			/* @TODO: Fix or cleanup
			if (!Ability.InputAction.IsNull())
			{
				AddBundleAsset(Ability.InputAction.ToSoftObjectPath());
			}*/
		}

		for (const FMGAGameFeatureAttributeSetMapping& Attributes : Entry.GrantedAttributes)
		{
			AddBundleAsset(Attributes.AttributeSet.ToSoftObjectPath());
			if (!Attributes.InitializationData.IsNull())
			{
				AddBundleAsset(Attributes.InitializationData.ToSoftObjectPath());
			}
		}

		for (const FMGAGameFeatureEffectMapping& Effect : Entry.GrantedEffects) 
		{
			AddBundleAsset(Effect.EffectType.ToSoftObjectPath());
		}
	}
}
#endif

#if WITH_EDITOR
EDataValidationResult UMGAGameFeatureAction_AddAbilities::IsDataValid(FDataValidationContext& Context) const

{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	int32 EntryIndex = 0;
	for (const FMGAGameFeatureAbilitiesEntry& Entry : AbilitiesList)
	{
		if (Entry.ActorClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("EntryHasNullActor", "Null ActorClass at index {0} in AbilitiesList"), FText::AsNumber(EntryIndex)));
		}

		if (Entry.GrantedAbilities.IsEmpty() && Entry.GrantedAttributes.IsEmpty() && Entry.GrantedEffects.IsEmpty())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(LOCTEXT("EntryHasNoAddOns", "Granted Abilities / Attributes / Effects are all empty. This action should grant at least one of these."));
		}

		int32 AbilityIndex = 0;
		for (const FMGAGameFeatureAbilityMapping& Ability : Entry.GrantedAbilities)
		{
			if (Ability.AbilityType.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullAbility", "Null AbilityType at index {0} in AbilitiesList[{1}].GrantedAbilities"), FText::AsNumber(AbilityIndex), FText::AsNumber(EntryIndex)));
			}
			++AbilityIndex;
		}

		int32 AttributesIndex = 0;
		for (const FMGAGameFeatureAttributeSetMapping& Attributes : Entry.GrantedAttributes)
		{
			if (Attributes.AttributeSet.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullAttributeSet", "Null AttributeSetType at index {0} in AbilitiesList[{1}].GrantedAttributes"), FText::AsNumber(AttributesIndex), FText::AsNumber(EntryIndex)));
			}
			++AttributesIndex;
		}

		int32 EffectsIndex = 0;
		for (const FMGAGameFeatureEffectMapping& Effect : Entry.GrantedEffects) 
		{
			if (Effect.EffectType.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullEffect", "Null GameplayEffectType at index {0} in AbilitiesList[{1}].GrantedEffects"), FText::AsNumber(EffectsIndex), FText::AsNumber(EntryIndex)));
			}
			++EffectsIndex;
		}

		++EntryIndex;
	}

	return Result;
}
#endif

void UMGAGameFeatureAction_AddAbilities::Reset()
{
	while (ActiveExtensions.Num() != 0)
	{
		const auto ExtensionIt = ActiveExtensions.CreateIterator();
		RemoveActorAbilities(ExtensionIt->Key);
	}

	ComponentRequests.Empty();
}

void UMGAGameFeatureAction_AddAbilities::HandleActorExtension(AActor* Actor, const FName EventName, const int32 EntryIndex)
{
	if (AbilitiesList.IsValidIndex(EntryIndex))
	{
		MGA_LOG(Verbose, TEXT("UMGAGameFeatureAction_AddAbilities::HandleActorExtension '%s'. EventName: %s"), *Actor->GetPathName(), *EventName.ToString());
		const FMGAGameFeatureAbilitiesEntry& Entry = AbilitiesList[EntryIndex];
		if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
		{
			MGA_LOG(Verbose, TEXT("UMGAGameFeatureAction_AddAbilities::HandleActorExtension remove '%s'. Abilities will be removed."), *Actor->GetPathName());
			RemoveActorAbilities(Actor);
		}
		else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
		{
			MGA_LOG(Verbose, TEXT("UMGAGameFeatureAction_AddAbilities::HandleActorExtension add '%s'. Abilities will be granted."), *Actor->GetPathName());
			AddActorAbilities(Actor, Entry);
		}
	}
}

void UMGAGameFeatureAction_AddAbilities::AddActorAbilities(AActor* Actor, const FMGAGameFeatureAbilitiesEntry& AbilitiesEntry)
{
	if (!IsValid(Actor))
	{
		MGA_LOG(Error, TEXT("Failed to find/add an ability component. Target Actor is not valid"));
		return;
	}

	// TODO: Remove coupling to UModularAbilitySystemComponent. Should work off just an UAbilitySystemComponent
	// Right now, required because of TryBindAbilityInput and necessity for OnGiveAbilityDelegate, but delegate could be reworked to be from an Interface

	// Go through IAbilitySystemInterface::GetAbilitySystemComponent() to handle target pawn using ASC on Player State
	UModularAbilitySystemComponent* ExistingASC = Cast<UModularAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor));
	// Not using the template version of FindOrAddComponentForActor() due a "use of function template name with no prior declaration in function call with explicit template arguments is a C++20 extension" only on linux 5.0 with strict includes,
	// ending up in this very long line
	UModularAbilitySystemComponent* AbilitySystemComponent = ExistingASC ? ExistingASC : Cast<UModularAbilitySystemComponent>(FMGAUtilities::FindOrAddComponentForActor(UModularAbilitySystemComponent::StaticClass(), Actor, ComponentRequests));
	
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogModularGameplayAbilities, Error, TEXT("Failed to find/add an ability component to '%s'. Abilities will not be granted."), *Actor->GetPathName());
		return;
	}

	AActor* OwnerActor = AbilitySystemComponent->GetOwnerActor();
	AActor* AvatarActor = AbilitySystemComponent->GetAvatarActor();
	
	UE_LOG(LogModularGameplayAbilities, Display, TEXT("Trying to add actor abilities from Game Feature action for Owner: %s, Avatar: %s, Original Actor: %s"), *GetNameSafe(OwnerActor), *GetNameSafe(AvatarActor), *GetNameSafe(Actor));

	// Handle cleaning up of previous attributes / abilities in case of respawns
	FActorExtensions* ActorExtensions = ActiveExtensions.Find(OwnerActor);
	if (ActorExtensions)
	{
		if (AbilitySystemComponent->bResetAttributesOnSpawn)
		{
			// ASC wants reset, remove attributes
			for (UAttributeSet* AttribSetInstance : ActorExtensions->Attributes)
			{
				AbilitySystemComponent->RemoveSpawnedAttribute(AttribSetInstance);
			}
		}

		if (AbilitySystemComponent->bResetAbilitiesOnSpawn)
		{
			// ASC wants reset, remove abilities
			//UGSCAbilityInputBindingComponent* InputComponent = AvatarActor ? AvatarActor->FindComponentByClass<UGSCAbilityInputBindingComponent>() : nullptr;
			for (const FGameplayAbilitySpecHandle& AbilityHandle : ActorExtensions->Abilities)
			{
				/* @TODO: Fix or cleanup
				if (InputComponent)
				{
					InputComponent->ClearInputBinding(AbilityHandle);
				}*/

				// Only Clear abilities on authority
				if (AbilitySystemComponent->IsOwnerActorAuthoritative())
				{
					AbilitySystemComponent->SetRemoveAbilityOnEnd(AbilityHandle);
				}
			}

			// Clear any delegate handled bound previously for this actor
			/* @TODO: Fix or cleanup
			for (FDelegateHandle DelegateHandle : ActorExtensions->InputBindingDelegateHandles)
			{
				AbilitySystemComponent->OnGiveAbilityDelegate.Remove(DelegateHandle);
				DelegateHandle.Reset();
			}*/
		}

		// Remove effects
		for (const FActiveGameplayEffectHandle& EffectHandle : ActorExtensions->EffectHandles)
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
		}

		ActiveExtensions.Remove(OwnerActor);
	}
	

	FActorExtensions AddedExtensions;
	AddedExtensions.Abilities.Reserve(AbilitiesEntry.GrantedAbilities.Num());
	AddedExtensions.Attributes.Reserve(AbilitiesEntry.GrantedAttributes.Num());
	AddedExtensions.EffectHandles.Reserve(AbilitiesEntry.GrantedEffects.Num());
	AddedExtensions.AbilitySetHandles.Reserve(AbilitiesEntry.GrantedAbilitySets.Num());

	for (const FMGAGameFeatureAbilityMapping& AbilityMapping : AbilitiesEntry.GrantedAbilities)
	{
		if (!AbilityMapping.AbilityType.IsNull())
		{
			// Try to grant the ability first
			FGameplayAbilitySpec AbilitySpec;
			FGameplayAbilitySpecHandle AbilityHandle;
			FMGAUtilities::TryGrantAbility(AbilitySystemComponent, AbilityMapping, AbilityHandle, AbilitySpec);

			// Handle Input Mapping now
			/* @TODO: Fix or cleanup
			if (!AbilityMapping.InputAction.IsNull())
			{
				FDelegateHandle DelegateHandle;
				FMGAUtilities::TryBindAbilityInput(AbilitySystemComponent, AbilityMapping, AbilityHandle, AbilitySpec, DelegateHandle, &ComponentRequests);
				AddedExtensions.InputBindingDelegateHandles.Add(MoveTemp(DelegateHandle));
			}
			*/
			AddedExtensions.Abilities.Add(AbilityHandle);
		}
	}

	for (const FMGAGameFeatureAttributeSetMapping& Attributes : AbilitiesEntry.GrantedAttributes)
	{
		if (!Attributes.AttributeSet.IsNull() && AbilitySystemComponent->IsOwnerActorAuthoritative())
		{
			UAttributeSet* AddedAttributeSet = nullptr;
			FMGAUtilities::TryGrantAttributes(AbilitySystemComponent, Attributes, AddedAttributeSet);

			if (AddedAttributeSet)
			{
				AddedExtensions.Attributes.Add(AddedAttributeSet);
			}
		}
	}

	for (const FMGAGameFeatureEffectMapping& Effect : AbilitiesEntry.GrantedEffects)
	{
		if (!Effect.EffectType.IsNull())
		{
			FMGAUtilities::TryGrantGameplayEffect(AbilitySystemComponent, Effect.EffectType.LoadSynchronous(), Effect.Level, AddedExtensions.EffectHandles);
		}
	}

	for (const TSoftObjectPtr<UMGAAbilitySet>& AbilitySetEntry : AbilitiesEntry.GrantedAbilitySets)
	{
		const UMGAAbilitySet* AbilitySet = AbilitySetEntry.LoadSynchronous();
		if (!AbilitySet)
		{
			continue;
		}

		FMGAAbilitySetHandle Handle;
		if (!FMGAUtilities::TryGrantAbilitySet(AbilitySystemComponent, AbilitySet, Handle, &ComponentRequests))
		{
			continue;
		}

		AddedExtensions.AbilitySetHandles.Add(Handle);
	}
	
	// Make sure to notify we may have added attributes
	AbilitySystemComponent->RegisterDelegates();

	ActiveExtensions.Add(OwnerActor, AddedExtensions);
}

void UMGAGameFeatureAction_AddAbilities::RemoveActorAbilities(const AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	if (FActorExtensions* ActorExtensions = ActiveExtensions.Find(Actor))
	{
		UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
		if (AbilitySystemComponent)
		{
			// Remove effects
			for (const FActiveGameplayEffectHandle& EffectHandle : ActorExtensions->EffectHandles)
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
			}
			
			// Remove attributes
			for (UAttributeSet* AttribSetInstance : ActorExtensions->Attributes)
			{
				AbilitySystemComponent->RemoveSpawnedAttribute(AttribSetInstance);
			}

			// Remove abilities
			/* @TODO: Fix or cleanup
			UGSCAbilityInputBindingComponent* InputComponent = Actor->FindComponentByClass<UGSCAbilityInputBindingComponent>();
			*/
			for (const FGameplayAbilitySpecHandle& AbilityHandle : ActorExtensions->Abilities)
			{
				/* @TODO: Fix or cleanup
				if (InputComponent)
				{
					InputComponent->ClearInputBinding(AbilityHandle);
				}
				*/
				// Only Clear abilities on authority
				if (AbilitySystemComponent->IsOwnerActorAuthoritative())
				{
					AbilitySystemComponent->SetRemoveAbilityOnEnd(AbilityHandle);
				}
			}

			// Remove sets
			for (FMGAAbilitySetHandle& Handle : ActorExtensions->AbilitySetHandles)
			{
				FText ErrorText;
				if (!UMGAAbilitySet::RemoveFromAbilitySystem(AbilitySystemComponent, Handle, &ErrorText, false))
				{
					UE_LOG(LogModularGameplayAbilities, Error, TEXT("Error trying to remove ability set %s - %s"), *Handle.AbilitySetPathName, *ErrorText.ToString());
				}
			}
		}
		else
		{
			UE_LOG(LogModularGameplayAbilities, Warning, TEXT("Not able to find AbilitySystemComponent for %s. This may happen for Player State ASC when game is shut downed."), *GetNameSafe(Actor))
		}

		// We need to clean up give ability delegates
		/* @TODO: Fix or cleanup
		if (UModularAbilitySystemComponent* ASC = Cast<UModularAbilitySystemComponent>(AbilitySystemComponent))
		{
			// Clear any delegate handled bound previously for this actor
			for (FDelegateHandle InputBindingDelegateHandle : ActorExtensions->InputBindingDelegateHandles)
			{
				ASC->OnGiveAbilityDelegate.Remove(InputBindingDelegateHandle);
				InputBindingDelegateHandle.Reset();
			}
		}
		*/
		ActiveExtensions.Remove(Actor);
	}
}

void UMGAGameFeatureAction_AddAbilities::AddToWorld(const FWorldContext& WorldContext)
{
	const UWorld* World = WorldContext.World();
	const UGameInstance* GameInstance = WorldContext.OwningGameInstance;

	if ((GameInstance != nullptr) && (World != nullptr) && World->IsGameWorld())
	{
		UGameFrameworkComponentManager* ComponentMan = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);

		if (ComponentMan)
		{
			int32 EntryIndex = 0;

			UE_LOG(LogModularGameplayAbilities, Verbose, TEXT("Adding abilities for %s to world %s"), *GetPathNameSafe(this), *World->GetDebugDisplayName());

			for (const FMGAGameFeatureAbilitiesEntry& Entry : AbilitiesList)
			{
				if (!Entry.ActorClass.IsNull())
				{
					const UGameFrameworkComponentManager::FExtensionHandlerDelegate AddAbilitiesDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
						this,
						&UMGAGameFeatureAction_AddAbilities::HandleActorExtension,
						EntryIndex
					);
					TSharedPtr<FComponentRequestHandle> ExtensionRequestHandle = ComponentMan->AddExtensionHandler(Entry.ActorClass, AddAbilitiesDelegate);

					ComponentRequests.Add(ExtensionRequestHandle);
					EntryIndex++;
				}
			}
		}
	}
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UMGAGameFeatureAction_AddAbilities::HandleGameInstanceStart(UGameInstance* GameInstance)
{
	if (const FWorldContext* WorldContext = GameInstance->GetWorldContext())
	{
		AddToWorld(*WorldContext);
	}
}

#undef LOCTEXT_NAMESPACE
