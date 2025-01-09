// Copyright Halcyonyx Studios.

#include "Attributes/ModularAttributeSetBase.h"

#include "GameplayEffectExtension.h"
#include "ModularGameplayAbilitiesLogChannels.h"
#include "ActorComponent/ModularAbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Misc/DataValidation.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "MGAConstants.h"
#include "GameplayEffectExtension.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/PlayerController.h"
#include "Misc/EngineVersionComparison.h"
#include "UObject/UnrealType.h"

FMGAAttributeSetExecutionData::FMGAAttributeSetExecutionData(const FGameplayEffectModCallbackData& InModCallbackData)
{
	Context = InModCallbackData.EffectSpec.GetContext();
	SourceASC = Context.GetOriginalInstigatorAbilitySystemComponent();
	TargetASC = &InModCallbackData.Target;
	SourceTags = *InModCallbackData.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	InModCallbackData.EffectSpec.GetAllAssetTags(SpecAssetTags);

	const TSharedPtr<FGameplayAbilityActorInfo> TargetActorInfo = InModCallbackData.Target.AbilityActorInfo;
	const TSharedPtr<FGameplayAbilityActorInfo> SourceActorInfo = SourceASC ? SourceASC->AbilityActorInfo : nullptr;

	if (TargetActorInfo.IsValid())
	{
		TargetActor = TargetActorInfo->AvatarActor.Get();
		TargetController = TargetActorInfo->PlayerController.Get();
	}

	// Set the source actor based on context if it's set
	if (Context.GetEffectCauser())
	{
		SourceActor = Context.GetEffectCauser();
	}
	else if (SourceActorInfo.IsValid())
	{
		SourceActor = SourceActorInfo->AvatarActor.Get();
	}

	if (SourceActorInfo.IsValid())
	{
		SourceController = SourceActorInfo->PlayerController.Get();
	}

	SourceObject = InModCallbackData.EffectSpec.GetEffectContext().GetSourceObject();

	// Compute the delta between old and new, if it is available
	MagnitudeValue = InModCallbackData.EvaluatedData.Magnitude;

	if (InModCallbackData.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		DeltaValue = InModCallbackData.EvaluatedData.Magnitude;
	}
}

FString FMGAAttributeSetExecutionData::ToString(const FString& InSeparator) const
{
	const TArray Results = {
		FString::Printf(TEXT("SourceActor: %s"), *GetNameSafe(SourceActor)),
		FString::Printf(TEXT("TargetActor: %s"), *GetNameSafe(TargetActor)),
		FString::Printf(TEXT("SourceASC: %s"), *GetNameSafe(SourceASC)),
		FString::Printf(TEXT("TargetASC: %s"), *GetNameSafe(TargetASC)),
		FString::Printf(TEXT("SourceController: %s"), *GetNameSafe(SourceController)),
		FString::Printf(TEXT("TargetController: %s"), *GetNameSafe(TargetController)),
		FString::Printf(TEXT("SourceObject: %s"), *GetNameSafe(SourceObject)),
		FString::Printf(TEXT("SourceTags: %s"), *SourceTags.ToString()),
		FString::Printf(TEXT("SpecAssetTags: %s"), *SpecAssetTags.ToString()),
	};

	return FString::Join(Results, *InSeparator);
}

float FMGAAttributeClampDefinition::GetValueForClamping(const UAttributeSet* InOwnerSet) const
{
	check(InOwnerSet);

	float ResultValue;
	GetValueForClamping(InOwnerSet, ResultValue);

	return ResultValue;
}

