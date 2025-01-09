// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "SourceView/MGASourceViewOnRepListItem.h"

#include "LineEndings/MGALineEndings.h"
#include "Models/MGAAttributeSetWizardViewModel.h"

#define LOCTEXT_NAMESPACE "FMGASourceViewOnRepListItem"

FMGAHeaderViewListItemPtr FMGASourceViewOnRepListItem::Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel, const FProperty& InProperty)
{
	return MakeShared<FMGASourceViewOnRepListItem>(InViewModel, InProperty);
}

void FMGASourceViewOnRepListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGASourceViewOnRepListItem::FMGASourceViewOnRepListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel, const FProperty& InProperty)
{
	check(InViewModel.IsValid());
	
	if (!InViewModel->GetSelectedBlueprint().IsValid())
	{
		return;
	}
	
	const FString ClampedPropertyTypename = TEXT("FMGAClampedAttributeData");
	
	// FMGAClampedAttributeData handling requires logic in PostGameplayEffectExecute handled by UMGAAttributeSetBlueprintBase
	const bool bSupportsClampedAttributeData = IsSupportingClampedAttributeData(InViewModel);
	const bool bIsClampedAttributeData = InProperty.GetCPPType() == ClampedPropertyTypename;

	const UBlueprint* Blueprint = InViewModel->GetSelectedBlueprint().Get();
	const FString DesiredClassName = GetClassNameToGenerate(Blueprint, InViewModel->GetNewClassName());
	const FString VarName = InProperty.GetAuthoredName();

	// Add the function declaration line
	// i.e. void UDesiredClassName::OnRep_AttributeName(const FGameplayAttributeData& OldVarName)
	{
		FString Typename = GetCPPTypenameForProperty(&InProperty, /*bIsMemberProperty=*/true);
		const bool bLegalName = IsValidCPPIdentifier(VarName);

		if (bIsClampedAttributeData && !bSupportsClampedAttributeData)
		{
			// Convert FMGAClampedAttributeData to regular FGameplayAttributeData in case parent class is not
			// of type UMGAAttributeSetBlueprintBase, whose PostGameplayEffectExecute (or PreAttributeChange / PreAttributeBaseChange
			// if reworked)
			Typename = TEXT("FGameplayAttributeData");
		}

		const FString* IdentifierDecorator = &MGA::HeaderViewSyntaxDecorators::IdentifierDecorator;
		if (!bLegalName)
		{
			IllegalName = FName(VarName);
			IdentifierDecorator = &MGA::HeaderViewSyntaxDecorators::ErrorDecorator;
		}

		if (ShouldUseConstRef(Typename))
		{
			RawItemString += FString::Printf(TEXT("void %s::OnRep_%s(const %s& Old%s)\n"), *DesiredClassName, *VarName, *Typename, *VarName);
			RichTextString += FString::Printf(
				TEXT("<%s>void</> <%s>%s</>::<%s>OnRep_%s</>(<%s>const</> <%s>%s</>& <%s>Old%s</>)\n"),
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*DesiredClassName,
				**IdentifierDecorator,
				*VarName,
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*Typename,
				**IdentifierDecorator,
				*VarName
			);
		}
		else
		{
			RawItemString += FString::Printf(TEXT("void %s::OnRep_%s(%s Old%s)\n"), *DesiredClassName, *VarName, *Typename, *VarName);
			RichTextString += FString::Printf(
				TEXT("<%s>void</> <%s>%s</>::<%s>OnRep_%s</>(<%s>%s</> <%s>Old%s</>)\n"),
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*DesiredClassName,
				**IdentifierDecorator,
				*VarName,
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*Typename,
				**IdentifierDecorator,
				*VarName
			);
		}
	}

	// Add opening brace line
	RawItemString += TEXT("\n{\n");
	RichTextString += TEXT("\n{\n");

	// Add macro definition
	{
		RawItemString += FString::Printf(
			TEXT("\tGAMEPLAYATTRIBUTE_REPNOTIFY(%s, %s, Old%s);\n"),
			*DesiredClassName,
			*VarName,
			*VarName
		);
		RichTextString += FString::Printf(
			TEXT("\t<%s>GAMEPLAYATTRIBUTE_REPNOTIFY</>(<%s>%s</>, <%s>%s</>, Old%s);\n"),
			*MGA::HeaderViewSyntaxDecorators::MacroDecorator,
			*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
			*DesiredClassName,
			*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
			*VarName,
			*VarName
		);
	}

	// Add closing brace line
	RawItemString += TEXT("}\n");
	RichTextString += TEXT("}\n");

	// normalize to platform newlines
	MGA::String::ToHostLineEndingsInline(RawItemString);
	MGA::String::ToHostLineEndingsInline(RichTextString);
}

#undef LOCTEXT_NAMESPACE
