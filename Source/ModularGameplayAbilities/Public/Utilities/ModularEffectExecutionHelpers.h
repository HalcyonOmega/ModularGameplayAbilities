﻿// Copyright Halcyonyx Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ModularEffectExecutionHelpers.generated.h"

struct FGameplayAttribute;
struct FGameplayEffectAttributeCaptureDefinition;
struct FGameplayEffectContextHandle;
struct FGameplayEffectCustomExecutionOutput;
struct FGameplayEffectCustomExecutionParameters;
struct FGameplayEffectSpec;
struct FGameplayTagContainer;

/**
 * Blueprint library for blueprint attribute sets.
 *
 * Includes Gameplay Effect Execution Calculation helpers for Blueprint implementation of Exec Calc classes.
 */
UCLASS()
class MODULARGAMEPLAYABILITIES_API UModularEffectExecutionHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Exec Calc")
	static const FGameplayEffectSpec& GetOwningSpec(const FGameplayEffectCustomExecutionParameters& InExecutionParams);

	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Exec Calc")
	static FGameplayEffectContextHandle GetEffectContext(const FGameplayEffectCustomExecutionParameters& InExecutionParams);
	
	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Exec Calc")
	static const FGameplayTagContainer& GetSourceTags(const FGameplayEffectCustomExecutionParameters& InExecutionParams);
	
	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Exec Calc")
	static const FGameplayTagContainer& GetTargetTags(const FGameplayEffectCustomExecutionParameters& InExecutionParams);
	
	UFUNCTION(BlueprintPure, Category = "ModularGameplayAbilities|Exec Calc")
	static bool AttemptCalculateCapturedAttributeMagnitude(UPARAM(ref) const FGameplayEffectCustomExecutionParameters& InExecutionParams, UPARAM(ref) const TArray<FGameplayEffectAttributeCaptureDefinition>& InRelevantAttributesToCapture, const FGameplayAttribute InAttribute, float& OutMagnitude);
	
	UFUNCTION(BlueprintCallable, Category = "ModularGameplayAbilities|Exec Calc")
	static bool AttemptCalculateCapturedAttributeMagnitudeWithBase(UPARAM(ref) const FGameplayEffectCustomExecutionParameters& InExecutionParams, UPARAM(ref) const TArray<FGameplayEffectAttributeCaptureDefinition>& InRelevantAttributesToCapture, const FGameplayAttribute InAttribute, const float InBaseValue, float& OutMagnitude);
	
	UFUNCTION(BlueprintCallable, Category = "ModularGameplayAbilities|Exec Calc")
	static const FGameplayEffectCustomExecutionOutput& AddOutputModifier(UPARAM(ref) FGameplayEffectCustomExecutionOutput& InExecutionOutput, const FGameplayAttribute InAttribute, const EGameplayModOp::Type InModOp, const float InMagnitude);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "ModularGameplayAbilities|Exec Calc")
	static const TMap<FGameplayTag, float> GetSetByCallerTagMagnitudes(const FGameplayEffectCustomExecutionParameters& InExecutionParams);
};
