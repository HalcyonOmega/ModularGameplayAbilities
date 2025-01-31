// Copyright Epic Games, Inc. All Rights Reserved.

#include "DataAsset/ModularAbilityPawnData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularAbilityPawnData)

UModularAbilityPawnData::UModularAbilityPawnData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultCameraMode = nullptr;
}

TArray<UModularAbilitySet*> UModularAbilityPawnData::GetAbilitySet() const
{
	return AbilitySets;
}

UModularAbilityTagRelationshipMapping* UModularAbilityPawnData::GetTagRelationshipMapping() const
{
	return TagRelationshipMapping;
}

