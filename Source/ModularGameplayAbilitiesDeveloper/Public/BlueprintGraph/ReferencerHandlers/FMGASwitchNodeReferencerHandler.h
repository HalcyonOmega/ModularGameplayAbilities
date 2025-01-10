// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"
#include "ReferencerHandlers/IMGAAttributeReferencerHandler.h"
#include "UObject/WeakFieldPtr.h"

#if UE_VERSION_NEWER_THAN(5, 2, -1)
#include "AssetRegistry/AssetIdentifier.h"
#else
#include "AssetRegistry/AssetData.h"
#endif

/**
 * Handlers for attribute renames on UMGAK2Node_SwitchGameplayAttribute nodes.
 */
class MODULARGAMEPLAYABILITIESDEVELOPER_API FMGASwitchNodeReferencerHandler : public IMGAAttributeReferencerHandler
{
public:
	/** Data holder for a Pin Attribute property that needs an update */
	struct FPinAttributeToReplace
	{
		int32 Index = 0;
		TWeakFieldPtr<FProperty> Property = nullptr;

		FPinAttributeToReplace(const int32 Index, FProperty* Property)
			: Index(Index)
			, Property(Property)
		{
		}
	};
	
	/** Data holder for cache map, representing a reference to a FGameplayAttribute property in a K2SwitchNode PinAttributes */
	struct FAttributeReference
	{
		FString PackageNameOwner;
		FString AttributeName;
		int32 Index = -1;

		FAttributeReference() = default;
	};
	
	static TSharedPtr<IMGAAttributeReferencerHandler> Create();
	virtual ~FMGASwitchNodeReferencerHandler() override;

	//~ Begin IMGAAttributeReferencerHandler
	virtual void OnPreCompile(const FString& InPackageName) override;
	virtual void OnPostCompile(const FString& InPackageName) override;
	virtual bool HandlePreCompile(const FAssetIdentifier& InAssetIdentifier, const FMGAAttributeReferencerPayload& InPayload) override;
	virtual bool HandleAttributeRename(const FAssetIdentifier& InAssetIdentifier, const FMGAAttributeReferencerPayload& InPayload, TArray<TSharedRef<FTokenizedMessage>>& OutMessages) override;
	//~ End IMGAAttributeReferencerHandler
protected:
	TMap<FAssetIdentifier, TArray<FAttributeReference>> PinAttributesCacheMap;
	
	bool GetCachedAttributeForIndex(const FAssetIdentifier& InAssetIdentifier, int32 InIndex, FAttributeReference& OutAttributeReference);
};
