// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Slate/SMGAAttributeSetWizard.h"

#include "MGAScaffoldLog.h"
#include "MGAScaffoldUtils.h"
#include "Slate/SMGAClassInfo.h"
#include "Slate/SMGAHeaderView.h"
#include "Slate/SMGASuggestedIDEWidget.h"
#include "SlateOptMacros.h"
#include "SourceCodeNavigation.h"
#include "Attributes/MGAAttributeSetBlueprintBase.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Blueprint.h"
#include "Misc/MessageDialog.h"
#include "Models/MGAAttributeSetWizardViewModel.h"
#include "Styling/MGAAppStyle.h"
#include "Widgets/Workflow/SWizard.h"

#define LOCTEXT_NAMESPACE "SMGAHeaderView"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

// ReSharper disable once CppParameterNeverUsed
void SMGAAttributeSetWizard::Construct(const FArguments& InArgs, const FAssetData& InAssetData)
{
	const FMargin PaddingAmount = FMargin(18.f);
	const FMargin PaddingInternalAmount = FMargin(0.f, 0.f, 0.f, 0.f);

	ViewModel = MakeShared<FMGAAttributeSetWizardViewModel>(UMGAAttributeSetBlueprintBase::StaticClass());
	ViewModel->Initialize();
	
	AssetData = InAssetData;
	ViewModel->SetNewClassName(GetBlueprintName(InAssetData));

	ViewModel->OnParentClassInfoChanged().AddSP(this, &SMGAAttributeSetWizard::HandleClassInfoChanged);
	ViewModel->OnSelectedModuleInfoChanged().AddSP(this, &SMGAAttributeSetWizard::HandleSelectedModuleInfoChanged);
	ViewModel->OnSelectedBlueprintChanged().AddSP(this, &SMGAAttributeSetWizard::HandleSelectedBlueprintChanged);

	ChildSlot
	[
		SNew(SBorder)
		.Padding(PaddingAmount)
		.BorderImage(FMGAAppStyle::GetBrush(TEXT("Docking.Tab.ContentAreaBrush")))
		[
			SAssignNew(Wizard, SWizard)
			.ShowPageList(false)
			.CanFinish(this, &SMGAAttributeSetWizard::CanFinish)
			.FinishButtonText(LOCTEXT("FinishButtonText_Native", "Create Class"))
			.FinishButtonToolTip(LOCTEXT("FinishButtonToolTip_Native", "Creates the code files to add your new class."))
			.OnCanceled(this, &SMGAAttributeSetWizard::CancelClicked)
			.OnFinished(this, &SMGAAttributeSetWizard::FinishClicked)
			.PageFooter()
			[
				// Get IDE information
				SNew(SBorder)
				.Visibility(this, &SMGAAttributeSetWizard::GetGlobalErrorLabelVisibility)
				.BorderImage(FAppStyle::Get().GetBrush("RoundedError"))
				.Padding(FMargin(8.f))
				.Content()
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.VAlign(VAlign_Top)
					.Padding(0.f, 0.f, 8.f, 0.f)
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Icons.ErrorWithColor"))
					]

					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SMGAAttributeSetWizard::GetGlobalErrorLabelText)
						.AutoWrapText(true)
					]

					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.AutoWidth()
					.Padding(5.f, 0.f)
					[
						SNew(SMGASuggestedIDEWidget)
					]
				]
			]

			// Name class
			+SWizard::Page()
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(PaddingInternalAmount)
				[
					SNew(SMGAClassInfo, ViewModel)
				]

				+SVerticalBox::Slot()
				.Padding(PaddingInternalAmount)
				[
					SAssignNew(HeaderViewWidget, SMGAHeaderView, InAssetData, ViewModel)
				]
			]
		]
	];

	ViewModel->UpdateInputValidity();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMGAAttributeSetWizard::~SMGAAttributeSetWizard()
{
}

void SMGAAttributeSetWizard::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	check(ViewModel.IsValid());
	ViewModel->RunPeriodicValidationOnTick(InCurrentTime);
}

void SMGAAttributeSetWizard::CancelClicked()
{
	CloseContainingWindow();
}

bool SMGAAttributeSetWizard::CanFinish() const
{
	check(ViewModel.IsValid());
	return ViewModel->IsLastInputValidityCheckSuccessful() && FSourceCodeNavigation::IsCompilerAvailable();
}

