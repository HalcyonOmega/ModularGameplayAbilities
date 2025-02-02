﻿// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "AttributeReferenceViewer/MGAAttributeListReferenceViewer.h"

#include "AbilitySystemComponent.h"
#include "Editor.h"
#include "MGAEditorLog.h"
#include "MGAEditorSettings.h"
#include "IMGAEditorModule.h"
#include "Attributes/MGAAttributeSetBlueprint.h"
#include "Misc/EngineVersionComparison.h"
#include "UObject/UObjectIterator.h"
#include "Utilities/MGAUtilities.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "AssetRegistry/AssetIdentifier.h"

#define LOCTEXT_NAMESPACE "MGAAttributeListReferenceViewer"

/** The item used for visualizing the attribute in the list. */
class SMGAAttributeListReferenceViewerItem : public SComboRow<TSharedPtr<FMGAAttributeListReferenceViewerNode>>
{
public:
	SLATE_BEGIN_ARGS(SMGAAttributeListReferenceViewerItem)
			: _TextColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
		{
		}

		/** The text this item should highlight, if any. */
		SLATE_ARGUMENT(FText, HighlightText)
		/** The color text this item will use. */
		SLATE_ARGUMENT(FSlateColor, TextColor)
		/** The node this item is associated with. */
		SLATE_ARGUMENT(TSharedPtr<FMGAAttributeListReferenceViewerNode>, AssociatedNode)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		AssociatedNode = InArgs._AssociatedNode;

		ChildSlot
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 3.0f, 6.0f, 3.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(*AssociatedNode->AttributeName.Get()))
				.HighlightText(InArgs._HighlightText)
				.ColorAndOpacity(this, &SMGAAttributeListReferenceViewerItem::GetTextColor)
				.IsEnabled(true)
			]
		];

		TextColor = InArgs._TextColor;

		ConstructInternal(
			STableRow::FArguments()
			.ShowSelection(true),
			InOwnerTableView
		);
	}

	/** Returns the text color for the item based on if it is selected or not. */
	FSlateColor GetTextColor() const
	{
		const TSharedPtr<ITypedTableView<TSharedPtr<FMGAAttributeListReferenceViewerNode>>> OwnerWidget = OwnerTablePtr.Pin();
		const TSharedPtr<FMGAAttributeListReferenceViewerNode>* MyItem = OwnerWidget->Private_ItemFromWidget(this);
		const bool bIsSelected = OwnerWidget->Private_IsItemSelected(*MyItem);

		if (bIsSelected)
		{
			return FSlateColor::UseForeground();
		}

		return TextColor;
	}

private:
	/** The text color for this item. */
	FSlateColor TextColor;

	/** The Attribute Viewer Node this item is associated with. */
	TSharedPtr<FMGAAttributeListReferenceViewerNode> AssociatedNode;
};

void SMGAAttributeListReferenceViewer::Construct(const FArguments& InArgs)
{
	struct FLocal
	{
		static void AttributeToStringArray(const FProperty& Property, OUT TArray<FString>& StringArray)
		{
			// MGA Changed ...
			const UClass* Class = Property.GetOwnerClass();
			if (Class->IsChildOf(UAttributeSet::StaticClass()) ||
				(Class->IsChildOf(UAbilitySystemComponent::StaticClass()) && !Class->ClassGeneratedBy))
			{
				StringArray.Add(FString::Printf(TEXT("%s.%s"), *Class->GetName(), *Property.GetName()));
			}
		}
	};
	
	// Setup text filtering
	AttributeTextFilter = MakeShared<FAttributeTextFilter>(FAttributeTextFilter::FItemToStringArray::CreateStatic(&FLocal::AttributeToStringArray));
	
	// Preload to ensure BP Attributes are loaded in memory so that they can be listed here
	IMGAEditorModule::Get().PreloadAssetsByClass(UMGAAttributeSetBlueprint::StaticClass());
	
	UpdatePropertyOptions();
	
	TWeakPtr<SMGAAttributeListReferenceViewer> WeakSelf = StaticCastWeakPtr<SMGAAttributeListReferenceViewer>(AsWeak());

	const TSharedRef<SWidget> Picker = 
		SNew(SBorder)
		.Padding(InArgs._Padding)
		.BorderImage(FStyleDefaults::GetNoBrush())
		[
			SNew(SVerticalBox)

			// Gameplay Tag Tree controls
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)

				// Search
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(1.f)
				.Padding(0,1,5,1)
				[
					SAssignNew(SearchAttributeBox, SSearchBox)
					.HintText(LOCTEXT("GameplayTagPicker_SearchBoxHint", "Search Gameplay Attributes"))
					.OnTextChanged(this, &SMGAAttributeListReferenceViewer::OnFilterTextChanged)
					.DelayChangeNotificationsWhileTyping(true)
				]
			]

			// Gameplay Tags tree
			+SVerticalBox::Slot()
			.FillHeight(1)
			[
				SAssignNew(AttributesContainerWidget, SBorder)
				.BorderImage(FStyleDefaults::GetNoBrush())
				[
					SAssignNew(AttributeList, SListView<TSharedPtr<FMGAAttributeListReferenceViewerNode>>)
					.Visibility(EVisibility::Visible)
					.SelectionMode(ESelectionMode::Single)
					.ListItemsSource(&PropertyOptions)

					// Generates the actual widget for a tree item
					.OnGenerateRow(this, &SMGAAttributeListReferenceViewer::OnGenerateRowForAttributeViewer)

					// Find out when the user selects something in the tree
					.OnSelectionChanged(this, &SMGAAttributeListReferenceViewer::OnAttributeSelectionChanged)
				]
			]
		];
	
	ChildSlot
	[
		// Populate the widget
		Picker
	];
}

