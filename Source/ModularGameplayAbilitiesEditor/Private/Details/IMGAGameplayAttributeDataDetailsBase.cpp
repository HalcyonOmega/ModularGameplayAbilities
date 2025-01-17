// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Details/IMGAGameplayAttributeDataDetailsBase.h"

#include "AttributeSet.h"
#include "MGAEditorLog.h"
#include "MGAEditorSettings.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Attributes/ModularAttributeSetBase.h"

#define LOCTEXT_NAMESPACE "MGAGameplayAttributeDataDetails"

IMGAGameplayAttributeDataDetailsBase::IMGAGameplayAttributeDataDetailsBase()
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("Construct IMGAGameplayAttributeDataDetailsBase ..."))
}

IMGAGameplayAttributeDataDetailsBase::~IMGAGameplayAttributeDataDetailsBase()
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("Destroyed IMGAGameplayAttributeDataDetailsBase ..."))
	PropertyBeingCustomized.Reset();
	AttributeSetBeingCustomized.Reset();
	AttributeDataBeingCustomized.Reset();

	UMGAEditorSettings::GetMutable().OnSettingChanged().RemoveAll(this);
}

void IMGAGameplayAttributeDataDetailsBase::Initialize()
{
	UMGAEditorSettings::GetMutable().OnSettingChanged().AddSP(this, &IMGAGameplayAttributeDataDetailsBase::HandleSettingsChanged);
}

void IMGAGameplayAttributeDataDetailsBase::InitializeFromStructHandle(const TSharedRef<IPropertyHandle>& InStructPropertyHandle, IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
{
	PropertyUtilities = InStructCustomizationUtils.GetPropertyUtilities();

	PropertyBeingCustomized = InStructPropertyHandle->GetProperty();
	check(PropertyBeingCustomized.IsValid());

	const UClass* OwnerClass = PropertyBeingCustomized->Owner.GetOwnerClass();
	// Class can be undefined if attribute is defined in a Struct Blueprint
	if (!IsValidOwnerClass(OwnerClass))
	{
		return;
	}
	
	check(OwnerClass);
	
	const UClass* OuterBaseClass = InStructPropertyHandle->GetOuterBaseClass();
	check(OuterBaseClass);

	AttributeSetBeingCustomized = Cast<UAttributeSet>(OuterBaseClass->GetDefaultObject());
	
	if (FGameplayAttribute::IsGameplayAttributeDataProperty(PropertyBeingCustomized.Get()))
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(PropertyBeingCustomized.Get());
		check(StructProperty);

		if (FGameplayAttributeData* DataPtr = StructProperty->ContainerPtrToValuePtr<FGameplayAttributeData>(AttributeSetBeingCustomized.Get()))
		{
			AttributeDataBeingCustomized = MakeShared<FGameplayAttributeData*>(DataPtr);
		}
	}
}

FGameplayAttributeData* IMGAGameplayAttributeDataDetailsBase::GetGameplayAttributeData() const
{
	if (!AttributeDataBeingCustomized.IsValid())
	{
		return nullptr;
	}

	return *AttributeDataBeingCustomized.Get();
}

// ReSharper disable once CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef
void IMGAGameplayAttributeDataDetailsBase::HandleSettingsChanged(UObject* InObject, FPropertyChangedEvent& InPropertyChangedEvent)
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("IMGAGameplayAttributeDataDetailsBase::HandleSettingsChanged - InObject: %s, Property: %s"), *GetNameSafe(InObject), *GetNameSafe(InPropertyChangedEvent.Property))

	if (const TSharedPtr<IPropertyUtilities> Utilities = PropertyUtilities.Pin())
	{
		MGA_EDITOR_LOG(VeryVerbose, TEXT("IMGAGameplayAttributeDataDetailsBase::HandleSettingsChanged - ForceRefresh InObject: %s, Property: %s"), *GetNameSafe(InObject), *GetNameSafe(InPropertyChangedEvent.Property))
		Utilities->ForceRefresh();
	}
}

bool IMGAGameplayAttributeDataDetailsBase::IsCompactView()
{
	return UMGAEditorSettings::Get().bUseCompactView;
}

bool IMGAGameplayAttributeDataDetailsBase::IsValidOwnerClass(const UClass* InOwnerClass)
{
	if (!InOwnerClass)
	{
		// Null pointer owner, most likely AttributeData was defined as a variable in a BP struct.
		return false;
	}

	return InOwnerClass->IsChildOf(UModularAttributeSetBase::StaticClass());
}

FText IMGAGameplayAttributeDataDetailsBase::GetHeaderBaseValueText() const
{
	const FGameplayAttributeData* AttributeData = GetGameplayAttributeData();
	// check(AttributeData);

	// BP Struct ...
	if (!AttributeData)
	{
		return LOCTEXT("Invalid", "Invalid Attribute Data!");
	}

	return FText::Format(
		UMGAEditorSettings::Get().HeaderFormatText,  
		FText::FromString(FString::Printf(TEXT("%.2f"), AttributeData->GetBaseValue())),
		FText::FromString(FString::Printf(TEXT("%.2f"), AttributeData->GetCurrentValue()))
	);
}

#undef LOCTEXT_NAMESPACE
