// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class MODULARGAMEPLAYABILITIESEDITORCOMMON_API FMGAEditorStyle : public FSlateStyleSet
{
public:
	FMGAEditorStyle();
	virtual ~FMGAEditorStyle() override;

	static FMGAEditorStyle& Get()
	{
		static FMGAEditorStyle Inst;
		return Inst;
	}
};
