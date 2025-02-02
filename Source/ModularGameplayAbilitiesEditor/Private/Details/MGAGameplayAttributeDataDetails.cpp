﻿// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Details/MGAGameplayAttributeDataDetails.h"

#include "DetailWidgetRow.h"
#include "MGAConstants.h"
#include "MGAEditorLog.h"
#include "MGAEditorSettings.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MGAGameplayAttributeDataDetails"

FMGAGameplayAttributeDataDetails::FMGAGameplayAttributeDataDetails()
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("Construct FMGAGameplayAttributeDataDetails ..."))
}

FMGAGameplayAttributeDataDetails::~FMGAGameplayAttributeDataDetails()
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("Destroyed FMGAGameplayAttributeDataDetails ..."))
}

TSharedRef<IPropertyTypeCustomization> FMGAGameplayAttributeDataDetails::MakeInstance()
{
	TSharedRef<FMGAGameplayAttributeDataDetails> Details = MakeShared<FMGAGameplayAttributeDataDetails>();
	Details->Initialize();
	return Details;
}

void FMGAGameplayAttributeDataDetails::CustomizeHeader(const TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGAGameplayAttributeDataDetails::CustomizeHeader ..."))
	// Sets up PropertyUtilities, PropertyBeingCustomized, AttributeSetBeingCustomized, BlueprintBeingCustomized, AttributeDataBeingCustomized
	InitializeFromStructHandle(InStructPropertyHandle, InStructCustomizationUtils);

	
	// Check if we want to display details in compact mode
	if (IsCompactView())
	{
		MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGAGameplayAttributeDataDetails::CustomizeHeader bUseCompactView: true"))
		// Not adding anything to the row will make the header to not display
		return;
	}

	MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGAGameplayAttributeDataDetails::CustomizeHeader bUseCompactView: false"))

	InHeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(500)
		.MaxDesiredWidth(4096)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.Padding(0.f, 0.f, 2.f, 0.f)
			[
				SNew(STextBlock).Text(this, &FMGAGameplayAttributeDataDetails::GetHeaderBaseValueText)
			]
		];
}

void FMGAGameplayAttributeDataDetails::CustomizeChildren(const TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InStructBuilder, IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGAGameplayAttributeDataDetails::CustomizeChildren ..."))
	InitializeFromStructHandle(InStructPropertyHandle, InStructCustomizationUtils);

	CustomizeBaseValueChildren(InStructPropertyHandle, InStructBuilder, InStructCustomizationUtils, true);
	CustomizeCurrentValueChildren(InStructPropertyHandle, InStructBuilder, InStructCustomizationUtils);
}

void FMGAGameplayAttributeDataDetails::CustomizeBaseValueChildren(const TSharedRef<IPropertyHandle>& InStructPropertyHandle, IDetailChildrenBuilder& InStructBuilder, IPropertyTypeCustomizationUtils& InStructCustomizationUtils, const bool bAllowCompactView) const
{
	if (!InStructPropertyHandle->IsValidHandle())
	{
		return;
	}

	const TSharedPtr<IPropertyHandle> BaseValueHandle = InStructPropertyHandle->GetChildHandle(MGA::Constants::BaseValuePropertyName);
	if (!BaseValueHandle.IsValid())
	{
		return;
	}

	// Add the BaseValue generated property row
	IDetailPropertyRow& DetailPropertyRow = InStructBuilder.AddProperty(BaseValueHandle.ToSharedRef());

	// Handle display name tweak in case of compact view and tooltips
	if (!PropertyBeingCustomized.IsValid())
	{
		return;
	}
	
	const FText TooltipOverride = FText::Format(LOCTEXT(
		"BaseValueTooltip",
		"BaseValue for '{0}' Gameplay Attribute.\n\n"
		"CurrentValue is kept in sync with BaseValue, when edited in the details panel."
	), InStructPropertyHandle->GetPropertyDisplayName());

	if (bAllowCompactView && IsCompactView())
	{
		DetailPropertyRow.DisplayName(PropertyBeingCustomized->GetDisplayNameText());

		// In case BP property has been given a description in the Editor
		const FText UserDefinedTooltip = PropertyBeingCustomized->GetToolTipText();

		// Prepend user defined tooltip in case it was defined
		const FText PropertyTooltip = UserDefinedTooltip.IsEmpty() ? TooltipOverride : FText::Format(LOCTEXT("UserDefinedTooltipAndBaseValue", "{0}\n\n---\n\n{1}"), UserDefinedTooltip, TooltipOverride);

		DetailPropertyRow.ToolTip(PropertyTooltip);
	}
	else
	{
		DetailPropertyRow.ToolTip(TooltipOverride);
	}
}

void FMGAGameplayAttributeDataDetails::CustomizeCurrentValueChildren(const TSharedRef<IPropertyHandle>& InStructPropertyHandle, IDetailChildrenBuilder& InStructBuilder, IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
{
	if (IsCompactView())
	{
		return;
	}
	
	if (!InStructPropertyHandle->IsValidHandle())
	{
		return;
	}

	const TSharedPtr<IPropertyHandle> CurrentValueHandle = InStructPropertyHandle->GetChildHandle(MGA::Constants::CurrentValuePropertyName);
	if (!CurrentValueHandle.IsValid())
	{
		return;
	}

	// Add CurrentValue (read-only) property row if setting is set for it (by default, it's hidden)
	if (UMGAEditorSettings::Get().bDisplayCurrentValue)
	{
		InStructBuilder.AddProperty(CurrentValueHandle.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE
