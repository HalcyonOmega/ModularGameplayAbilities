// Copyright Chronicler.

#include "DataAsset/IAbilityPawnDataInterface.h"

#include "ModularGameplayAbilities/Public/Abilities/ModularAbilityTagRelationshipMapping.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IAbilityPawnDataInterface)


TArray<UModularAbilitySet*> IAbilityPawnDataInterface::GetAbilitySet() const
{
	return {};
}

UModularAbilityTagRelationshipMapping* IAbilityPawnDataInterface::GetTagRelationshipMapping() const
{
	return nullptr;
}
