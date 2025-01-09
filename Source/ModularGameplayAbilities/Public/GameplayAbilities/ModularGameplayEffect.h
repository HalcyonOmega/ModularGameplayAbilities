// Copyright Halcyonyx Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ModularGameplayEffect.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class MODULARGAMEPLAYABILITIES_API UModularGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()
};
// Evaluate below UPROPERTY specifiers for expandable, instanced GE cooldown specification
/* These Gameplay Effect Components define how this Gameplay Effect behaves when applied */
// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "GameplayEffect", meta = (DisplayName = "Components", TitleProperty = EditorFriendlyName, ShowOnlyInnerProperties, DisplayPriority = 0))
// TArray<TObjectPtr<UGameplayEffectComponent>> GEComponents;