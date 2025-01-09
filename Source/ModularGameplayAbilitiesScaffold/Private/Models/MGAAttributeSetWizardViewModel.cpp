// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Models/MGAAttributeSetWizardViewModel.h"

#include "AttributeSet.h"
#include "MGAScaffoldUtils.h"
#include "SourceCodeNavigation.h"
#include "Framework/Application/SlateApplication.h"

TArray<FMGARequiredModuleDependency> FMGAAttributeSetWizardViewModel::ReservedModuleDependencies = {
	{ TEXT("GameplayAbilities"), NSLOCTEXT("MGAAttributeSetWizardViewModel", "Reserved", "Always included") },
	{ TEXT("GameplayTags"), NSLOCTEXT("MGAAttributeSetWizardViewModel", "Reserved", "Always included") },
	{ TEXT("GameplayTasks"), NSLOCTEXT("MGAAttributeSetWizardViewModel", "Reserved", "Always included") }
};

#define LOCTEXT_NAMESPACE "FMGAAttributeSetWizardViewModel"

FMGAAttributeSetWizardViewModel::FMGAAttributeSetWizardViewModel()
	: ClassLocation(GameProjectUtils::EClassLocation::Public)
	, ParentClassInfo(FNewClassInfo(UAttributeSet::StaticClass()))
	, CurrentPreviewValue(EMGAPreviewCppType::Header)
	, RequiredModuleDependencies(ReservedModuleDependencies)
	, bSatisfiesModuleDependencies(false)
{
}

FMGAAttributeSetWizardViewModel::FMGAAttributeSetWizardViewModel(const FNewClassInfo& InParentClassInfo)
	: ClassLocation(GameProjectUtils::EClassLocation::Public)
	, ParentClassInfo(InParentClassInfo)
	, CurrentPreviewValue(EMGAPreviewCppType::Header)
	, RequiredModuleDependencies(ReservedModuleDependencies)
	, bSatisfiesModuleDependencies(false)
{
}

FMGAAttributeSetWizardViewModel::FMGAAttributeSetWizardViewModel(const UClass* InClass)
	: ClassLocation(GameProjectUtils::EClassLocation::Public)
	, ParentClassInfo(FNewClassInfo(InClass))
	, CurrentPreviewValue(EMGAPreviewCppType::Header)
	, RequiredModuleDependencies(ReservedModuleDependencies)
	, bSatisfiesModuleDependencies(false)
{
}

void FMGAAttributeSetWizardViewModel::Initialize()
{
}

