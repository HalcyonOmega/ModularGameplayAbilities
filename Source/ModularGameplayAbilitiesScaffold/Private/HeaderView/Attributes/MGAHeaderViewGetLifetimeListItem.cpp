// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "HeaderView/Attributes/MGAHeaderViewGetLifetimeListItem.h"

#include "LineEndings/MGALineEndings.h"
#include "Models/MGAAttributeSetWizardViewModel.h"

#define LOCTEXT_NAMESPACE "FMGAHeaderViewGetLifetimeListItem"

void FMGAHeaderViewGetLifetimeListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGAHeaderViewGetLifetimeListItem::FMGAHeaderViewGetLifetimeListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	check(InViewModel.IsValid());
	
	if (const UBlueprint* Blueprint = InViewModel->GetSelectedBlueprint().Get())
	{
		// Add new line
		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		// Format comment
		{
			const FString ClassName = GetClassNameToGenerate(Blueprint, InViewModel->GetNewClassName());
			const FString Comment = FString::Printf(TEXT("You will need to add DOREPLIFETIME(%s, VarName) to GetLifetimeReplicatedProps"), *ClassName);

			FString RawString;
			FString RichString;
			FormatCommentString(Comment, RawString, RichString);

			RawItemString += RawString;
			RichTextString += RichString;
		}

		// Add the function declaration line
		// i.e. virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		{
			RawItemString += TEXT("\nvirtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;");
			RichTextString += FString::Printf(
				TEXT("\n<%s>virtual</> <%s>void</> <%s>GetLifetimeReplicatedProps</>(<%s>TArray<FLifetimeProperty></>& <%s>OutLifetimeProps</>) const override;"),
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
				*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
				*MGA::HeaderViewSyntaxDecorators::TypenameDecorator,
				*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator
			);
		}

		// Add new line
		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		// indent item
		RawItemString.InsertAt(0, TEXT("\t"));
		RichTextString.InsertAt(0, TEXT("\t"));
		RawItemString.ReplaceInline(TEXT("\n"), TEXT("\n\t"));
		RichTextString.ReplaceInline(TEXT("\n"), TEXT("\n\t"));

		// normalize to platform newlines
		MGA::String::ToHostLineEndingsInline(RawItemString);
		MGA::String::ToHostLineEndingsInline(RichTextString);
	}
}

FMGAHeaderViewListItemPtr FMGAHeaderViewGetLifetimeListItem::Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	return MakeShared<FMGAHeaderViewGetLifetimeListItem>(InViewModel);
}

#undef LOCTEXT_NAMESPACE