void SMGAAttributeSetWizard::FinishClicked()
{
	check(CanFinish());
	check(ViewModel.IsValid());

	MGA_SCAFFOLD_LOG(Verbose, TEXT("Finish clicked, generate class now"))

	const FString NewClassName = ViewModel->GetNewClassName();
	const FString SelectedClassPath = ViewModel->GetSelectedClassPath();
	const FString NewClassPath = ViewModel->GetNewClassPath();
	const FString CalculatedClassHeaderName = ViewModel->GetCalculatedClassHeaderName();
	const FString CalculatedClassSourceName = ViewModel->GetCalculatedClassSourceName();
	const TSharedPtr<FModuleContextInfo> SelectedModuleInfo = ViewModel->GetSelectedModuleInfo();

	MGA_SCAFFOLD_LOG(Verbose, TEXT("\t NewClassName: %s"), *NewClassName)
	MGA_SCAFFOLD_LOG(Verbose, TEXT("\t SelectedClassPath: %s"), *SelectedClassPath)
	MGA_SCAFFOLD_LOG(Verbose, TEXT("\t NewClassPath: %s"), *NewClassPath)
	MGA_SCAFFOLD_LOG(Verbose, TEXT("\t CalculatedClassHeaderName: %s"), *CalculatedClassHeaderName)
	MGA_SCAFFOLD_LOG(Verbose, TEXT("\t CalculatedClassSourceName: %s"), *CalculatedClassSourceName)

	if (SelectedModuleInfo.IsValid())
	{
		MGA_SCAFFOLD_LOG(Verbose, TEXT("\t SelectedModuleInfo - ModuleSourcePath: %s"), *SelectedModuleInfo->ModuleSourcePath)
		MGA_SCAFFOLD_LOG(Verbose, TEXT("\t SelectedModuleInfo - ModuleName: %s"), *SelectedModuleInfo->ModuleName)
	}
	else
	{
		MGA_SCAFFOLD_LOG(Error, TEXT("Invalid module info"))
		CloseContainingWindow();
		return;
	}


	const FString HeaderContent = HeaderViewWidget->GetHeaderContent();
	const FString SourceContent = HeaderViewWidget->GetSourceContent();

	const FString HeaderDestination = CalculatedClassHeaderName;
	const FString SourceDestination = CalculatedClassSourceName;

	// Generate Class
	FText ErrorText;
	GameProjectUtils::EReloadStatus HotReloadStatus;
	const GameProjectUtils::EAddCodeToProjectResult Result = FMGAScaffoldUtils::AddClassToProject(
		NewClassName,
		NewClassPath,
		HeaderDestination,
		HeaderContent,
		SourceDestination,
		SourceContent,
		*SelectedModuleInfo.Get(),
		HotReloadStatus,
		ErrorText
	);
	
	if (Result != GameProjectUtils::EAddCodeToProjectResult::Succeeded)
	{
		CloseContainingWindow();
		
		MGA_SCAFFOLD_LOG(Error, TEXT("Failed to generate class: %s"), *ErrorText.ToString())
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("GenFailed", "Failed to generate class - Reason: {0}"), ErrorText));
		return;
	}

	CloseContainingWindow();
}

void SMGAAttributeSetWizard::CloseContainingWindow()
{
	const TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	if (ContainingWindow.IsValid())
	{
		ContainingWindow->RequestDestroyWindow();
	}
}

FString SMGAAttributeSetWizard::GetBlueprintName(const FAssetData& InAssetData)
{
	if (const UBlueprint* Blueprint = Cast<UBlueprint>(InAssetData.GetAsset()))
	{
		return Blueprint->GetName();
	}

	return TEXT("");
}

EVisibility SMGAAttributeSetWizard::GetGlobalErrorLabelVisibility() const
{
	return GetGlobalErrorLabelText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SMGAAttributeSetWizard::GetGlobalErrorLabelText() const
{
	// Check environment first, and warn if compiler is not available
	if (!FSourceCodeNavigation::IsCompilerAvailable())
	{
#if PLATFORM_LINUX
		return FText::Format(LOCTEXT("NoCompilerFoundNewClassLinux", "In order to create a C++ class, you must first install {0}."), FSourceCodeNavigation::GetSuggestedSourceCodeIDE());
#else
		return FText::Format(LOCTEXT("NoCompilerFoundNewClass", "No compiler was found. In order to create C++ code, you must first install {0}."), FSourceCodeNavigation::GetSuggestedSourceCodeIDE());
#endif
	}

	check(ViewModel.IsValid());

	// Then display any errors returned by ClassInfo widget
	if (!ViewModel->IsLastInputValidityCheckSuccessful())
	{
		return ViewModel->GetLastInputValidityErrorText();
	}

	return FText::GetEmpty();
}

void SMGAAttributeSetWizard::HandleClassInfoChanged(const FNewClassInfo& InOldClassInfo, const FNewClassInfo& InNewClassInfo) const
{
	UpdateRequiredModuleDependenciesIfNeeded();
}

void SMGAAttributeSetWizard::HandleSelectedModuleInfoChanged(const TSharedPtr<FModuleContextInfo>& InOldModuleContextInfo, const TSharedPtr<FModuleContextInfo>& InNewModuleContextInfo) const
{
	UpdateRequiredModuleDependenciesIfNeeded();
}

void SMGAAttributeSetWizard::HandleSelectedBlueprintChanged(const TWeakObjectPtr<UBlueprint>& InOldBlueprint, const TWeakObjectPtr<UBlueprint>& InNewBlueprint) const
{
	UpdateRequiredModuleDependenciesIfNeeded();
}

void SMGAAttributeSetWizard::UpdateRequiredModuleDependenciesIfNeeded() const
{
	check(ViewModel.IsValid());

	ViewModel->ResetRequiredModuleDependencies();

	const FNewClassInfo ParentClassInfo = ViewModel->GetParentClassInfo();
	TSharedPtr<FModuleContextInfo> ModuleContextInfo = ViewModel->GetSelectedModuleInfo();
	const TWeakObjectPtr<UBlueprint> Blueprint = ViewModel->GetSelectedBlueprint();
	const FString ParentClassModuleName = FMGAScaffoldUtils::GetContainingModuleName(ParentClassInfo.BaseClass);

	ViewModel->AddRequiredModuleDependency(ParentClassModuleName, LOCTEXT("RequiredParentClassModule", "Parent Class"));

	if (Blueprint.IsValid())
	{
		const bool bHasClampedProperties = FMGAHeaderViewListItem::IsUsingClampedAttributeData(Blueprint->SkeletonGeneratedClass);
		if (bHasClampedProperties)
		{
			ViewModel->AddRequiredModuleDependency(TEXT("BlueprintAttributes"), LOCTEXT("RequiredClampedStructModule", "Using FMGAClampedAttributeData struct"));
		}
	}
	
	ViewModel->UpdateInputValidity();	
}

#undef LOCTEXT_NAMESPACE
