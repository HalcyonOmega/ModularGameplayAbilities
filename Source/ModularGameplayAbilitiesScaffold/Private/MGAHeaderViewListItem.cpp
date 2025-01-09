// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "MGAHeaderViewListItem.h"

#include "EdGraphSchema_K2.h"
#include "MGAScaffoldLog.h"
#include "MGAScaffoldModule.h"
#include "MGAScaffoldPreviewSettings.h"
#include "Attributes/ModularAttributeSetBase.h"
#include "Engine/Blueprint.h"
#include "Framework/Text/ITextDecorator.h"
#include "Framework/Text/SlateTextRun.h"
#include "Internationalization/Regex.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "LineEndings/MGALineEndings.h"
#include "Models/MGAAttributeSetWizardViewModel.h"
#include "UObject/TextProperty.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/SRichTextBlock.h"

#define LOCTEXT_NAMESPACE "MGAHeaderViewListItem"

// MGA::HeaderViewSyntaxDecorators /////////////////////////////////////////////////

namespace MGA::HeaderViewSyntaxDecorators
{
	const FString CommentDecorator = TEXT("comment");
	const FString ErrorDecorator = TEXT("error");
	const FString IdentifierDecorator = TEXT("identifier");
	const FString KeywordDecorator = TEXT("keyword");
	const FString MacroDecorator = TEXT("macro");
	const FString TypenameDecorator = TEXT("typename");
	const FString StringLiteral = TEXT("string");
	const FString NumericLiteral = TEXT("numeric");
}

// FHeaderViewSyntaxDecorator /////////////////////////////////////////////////

namespace
{
	class FMGAHeaderViewSyntaxDecorator : public ITextDecorator
	{
	public:
		static TSharedRef<FMGAHeaderViewSyntaxDecorator> Create(FString InName, const FSlateColor& InColor)
		{
			return MakeShareable(new FMGAHeaderViewSyntaxDecorator(MoveTemp(InName), InColor));
		}

		virtual bool Supports(const FTextRunParseResults& RunInfo, const FString& Text) const override
		{
			return RunInfo.Name == DecoratorName;
		}

