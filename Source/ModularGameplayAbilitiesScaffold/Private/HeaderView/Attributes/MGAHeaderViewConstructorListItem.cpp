// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "HeaderView/Attributes/MGAHeaderViewConstructorListItem.h"

#include "LineEndings/MGALineEndings.h"
#include "Models/MGAAttributeSetWizardViewModel.h"

#define LOCTEXT_NAMESPACE "FMGAHeaderViewConstructorListItem"

void FMGAHeaderViewConstructorListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGAHeaderViewConstructorListItem::FMGAHeaderViewConstructorListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	check(InViewModel.IsValid());

	if (!InViewModel->GetSelectedBlueprint().IsValid())
	{
		return;
	}

	const UBlueprint* Blueprint = InViewModel->GetSelectedBlueprint().Get();
	check(Blueprint);
	
	const FString DesiredClassName = GetClassNameToGenerate(Blueprint, InViewModel->GetNewClassName());

	// Add new line
	RawItemString += TEXT("\n");
	RichTextString += TEXT("\n");
	
	// Comment
	{
		const FString ClassName = GetClassNameToGenerate(Blueprint, InViewModel->GetNewClassName());
		const FString Comment = TEXT("Default constructor");

		FString RawString;
		FString RichString;
		FormatCommentString(Comment, RawString, RichString);

		RawItemString += RawString;
		RichTextString += RichString;
	}
	
	RawItemString += TEXT("\n");
	RichTextString += TEXT("\n");
	
	RawItemString += FString::Printf(TEXT("%s();"), *DesiredClassName);
	RichTextString += FString::Printf(
		TEXT("<%s>%s</>();"),
		*MGA::HeaderViewSyntaxDecorators::IdentifierDecorator,
		*DesiredClassName
	);

	RawItemString.InsertAt(0, TEXT("\t"));
	RichTextString.InsertAt(0, TEXT("\t"));
	RawItemString.ReplaceInline(TEXT("\n"), TEXT("\n\t"));
	RichTextString.ReplaceInline(TEXT("\n"), TEXT("\n\t"));

	// normalize to platform newlines
	MGA::String::ToHostLineEndingsInline(RawItemString);
	MGA::String::ToHostLineEndingsInline(RichTextString);
}

FMGAHeaderViewListItemPtr FMGAHeaderViewConstructorListItem::Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	return MakeShared<FMGAHeaderViewConstructorListItem>(InViewModel);
}

#undef LOCTEXT_NAMESPACE
