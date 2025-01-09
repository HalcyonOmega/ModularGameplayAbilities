// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "MGAScaffoldPreviewSettings.h"
#include "MGAScaffoldModule.h"

FMGAPreviewSyntaxColors::FMGAPreviewSyntaxColors()
	: Comment(0.107023f, 0.124772f, 0.162029f)
	, Error(0.904661f, 0.06301f, 0.06301f)
	, Macro(0.14996f, 0.300544f, 0.83077f)
	, Typename(0.783538f, 0.527115f, 0.198069f)
	, Identifier(0.745404f, 0.14996f, 0.177888f)
	, Keyword(0.564712f, 0.187821f, 0.723055f)
	, String(0.258183f, 0.539479f, 0.068478f)
	, Numeric(0.637597f, 0.323143f, 0.132868f)
{
}

UMGAScaffoldPreviewSettings::UMGAScaffoldPreviewSettings()
{
	
}

FName UMGAScaffoldPreviewSettings::GetCategoryName() const
{
	return PluginCategoryName;
}

FText UMGAScaffoldPreviewSettings::GetSectionText() const
{
	return NSLOCTEXT("ScaffoldPreviewSettings", "SectionText", "Scaffold Preview Settings");
}

void UMGAScaffoldPreviewSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.MemberProperty)
	{
		return;
	}

	const FName PropertyChangedName = PropertyChangedEvent.MemberProperty->GetFName();

	if (PropertyChangedName == GET_MEMBER_NAME_CHECKED(UMGAScaffoldPreviewSettings, FontSize))
	{
		FMGAScaffoldModule::HeaderViewTextStyle.SetFontSize(FontSize);
	}

	if (PropertyChangedName == GET_MEMBER_NAME_CHECKED(UMGAScaffoldPreviewSettings, SelectionColor))
	{
		FMGAScaffoldModule::HeaderViewTableRowStyle.ActiveBrush.TintColor = SelectionColor;
		FMGAScaffoldModule::HeaderViewTableRowStyle.ActiveHoveredBrush.TintColor = SelectionColor;
	}
}
