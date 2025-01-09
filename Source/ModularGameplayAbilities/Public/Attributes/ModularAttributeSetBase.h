// Copyright Halcyonyx Studios.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "ModularAttributeSetBase.generated.h"

struct FGameplayTagContainer;

/** Structure holding various information to deal with AttributeSet PostGameplayEffectExecute, extracting info from FGameplayEffectModCallbackData */
USTRUCT(BlueprintType)
struct FMGAAttributeSetExecutionData
{
	GENERATED_BODY()
	
	/** The physical representation of the Source ASC (The ability system component of the instigator that started the whole chain) */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	TObjectPtr<AActor> SourceActor = nullptr;

	/** The physical representation of the owner (Avatar) for the target we intend to apply to  */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** The ability system component of the instigator that started the whole chain */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;

	/** The ability system component we intend to apply to */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	TObjectPtr<UAbilitySystemComponent> TargetASC = nullptr;

	/** PlayerController associated with the owning actor for the Source ASC (The ability system component of the instigator that started the whole chain) */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	TObjectPtr<AController> SourceController = nullptr;

	/** PlayerController associated with the owning actor for the target we intend to apply to */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	TObjectPtr<AController> TargetController = nullptr;

	/** The object this effect was created from. */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	TObjectPtr<UObject> SourceObject = nullptr;

	/** This tells us how we got here (who / what applied us) */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	FGameplayEffectContextHandle Context;

	/** Combination of spec and actor tags for the captured Source Tags on GameplayEffectSpec creation */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	FGameplayTagContainer SourceTags;

	/** All tags that apply to the gameplay effect spec */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	FGameplayTagContainer SpecAssetTags;

	/** Holds the modifier magnitude value, if it is available (for scalable floats) */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	float MagnitudeValue = 0.f;

	/** Holds the delta value between old and new, if it is available (for Additive Operations) */
	UPROPERTY(BlueprintReadOnly, Category = "Blueprint Attributes")
	float DeltaValue = 0.f;

	/** Default constructor */
	FMGAAttributeSetExecutionData() = default;

	/**
	 * Fills out FMGAAttributeSetExecutionData structure based on provided FGameplayEffectModCallbackData data.
	 *
	 * @param InModCallbackData The gameplay effect mod callback data available in attribute sets' Pre/PostGameplayEffectExecute
	 */
	explicit FMGAAttributeSetExecutionData(const FGameplayEffectModCallbackData& InModCallbackData);

	/** Returns a simple string representation for this structure */
	FString ToString(const FString& InSeparator = TEXT(", ")) const;
};

/** Enumeration outlining the possible gameplay effect magnitude calculation policies. */
UENUM()
enum class EMGAAttributeClampingType : uint8
{
	/** Clamping is disabled for this definition */
	None,
	
	/** Use a simple, static float for the clamping. */
	Float,
	
	/** Perform a clamping based upon another attribute. */
	AttributeBased,
};

USTRUCT()
struct MODULARGAMEPLAYABILITIES_API FMGAAttributeClampDefinition
{
	GENERATED_BODY()
	
	/** Type of clamping to perform (either static float or attribute based) */
	UPROPERTY(EditDefaultsOnly, Category = "Clamp")
	EMGAAttributeClampingType ClampType = EMGAAttributeClampingType::Float;

	/** Float value to base the clamping on */
	UPROPERTY(EditDefaultsOnly, Category = "Clamp", meta=(EditConditionHides, EditCondition="ClampType == EMGAAttributeClampingType::Float"))
	float Value = 0.f;
	
	/**
	 * Gameplay Attribute to base the clamping on (for example, MaxHealth when clamping the Health attribute)
	 *
	 * Only "owned" attributes will be displayed here, meaning attributes that are part of the same Attribute Set class (eg. same owner)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Clamp", meta=(EditConditionHides, EditCondition="ClampType == EMGAAttributeClampingType::AttributeBased", ShowOnlyOwnedAttributes))
	FGameplayAttribute Attribute;

	/** default constructor */
	FMGAAttributeClampDefinition() = default;
	virtual ~FMGAAttributeClampDefinition() = default;

	/** Returns string representation of all member variables of this struct */
	FString ToString() const
	{
		return FString::Printf(TEXT("ClampType: %s, Value: %f, Attribute: %s"), *UEnum::GetValueAsString(ClampType), Value, *Attribute.GetName());
	}

	/** Returns the actual float value to use for the clamping, based on the ClampType used (eg. the backing attribute value or the static float) */
	float GetValueForClamping(const UAttributeSet* InOwnerSet) const;
	
	/**
	 * Returns the actual float value to use for the clamping, based on the ClampType used (eg. the backing attribute value or the static float)
	 *
	 * Function return value indicate whether the clamp definition is valid, eg. if attribute based attribute is valid for instance
	 */
	bool GetValueForClamping(const UAttributeSet* InOwnerSet, float& OutValue) const;
};

/**
 * Place in a Blueprint AttributeSet to create an attribute that can be accessed using FGameplayAttribute.
 *
 * This one has clamping functionality built-in (compared to FGameplayAttributeData)
 */
USTRUCT(DisplayName="Clamped Modular Attribute Data")
struct MODULARGAMEPLAYABILITIES_API FMGAClampedAttributeData : public FGameplayAttributeData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Min")
	FMGAAttributeClampDefinition MinValue;
	
	UPROPERTY(EditDefaultsOnly, Category = "Max")
	FMGAAttributeClampDefinition MaxValue;

	FMGAClampedAttributeData() = default;

	FMGAClampedAttributeData(const float DefaultValue)
		: FGameplayAttributeData(DefaultValue)
	{
	}
};

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** 
 * Delegate used to broadcast attribute events, some of these parameters may be null on clients: 
 * @param EffectInstigator	The original instigating actor for this event
 * @param EffectCauser		The physical actor that caused the change
 * @param EffectSpec		The full effect spec for this change
 * @param EffectMagnitude	The raw magnitude, this is before clamping
 * @param OldValue			The value of the attribute before it was changed
 * @param NewValue			The value after it was changed
*/
DECLARE_MULTICAST_DELEGATE_SixParams(FMGAAttributeEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/, const FGameplayEffectSpec* /*EffectSpec*/, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/);

/**
 * Base Attribute Set Class Used By This Plugin
 */
UCLASS(Abstract)
class MODULARGAMEPLAYABILITIES_API UModularAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	// Sets default values for this AttributeSet attributes
	UModularAttributeSetBase();

	// AttributeSet Overrides
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Helper function to get the minimum clamp value for a given attribute. Subclasses are expected to override this. */
	virtual float GetClampMinimumValueFor(const FGameplayAttribute& Attribute);

protected:
	/**
	 * Fills out FMGAAttributeSetExecutionData structure based on provided data.
	 *
	 * @param Data The gameplay effect mod callback data available in attribute sets' PostGameplayEffectExecute
	 * @param OutExecutionData Returned structure with various information extracted from Data (Source / Target Actor, Controllers, etc.)
	 */
	virtual void GetExecutionDataFromMod(const FGameplayEffectModCallbackData& Data, OUT FMGAAttributeSetExecutionData& OutExecutionData);
};
