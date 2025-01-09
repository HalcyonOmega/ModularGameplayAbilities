// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "HeaderView/Attributes/MGAHeaderViewOnRepListItem.h"

#include "LineEndings/MGALineEndings.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "FMGAHeaderViewOnRepListItem"

FMGAHeaderViewListItemPtr FMGAHeaderViewOnRepListItem::Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel, const FProperty& InVarProperty)
{
	return MakeShared<FMGAHeaderViewOnRepListItem>(InViewModel, InVarProperty);
}

void FMGAHeaderViewOnRepListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGAHeaderViewOnRepListItem::FMGAHeaderViewOnRepListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel, const FProperty& VarProperty)
{
	// Add the UFUNCTION specifiers
	// i.e. UFUNCTION()
	{
		RawItemString += TEXT("\nUFUNCTION()");
		RichTextString += FString::Printf(TEXT("\n<%s>UFUNCTION</>()"), *MGA::HeaderViewSyntaxDecorators::MacroDecorator);
	}
	
	const FString ClampedPropertyTypename = TEXT("FMGAClampedAttributeData");
	
	// FMGAClampedAttributeData handling requires logic in PostGameplayEffectExecute handled by UModularAttributeSetBase
	const bool bSupportsClampedAttributeData = IsSupportingClampedAttributeData(InViewModel);
	const bool bIsClampedAttributeData = VarProperty.GetCPPType() == ClampedPropertyTypename;

	// Add the function declaration line
	// i.e. virtual void OnRep_AttributeName(Type OldVarName)
	{
		FString Typename = GetCPPTypenameForProperty(&VarProperty, /*bIsMemberProperty=*/true);
		const FString VarName = VarProperty.GetAuthoredName();
		const bool bLegalName = IsValidCPPIdentifier(VarName);
		
		if (bIsClampedAttributeData && !bSupportsClampedAttributeData)
		{
			// Convert FMGAClampedAttributeData to regular FGameplayAttributeData in case parent class is not
			// of type UModularAttributeSetBase, whose PostGameplayEffectExecute (or PreAttributeChange / PreAttributeBaseChange
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
			RawItemString += FString::Printf(TEXT("\nvirtual void OnRep_%s(const %s& Old%s);"), *VarName, *Typename, *VarName);
			RichTextString += FString::Printf(
				TEXT("\n<%s>virtual</> <%s>void</> <%s>OnRep_%s</>(<%s>const</> <%s>%s</>& <%s>Old%s</>);"),
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
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
			RawItemString += FString::Printf(TEXT("\nvirtual void OnRep_%s(%s Old%s);"), *VarName, *Typename, *VarName);
			RichTextString += FString::Printf(
				TEXT("\n<%s>virtual</> <%s>void</> <%s>OnRep_%s</>(<%s>%s</> <%s>Old%s</>);"),
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				**IdentifierDecorator,
				*VarName,
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*Typename,
				**IdentifierDecorator,
				*VarName
			);
		}
	}

	// indent item
	RawItemString.InsertAt(0, TEXT("\t"));
	RichTextString.InsertAt(0, TEXT("\t"));
	RawItemString.ReplaceInline(TEXT("\n"), TEXT("\n\t"));
	RichTextString.ReplaceInline(TEXT("\n"), TEXT("\n\t"));

	// normalize to platform newlines
	MGA::String::ToHostLineEndingsInline(RawItemString);
	MGA::String::ToHostLineEndingsInline(RichTextString);
}

#undef LOCTEXT_NAMESPACE
