// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "HeaderView/Attributes/MGAHeaderViewAttributeAccessorsListItem.h"

#include "LineEndings/MGALineEndings.h"

#define LOCTEXT_NAMESPACE "FMGAHeaderViewAttributesAccessorsListItem"

void FMGAHeaderViewAttributesAccessorsListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGAHeaderViewAttributesAccessorsListItem::FMGAHeaderViewAttributesAccessorsListItem()
{
	FormatSingleLineCommentString(TEXT("Attribute accessors macros from AttributeSet.h"), RawItemString, RichTextString);

	// Add new line
	RawItemString += TEXT("\n");
	RichTextString += TEXT("\n");

	// Add macros
	{
		RawItemString += FString::Printf(TEXT("#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \\"));
		RichTextString += FString::Printf(
			TEXT("<%s>#define</> <%s>ATTRIBUTE_ACCESSORS</>(ClassName, PropertyName) \\"),
			*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
			*MGA::HeaderViewSyntaxDecorators::MacroDecorator
		);

		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		RawItemString += FString::Printf(TEXT("\tGAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \\"));
		RichTextString += FString::Printf(
			TEXT("\t<%s>GAMEPLAYATTRIBUTE_PROPERTY_GETTER</>(ClassName, PropertyName) \\"),
			*MGA::HeaderViewSyntaxDecorators::MacroDecorator
		);

		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		RawItemString += FString::Printf(TEXT("\tGAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \\"));
		RichTextString += FString::Printf(
			TEXT("\t<%s>GAMEPLAYATTRIBUTE_VALUE_GETTER</>(PropertyName) \\"),
			*MGA::HeaderViewSyntaxDecorators::MacroDecorator
		);

		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		RawItemString += FString::Printf(TEXT("\tGAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \\"));
		RichTextString += FString::Printf(
			TEXT("\t<%s>GAMEPLAYATTRIBUTE_VALUE_SETTER</>(PropertyName) \\"),
			*MGA::HeaderViewSyntaxDecorators::MacroDecorator
		);

		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		RawItemString += FString::Printf(TEXT("\tGAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)"));
		RichTextString += FString::Printf(
			TEXT("\t<%s>GAMEPLAYATTRIBUTE_VALUE_INITTER</>(PropertyName)"),
			*MGA::HeaderViewSyntaxDecorators::MacroDecorator
		);
	}

	// Add new line
	RawItemString += TEXT("\n");
	RichTextString += TEXT("\n");

	// normalize to platform newlines
	MGA::String::ToHostLineEndingsInline(RawItemString);
	MGA::String::ToHostLineEndingsInline(RichTextString);
}

FMGAHeaderViewListItemPtr FMGAHeaderViewAttributesAccessorsListItem::Create()
{
	return MakeShared<FMGAHeaderViewAttributesAccessorsListItem>();
}

#undef LOCTEXT_NAMESPACE
