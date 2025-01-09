// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "HeaderView/Attributes/MGAHeaderViewCopyrightListItem.h"

#include "GeneralProjectSettings.h"
#include "LineEndings/MGALineEndings.h"

#define LOCTEXT_NAMESPACE "FMGAHeaderViewCopyrightListItem"

void FMGAHeaderViewCopyrightListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGAHeaderViewCopyrightListItem::FMGAHeaderViewCopyrightListItem()
{
	const FString CopyrightNotice = GetDefault<UGeneralProjectSettings>()->CopyrightNotice;
	if (CopyrightNotice.IsEmpty())
	{
		return;
	}

	// Format comment
	FormatSingleLineCommentString(CopyrightNotice, RawItemString, RichTextString);

	// Add new line
	RawItemString += TEXT("\n");
	RichTextString += TEXT("\n");

	// normalize to platform newlines
	MGA::String::ToHostLineEndingsInline(RawItemString);
	MGA::String::ToHostLineEndingsInline(RichTextString);
}

FMGAHeaderViewListItemPtr FMGAHeaderViewCopyrightListItem::Create()
{
	return MakeShared<FMGAHeaderViewCopyrightListItem>();
}

#undef LOCTEXT_NAMESPACE
