// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "SourceView/MGASourceViewGetLifetimeListItem.h"

#include "Engine/Blueprint.h"
#include "LineEndings/MGALineEndings.h"
#include "Models/MGAAttributeSetWizardViewModel.h"
#include "Utilities/MGAUtilities.h"

#define LOCTEXT_NAMESPACE "FMGASourceViewGetLifetimeListItem"

FMGAHeaderViewListItemPtr FMGASourceViewGetLifetimeListItem::Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	return MakeShared<FMGASourceViewGetLifetimeListItem>(InViewModel);
}

void FMGASourceViewGetLifetimeListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGASourceViewGetLifetimeListItem::FMGASourceViewGetLifetimeListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	check(InViewModel.IsValid());
	
	if (const UBlueprint* Blueprint = InViewModel->GetSelectedBlueprint().Get())
	{
		const FString DesiredClassName = GetClassNameToGenerate(Blueprint, InViewModel->GetNewClassName());
		
		// Add the function definition line
		// i.e. void UDesiredClassName::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
		{
			RawItemString += FString::Printf(TEXT("void %s::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const"), *DesiredClassName);
			RichTextString += FString::Printf(
				TEXT("<%s>void</> <%s>%s</>::<%s>GetLifetimeReplicatedProps</>(<%s>TArray<FLifetimeProperty></>& <%s>OutLifetimeProps</>) <%s>const</>"),
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*DesiredClassName,
				*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator
			);
		}

		// Add opening brace line
		RawItemString += TEXT("\n{\n");
		RichTextString += TEXT("\n{\n");
		
		// Add the super line
		// i.e. Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		{
			RawItemString += TEXT("\tSuper::GetLifetimeReplicatedProps(OutLifetimeProps);\n");
			RichTextString += FString::Printf(
				TEXT("\t<%s>Super</>::<%s>GetLifetimeReplicatedProps</>(<%s>OutLifetimeProps</>);\n"),
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
				*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator
			);
		}

		// Add new line
		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		TArray<const FProperty*> ReplicatedProps = GetAllProperties(Blueprint->SkeletonGeneratedClass, true);

		// Add the DOREPLIFETIME definition lines
		// ie. DOREPLIFETIME_CONDITION_NOTIFY(UDesiredClassName, Property, COND_None, REPNOTIFY_Always);
		for (const FProperty* Property : ReplicatedProps)
		{
			if (!Property)
			{
				continue;
			}
			
			if (FMGAUtilities::IsValidCPPType(Property->GetCPPType()))
			{
				RawItemString += FString::Printf(
					TEXT("\tDOREPLIFETIME_CONDITION_NOTIFY(%s, %s, COND_None, REPNOTIFY_Always);\n"),
					*DesiredClassName,
					*Property->GetAuthoredName()
				);
				RichTextString += FString::Printf(
					TEXT("\t<%s>DOREPLIFETIME_CONDITION_NOTIFY</>(<%s>%s</>, <%s>%s</>, COND_None, REPNOTIFY_Always);\n"),
					*MGA::HeaderViewSyntaxDecorators::MacroDecorator,
					*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
					*DesiredClassName,
					*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
					*Property->GetAuthoredName()
				);
			}
			else
			{
				RawItemString += FString::Printf(
					TEXT("\tDOREPLIFETIME(%s, %s);\n"),
					*DesiredClassName,
					*Property->GetAuthoredName()
				);
				RichTextString += FString::Printf(
					TEXT("\t<%s>DOREPLIFETIME</>(<%s>%s</>, <%s>%s</>);\n"),
					*MGA::HeaderViewSyntaxDecorators::MacroDecorator,
					*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
					*DesiredClassName,
					*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
					*Property->GetAuthoredName()
				);
			}
		}
		
		// Add closing brace line
		RawItemString += TEXT("}\n");
		RichTextString += TEXT("}\n");

		// normalize to platform newlines
		MGA::String::ToHostLineEndingsInline(RawItemString);
		MGA::String::ToHostLineEndingsInline(RichTextString);
	}
}

#undef LOCTEXT_NAMESPACE
