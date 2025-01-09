// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "AttributeSet.h"
#include "EdGraphUtilities.h"
#include "EdGraphSchema_K2.h"
#include "SMGAGameplayAttributeGraphPin.h"
#include "SGraphPin.h"

class FMGAGraphPanelPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* InPin) const override
	{
		// TODO: Pass down containing owner to SMGAGameplayAttributeGraphPin to be able to list only owned attributes
		// FBlueprintEditorUtils::FindBlueprintForNode(UEdGraphNode*)
		if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct
			&& InPin->PinType.PinSubCategoryObject == FGameplayAttribute::StaticStruct()
			&& InPin->Direction == EGPD_Input)
		{
			return SNew(SMGAGameplayAttributeGraphPin, InPin);
		}
		
		return nullptr;
	}
};