void SMGAAttributeListReferenceViewer::OnFilterTextChanged(const FText& InFilterText)
{
	AttributeTextFilter->SetRawFilterText(InFilterText);
	SearchAttributeBox->SetError(AttributeTextFilter->GetFilterErrorText());

	UpdatePropertyOptions();

	AttributeList->RequestListRefresh();
}

TSharedPtr<SWidget> SMGAAttributeListReferenceViewer::GetWidgetToFocusOnOpen()
{
	return SearchAttributeBox;
}

void SMGAAttributeListReferenceViewer::UpdatePropertyOptions()
{
	PropertyOptions.Empty();

	const UMGAEditorSettings& Settings = UMGAEditorSettings::Get();

	// Gather all UAttribute classes
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		const UClass* Class = *ClassIt;

		if (FMGAUtilities::IsValidAttributeClass(Class))
		{
			// Allow entire classes to be filtered globally
			if (Class->HasMetaData(TEXT("HideInDetailsView")))
			{
				continue;
			}

			for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
			{
				FProperty* Property = *PropertyIt;

				// if we have a search string and this doesn't match, don't show it
				if (AttributeTextFilter.IsValid() && !AttributeTextFilter->PassesFilter(*Property))
				{
					MGA_EDITOR_NS_LOG(Warning, TEXT("%s filtered"), *Property->GetName());
					continue;
				}

				// Allow properties to be filtered globally (never show up)
				if (Property->HasMetaData(TEXT("HideInDetailsView")))
				{
					continue;
				}

				// Only allow field of expected types
				FString CPPType = Property->GetCPPType();
				if (!FMGAUtilities::IsValidCPPType(CPPType))
				{
					continue;
				}

				const FString AttributeName = FString::Printf(TEXT("%s.%s"), *FMGAUtilities::GetAttributeClassName(Class), *Property->GetName());

				// Allow properties to be filtered globally via Developer Settings (never show up)
				if (UMGAEditorSettings::IsAttributeFiltered(Settings.FilterAttributesList, AttributeName))
				{
					continue;
				}

				PropertyOptions.Add(MakeShared<FMGAAttributeListReferenceViewerNode>(Property, AttributeName));
			}
		}

		// UAbilitySystemComponent can add 'system' attributes
		if (Class->IsChildOf(UAbilitySystemComponent::StaticClass()) && !Class->ClassGeneratedBy)
		{
			for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
			{
				FProperty* Property = *PropertyIt;

				// SystemAttributes have to be explicitly tagged
				if (Property->HasMetaData(TEXT("SystemGameplayAttribute")) == false)
				{
					continue;
				}

				// if we have a search string and this doesn't match, don't show it
				if (AttributeTextFilter.IsValid() && !AttributeTextFilter->PassesFilter(*Property))
				{
					continue;
				}

				const FString AttributeName = FString::Printf(TEXT("%s.%s"), *Class->GetName(), *Property->GetName());
				
				// Allow properties to be filtered globally via Developer Settings (never show up)
				if (UMGAEditorSettings::IsAttributeFiltered(Settings.FilterAttributesList, AttributeName))
				{
					continue;
				}

				PropertyOptions.Add(MakeShared<FMGAAttributeListReferenceViewerNode>(Property, AttributeName));
			}
		}
	}
}

TSharedRef<ITableRow> SMGAAttributeListReferenceViewer::OnGenerateRowForAttributeViewer(TSharedPtr<FMGAAttributeListReferenceViewerNode> InItem, const TSharedRef<STableViewBase>& InOwnerTable) const
{
	TSharedRef<SMGAAttributeListReferenceViewerItem> ReturnRow = SNew(SMGAAttributeListReferenceViewerItem, InOwnerTable)
		.HighlightText(SearchAttributeBox->GetText())
		.TextColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.f))
		.AssociatedNode(InItem);

	return ReturnRow;
}

void SMGAAttributeListReferenceViewer::OnAttributeSelectionChanged(TSharedPtr<FMGAAttributeListReferenceViewerNode> InItem, ESelectInfo::Type SelectInfo) const
{
	if (InItem.IsValid() && InItem->Attribute.IsValid())
	{
		TArray<FAssetIdentifier> AssetIdentifiers;
		const FName Name = FName(*FString::Printf(TEXT("%s.%s"), *InItem->Attribute->GetOwnerVariant().GetName(), *InItem->Attribute->GetName()));
		AssetIdentifiers.Add(FAssetIdentifier(FGameplayAttribute::StaticStruct(), Name));
		FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());	
	}
}

#undef LOCTEXT_NAMESPACE
