// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "HeaderView/Attributes/MGAHeaderViewIncludesListItem.h"

#include "Attributes/ModularAttributeSetBase.h"
#include "Algo/Unique.h"
#include "Attributes/ModularAttributeSetBase.h"
#include "Engine/Blueprint.h"
#include "LineEndings/MGALineEndings.h"
#include "Models/MGAAttributeSetWizardViewModel.h"

#define LOCTEXT_NAMESPACE "FMGAHeaderViewIncludesListItem"

FMGAHeaderViewIncludesListItem::FMGAHeaderViewIncludesListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	check(InViewModel.IsValid());
	
	if (const UBlueprint* Blueprint = InViewModel->GetSelectedBlueprint().Get())
	{
		const FString ClassName = InViewModel->GetNewClassName().IsEmpty() ? Blueprint->GetName() : InViewModel->GetNewClassName();
		const UClass* ParentClass = InViewModel->GetParentClassInfo().BaseClass;

		const FName IncludePathKey = TEXT("IncludePath");
		const FName ModuleRelativePathKey = TEXT("ModuleRelativePath");
		const FString ParentClassIncludePath = ParentClass && ParentClass->HasMetaData(IncludePathKey) ?
			ParentClass->GetMetaData(IncludePathKey) :
			TEXT("AttributeSet.h");

		TArray<FString> Includes = {
			TEXT("CoreMinimal.h"),
			TEXT(""),
			TEXT("AbilitySystemComponent.h"),
			TEXT("AttributeSet.h"),
			ParentClassIncludePath
		};

		if (IsSupportingClampedAttributeData(InViewModel) && IsUsingClampedAttributeData(Blueprint->SkeletonGeneratedClass))
		{
			const UScriptStruct* ClampedStaticStruct = FMGAClampedAttributeData::StaticStruct();
			if (ClampedStaticStruct->HasMetaData(ModuleRelativePathKey))
			{
				FString ModuleRelativePath = ClampedStaticStruct->GetMetaData(ModuleRelativePathKey);
				ModuleRelativePath.RemoveFromStart(TEXT("Public/"));
				Includes.Add(ModuleRelativePath);
			}
		}

		Includes.Add(FString::Printf(TEXT("%s.generated.h"), *ClassName));

		// Make sure to remove all duplicates (to filter out two times "AttributeSet.h")
		Includes.SetNum(Algo::Unique(Includes));

		FormatIncludeDirectives(Includes, RawItemString, RichTextString);

		// Add new line
		RawItemString += TEXT("\n");
		RichTextString += TEXT("\n");

		// normalize to platform newlines
		MGA::String::ToHostLineEndingsInline(RawItemString);
		MGA::String::ToHostLineEndingsInline(RichTextString);
	}
}

FMGAHeaderViewListItemPtr FMGAHeaderViewIncludesListItem::Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	return MakeShared<FMGAHeaderViewIncludesListItem>(InViewModel);
}

void FMGAHeaderViewIncludesListItem::ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset)
{
}

void FMGAHeaderViewIncludesListItem::FormatIncludeDirective(const FString& InIncludePath, FString& OutRawString, FString& OutRichString)
{
	if (InIncludePath.IsEmpty())
	{
		return;
	}
	
	// Add the comment to the raw string representation
	OutRawString = FString::Printf(TEXT("#include \"%s\""), *InIncludePath);

	// Mark the comment for rich text representation
	OutRichString = FString::Printf(
		TEXT("<%s>#include</> <%s>\"%s\"</>"),
		*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
		*MGA::HeaderViewSyntaxDecorators::StringLiteral,
		*InIncludePath
	);
}

void FMGAHeaderViewIncludesListItem::FormatIncludeDirectives(const TArray<FString>& InIncludePaths, FString& OutRawString, FString& OutRichString)
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

	// Prepend the list with a pragma once
	OutRawString = TEXT("#pragma once");
	OutRichString = FString::Printf(TEXT("<%s>#pragma once</>"), *MGA::HeaderViewSyntaxDecorators::KeywordDecorator);

	// Add new line
	OutRawString += TEXT("\n\n");
	OutRichString += TEXT("\n\n");

	// Build the list
	OutRawString += FString::Join(RawStrings, TEXT("\n"));
	OutRichString += FString::Join(RichStrings, TEXT("\n"));
}

#undef LOCTEXT_NAMESPACE
