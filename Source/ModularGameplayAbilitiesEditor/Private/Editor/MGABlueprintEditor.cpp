// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Editor/MGABlueprintEditor.h"

#include "AttributeSet.h"
#include "EdGraphSchema_K2.h"
#include "MGADelegates.h"
#include "MGAEditorLog.h"
#include "IMGAEditorModule.h"
#include "IMGAScaffoldModule.h"
#include "Attributes/MGAAttributeSetBlueprint.h"
#include "Details/Slate/MGANewAttributeViewModel.h"
#include "Details/Slate/SMGANewAttributeVariableWidget.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/MGAEditorStyle.h"

#define LOCTEXT_NAMESPACE "MGABlueprintEditor"

FMGABlueprintEditor::FMGABlueprintEditor()
{
}

FMGABlueprintEditor::~FMGABlueprintEditor()
{
	if (AttributeWizardWindow.IsValid())
	{
		AttributeWizardWindow.Reset();
	}
}

void FMGABlueprintEditor::InitAttributeSetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, const bool bShouldOpenInDefaultsMode)
{
	MGA_EDITOR_LOG(Verbose, TEXT("FMGABlueprintEditor::InitAttributeSetEditor"))
	InitBlueprintEditor(Mode, InitToolkitHost, InBlueprints, bShouldOpenInDefaultsMode);

	const TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension(
		TEXT("Settings"),
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FMGABlueprintEditor::FillToolbar)
	);

	AddToolbarExtender(ToolbarExtender);
	RegenerateMenusAndToolbars();

	if (InBlueprints.IsValidIndex(0))
	{
		if (UMGAAttributeSetBlueprint* Blueprint = Cast<UMGAAttributeSetBlueprint>(InBlueprints[0]))
		{
			Blueprint->RegisterDelegates();
		}
	}
}

void FMGABlueprintEditor::Compile()
{
	const UBlueprint* Blueprint = GetBlueprintObj();
	
	MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGABlueprintEditor::Compile - PreCompile for %s"), *GetNameSafe(Blueprint))
	if (Blueprint)
	{
		if (const UPackage* Package = Blueprint->GetPackage())
		{
			FMGADelegates::OnPreCompile.Broadcast(Package->GetFName());
		}
	}
	
	FBlueprintEditor::Compile();
	MGA_EDITOR_LOG(VeryVerbose, TEXT("FMGABlueprintEditor::Compile - PostCompile for %s"), *GetNameSafe(Blueprint))

	// Bring toolkit back to front, cause UMGAEditorSubsystem will close any GE referencers and re-open
	// And the re-open will always focus the last Gameplay Effect BP, this focus window will happen after and make sure we don't loose focus
	// when we click the Compile button (but won't handle compile "in background" when hitting Play and some BP are automatically compiled by the editor)
	FocusWindow();
}

void FMGABlueprintEditor::InitToolMenuContext(FToolMenuContext& MenuContext)
{
	FBlueprintEditor::InitToolMenuContext(MenuContext);
}

FName FMGABlueprintEditor::GetToolkitFName() const
{
	return FName("MGABlueprintEditor");
}

FText FMGABlueprintEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AttributeSetEditorAppLabel", "Gameplay Attributes Editor");
}

FText FMGABlueprintEditor::GetToolkitToolTipText() const
{
	const UObject* EditingObject = GetEditingObject();

	check(EditingObject != nullptr);

	return GetToolTipTextForObject(EditingObject);
}

FString FMGABlueprintEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("BlueprintAttributeSetEditor");
}

FLinearColor FMGABlueprintEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FMGABlueprintEditor::GetDocumentationLink() const
{
	return FBlueprintEditor::GetDocumentationLink(); // todo: point this at the correct documentation
}

TWeakObjectPtr<UObject> FMGABlueprintEditor::GetLastPinSubCategoryObject() const
{
	return LastPinSubCategoryObject;
}

void FMGABlueprintEditor::SetLastPinSubCategoryObject(const TWeakObjectPtr<UObject>& InLastPinSubCategoryObject)
{
	LastPinSubCategoryObject = InLastPinSubCategoryObject;
}

bool FMGABlueprintEditor::GetLastReplicatedState() const
{
	return bLastReplicatedState;
}

void FMGABlueprintEditor::SetLastReplicatedState(const bool bInLastReplicatedState)
{
	bLastReplicatedState = bInLastReplicatedState;
}

FString FMGABlueprintEditor::GetLastUsedVariableName() const
{
	return LastUsedVariableName;
}

void FMGABlueprintEditor::SetLastUsedVariableName(const FString& InLastUsedVariableName)
{
	LastUsedVariableName = InLastUsedVariableName;
}

