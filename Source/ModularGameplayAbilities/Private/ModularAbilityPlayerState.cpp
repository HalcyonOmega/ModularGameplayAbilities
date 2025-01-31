#include "ModularAbilityPlayerState.h"

#include "ActorComponent/ModularAbilitySystemComponent.h"
#include "GameplayTagStack.h"
#include "Components/GameFrameworkComponentManager.h"
#include "DataAsset/ModularAbilityPawnData.h"
#include "GameplayAbilities/ModularAbilitySet.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularAbilityPlayerState)

class UModularAbilityPawnData;
class UModularAbilitySet;
const FName AModularAbilityPlayerState::NAME_ModularAbilityReady("ModularAbilitiesReady");

AModularAbilityPlayerState::AModularAbilityPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ModularAbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UModularAbilitySystemComponent>(this, TEXT("ModularAbilitySystemComponent"));
	ModularAbilitySystemComponent->SetIsReplicated(true);
	ModularAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	// AbilitySystemComponent needs to be updated at a high frequency.
	SetNetUpdateFrequency(100.0f);
}

void AModularAbilityPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AModularAbilityPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	check(ModularAbilitySystemComponent);
	ModularAbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

	// @Game-Change delete CallOrRegister_OnExperienceLoaded section, logic moved to ACorePlayerState::RegisterToExperienceLoadedToSetPawnData()
}

void AModularAbilityPlayerState::SetPawnData(const UModularPawnData* InPawnData)
{
	Super::SetPawnData(InPawnData);
	/* @TODO: Temporarily Commented Out Below - Have Extension Component Handle Ability Set Granting? */
	/*if (const UModularAbilityPawnData* ModularPawnData = GetPawnData<UModularAbilityPawnData>())
	{
		
		for (const UModularAbilitySet* AbilitySet : ModularPawnData->AbilitySets)
		{
			if (AbilitySet)
			{
				AbilitySet->GiveToAbilitySystem(ModularAbilitySystemComponent, nullptr);
			}
		}

		UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_ModularAbilityReady);

		ForceNetUpdate();
	}*/
}
