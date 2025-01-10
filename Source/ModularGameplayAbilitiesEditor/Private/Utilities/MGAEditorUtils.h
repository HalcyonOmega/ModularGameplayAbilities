// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FMGABlueprintEditor;
class IDetailLayoutBuilder;
class UAttributeSet;
class UBlueprint;

namespace UE::MGA::EditorUtils
{
	UAttributeSet* GetAttributeBeingCustomized(const IDetailLayoutBuilder& InDetailLayout);
	UBlueprint* GetBlueprintFromClass(const UClass* InClass);
	TWeakPtr<FMGABlueprintEditor> FindBlueprintEditorForAsset(UObject* InObject);
}
