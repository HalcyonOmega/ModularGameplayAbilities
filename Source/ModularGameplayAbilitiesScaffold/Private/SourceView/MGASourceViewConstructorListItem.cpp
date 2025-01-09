// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "SourceView/MGASourceViewConstructorListItem.h"

#include "MGAScaffoldLog.h"
#include "Attributes/ModularAttributeSetBase.h"
#include "Attributes/ModularAttributeSetBase.h"
#include "Engine/Blueprint.h"
#include "LineEndings/MGALineEndings.h"
#include "Models/MGAAttributeSetWizardViewModel.h"

#define LOCTEXT_NAMESPACE "FMGASourceViewConstructorListItem"

FMGAHeaderViewListItemPtr FMGASourceViewConstructorListItem::Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	return MakeShared<FMGASourceViewConstructorListItem>(InViewModel);
}

void FMGASourceViewConstructorListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGASourceViewConstructorListItem::FMGASourceViewConstructorListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	check(InViewModel.IsValid());

	if (!InViewModel->GetSelectedBlueprint().IsValid())
	{
		return;
	}

	const UBlueprint* Blueprint = InViewModel->GetSelectedBlueprint().Get();
	check(Blueprint);

	const FString DesiredClassName = GetClassNameToGenerate(Blueprint, InViewModel->GetNewClassName());

	RawItemString += FString::Printf(TEXT("%s::%s()"), *DesiredClassName, *DesiredClassName);
	RichTextString += FString::Printf(
		TEXT("<%s>%s</>::<%s>%s</>()"),
		*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
		*DesiredClassName,
		*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
		*DesiredClassName
	);

	// Add opening brace line
	RawItemString += TEXT("\n{\n");
	RichTextString += TEXT("\n{\n");

	// Add clamp definitions if we have any clamped attribute to handle
	const bool bSupportsClampedAttributeData = IsSupportingClampedAttributeData(InViewModel);
	if (IsSupportingClampedAttributeData(InViewModel))
	{
		// Filter to only clamped attributes
		// TArray<const FProperty*> Props = GetAllProperties(Blueprint->GeneratedClass).FilterByPredicate([](const FProperty* InProperty)
		TArray<const FProperty*> Props = GetAllProperties(Blueprint->GeneratedClass).FilterByPredicate([](const FProperty* InProperty)
		{
			return InProperty->GetCPPType() == TEXT("FMGAClampedAttributeData");
		});

		MGA_SCAFFOLD_LOG(Verbose, TEXT("Props: %d"), Props.Num())
		int32 CurrentIndex = 0;
		for (const FProperty* Prop : Props)
		{
			if (!Prop)
			{
				continue;
			}


			FString AttributeRawItemString;
			FString AttributeRichTextString;
			GetCodeTextsForClampedProperty(Prop, AttributeRawItemString, AttributeRichTextString);

			CurrentIndex++;

			RawItemString += AttributeRawItemString;
			RichTextString += AttributeRichTextString;

			if (CurrentIndex < Props.Num())
			{
				RawItemString += TEXT("\n");
				RichTextString += TEXT("\n");
			}
		}
	}
	
	// Add closing brace line
	RawItemString += TEXT("}\n");
	RichTextString += TEXT("}\n");

	// normalize to platform newlines
	MGA::String::ToHostLineEndingsInline(RawItemString);
	MGA::String::ToHostLineEndingsInline(RichTextString);
}

void FMGASourceViewConstructorListItem::GetCodeTextsForClampedProperty(const FProperty* InProperty, FString& OutRawItemString, FString& OutRichTextString)
{
	check(InProperty);

	const FString PropertyName = InProperty->GetName();

	const TSharedPtr<FMGAClampedAttributeData> AttributeData = GetGameplayAttributeData<FMGAClampedAttributeData>(*InProperty);
	check(AttributeData.IsValid());

	FMGAAttributeClampDefinition MaxValue = AttributeData->MaxValue;

	GetCodeTextsForClampDefinition(AttributeData->MinValue, PropertyName, TEXT("MinValue"), OutRawItemString, OutRichTextString);
	OutRawItemString += TEXT("\n");
	OutRichTextString += TEXT("\n");
	OutRawItemString += TEXT("\n");
	OutRichTextString += TEXT("\n");
	
	GetCodeTextsForClampDefinition(AttributeData->MaxValue, PropertyName, TEXT("MaxValue"), OutRawItemString, OutRichTextString);
	
	OutRawItemString += TEXT("\n");
	OutRichTextString += TEXT("\n");
}

void FMGASourceViewConstructorListItem::GetCodeTextsForClampDefinition(const FMGAAttributeClampDefinition& InDefinition, const FString& PropertyName, const FString& ClampDefinitionPropertyName, FString& OutRawItemString, FString& OutRichTextString)
{
	OutRawItemString += FString::Printf(
		TEXT("\t%s.%s.ClampType = %s;"),
		*PropertyName,
		*ClampDefinitionPropertyName,
		*UEnum::GetValueAsString(InDefinition.ClampType)
	);

	FString EnumName;
	FString ValueName;
	UEnum::GetValueAsString(InDefinition.ClampType).Split(TEXT("::"), &EnumName, &ValueName);
	
	OutRichTextString += FString::Printf(
		TEXT("\t<%s>%s</>.<%s>%s</>.<%s>ClampType</> = <%s>%s</>::%s;"),
		*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
		*PropertyName,
		*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
		*ClampDefinitionPropertyName,
		*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
		*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
		*EnumName,
		*ValueName
	);
	
	OutRawItemString += TEXT("\n");
	OutRichTextString += TEXT("\n");

	if (InDefinition.ClampType == EMGAAttributeClampingType::Float)
	{
		OutRawItemString += FString::Printf(
			TEXT("\t%s.%s.Value = %.2ff;"),
			*PropertyName,
			*ClampDefinitionPropertyName,
			InDefinition.Value
		);
		
		OutRichTextString += FString::Printf(
			TEXT("\t<%s>%s</>.<%s>%s</>.<%s>Value</> = <%s>%.2ff</>;"),
			*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
			*PropertyName,
			*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
			*ClampDefinitionPropertyName,
			*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
			*MGA::HeaderViewSyntaxDecorators::NumericLiteral,
			InDefinition.Value
		);
	}
	else if (InDefinition.ClampType == EMGAAttributeClampingType::AttributeBased)
	{
		OutRawItemString += FString::Printf(
			TEXT("\t%s.%s.Attribute = Get%sAttribute();"),
			*PropertyName,
			*ClampDefinitionPropertyName,
			*InDefinition.Attribute.GetName()
		);
		
		OutRichTextString += FString::Printf(
			TEXT("\t<%s>%s</>.<%s>%s</>.<%s>Attribute</> = <%s>Get%sAttribute()</>;"),
			*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
			*PropertyName,
			*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
			*ClampDefinitionPropertyName,
			*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
			*MGA::HeaderViewSyntaxDecorators::MacroDecorator,
			*InDefinition.Attribute.GetName()
		);
	}
}


#undef LOCTEXT_NAMESPACE
