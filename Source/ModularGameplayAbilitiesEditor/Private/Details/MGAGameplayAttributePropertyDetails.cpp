// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.


#include "Details/MGAGameplayAttributePropertyDetails.h"

#include "AttributeSet.h"
#include "DetailWidgetRow.h"
#include "MGADelegates.h"
#include "MGAEditorLog.h"
#include "IPropertyUtilities.h"
#include "Details/Slate/SMGAGameplayAttributeWidget.h"

FMGAGameplayAttributePropertyDetails::~FMGAGameplayAttributePropertyDetails()
{
	FMGADelegates::OnRequestDetailsRefresh.RemoveAll(this);
}

TSharedRef<IPropertyTypeCustomization> FMGAGameplayAttributePropertyDetails::MakeInstance()
{
	return MakeShared<FMGAGameplayAttributePropertyDetails>();
}

// ReSharper disable once CppParameterMayBeConst
void FMGAGameplayAttributePropertyDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	MGA_EDITOR_LOG(Verbose, TEXT("FMGAGameplayAttributePropertyDetails::CustomizeHeader ..."))

	const TSharedPtr<IPropertyUtilities> Utilities = StructCustomizationUtils.GetPropertyUtilities();
	FMGADelegates::OnRequestDetailsRefresh.AddSP(this, &FMGAGameplayAttributePropertyDetails::HandleRequestRefresh, Utilities);

	// Can't use GET_MEMBER_NAME_CHECKED for those two props since they're private and requires adding this class as a friend class to FGameplayAttribute
	//
	// MyProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGameplayAttribute, Attribute));
	// OwnerProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGameplayAttribute, AttributeOwner));

	MyProperty = StructPropertyHandle->GetChildHandle(FName(TEXT("Attribute")));
	OwnerProperty = StructPropertyHandle->GetChildHandle(FName(TEXT("AttributeOwner")));
	NameProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGameplayAttribute, AttributeName));

	const FString& FilterMetaStr = StructPropertyHandle->GetProperty()->GetMetaData(TEXT("FilterMetaTag"));
	const bool bShowOnlyOwnedAttributes = StructPropertyHandle->GetProperty()->HasMetaData(TEXT("ShowOnlyOwnedAttributes"));
	const UClass* OuterBaseClass = StructPropertyHandle->GetOuterBaseClass();
	MGA_EDITOR_LOG(
		VeryVerbose,
		TEXT("FMGAGameplayAttributePropertyDetails::CustomizeHeader - OuterBaseClass: %s (bShowOnlyOwnedAttributed: %s)"),
		*GetNameSafe(OuterBaseClass),
		*LexToString(bShowOnlyOwnedAttributes)
	)

	FProperty* PropertyValue = nullptr;
	if (MyProperty.IsValid())
	{
		FProperty* ObjPtr = nullptr;
		MyProperty->GetValue(ObjPtr);
		PropertyValue = ObjPtr;
	}

	AttributeWidget = SNew(SMGAGameplayAttributeWidget)
		.OnAttributeChanged(this, &FMGAGameplayAttributePropertyDetails::OnAttributeChanged)
		.DefaultProperty(PropertyValue)
		.FilterMetaData(FilterMetaStr)
		.ShowOnlyOwnedAttributes(bShowOnlyOwnedAttributes)
		.FilterClass(bShowOnlyOwnedAttributes ? OuterBaseClass : nullptr);

	HeaderRow.
	NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
		.MinDesiredWidth(500)
		.MaxDesiredWidth(4096)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.Padding(0.f, 0.f, 2.f, 0.f)
			[
				AttributeWidget.ToSharedRef()
			]
		];
}

void FMGAGameplayAttributePropertyDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

void FMGAGameplayAttributePropertyDetails::OnAttributeChanged(FProperty* SelectedAttribute) const
{
	if (MyProperty.IsValid())
	{
		MyProperty->SetValue(SelectedAttribute);

		// When we set the attribute we should also set the owner and name info
		if (OwnerProperty.IsValid())
		{
			OwnerProperty->SetValue(SelectedAttribute ? SelectedAttribute->GetOwnerStruct() : nullptr);
		}

		if (NameProperty.IsValid())
		{
			FString AttributeName = TEXT("None");
			if (SelectedAttribute)
			{
				SelectedAttribute->GetName(AttributeName);
			}
			NameProperty->SetValue(AttributeName);
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void FMGAGameplayAttributePropertyDetails::HandleRequestRefresh(const TSharedPtr<IPropertyUtilities> InPropertyUtilities)
{
	MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGAGameplayAttributePropertyDetails::HandleRequestRefresh ..."))
	if (InPropertyUtilities.IsValid())
	{
		InPropertyUtilities->ForceRefresh();
	}
}
