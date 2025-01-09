// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "SourceView/MGASourceViewIncludesListItem.h"

#include "Engine/Blueprint.h"
#include "LineEndings/MGALineEndings.h"
#include "Models/MGAAttributeSetWizardViewModel.h"

#define LOCTEXT_NAMESPACE "FMGASourceViewIncludesListItem"

FMGAHeaderViewListItemPtr FMGASourceViewIncludesListItem::Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	return MakeShared<FMGASourceViewIncludesListItem>(InViewModel);
}

void FMGASourceViewIncludesListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

FMGASourceViewIncludesListItem::FMGASourceViewIncludesListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	check(InViewModel.IsValid());
	
	if (const UBlueprint* Blueprint = InViewModel->GetSelectedBlueprint().Get())
	{
		const FString DesiredClassName = GetClassNameToGenerate(Blueprint, InViewModel->GetNewClassName(), false);

		FString DesiredClassPath = InViewModel->GetSelectedClassPath();
		if (!DesiredClassPath.IsEmpty() && !DesiredClassPath.EndsWith(TEXT("/")))
		{
			// Ensure trailing slash
			DesiredClassPath += TEXT("/");
		}

		// For public, include should be relative to the module. For private, should be empty (.h / .cpp should be in same Private folder)
		const FString RelativePath = InViewModel->GetClassLocation() == GameProjectUtils::EClassLocation::Public ? DesiredClassPath : TEXT("");
		const FString HeaderIncludePath = FString::Printf(TEXT("%s%s.h"), *RelativePath, *DesiredClassName);

		// Add header include for the class
		FString RawString;
		FString RichString;
		FormatIncludeDirective(HeaderIncludePath, RawString, RichString);

		RawItemString += RawString;
		RichTextString += RichString;
		
		// Add new line (between first header include and additional ones)
		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		const TArray<const FProperty*> ReplicatedProps = GetAllProperties(Blueprint->SkeletonGeneratedClass, true);

		// Add additional includes
		TArray<FString> Includes = { TEXT("GameplayEffectExtension.h") };
		if (!ReplicatedProps.IsEmpty())
		{
			Includes.Add(TEXT("Net/UnrealNetwork.h"));
		}

		FormatIncludeDirectives(Includes, RawItemString, RichTextString);

		// Add new line
		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		// normalize to platform newlines
		MGA::String::ToHostLineEndingsInline(RawItemString);
		MGA::String::ToHostLineEndingsInline(RichTextString);
	}
}

void FMGASourceViewIncludesListItem::FormatIncludeDirective(const FString& InIncludePath, FString& OutRawString, FString& OutRichString)
{
	// Add the comment to the raw string representation
	OutRawString = FString::Printf(TEXT("#include \"%s\""), *InIncludePath);

	// Mark the comment for rich text representation
	OutRichString = FString::Printf(
		TEXT("<%s>#include</> <%s>\"%s\"</>"),
		*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
		*MGA::HeaderViewSyntaxDecorators::CommentDecorator,
		*InIncludePath
	);
}

void FMGASourceViewIncludesListItem::FormatIncludeDirectives(const TArray<FString>& InIncludePaths, FString& OutRawString, FString& OutRichString)
{
	TArray<FString> RawStrings;
	TArray<FString> RichStrings;

	RawStrings.Reserve(InIncludePaths.Num());
	RichStrings.Reserve(InIncludePaths.Num());

	for (const FString& IncludePath : InIncludePaths)
	{
		FString RawString;
		FString RichString;
		FormatIncludeDirective(IncludePath, RawString, RichString);

		RawStrings.Add(RawString);
		RichStrings.Add(RichString);
	}
	
	// Add new line
	OutRawString += TEXT("\n\n");
	OutRichString += TEXT("\n\n");

	// Build the list
	OutRawString += FString::Join(RawStrings, TEXT("\n"));
	OutRichString += FString::Join(RichStrings, TEXT("\n"));
}

#undef LOCTEXT_NAMESPACE
