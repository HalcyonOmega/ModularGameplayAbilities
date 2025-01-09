// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Details/MGAAttributeSetDetails.h"

#include "AttributeSet.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "EdGraphSchema_K2.h"
#include "MGAEditorLog.h"
#include "Details/Slate/SMGANewAttributeVariableWidget.h"
#include "Details/Slate/SMGAPositiveActionButton.h"
#include "Editor/MGABlueprintEditor.h"
#include "Engine/Blueprint.h"
#include "Details/Slate/MGANewAttributeViewModel.h"
#include "Styling/MGAAppStyle.h"
#include "Utilities/MGAEditorUtils.h"

#define LOCTEXT_NAMESPACE "FMGAAttributeSetDetails"

TSharedRef<IDetailCustomization> FMGAAttributeSetDetails::MakeInstance()
{
	return MakeShared<FMGAAttributeSetDetails>();
}

void FMGAAttributeSetDetails::CustomizeDetails(IDetailLayoutBuilder& InDetailLayout)
{
	AttributeBeingCustomized = UE::MGA::EditorUtils::GetAttributeBeingCustomized(InDetailLayout);
	if (!AttributeBeingCustomized.IsValid())
	{
		return;
	}

	BlueprintBeingCustomized = UE::MGA::EditorUtils::GetBlueprintFromClass(AttributeBeingCustomized->GetClass());
	if (!BlueprintBeingCustomized.IsValid())
	{
		return;
	}

	BlueprintEditorPtr = UE::MGA::EditorUtils::FindBlueprintEditorForAsset(BlueprintBeingCustomized.Get());
	if (!BlueprintEditorPtr.IsValid())
	{
		return;
	}

	IDetailCategoryBuilder& Category = InDetailLayout.EditCategory("Default", LOCTEXT("DefaultsCategory", "Variables"));
	Category.RestoreExpansionState(false);

	const TSharedRef<SHorizontalBox> HeaderContentWidget = SNew(SHorizontalBox);
	HeaderContentWidget->AddSlot()
	[
		SNew(SHorizontalBox)
	];

	HeaderContentWidget->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(SMGAPositiveActionButton)
		.ToolTipText(LOCTEXT("NewInputPoseTooltip", "Create a new Attribute"))
		.Icon(FMGAAppStyle::GetBrush("Icons.Plus"))
		.Text(LOCTEXT("AddNewLabel", "Add Attribute"))
		.OnGetMenuContent(this, &FMGAAttributeSetDetails::CreateNewAttributeVariableWidget)
	];
	Category.HeaderContent(HeaderContentWidget);
}

TSharedRef<SWidget> FMGAAttributeSetDetails::CreateNewAttributeVariableWidget()
{
	TSharedRef<FMGANewAttributeViewModel> ViewModel = MakeShared<FMGANewAttributeViewModel>();
	ViewModel->SetCustomizedBlueprint(BlueprintBeingCustomized);

	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
	PinType.PinSubCategory = NAME_None;
	PinType.PinSubCategoryObject = FGameplayAttributeData::StaticStruct();

	// Init from last saved variable states
	if (const TSharedPtr<FMGABlueprintEditor> BlueprintEditor = BlueprintEditorPtr.Pin()) 
	{
		ViewModel->SetVariableName(BlueprintEditor->GetLastUsedVariableName());
		ViewModel->SetbIsReplicated(BlueprintEditor->GetLastReplicatedState());
		if (BlueprintEditor->GetLastPinSubCategoryObject().IsValid())
		{
			PinType.PinSubCategoryObject = BlueprintEditor->GetLastPinSubCategoryObject();
		}
	}
	ViewModel->SetPinType(PinType);

	return SNew(SMGANewAttributeVariableWidget, ViewModel)
		.OnCancel_Static(&FMGAAttributeSetDetails::HandleAttributeWindowCancel)
		.OnFinish(this, &FMGAAttributeSetDetails::HandleAttributeWindowFinish);
}

void FMGAAttributeSetDetails::HandleAttributeWindowCancel(const TSharedPtr<FMGANewAttributeViewModel>& InViewModel)
{
	check(InViewModel.IsValid());
	MGA_EDITOR_LOG(Verbose, TEXT("Cancel -> %s"), *InViewModel->ToString())
}

void FMGAAttributeSetDetails::HandleAttributeWindowFinish(const TSharedPtr<FMGANewAttributeViewModel>& InViewModel) const
{
	check(InViewModel.IsValid());
	MGA_EDITOR_LOG(Verbose, TEXT("Finish -> %s"), *InViewModel->ToString())

	if (!BlueprintBeingCustomized.IsValid())
	{
		MGA_EDITOR_LOG(Error, TEXT("Failed to add new variable to blueprint because BlueprintBeingCustomized is null"))
		return;
	}

	if (const TSharedPtr<FMGABlueprintEditor> BlueprintEditor = BlueprintEditorPtr.Pin()) 
	{
		BlueprintEditor->SetLastPinSubCategoryObject(InViewModel->GetPinType().PinSubCategoryObject);
		BlueprintEditor->SetLastReplicatedState(InViewModel->GetbIsReplicated());
	}

	SMGANewAttributeVariableWidget::AddMemberVariable(
		BlueprintBeingCustomized.Get(),
		InViewModel->GetVariableName(),
		InViewModel->GetPinType(),
		InViewModel->GetDescription(),
		InViewModel->GetbIsReplicated()
	);
}

#undef LOCTEXT_NAMESPACE
