// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Details/MGAGameplayAttributeDataClampedDetails.h"

#include "AttributeSet.h"
#include "DetailWidgetRow.h"
#include "MGAEditorLog.h"
#include "IDetailChildrenBuilder.h"
#include "Attributes/ModularAttributeSetBase.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MGAGameplayAttributeDataDetails"

FMGAGameplayAttributeDataClampedDetails::FMGAGameplayAttributeDataClampedDetails()
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("Construct FMGAGameplayAttributeDataClampedDetails ..."))
}

FMGAGameplayAttributeDataClampedDetails::~FMGAGameplayAttributeDataClampedDetails()
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("Destroyed FMGAGameplayAttributeDataClampedDetails ..."))
}

TSharedRef<IPropertyTypeCustomization> FMGAGameplayAttributeDataClampedDetails::MakeInstance()
{
	TSharedRef<FMGAGameplayAttributeDataClampedDetails> Details = MakeShared<FMGAGameplayAttributeDataClampedDetails>();
	Details->Initialize();
	return Details;
}

void FMGAGameplayAttributeDataClampedDetails::CustomizeHeader(const TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGAGameplayAttributeDataClampedDetails::CustomizeHeader ..."))
	InitializeFromStructHandle(InStructPropertyHandle, InStructCustomizationUtils);

	InHeaderRow.
		NameContent()
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
				SNew(STextBlock).Text(this, &FMGAGameplayAttributeDataClampedDetails::GetHeaderBaseValueText)
			]
		];
}

void FMGAGameplayAttributeDataClampedDetails::CustomizeChildren(const TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& InStructBuilder, IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGAGameplayAttributeDataClampedDetails::CustomizeChildren ..."))
	InitializeFromStructHandle(InStructPropertyHandle, InStructCustomizationUtils);
	
	CustomizeBaseValueChildren(InStructPropertyHandle, InStructBuilder, InStructCustomizationUtils, false);
	CustomizeCurrentValueChildren(InStructPropertyHandle, InStructBuilder, InStructCustomizationUtils);

	if (!InStructPropertyHandle->IsValidHandle())
	{
		return;
	}

	const TSharedPtr<IPropertyHandle> MinValueHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMGAClampedAttributeData, MinValue));
	if (MinValueHandle.IsValid())
	{
		IDetailPropertyRow& Row = InStructBuilder.AddProperty(MinValueHandle.ToSharedRef());
		Row.ShouldAutoExpand(true);
	}
	
	const TSharedPtr<IPropertyHandle> MaxValueHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMGAClampedAttributeData, MaxValue));
	if (MaxValueHandle.IsValid())
	{
		IDetailPropertyRow& Row = InStructBuilder.AddProperty(MaxValueHandle.ToSharedRef());
		Row.ShouldAutoExpand(true);
	}
}

#undef LOCTEXT_NAMESPACE
