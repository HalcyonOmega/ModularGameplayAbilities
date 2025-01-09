// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "AssetTypes/MGAAssetTypeActions_AttributeSet.h"

#include "Attributes/ModularAttributeSetBase.h"
#include "AssetTypes/MGABlueprintFactory.h"
#include "Editor/MGABlueprintEditor.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "MGAAssetTypeActions_AttributeSet"

FMGAAssetTypeActions_AttributeSet::FMGAAssetTypeActions_AttributeSet(const EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

void FMGAAssetTypeActions_AttributeSet::OpenAssetEditor(const TArray<UObject*>& InObjects, const TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(*ObjIt))
		{
			const TSharedRef<FMGABlueprintEditor> NewEditor(new FMGABlueprintEditor());

			TArray<UBlueprint*> Blueprints;
			Blueprints.Add(Blueprint);

			NewEditor->InitAttributeSetEditor(Mode, EditWithinLevelEditor, Blueprints, false);
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToLoadAttributeSetBlueprint", "Attribute Set Blueprint could not be loaded because it derives from an invalid class. Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

const TArray<FText>& FMGAAssetTypeActions_AttributeSet::GetSubMenus() const
{
	static const TArray SubMenus
	{
		LOCTEXT("AssetTypeActions_BlueprintAttributesSubMenu", "Attributes")
	};

	return SubMenus;
}

UFactory* FMGAAssetTypeActions_AttributeSet::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	check(InBlueprint && InBlueprint->IsA(UMGAAttributeSetBlueprint::StaticClass()));
	UMGABlueprintFactory* Factory = NewObject<UMGABlueprintFactory>();
	Factory->ParentClass = TSubclassOf<UModularAttributeSetBase>(*InBlueprint->GeneratedClass);
	return Factory;
}

#undef LOCTEXT_NAMESPACE
