// Copyright Halcyonyx Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ModularAttributesHelpers.generated.h"

struct FMGAAttributeSetExecutionData;
struct FGameplayAttribute;


/** Blueprint library for blueprint attribute sets */
UCLASS()
class MODULARGAMEPLAYABILITIES_API UModularAttributesHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns an FString representation of a FMGAAttributeSetExecutionData structure for debugging purposes.
	 *
	 * The separator used to join the data structure values is a line break (new line).
	 *
	 * @param ExecutionData	The data to get the debug string from.
	 */
	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Attribute")
	static FString GetDebugStringFromExecutionData(const FMGAAttributeSetExecutionData& ExecutionData);

	/**
	 * Returns an FString representation of a FMGAAttributeSetExecutionData structure for debugging purposes.
	 *
	 * @param ExecutionData	The data to get the debug string from.
	 * @param Separator String separator to use when joining the structure values (Default: "\n" - new line)
	 */
	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Attribute")
	static FString GetDebugStringFromExecutionDataSeparator(const FMGAAttributeSetExecutionData& ExecutionData, const FString& Separator = TEXT(", "));

	/** Returns the Attribute name */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ModularGameplayAbilities|Attribute")
	static FString GetDebugStringFromAttribute(const FGameplayAttribute& Attribute);

	/** Simple equality operator for gameplay attributes and string (for K2 Switch Node) */
	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Attribute | PinOptions", meta = (BlueprintInternalUseOnly = "true"))
	static bool NotEqual_GameplayAttributeGameplayAttribute(FGameplayAttribute A, FString B);

	/** Returns the Attribute name as an FText */
	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Attribute")
	static FText GetAttributeDisplayNameText(const FGameplayAttribute& InAttribute);
};
