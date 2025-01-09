// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "UObject/WeakFieldPtr.h"

class FMGABlueprintEditor;
class UAttributeSet;
struct FGameplayAttributeData;

/**
 * Details customization for FGameplayAttributeData / FMGAClampedAttributeData (Base Class).
 *
 * And ability to view / set BaseValue and CurrentValue (as DefaultValue)
 *
 * This is the default Base class shared by both FGameplayAttributeData and FMGAClampedAttributeData details customization classes.
 */
class IMGAGameplayAttributeDataDetailsBase : public IPropertyTypeCustomization
{
public:
	IMGAGameplayAttributeDataDetailsBase();
	virtual ~IMGAGameplayAttributeDataDetailsBase() override;

	/** Setup - mainly to register SP delegates as they need to happen after construction */
	virtual void Initialize();
	virtual void InitializeFromStructHandle(const TSharedRef<IPropertyHandle>& InStructPropertyHandle, IPropertyTypeCustomizationUtils& InStructCustomizationUtils);
	virtual FGameplayAttributeData* GetGameplayAttributeData() const;

	//~ Begin IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InStructCustomizationUtils) override {}
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InStructBuilder, IPropertyTypeCustomizationUtils& InStructCustomizationUtils) override {}
	//~ End IPropertyTypeCustomization interface

protected:
	/** Keep track of the property we are editing */
	TWeakFieldPtr<FProperty> PropertyBeingCustomized;

	/** The UObject we are editing */
	TWeakObjectPtr<UAttributeSet> AttributeSetBeingCustomized;
	
	/**
	 * Keep track of the FGameplayAttributeData struct we are editing
	 *
	 * This is mostly for GetGameplayAttributeData() implementation and GetHeaderBaseValueText() handling (reading
	 * Base and CurrentValue to display in headings)
	 */
	TSharedPtr<FGameplayAttributeData*> AttributeDataBeingCustomized;

	/** A weak reference to the property utilities used by this type customization */
	TWeakPtr<IPropertyUtilities> PropertyUtilities;

	void HandleSettingsChanged(UObject* InObject, FPropertyChangedEvent& InPropertyChangedEvent);

	static bool IsCompactView();

	/**
	 * Validation of the container / owner class of the Attribute Data property.
	 *
	 * Attribute Data definition should only happen within a UAttributeSet class and BP, and specifically a
	 * UModularAttributeSetBase (which is a child class of UAttributeSet specific to this plugin).
	 *
	 * This returns false whenever the owner class is not a child of UAttributeSet (invalid owner in case of BP struct,
	 * if defined in an Actor, Character or any other BP etc.)
	 */
	static bool IsValidOwnerClass(const UClass* InOwnerClass);

	FText GetHeaderBaseValueText() const;
};
