// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "MGAEditorSettings.h"

#include "MGAEditorLog.h"

UMGAEditorSettings::UMGAEditorSettings()
{
	HeaderFormatText = FText::FromString(TEXT("Base: {0}, Current: {1}"));
}

const UMGAEditorSettings& UMGAEditorSettings::Get()
{
	const UMGAEditorSettings* Settings = GetDefault<UMGAEditorSettings>();
	check(Settings);
	return *Settings;
}

UMGAEditorSettings& UMGAEditorSettings::GetMutable()
{
	UMGAEditorSettings* Settings = GetMutableDefault<UMGAEditorSettings>();
	check(Settings);
	return *Settings;
}

FName UMGAEditorSettings::GetCategoryName() const
{
	return PluginCategoryName;
}

FText UMGAEditorSettings::GetSectionText() const
{
	return NSLOCTEXT("MGAEditorSettings", "SectionText", "Details Customizations");
}

void UMGAEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	MGA_EDITOR_LOG(VeryVerbose, TEXT("UMGAEditorSettings::PostEditChangeProperty ... PropertyThatChanged: %s"), *GetNameSafe(PropertyChangedEvent.Property))
}

bool UMGAEditorSettings::IsAttributeFiltered(const TSet<FString>& InFilterList, const FString& InAttributeName)
{
	MGA_EDITOR_LOG(
		VeryVerbose,
		TEXT("UMGAEditorSettings::IsAttributeFiltered - InAttributeName: %s, InFilterList: %s"),
		*InAttributeName,
		*FString::Join(InFilterList, TEXT(", "))
	)

	for (const FString& FilterItem : InFilterList)
	{
		if (InAttributeName.StartsWith(FilterItem))
		{
			MGA_EDITOR_LOG(Verbose, TEXT("\tUMGADeveloperSettings::IsAttributeFiltered - %s filtered by %s"), *InAttributeName, *FilterItem)
			return true;
		}
	}

	return false;
}
