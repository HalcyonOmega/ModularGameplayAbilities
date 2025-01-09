// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Utilities/MGAEditorUtils.h"

#include "AttributeSet.h"
#include "DetailLayoutBuilder.h"
#include "Editor.h"
#include "Editor/MGABlueprintEditor.h"
#include "Misc/EngineVersionComparison.h"
#include "Subsystems/AssetEditorSubsystem.h"

UAttributeSet* UE::MGA::EditorUtils::GetAttributeBeingCustomized(const IDetailLayoutBuilder& InDetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	InDetailLayout.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	TArray<UAttributeSet*> AttributeSetsBeingCustomized;
	for (TWeakObjectPtr<UObject> ObjectBeingCustomized : ObjectsBeingCustomized)
	{
		if (UAttributeSet* AttributeSet = Cast<UAttributeSet>(ObjectBeingCustomized.Get()))
		{
			AttributeSetsBeingCustomized.Add(AttributeSet);
		}
	}

	return AttributeSetsBeingCustomized.IsValidIndex(0) ? AttributeSetsBeingCustomized[0] : nullptr;
}

UBlueprint* UE::MGA::EditorUtils::GetBlueprintFromClass(const UClass* InClass)
{
	if (!InClass)
	{
		return nullptr;
	}

	return Cast<UBlueprint>(InClass->ClassGeneratedBy);
}

TWeakPtr<FMGABlueprintEditor> UE::MGA::EditorUtils::FindBlueprintEditorForAsset(UObject* InObject)
{
	if (!GEditor || !IsValid(InObject))
	{
		return nullptr;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!IsValid(AssetEditorSubsystem))
	{
		return nullptr;
	}

	IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(InObject, false);
	if (!EditorInstance)
	{
		return nullptr;
	}

	FMGABlueprintEditor* BlueprintEditor = StaticCast<FMGABlueprintEditor*>(EditorInstance);
	if (!BlueprintEditor)
	{
		return nullptr;
	}
#if UE_VERSION_NEWER_THAN(5, 1, -1)
	return StaticCastWeakPtr<FMGABlueprintEditor>(BlueprintEditor->AsWeak());
#else
	TWeakPtr<FMGABlueprintEditor> WeakPtr(StaticCastSharedRef<FMGABlueprintEditor>(BlueprintEditor->AsShared()));
	return WeakPtr;
#endif
}
