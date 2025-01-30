// Copyright Halcyonyx Studios.

#include "Utilities/ModularEffectExecutionHelpers.h"

#include "GameplayEffectExecutionCalculation.h"
#include "ModularGameplayAbilitiesLogChannels.h"

const FGameplayEffectSpec& UModularEffectExecutionHelpers::GetOwningSpec(const FGameplayEffectCustomExecutionParameters& InExecutionParams)
{
	return InExecutionParams.GetOwningSpec();
}

FGameplayEffectContextHandle UModularEffectExecutionHelpers::GetEffectContext(const FGameplayEffectCustomExecutionParameters& InExecutionParams)
{
	const FGameplayEffectSpec& Spec = InExecutionParams.GetOwningSpec();
	return Spec.GetContext();
}

const FGameplayTagContainer& UModularEffectExecutionHelpers::GetSourceTags(const FGameplayEffectCustomExecutionParameters& InExecutionParams)
{
	const FGameplayEffectSpec& Spec = InExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	if (!SourceTags)
	{
		static FGameplayTagContainer DummyContainer;
		return DummyContainer;
	}
	
	return *SourceTags;
}

const FGameplayTagContainer& UModularEffectExecutionHelpers::GetTargetTags(const FGameplayEffectCustomExecutionParameters& InExecutionParams)
{
	const FGameplayEffectSpec& Spec = InExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	if (!TargetTags)
	{
		static FGameplayTagContainer DummyContainer;
		return DummyContainer;
	}
	
	return *TargetTags;
}

bool UModularEffectExecutionHelpers::AttemptCalculateCapturedAttributeMagnitude(const FGameplayEffectCustomExecutionParameters& InExecutionParams, const TArray<FGameplayEffectAttributeCaptureDefinition>& InRelevantAttributesToCapture, const FGameplayAttribute InAttribute, float& OutMagnitude)
{
	// First, figure out which capture definition to use - This assumes InRelevantAttributesToCapture is properly populated and passed in by user
	const FGameplayEffectAttributeCaptureDefinition* FoundCapture = InRelevantAttributesToCapture.FindByPredicate([InAttribute](const FGameplayEffectAttributeCaptureDefinition& Entry)
	{
		return Entry.AttributeToCapture == InAttribute;
	});

	if (!FoundCapture)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("Unable to retrieve a valid Capture Definition from passed in RelevantAttributesToCapture and Attribute: %s"), *InAttribute.GetName())
		return false;
	}

	const FGameplayEffectAttributeCaptureDefinition CaptureDefinition = *FoundCapture;
	const FGameplayEffectSpec& Spec = InExecutionParams.GetOwningSpec();
	
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	return InExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDefinition, EvaluateParameters, OutMagnitude);
}

bool UModularEffectExecutionHelpers::AttemptCalculateCapturedAttributeMagnitudeWithBase(const FGameplayEffectCustomExecutionParameters& InExecutionParams, const TArray<FGameplayEffectAttributeCaptureDefinition>& InRelevantAttributesToCapture, const FGameplayAttribute InAttribute, const float InBaseValue, float& OutMagnitude)
{
	// First, figure out which capture definition to use - This assumes InRelevantAttributesToCapture is properly populated and passed in by user
	const FGameplayEffectAttributeCaptureDefinition* FoundCapture = InRelevantAttributesToCapture.FindByPredicate([InAttribute](const FGameplayEffectAttributeCaptureDefinition& Entry)
	{
		return Entry.AttributeToCapture == InAttribute;
	});

	if (!FoundCapture)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("Unable to retrieve a valid Capture Definition from passed in RelevantAttributesToCapture and Attribute: %s"), *InAttribute.GetName())
		return false;
	}

	const FGameplayEffectAttributeCaptureDefinition CaptureDefinition = *FoundCapture;
	const FGameplayEffectSpec& Spec = InExecutionParams.GetOwningSpec();
	
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	return InExecutionParams.AttemptCalculateCapturedAttributeMagnitudeWithBase(CaptureDefinition, EvaluateParameters, InBaseValue, OutMagnitude);
}

const FGameplayEffectCustomExecutionOutput& UModularEffectExecutionHelpers::AddOutputModifier(FGameplayEffectCustomExecutionOutput& InExecutionOutput, const FGameplayAttribute InAttribute, const EGameplayModOp::Type InModOp, const float InMagnitude)
{
	InExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(InAttribute, InModOp, InMagnitude));
	return MoveTemp(InExecutionOutput);
}

const TMap<FGameplayTag, float> UModularEffectExecutionHelpers::GetSetByCallerTagMagnitudes(
	const FGameplayEffectCustomExecutionParameters& InExecutionParams)
{
	return InExecutionParams.GetOwningSpec().SetByCallerTagMagnitudes;
}