		virtual TSharedRef<ISlateRun> Create(const TSharedRef<FTextLayout>& TextLayout, const FTextRunParseResults& RunParseResult, const FString& OriginalText, const TSharedRef<FString>& ModelText, const ISlateStyle* Style) override
		{
			FRunInfo RunInfo(RunParseResult.Name);
			for (const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
			{
				RunInfo.MetaData.Add(Pair.Key, OriginalText.Mid(Pair.Value.BeginIndex, Pair.Value.EndIndex - Pair.Value.BeginIndex));
			}

			ModelText->Append(OriginalText.Mid(RunParseResult.ContentRange.BeginIndex, RunParseResult.ContentRange.Len()));

			FTextBlockStyle TextStyle = FMGAScaffoldModule::HeaderViewTextStyle;
			TextStyle.SetColorAndOpacity(TextColor);

			return FSlateTextRun::Create(RunInfo, ModelText, TextStyle);
		}

	private:
		FMGAHeaderViewSyntaxDecorator(FString&& InName, const FSlateColor& InColor)
			: DecoratorName(MoveTemp(InName))
			  , TextColor(InColor)
		{
		}

		/** Name of this decorator */
		FString DecoratorName;

		/** Color of this decorator */
		FSlateColor TextColor;
	};
}

// FMGAHeaderViewListItem ///////////////////////////////////////////////////////

const FText FMGAHeaderViewListItem::InvalidCPPIdentifierErrorText = LOCTEXT("CPPIdentifierError", "Name is not a valid C++ Identifier");

TSharedRef<SWidget> FMGAHeaderViewListItem::GenerateWidgetForItem() const
{
	const UMGAScaffoldPreviewSettings* HeaderViewSettings = GetDefault<UMGAScaffoldPreviewSettings>();
	const FMGAPreviewSyntaxColors& SyntaxColors = HeaderViewSettings->SyntaxColors;

	return SNew(SBox)
		.HAlign(HAlign_Fill)
		.Padding(FMargin(4.0f))
	[
		SNew(SRichTextBlock)
			.Text(FText::FromString(RichTextString))
			.TextStyle(&FMGAScaffoldModule::HeaderViewTextStyle)
		+ SRichTextBlock::Decorator(FMGAHeaderViewSyntaxDecorator::Create(MGA::HeaderViewSyntaxDecorators::CommentDecorator, SyntaxColors.Comment))
		+ SRichTextBlock::Decorator(FMGAHeaderViewSyntaxDecorator::Create(MGA::HeaderViewSyntaxDecorators::ErrorDecorator, SyntaxColors.Error))
		+ SRichTextBlock::Decorator(FMGAHeaderViewSyntaxDecorator::Create(MGA::HeaderViewSyntaxDecorators::IdentifierDecorator, SyntaxColors.Identifier))
		+ SRichTextBlock::Decorator(FMGAHeaderViewSyntaxDecorator::Create(MGA::HeaderViewSyntaxDecorators::KeywordDecorator, SyntaxColors.Keyword))
		+ SRichTextBlock::Decorator(FMGAHeaderViewSyntaxDecorator::Create(MGA::HeaderViewSyntaxDecorators::MacroDecorator, SyntaxColors.Macro))
		+ SRichTextBlock::Decorator(FMGAHeaderViewSyntaxDecorator::Create(MGA::HeaderViewSyntaxDecorators::TypenameDecorator, SyntaxColors.Typename))
		+ SRichTextBlock::Decorator(FMGAHeaderViewSyntaxDecorator::Create(MGA::HeaderViewSyntaxDecorators::StringLiteral, SyntaxColors.String))
		+ SRichTextBlock::Decorator(FMGAHeaderViewSyntaxDecorator::Create(MGA::HeaderViewSyntaxDecorators::NumericLiteral, SyntaxColors.Numeric))
	];
}

TSharedPtr<FMGAHeaderViewListItem> FMGAHeaderViewListItem::Create(FString InRawString, FString InRichText)
{
	return MakeShareable(new FMGAHeaderViewListItem(MoveTemp(InRawString), MoveTemp(InRichText)));
}

FMGAHeaderViewListItem::FMGAHeaderViewListItem(FString&& InRawString, FString&& InRichText)
	: RichTextString(MoveTemp(InRichText))
	, RawItemString(MoveTemp(InRawString))
{
}

TArray<const FProperty*> FMGAHeaderViewListItem::GetAllProperties(const UStruct* InStruct, const bool bInFilterReplicated)
{
	TArray<const FProperty*> VarProperties;
	for (TFieldIterator<FProperty> PropertyIt(InStruct, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		if (FProperty* VarProperty = *PropertyIt)
		{
			if (!VarProperty || !VarProperty->HasAnyPropertyFlags(CPF_BlueprintVisible))
			{
				continue;
			}

			if (bInFilterReplicated)
			{
				if (VarProperty->HasAnyPropertyFlags(CPF_Net))
				{
					VarProperties.Add(VarProperty);
				}
			}
			else
			{
				VarProperties.Add(VarProperty);
			}
		}
	}

	const UMGAScaffoldPreviewSettings* BlueprintHeaderViewSettings = GetDefault<UMGAScaffoldPreviewSettings>();
	if (BlueprintHeaderViewSettings->SortMethod == EMGAPreviewSortMethod::SortByAccessSpecifier)
	{
		VarProperties.Sort([](const FProperty& LeftProp, const FProperty& RightProp)
			{
				return !LeftProp.GetBoolMetaData(FBlueprintMetadata::MD_Private) && RightProp.GetBoolMetaData(FBlueprintMetadata::MD_Private);
			});
	}
	else if (BlueprintHeaderViewSettings->SortMethod == EMGAPreviewSortMethod::SortForOptimalPadding)
	{
		SortPropertiesForPadding(VarProperties);
	}
	
	return VarProperties;
}

bool FMGAHeaderViewListItem::IsUsingClampedAttributeData(const UStruct* InStruct)
{
	return GetAllProperties(InStruct).ContainsByPredicate([](const FProperty* InProperty)
	{
		return InProperty->GetCPPType() == TEXT("FMGAClampedAttributeData");
	});
}

bool FMGAHeaderViewListItem::IsSupportingClampedAttributeData(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel)
{
	const FNewClassInfo ParentClassInfo = InViewModel->GetParentClassInfo();

	// FMGAClampedAttributeData handling requires logic in PostGameplayEffectExecute handled by UModularAttributeSetBase
	return ParentClassInfo.BaseClass && ParentClassInfo.BaseClass->IsChildOf(UModularAttributeSetBase::StaticClass());
}

FString FMGAHeaderViewListItem::GetClassNameToGenerate(const UBlueprint* InBlueprint, const FString& InDesiredClassName, const bool bIncludePrefix)
{
	check(InBlueprint);
	const FString DesiredClassName = InDesiredClassName.IsEmpty() ? InBlueprint->GetName() : InDesiredClassName;
	return bIncludePrefix ? InBlueprint->SkeletonGeneratedClass->GetPrefixCPP() + DesiredClassName : DesiredClassName;
}

void FMGAHeaderViewListItem::FormatCommentString(FString InComment, FString& OutRawString, FString& OutRichString)
{
	// normalize newlines to \n
	MGA::String::FromHostLineEndingsInline(InComment);

	if (InComment.Contains("\n"))
	{
		/**
		 * Format into a multiline C++ comment, like this one
		 */
		InComment = TEXT("/**\n") + InComment;
		InComment.ReplaceInline(TEXT("\n"), TEXT("\n * "));
		InComment.Append(TEXT("\n */"));
	}
	else
	{
		/** Format into a single line C++ comment, like this one */
		InComment = FString::Printf(TEXT("/** %s */"), *InComment);
	}

	// add the comment to the raw string representation
	OutRawString = InComment;

	// mark each line of the comment as the beginning and end of a comment style for the rich text representation
	InComment.ReplaceInline(TEXT("\n"), *FString::Printf(TEXT("</>\n<%s>"), *MGA::HeaderViewSyntaxDecorators::CommentDecorator));
	OutRichString = FString::Printf(TEXT("<%s>%s</>"), *MGA::HeaderViewSyntaxDecorators::CommentDecorator, *InComment);
}

void FMGAHeaderViewListItem::FormatSingleLineCommentString(FString InComment, FString& OutRawString, FString& OutRichString)
{
	// normalize newlines to \n
	MGA::String::FromHostLineEndingsInline(InComment);

	/// Format into a single line C++ comment, like this one
	InComment = FString::Printf(TEXT("// %s"), *InComment);

	// add the comment to the raw string representation
	OutRawString = InComment;

	// mark the comment for rich text representation
	OutRichString = FString::Printf(TEXT("<%s>%s</>"), *MGA::HeaderViewSyntaxDecorators::CommentDecorator, *InComment);
}

FString FMGAHeaderViewListItem::GetCPPTypenameForProperty(const FProperty* InProperty, bool bIsMemberProperty/*=false*/)
{
	if (InProperty)
	{
		FString ExtendedTypeText;
		FString Typename = InProperty->GetCPPType(&ExtendedTypeText) + ExtendedTypeText;
		if (Typename == TEXT("double"))
		{
			Typename = TEXT("float");
		}

		if (bIsMemberProperty)
		{
			// ReSharper disable once CppDeclaratorNeverUsed
			if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(InProperty))
			{
				// Replace native pointer with TObjectPtr
				Typename.LeftChopInline(1);
				Typename = FString::Printf(TEXT("TObjectPtr<%s>"), *Typename);
			}
		}

		return Typename;
	}
	else
	{
		return TEXT("");
	}
}

bool FMGAHeaderViewListItem::IsValidCPPIdentifier(const FString& InIdentifier)
{
	// Only matches strings that start with a Letter or Underscore with any number of letters, digits or underscores following
	// _Legal, Legal0, legal_0
	// 0Illegal, Illegal&, Illegal-0
	static const FRegexPattern RegexPattern = FRegexPattern(TEXT("^[A-Za-z_]\\w*$"));
	return FRegexMatcher(RegexPattern, InIdentifier).FindNext();
}

const FText& FMGAHeaderViewListItem::GetErrorTextFromValidatorResult(EValidatorResult Result)
{
	static const TMap<EValidatorResult, FText> ErrorTextMap =
	{
		{EValidatorResult::AlreadyInUse, LOCTEXT("AlreadyInUse", "The name is already in use.")},
		{EValidatorResult::EmptyName, LOCTEXT("EmptyName", "The name is empty.")},
		{EValidatorResult::TooLong, LOCTEXT("TooLong", "The name is too long.")},
		{EValidatorResult::LocallyInUse, LOCTEXT("LocallyInUse", "The name is already in use locally.")}
	};

	if (const FText* ErrorText = ErrorTextMap.Find(Result))
	{
		return *ErrorText;
	}

	return FText::GetEmpty();
}

void FMGAHeaderViewListItem::SortPropertiesForPadding(TArray<const FProperty*>& InOutProperties)
{
	TArray<const FProperty*> SortedProperties;
	SortedProperties.Reserve(InOutProperties.Num());

	auto GetNeededPadding = [] (int32 Offset, int32 Alignment)
	{
		if (Offset % Alignment == 0)
		{
			return 0;
		}

		return Alignment - (Offset % Alignment);
	};

	/**
	 * For optimal struct size, we always want to place a property that can be aligned
	 * on the current boundary with the least amount of padding. If there are multiple
	 * properties that could be placed with the same amount of padding, we should pick
	 * the one with the greatest alignment because that one will be harder to place
	 * perfectly.
	 * 
	 * Any types' size will be a multiple of its alignment, so an 8-aligned property
	 * can always be followed by a 4-aligned property with no padding, so if we're 
	 * at an 8-byte boundary we should always put the 8 first.
	 */

	int32 CurrentOffset = 0;
	while (!InOutProperties.IsEmpty())
	{
		int32 BestIndex = INDEX_NONE;
		int32 BestAlignment = 0;
		int32 BestPadding = 0;
		for (int32 PropIndex = 0; PropIndex < InOutProperties.Num(); ++PropIndex)
		{
			const FProperty* Property = InOutProperties[PropIndex];
			const int32 PropAlignment = Property->GetMinAlignment();
			const int32 PropPadding = GetNeededPadding(CurrentOffset, PropAlignment);

			if (BestIndex == INDEX_NONE
				|| PropPadding < BestPadding 
				|| (PropPadding == BestPadding && PropAlignment > BestAlignment))
			{
				BestIndex = PropIndex;
				BestAlignment = PropAlignment;
				BestPadding = PropPadding;
			}
		}

		SortedProperties.Add(InOutProperties[BestIndex]);
		InOutProperties.RemoveAt(BestIndex, 1, EAllowShrinking::No);
		CurrentOffset += BestPadding + SortedProperties.Last()->GetSize();
	}

	InOutProperties = MoveTemp(SortedProperties);
}

FString FMGAHeaderViewListItem::GetPropertyValue(const FProperty& InProperty, const UObject* InContainer, const FString& InDesiredClassName, const bool bInIsRichText) const
{
	if (const FBoolProperty* Prop = GetProperty<FBoolProperty>(&InProperty))
	{
		const bool bValue = Prop->GetPropertyValue_InContainer(InContainer);
		const FString Value = bValue ? TEXT("true") : TEXT("false");
		return !bInIsRichText ? Value : FString::Printf(
			TEXT("<%s>%s</>"),
			*MGA::HeaderViewSyntaxDecorators::KeywordDecorator,
			*Value
		);
	}

	if (const FEnumProperty* Prop = GetProperty<FEnumProperty>(&InProperty))
	{
		// FNumericProperty* UnderlyingProperty = Prop->GetUnderlyingProperty();
		// const int64 Value = Prop->GetUnderlyingProperty()->GetSignedIntPropertyValue(nullptr);
		// FText DisplayName = Prop->GetEnum()->GetDisplayNameTextByValue(Value);
		MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a enum property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
		return TEXT("");
	}
	
	if (const FStructProperty* Prop = GetProperty<FStructProperty>(&InProperty))
	{
		MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a struct property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
		return TEXT("");
	}

	if (const FStrProperty* Prop = GetProperty<FStrProperty>(&InProperty))
	{
		// const FString PropertyValue = Prop->GetPropertyValue_InContainer(InContainer);
		// return !bInIsRichText ? FString::Printf(TEXT("TEXT(\"%s\")"), *PropertyValue) : FString::Printf(
		// 	TEXT("<%s>TEXT</>(<%s>\"%s\"</>)"),
		// 	*MGA::HeaderViewSyntaxDecorators::MacroDecorator,
		// 	*MGA::HeaderViewSyntaxDecorators::StringLiteral,
		// 	*PropertyValue
		// );
		return TEXT("");
	}
	
	if (const FNameProperty* Prop = GetProperty<FNameProperty>(&InProperty))
	{
		MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a name property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
		// const FName* PropertyValue = Prop->GetPropertyValuePtr_InContainer(InContainer);
		// if (PropertyValue)
		// {
		// 	return !bInIsRichText ? FString::Printf(TEXT("TEXT(\"%s\")"), *PropertyValue->ToString()) : FString::Printf(
		// 		TEXT("<%s>TEXT</>(<%s>\"%s\"</>)"),
		// 		*MGA::HeaderViewSyntaxDecorators::MacroDecorator,
		// 		*MGA::HeaderViewSyntaxDecorators::StringLiteral,
		// 		*PropertyValue->ToString()
		// 	);
		// }
		return TEXT("");
	}
	
	if (const FTextProperty* Prop = GetProperty<FTextProperty>(&InProperty))
	{
		MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a text property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
		// FText PropertyValue;
		// Prop->GetValue_InContainer(InContainer, &PropertyValue);
		// return !bInIsRichText ? FString::Printf(TEXT("NSLOCTEXT(\"%s\", \"NS\", \"%s\")"), *InDesiredClassName, *PropertyValue.ToString()) : FString::Printf(
		// 	TEXT("<%s>NSLOCTEXT</>(<%s>\"%s\"</>, <%s>\"%s\"</>, <%s>\"%s\"</>)"),
		// 	*MGA::HeaderViewSyntaxDecorators::MacroDecorator,
		// 	*MGA::HeaderViewSyntaxDecorators::StringLiteral,
		// 	*InDesiredClassName,
		// 	*MGA::HeaderViewSyntaxDecorators::StringLiteral,
		// 	*InProperty.GetAuthoredName(),
		// 	*MGA::HeaderViewSyntaxDecorators::StringLiteral,
		// 	*PropertyValue.ToString()
		// );
		return TEXT("");
	}

	if (const FArrayProperty* Prop = GetProperty<FArrayProperty>(&InProperty))
	{
		MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is an array property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
		return TEXT("");
	}
	
	if (const FSetProperty* Prop = GetProperty<FSetProperty>(&InProperty))
	{
		MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a set property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
		return TEXT("");
	}
	
	if (const FMapProperty* Prop = GetProperty<FMapProperty>(&InProperty))
	{
		MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a map property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
		return TEXT("");
	}
	
	if (const FNumericProperty* NumericProp = GetProperty<FNumericProperty>(&InProperty))
	{
		if (const FInt8Property* Prop = GetProperty<FInt8Property>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a int8 property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			return TEXT("");
		}
		
		if (const FByteProperty* Prop = GetProperty<FByteProperty>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a byte property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			// uint8 PropertyValue;
			// Prop->GetValue_InContainer(InContainer, &PropertyValue);

			const uint8 PropertyValue = Prop->GetPropertyValue_InContainer(InContainer);
			return !bInIsRichText ? FString::Printf(TEXT("%d"), PropertyValue) : FString::Printf(
				TEXT("<%s>%d</>"),
				*MGA::HeaderViewSyntaxDecorators::NumericLiteral,
				PropertyValue
			);
		}
		
		if (const FInt16Property* Prop = GetProperty<FInt16Property>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a int16 property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			return TEXT("");
		}
		
		if (const FUInt16Property* Prop = GetProperty<FUInt16Property>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a uint16 property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			return TEXT("");
		}
		
		if (const FIntProperty* Prop = GetProperty<FIntProperty>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a int property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			// int32 PropertyValue;
			// Prop->GetValue_InContainer(InContainer, &PropertyValue);

			const int32 PropertyValue = Prop->GetPropertyValue_InContainer(InContainer);
			return !bInIsRichText ? FString::Printf(TEXT("%d"), PropertyValue) : FString::Printf(
				TEXT("<%s>%d</>"),
				*MGA::HeaderViewSyntaxDecorators::NumericLiteral,
				PropertyValue
			);
		}
		
		if (const FUInt32Property* Prop = GetProperty<FUInt32Property>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a uint32 property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			return TEXT("");
		}
		
		if (const FInt64Property* Prop = GetProperty<FInt64Property>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a int64 property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			// int64 PropertyValue;
			// Prop->GetValue_InContainer(InContainer, &PropertyValue);

			const int64 PropertyValue = Prop->GetPropertyValue_InContainer(InContainer);
			return !bInIsRichText ? FString::Printf(TEXT("%lld"), PropertyValue) : FString::Printf(
				TEXT("<%s>%lld</>"),
				*MGA::HeaderViewSyntaxDecorators::NumericLiteral,
				PropertyValue
			);
		}
		
		if (const FUInt64Property* Prop = GetProperty<FUInt64Property>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a uint64 property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			return TEXT("");
		}
		
		if (const FFloatProperty* Prop = GetProperty<FFloatProperty>(&InProperty))
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a float property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			// float PropertyValue;
			// Prop->GetValue_InContainer(InContainer, &PropertyValue);
			
			const float PropertyValue = Prop->GetPropertyValue_InContainer(InContainer);
			return !bInIsRichText ? FString::Printf(TEXT("%f"), PropertyValue) : FString::Printf(
				TEXT("<%s>%f</>"),
				*MGA::HeaderViewSyntaxDecorators::NumericLiteral,
				PropertyValue
			);
		}
		
		if (const FDoubleProperty* Prop = GetProperty<FDoubleProperty>(&InProperty))
		{
			// Note: Float properties in BP are actually FDoubleProperty
			// double PropertyValue;
			// Prop->GetValue_InContainer(InContainer, &PropertyValue);
			//
			// MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a double property - value: %f"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName(), PropertyValue)
			//
			// return !bInIsRichText ? FString::Printf(TEXT("%.2ff"), PropertyValue) : FString::Printf(
			// 	TEXT("<%s>%.2ff</>"),
			// 	*MGA::HeaderViewSyntaxDecorators::NumericLiteral,
			// 	PropertyValue
			// );
			return TEXT("");
		}
		
		if (const FNumericProperty* Prop = NumericProp)
		{
			MGA_SCAFFOLD_LOG(VeryVerbose, TEXT("%s (%s) is a numeric property"), *InProperty.GetAuthoredName(), *Prop->GetAuthoredName())
			return TEXT("");
		}
	}

	return TEXT("");
}

bool FMGAHeaderViewListItem::ShouldUseConstRef(const FString& InCPPType)
{
	// List of cpp types that shouldn't use a const ref
	static TArray<FString> SimpleTypes = { TEXT("bool"), TEXT("float"), TEXT("double"), TEXT("uint8"), TEXT("int32"), TEXT("int64") };
	return !SimpleTypes.Contains(InCPPType);
}

TSharedPtr<FGameplayAttributeData> FMGAHeaderViewListItem::GetGameplayAttributeData(const FProperty& InProperty)
{
	return GetGameplayAttributeData<FGameplayAttributeData>(InProperty);
}

#undef LOCTEXT_NAMESPACE
