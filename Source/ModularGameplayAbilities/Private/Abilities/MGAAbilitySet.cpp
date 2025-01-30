// Copyright 2021-2022 Mickael Daniel. All Rights Reserved.

#include "Abilities/MGAAbilitySet.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ModularGameplayAbilitiesLogChannels.h"
#include "ActorComponent/ModularAbilityExtensionComponent.h"
#include "ActorComponent/ModularAbilitySystemComponent.h"
#include "Utilities/MGAUtilities.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "MGAAbilitySet"

bool UMGAAbilitySet::GrantToAbilitySystem(UAbilitySystemComponent* InASC, FMGAAbilitySetHandle& OutAbilitySetHandle, FText* OutErrorText, const bool bShouldRegisterCoreDelegates) const
{
	if (!IsValid(InASC))
	{
		const FText ErrorMessage = LOCTEXT("Invalid_ASC", "ASC is nullptr or invalid (pending kill)");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		UE_LOG(LogModularGameplayAbilities, Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}
	
	UModularAbilitySystemComponent* ASC = Cast<UModularAbilitySystemComponent>(InASC);
	if (!ASC)
	{
		const FText ErrorMessage = LOCTEXT("Invalid_ASC_Type", "%s is not a UModularAbilitySystemComponent");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		UE_LOG(LogModularGameplayAbilities, Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}
	
	const bool bSuccess = FMGAUtilities::TryGrantAbilitySet(ASC, this, OutAbilitySetHandle);

	// Make sure to re-register delegates, this set may have added
	if (bSuccess && bShouldRegisterCoreDelegates)
	{
		TryRegisterCoreComponentDelegates(InASC);
	}
	
	return bSuccess;
}

bool UMGAAbilitySet::GrantToAbilitySystem(const AActor* InActor, FMGAAbilitySetHandle& OutAbilitySetHandle, FText* OutErrorText) const
{
	if (!IsValid(InActor))
	{
		const FText ErrorMessage = LOCTEXT("Invalid_Actor", "Passed in actor is nullptr or invalid (pending kill)");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		UE_LOG(LogModularGameplayAbilities, Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	if (!ASC)
	{
		const FText ErrorMessage = FText::Format(LOCTEXT("Invalid_Actor_ASC", "Unable to get valid ASC from actor {0}"), FText::FromString(GetNameSafe(InActor)));
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		UE_LOG(LogModularGameplayAbilities, Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}

	return GrantToAbilitySystem(ASC, OutAbilitySetHandle, OutErrorText);
}

bool UMGAAbilitySet::RemoveFromAbilitySystem(UAbilitySystemComponent* InASC, FMGAAbilitySetHandle& InAbilitySetHandle, FText* OutErrorText, const bool bShouldRegisterCoreDelegates)
{
	if (!IsValid(InASC))
	{
		const FText ErrorMessage = LOCTEXT("Invalid_ASC", "ASC is nullptr or invalid (pending kill)");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		UE_LOG(LogModularGameplayAbilities, Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}
	
	// Make sure to notify we may have added attributes, this will shutdown all previously bound delegates to current attributes
	// Delegate registering will be called once again once we're done removing this set
	if (bShouldRegisterCoreDelegates)
	{
		TryUnregisterCoreComponentDelegates(InASC);
	}

	// Clear up abilities / bindings
	/* @TODO: Fix or cleanup
	UMGAAbilityInputBindingComponent* InputComponent = nullptr;
	if (InASC->AbilityActorInfo.IsValid())
	{
		if (const AActor* AvatarActor = InASC->GetAvatarActor())
		{
			InputComponent = AvatarActor->FindComponentByClass<UMGAAbilityInputBindingComponent>();
		}
	}
	*/
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : InAbilitySetHandle.Abilities)
	{
		if (!AbilitySpecHandle.IsValid())
		{
			continue;
		}

		// Failsafe cleanup of the input binding - even without clearing the input, this shouldn't cause issue as new binding
		// will always get a new InputID
		/* @TODO: Fix or cleanup
		if (InputComponent)
		{
			InputComponent->ClearInputBinding(AbilitySpecHandle);
		}
		*/
		// Only Clear abilities on authority, on end if currently active, or right away
		if (InASC->IsOwnerActorAuthoritative())
		{
			InASC->SetRemoveAbilityOnEnd(AbilitySpecHandle);
		}
	}

	// Remove Effects
	for (const FActiveGameplayEffectHandle& EffectHandle : InAbilitySetHandle.EffectHandles)
	{
		if (EffectHandle.IsValid())
		{
			InASC->RemoveActiveGameplayEffect(EffectHandle);
		}
	}
	
	// Remove Attributes
	for (UAttributeSet* AttributeSet : InAbilitySetHandle.Attributes)
	{
		InASC->RemoveSpawnedAttribute(AttributeSet);
	}

	// Remove Owned Gameplay Tags
	if (InAbilitySetHandle.OwnedTags.IsValid())
	{
		// Remove tags (on server, replicated to all other clients - on owning client, for itself)
		FMGAUtilities::RemoveLooseGameplayTagsUnique(InASC, InAbilitySetHandle.OwnedTags);
	}

	// Clear any delegate handled bound previously for this actor
	if (UModularAbilitySystemComponent* ASC = Cast<UModularAbilitySystemComponent>(InASC))
	{
		// Clear any delegate handled bound previously for this actor
		/* @TODO: Fix or cleanup
		for (FDelegateHandle InputBindingDelegateHandle : InAbilitySetHandle.InputBindingDelegateHandles)
		{
			ASC->OnGiveAbilityDelegate.Remove(InputBindingDelegateHandle);
			InputBindingDelegateHandle.Reset();
		}
		*/
	}

	// Make sure to re-register delegates, this set may have removed some but other sets may need delegate again
	if (bShouldRegisterCoreDelegates)
	{
		TryRegisterCoreComponentDelegates(InASC);
	}

	// Clears out the handle
	InAbilitySetHandle.Invalidate();
	return true;
}

bool UMGAAbilitySet::RemoveFromAbilitySystem(const AActor* InActor, FMGAAbilitySetHandle& InAbilitySetHandle, FText* OutErrorText)
{
	if (!IsValid(InActor))
	{
		const FText ErrorMessage = LOCTEXT("Invalid_Actor", "Passed in actor is nullptr or invalid (pending kill)");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		UE_LOG(LogModularGameplayAbilities, Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	if (!ASC)
	{
		const FText ErrorMessage = FText::Format(LOCTEXT("Invalid_Actor_ASC", "Unable to get valid ASC from actor {0}"), FText::FromString(GetNameSafe(InActor)));
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		UE_LOG(LogModularGameplayAbilities, Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}

	return RemoveFromAbilitySystem(ASC, InAbilitySetHandle, OutErrorText);
}

bool UMGAAbilitySet::HasInputBinding() const
{
	for (const FMGAGameFeatureAbilityMapping& GrantedAbility : GrantedAbilities)
	{
		// Needs binding whenever one of the granted abilities has a non nullptr Input Action
		if (GrantedAbility.InputTag.IsValid())
		{
			return true;
		}
	}
	
	return false;
}

void UMGAAbilitySet::TryRegisterCoreComponentDelegates(UAbilitySystemComponent* InASC)
{
	check(InASC);

	const AActor* AvatarActor = InASC->GetAvatarActor_Direct();
	if (!IsValid(AvatarActor))
	{
		return;
	}

	if (UModularAbilitySystemComponent* ASC = Cast<UModularAbilitySystemComponent>(InASC))
	{
		// Make sure to notify we may have added attributes (on server)
		ASC->RegisterDelegates();
	}
}

void UMGAAbilitySet::TryUnregisterCoreComponentDelegates(UAbilitySystemComponent* InASC)
{
	check(InASC);

	if (!InASC->AbilityActorInfo.IsValid())
	{
		return;
	}

	const AActor* AvatarActor = InASC->GetAvatarActor();
	if (!IsValid(AvatarActor))
	{
		return;
	}

	if (UModularAbilitySystemComponent* ASC = Cast<UModularAbilitySystemComponent>(InASC))
	{
		// Make sure to notify we may have removed attributes (on server)
		ASC->UnregisterDelegates();
	}
}

#undef LOCTEXT_NAMESPACE