bool FMGAAttributeClampDefinition::GetValueForClamping(const UAttributeSet* InOwnerSet, float& OutValue) const
{
	check(InOwnerSet);

	if (ClampType == EMGAAttributeClampingType::None)
	{
		OutValue = 0.f;
		return false;
	}
	
	if (ClampType == EMGAAttributeClampingType::Float)
	{
		OutValue = Value;
		return true;
	}

	if (ClampType == EMGAAttributeClampingType::AttributeBased && Attribute.IsValid())
	{
		// Check if valid class (eg. attribute is part of InOwnerSet and not from another set)
		if (const FProperty* Property = Attribute.GetUProperty())
		{
			const UStruct* OwnerStruct = Property->GetOwnerStruct();
			if (!InOwnerSet->GetClass()->IsChildOf(OwnerStruct))
			{
				MGA_LOG(
					Warning,
					TEXT("FMGAClampDefinition::GetValueForClamping - Unable to get attribute value for %s because it's not a member of %s"),
					*Attribute.GetName(),
					*GetNameSafe(InOwnerSet)
				)
				OutValue = 0.f;
				return false;
			}
		}
		
		const FGameplayAttributeData* AttributeData = Attribute.GetGameplayAttributeData(InOwnerSet);
		if (!AttributeData)
		{
			MGA_LOG(
				Warning,
				TEXT("FMGAClampDefinition::GetValueForClamping - Unable to get attribute value for %s because attribute failed to return its attribute data"),
				*Attribute.GetName()
			)
			MGA_LOG(Warning, TEXT("\t Is %s attribute a member of Attribute Set %s ?"), *Attribute.GetName(), *GetNameSafe(InOwnerSet))
			OutValue = 0.f;
			return false;
		}

		OutValue = AttributeData->GetCurrentValue();
		return true;
	}

	return false;
}

// Sets default values
UModularAttributeSetBase::UModularAttributeSetBase()
{
}

void UModularAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    // This is called whenever attributes change, so for max attributes we want to scale the current totals to match
    Super::PreAttributeChange(Attribute, NewValue);
}

void UModularAttributeSetBase::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void UModularAttributeSetBase::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
}

void UModularAttributeSetBase::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue,
	float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

bool UModularAttributeSetBase::PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void UModularAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    AActor* SourceActor = nullptr;
    AActor* TargetActor = nullptr;
    UModularAbilitySystemComponent* ASC = Cast<UModularAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	
    ASC->GetSourceAndTargetFromContext<AActor>(Data, SourceActor, TargetActor);
}

void UModularAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

float UModularAttributeSetBase::GetClampMinimumValueFor(const FGameplayAttribute& Attribute)
{
	// Subclass are expected to override this method for anything other than 0.f (if GetClampMinimumValueFor() is even used)
	return 0.f;
}

void UModularAttributeSetBase::GetExecutionDataFromMod(const FGameplayEffectModCallbackData& Data, FMGAAttributeSetExecutionData& OutExecutionData)
{
	OutExecutionData.Context = Data.EffectSpec.GetContext();
	OutExecutionData.SourceASC = OutExecutionData.Context.GetOriginalInstigatorAbilitySystemComponent();
	OutExecutionData.SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	Data.EffectSpec.GetAllAssetTags(OutExecutionData.SpecAssetTags);

	OutExecutionData.TargetActor = Data.Target.AbilityActorInfo->AvatarActor.IsValid() ? Data.Target.AbilityActorInfo->AvatarActor.Get() : nullptr;
	OutExecutionData.TargetController = Data.Target.AbilityActorInfo->PlayerController.IsValid() ? Data.Target.AbilityActorInfo->PlayerController.Get() : nullptr;
	OutExecutionData.TargetActor = Cast<APawn>(OutExecutionData.TargetActor);
	
	if (OutExecutionData.SourceASC && OutExecutionData.SourceASC->AbilityActorInfo.IsValid())
	{
		// Get the Source actor, which should be the damage causer (instigator)
		if (OutExecutionData.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
		{
			// Set the source actor based on context if it's set
			if (OutExecutionData.Context.GetEffectCauser())
			{
				OutExecutionData.SourceActor = OutExecutionData.Context.GetEffectCauser();
			}
			else
			{
				OutExecutionData.SourceActor = OutExecutionData.SourceASC->AbilityActorInfo->AvatarActor.IsValid()
					? OutExecutionData.SourceASC->AbilityActorInfo->AvatarActor.Get()
					: nullptr;
			}
		}

		OutExecutionData.SourceController = OutExecutionData.SourceASC->AbilityActorInfo->PlayerController.IsValid()
			? OutExecutionData.SourceASC->AbilityActorInfo->PlayerController.Get()
			: nullptr;
		OutExecutionData.SourceActor = Cast<APawn>(OutExecutionData.SourceActor);
	}

	OutExecutionData.SourceObject = Data.EffectSpec.GetEffectContext().GetSourceObject();

	// Compute the delta between old and new, if it is available
	OutExecutionData.DeltaValue = 0.f;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		OutExecutionData.DeltaValue = Data.EvaluatedData.Magnitude;
	}
}
