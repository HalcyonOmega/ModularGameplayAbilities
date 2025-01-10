// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayAbilitiesDeveloper/Public/BlueprintGraph/ReferencerHandlers/FMGASwitchNodeReferencerHandler.h"

#include "BlueprintGraph/MGAK2Node_SwitchGameplayAttribute.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "Subsystems/MGAEditorSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogModularGameplayAbilitiesDeveloper, Display, All);
DEFINE_LOG_CATEGORY(LogModularGameplayAbilitiesDeveloper);

#define NS_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogModularGameplayAbilitiesDeveloper, Verbosity, TEXT("%s - %s"), *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__)); \
}

TSharedPtr<IMGAAttributeReferencerHandler> FMGASwitchNodeReferencerHandler::Create()
{
	return MakeShared<FMGASwitchNodeReferencerHandler>();
}

FMGASwitchNodeReferencerHandler::~FMGASwitchNodeReferencerHandler()
{
	PinAttributesCacheMap.Reset();
}

void FMGASwitchNodeReferencerHandler::OnPreCompile(const FString& InPackageName)
{
	PinAttributesCacheMap.Reset();
}

void FMGASwitchNodeReferencerHandler::OnPostCompile(const FString& InPackageName)
{
}

bool FMGASwitchNodeReferencerHandler::HandlePreCompile(const FAssetIdentifier& InAssetIdentifier, const FMGAAttributeReferencerPayload& InPayload)
{
	NS_LOG(Verbose, TEXT("InAssetIdentifier: %s, InPayload: %s"), *InAssetIdentifier.ToString(), *InPayload.ToString())

	const UMGAK2Node_SwitchGameplayAttribute* Node = Cast<UMGAK2Node_SwitchGameplayAttribute>(InPayload.DefaultObject);
	if (!Node)
	{
		return false;
	}

	TArray<FGameplayAttribute> GameplayAttributes = Node->PinAttributes;
	
	TArray<FAttributeReference> AttributesCache;
	AttributesCache.Reserve(Node->PinAttributes.Num());

	int32 CurrentIndex = 0;
	for (const FGameplayAttribute& Attribute : Node->PinAttributes)
	{
		++CurrentIndex;
		
		if (!Attribute.IsValid())
		{
			continue;
		}

		FString FinalValue;
		FGameplayAttribute::StaticStruct()->ExportText(FinalValue, &Attribute, &Attribute, nullptr, PPF_SerializedAsImportText, nullptr);
		if (FinalValue.IsEmpty())
		{
			continue;
		}

		FAttributeReference Reference;
		if (UMGAEditorSubsystem::ParseAttributeFromDefaultValue(FinalValue, Reference.PackageNameOwner, Reference.AttributeName))
		{
			Reference.Index = CurrentIndex  - 1;
			AttributesCache.Add(Reference);
		}
	}

	PinAttributesCacheMap.Add(InAssetIdentifier, AttributesCache);
	return true;
}

bool FMGASwitchNodeReferencerHandler::HandleAttributeRename(const FAssetIdentifier& InAssetIdentifier, const FMGAAttributeReferencerPayload& InPayload, TArray<TSharedRef<FTokenizedMessage>>& OutMessages)
{
	NS_LOG(Verbose, TEXT("InAssetIdentifier: %s, InPayload: %s"), *InAssetIdentifier.ToString(), *InPayload.ToString())
	
	UMGAK2Node_SwitchGameplayAttribute* Node = Cast<UMGAK2Node_SwitchGameplayAttribute>(InPayload.DefaultObject);
	if (!Node)
	{
		return false;
	}

	const UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *InPayload.PackageName);
	if (!Blueprint)
	{
		NS_LOG(Warning, TEXT("FMGASwitchNodeReferencerHandler::HandleAttributeRename - Failed to update pin attributes because of invalid Blueprint for %s"), *InPayload.PackageName)
		return false;
	}

	int32 CurrentIndex = 0;
	TArray<FPinAttributeToReplace> PinAttributesToReplace;
	for (const FGameplayAttribute& Attribute : Node->PinAttributes)
	{
		FAttributeReference CachedAttribute;
		if (!GetCachedAttributeForIndex(InAssetIdentifier, CurrentIndex, CachedAttribute))
		{
			CurrentIndex++;
			continue;
		}

		if (!Attribute.IsValid() && CachedAttribute.AttributeName == InPayload.OldPropertyName)
		{
			if (!Blueprint->GeneratedClass)
			{
				CurrentIndex++;
				continue;
			}

			if (FProperty* Prop = FindFProperty<FProperty>(Blueprint->GeneratedClass, FName(*InPayload.NewPropertyName)))
			{
				PinAttributesToReplace.Add(FPinAttributeToReplace(CurrentIndex, Prop));
			}
		}

		CurrentIndex++;
	}

	// Now that we have the list of modifiers that needs an update, update modifiers
	for (const FPinAttributeToReplace& ModifierToReplace : PinAttributesToReplace)
	{
		const int32 Index = ModifierToReplace.Index;

		if (!Node->PinAttributes.IsValidIndex(Index))
		{
			continue;
		}

		Node->PinAttributes[Index] = FGameplayAttribute(ModifierToReplace.Property.Get());
	}

	if (!PinAttributesToReplace.IsEmpty())
	{
		Node->ReconstructNode();
		if (Node->GetGraph())
		{
			Node->GetGraph()->NotifyGraphChanged();
		}
	}

	return !PinAttributesToReplace.IsEmpty();
}

bool FMGASwitchNodeReferencerHandler::GetCachedAttributeForIndex(const FAssetIdentifier& InAssetIdentifier, int32 InIndex, FAttributeReference& OutAttributeReference)
{
	if (!PinAttributesCacheMap.Contains(InAssetIdentifier))
	{
		return false;
	}

	TArray<FAttributeReference> Attributes = PinAttributesCacheMap.FindChecked(InAssetIdentifier);
	FAttributeReference* FoundElement = Attributes.FindByPredicate([InIndex](const FAttributeReference& Item)
	{
		return Item.Index == InIndex;
	});

	if (!FoundElement)
	{
		return false;
	}

	OutAttributeReference = MoveTemp(*FoundElement);
	return true;
}

#undef NS_LOG