void FMGABlueprintEditor::FillToolbar(FToolBarBuilder& InToolbarBuilder)
{
	InToolbarBuilder.BeginSection(TEXT("BlueprintAttributes"));
	{
		InToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP(this, &FMGABlueprintEditor::GenerateToolbarMenu),
			LOCTEXT("ToolbarAddLabel", "Add Attribute"),
			LOCTEXT("ToolbarAddToolTip", "Create a new Attribute"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
			false
		);

		InToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FMGABlueprintEditor::CreateDataTableWindow)),
			NAME_None,
			LOCTEXT("ToolbarGenerateDataTableLabel", "Create DataTable"),
			LOCTEXT("ToolbarGenerateDataTableTooltip", "Automatically generate a DataTable from this Blueprint Attributes properties"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.DataTable")
		);

		InToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FMGABlueprintEditor::CreateAttributeWizard)),
			NAME_None,
			LOCTEXT("ToolbarGenerateCPPLabel", "Generate Equivalent C++"),
			LOCTEXT(
				"ToolbarGenerateCPPTooltip",
				"Provides a preview of what this class could look like in C++, "
				"and the ability to generate C++ header / source files from this Blueprint."
			),
			FSlateIcon(FMGAEditorStyle::Get().GetStyleSetName(), "Icons.HeaderView")
		);
	}
	InToolbarBuilder.EndSection();
}

TSharedRef<SWidget> FMGABlueprintEditor::GenerateToolbarMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	TSharedRef<FMGANewAttributeViewModel> ViewModel = MakeShared<FMGANewAttributeViewModel>();
	ViewModel->SetCustomizedBlueprint(GetBlueprintObj());
	ViewModel->SetVariableName(LastUsedVariableName);
	ViewModel->SetbIsReplicated(bLastReplicatedState);

	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
	PinType.PinSubCategory = NAME_None;
	PinType.PinSubCategoryObject = LastPinSubCategoryObject.IsValid() ? LastPinSubCategoryObject.Get() : FGameplayAttributeData::StaticStruct();
	ViewModel->SetPinType(PinType);

	TSharedRef<SMGANewAttributeVariableWidget> Widget = SNew(SMGANewAttributeVariableWidget, ViewModel)
		.OnCancel_Static(&FMGABlueprintEditor::HandleAttributeWindowCancel)
		.OnFinish(this, &FMGABlueprintEditor::HandleAttributeWindowFinish);

	MenuBuilder.AddWidget(Widget, FText::GetEmpty());
	return MenuBuilder.MakeWidget();
}

void FMGABlueprintEditor::HandleAttributeWindowCancel(const TSharedPtr<FMGANewAttributeViewModel>& InViewModel)
{
	check(InViewModel.IsValid());
}

void FMGABlueprintEditor::HandleAttributeWindowFinish(const TSharedPtr<FMGANewAttributeViewModel>& InViewModel)
{
	check(InViewModel.IsValid());

	LastPinSubCategoryObject = InViewModel->GetPinType().PinSubCategoryObject;
	bLastReplicatedState = InViewModel->GetbIsReplicated();
	LastUsedVariableName = InViewModel->GetVariableName();

	SMGANewAttributeVariableWidget::AddMemberVariable(
		GetBlueprintObj(),
		InViewModel->GetVariableName(),
		InViewModel->GetPinType(),
		InViewModel->GetDescription(),
		InViewModel->GetbIsReplicated()
	);
}

void FMGABlueprintEditor::CreateAttributeWizard()
{
	const FMGAAttributeWindowArgs WindowArgs;
	const FAssetData AssetData(GetBlueprintObj());
	if (!AttributeWizardWindow.IsValid())
	{
		AttributeWizardWindow = IMGAScaffoldModule::Get().CreateAttributeWizard(AssetData, WindowArgs);
		AttributeWizardWindow->SetOnWindowClosed(FOnWindowClosed::CreateSP(this, &FMGABlueprintEditor::HandleAttributeWizardClosed));
	}
	else
	{
		AttributeWizardWindow->BringToFront();
	}
}

// ReSharper disable once CppParameterNeverUsed
void FMGABlueprintEditor::HandleAttributeWizardClosed(const TSharedRef<SWindow>& InWindow)
{
	if (AttributeWizardWindow.IsValid())
	{
		AttributeWizardWindow.Reset();
	}
}

void FMGABlueprintEditor::CreateDataTableWindow()
{
	if (!DataTableWindow.IsValid())
	{
		const FMGADataTableWindowArgs WindowArgs;
		DataTableWindow = IMGAEditorModule::Get().CreateDataTableWindow(GetBlueprintObj(), WindowArgs);
		DataTableWindow->SetOnWindowClosed(FOnWindowClosed::CreateSP(this, &FMGABlueprintEditor::HandleDataTableWindowClosed));
	}
	else
	{
		DataTableWindow->BringToFront();
	}
}

// ReSharper disable once CppParameterNeverUsed
void FMGABlueprintEditor::HandleDataTableWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	if (DataTableWindow.IsValid())
	{
		DataTableWindow.Reset();
	}
}

#undef LOCTEXT_NAMESPACE
