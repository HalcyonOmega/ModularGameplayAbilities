// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Details/Slate/MGANewAttributeViewModel.h"

FMGANewAttributeViewModel::FMGANewAttributeViewModel()
	: bIsReplicated(true)
{
}

void FMGANewAttributeViewModel::Initialize()
{
}

FString FMGANewAttributeViewModel::ToString() const
{
	return FString::Printf(
		TEXT("VariableName: %s, Description: %s, bIsReplicated: %s, CustomizedBlueprint: %s, PinType: %s"),
		*GetVariableName(),
		*GetDescription(),
		GetbIsReplicated() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(GetCustomizedBlueprint().Get()),
		*GetNameSafe(GetPinType().PinSubCategoryObject.Get())
	);
}