void FMGAAttributeSetWizardViewModel::UpdateInputValidity()
{
	bLastInputValidityCheckSuccessful = true;

	check(SelectedModuleInfo.IsValid());

	// First check for actual source code files and prevent further progress if BP only project
	const bool bProjectHadCodeFiles = GameProjectUtils::ProjectHasCodeFiles();
	if (!bProjectHadCodeFiles)
	{
		bLastInputValidityCheckSuccessful = false;
		LastInputValidityErrorText = LOCTEXT(
			"NewClassError_NoSource",
			"Attribute Wizard cannot be used right away on Blueprint Only project, please first generate an empty C++ class "
			"using engine's file menu \"Tools > New C++ class\".\n\n"
			"It'll generate the necessary Source folder and Game module. Once done, restart the editor and try using "
			"the wizard again."
		);
		return;
	}

	FString NewCalculatedClassHeaderName;
	FString NewCalculatedClassSourceName;
	// Validate the path first since this has the side effect of updating the UI
	bLastInputValidityCheckSuccessful = GameProjectUtils::CalculateSourcePaths(GetNewClassPath(), *SelectedModuleInfo, NewCalculatedClassHeaderName, NewCalculatedClassSourceName, &LastInputValidityErrorText);
	NewCalculatedClassHeaderName /= GetNewClassName() + TEXT(".h");
	NewCalculatedClassSourceName /= GetNewClassName() + TEXT(".cpp");

	SetCalculatedClassHeaderName(NewCalculatedClassHeaderName);
	SetCalculatedClassSourceName(NewCalculatedClassSourceName);

	// If the source paths check as succeeded, check to see if we're using a Public/Private class
	if (bLastInputValidityCheckSuccessful)
	{
		GameProjectUtils::EClassLocation NewClassLocation;
		GameProjectUtils::GetClassLocation(GetNewClassPath(), *SelectedModuleInfo, NewClassLocation);

		// We only care about the Public and Private folders
		if (NewClassLocation != GameProjectUtils::EClassLocation::Public && NewClassLocation != GameProjectUtils::EClassLocation::Private)
		{
			SetClassLocation(GameProjectUtils::EClassLocation::UserDefined);
		}
		else
		{
			SetClassLocation(NewClassLocation);
		}
	}
	else
	{
		SetClassLocation(GameProjectUtils::EClassLocation::UserDefined);
	}

	// Validate the class name only if the path is valid
	if (bLastInputValidityCheckSuccessful)
	{
		const TSet<FString>& DisallowedHeaderNames = FSourceCodeNavigation::GetSourceFileDatabase().GetDisallowedHeaderNames();
		bLastInputValidityCheckSuccessful = GameProjectUtils::IsValidClassNameForCreation(GetNewClassName(), *SelectedModuleInfo, DisallowedHeaderNames, LastInputValidityErrorText);
	}

	// Validate that the class is valid for the currently selected module
	// As a project can have multiple modules, this lets us update the class validity as the user changes the target module
	{
		const UClass* BaseClass = GetParentClassInfo().BaseClass;
		if (bLastInputValidityCheckSuccessful && BaseClass)
		{
			bLastInputValidityCheckSuccessful = GameProjectUtils::IsValidBaseClassForCreation(BaseClass, *SelectedModuleInfo);
			if (!bLastInputValidityCheckSuccessful)
			{
				LastInputValidityErrorText = FText::Format(
					LOCTEXT("NewClassError_InvalidBaseClassForModule", "{0} cannot be used as a base class in the {1} module. Please make sure that {0} is API exported."),
					FText::FromString(BaseClass->GetName()),
					FText::FromString(SelectedModuleInfo->ModuleName)
				);
			}
		}
	}

	// Validate that the currently selected module has required dependencies in its Build.cs
	{
		FText ErrorText;

		TArray<FString> PublicModuleDependencies;
		Algo::Transform(RequiredModuleDependencies, PublicModuleDependencies, [](const FMGARequiredModuleDependency& InModuleDependency)
		{
			return InModuleDependency.ModuleName;
		});
		
		if (!FMGAScaffoldUtils::DoesBuildCSSatisfiesDependencies(*SelectedModuleInfo, PublicModuleDependencies, MissingModuleDependencies, ErrorText))
		{
			if (!ErrorText.IsEmpty())
			{
				bLastInputValidityCheckSuccessful = false;
				LastInputValidityErrorText = FText::Format(
					LOCTEXT("NewClassError_InternalDependencyError", "Internal Error: {0} \n\nPlease reach out the plugin author with a screenshot of the Attribute Wizard Window and the content of the output log."),
					ErrorText
				);
			}
		}

		if (bLastInputValidityCheckSuccessful && !MissingModuleDependencies.IsEmpty())
		{
			const FString Separator = TEXT("\n- ");
			auto JoinByPredicate = [](const FMGARequiredModuleDependency& Item)
			{
				return Item.ToString();
			};
			
			const FString RequiredModuleDependenciesTextComma = FString::JoinBy(RequiredModuleDependencies, *Separator, JoinByPredicate);

			const TArray<FMGARequiredModuleDependency> MissingDependencies = RequiredModuleDependencies.FilterByPredicate(
				[Missing = MissingModuleDependencies](const FMGARequiredModuleDependency& Item)
				{
					return Missing.Contains(Item.ModuleName);
				}
			);
			
			const FString MissingModuleDependenciesText = FString::JoinBy(MissingDependencies, *Separator, JoinByPredicate);
			
			bLastInputValidityCheckSuccessful = false;
			LastInputValidityErrorText = FText::Format(
				LOCTEXT(
					"NewClassError_InvalidBuildCSDependencies",
					// "Please update {2}.Build.cs file to include the following missing module dependencies:\n{3}\n\n"
					"{0} doesn't have the proper module dependencies defined for the class we are generating. "
					"Please update {0}.Build.cs file to include the following missing module dependencies:\n\n"
					"- {1}\n\n"
					"It is recommended to add them to the \"PublicDependencyModuleNames\" list in {0}.Build.cs file."
					"You can click the \"Required Module Dependencies\" with the warning icon to open {0}.Build.cs file in {2}."
				),
				FText::FromString(SelectedModuleInfo->ModuleName),
				FText::FromString(MissingModuleDependenciesText),
				FSourceCodeNavigation::GetSelectedSourceCodeIDE()
			);
		}

		SetbSatisfiesModuleDependencies(MissingModuleDependencies.IsEmpty());
	}

	LastPeriodicValidityCheckTime = FSlateApplication::Get().GetCurrentTime();

	// Since this function was invoked, periodic validity checks should be re-enabled if they were disabled.
	bPreventPeriodicValidityChecksUntilNextChange = false;

	// Notify caller
	OnUpdateValidityChangedDelegate.Broadcast(bLastInputValidityCheckSuccessful, LastInputValidityErrorText);
}

bool FMGAAttributeSetWizardViewModel::IsLastInputValidityCheckSuccessful() const
{
	return bLastInputValidityCheckSuccessful;
}

FText FMGAAttributeSetWizardViewModel::GetLastInputValidityErrorText() const
{
	return LastInputValidityErrorText;
}

void FMGAAttributeSetWizardViewModel::RunPeriodicValidationOnTick(const double InCurrentTime)
{
	if (!bPreventPeriodicValidityChecksUntilNextChange && InCurrentTime > LastPeriodicValidityCheckTime + PeriodicValidityCheckFrequency)
	{
		UpdateInputValidity();
	}
}

void FMGAAttributeSetWizardViewModel::AddRequiredModuleDependency(const FString& InModuleName, const FText& InReason)
{
	if (SelectedModuleInfo->ModuleName == InModuleName)
	{
		return;
	}

	if (RequiredModuleDependencies.Contains(InModuleName))
	{
		return;
	}

	RequiredModuleDependencies.Add(FMGARequiredModuleDependency(InModuleName, InReason));
	RequiredModuleDependencies.Sort();
}

void FMGAAttributeSetWizardViewModel::ResetRequiredModuleDependencies()
{
	RequiredModuleDependencies.Reset();
	RequiredModuleDependencies.Append(ReservedModuleDependencies);
}

#undef LOCTEXT_NAMESPACE
