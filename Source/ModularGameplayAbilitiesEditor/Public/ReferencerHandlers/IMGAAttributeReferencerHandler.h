// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/UObjectBaseUtility.h"
#include "UObject/WeakObjectPtr.h"

class UBlueprint;
class FTokenizedMessage;
struct FAssetIdentifier;

struct MODULARGAMEPLAYABILITIESEDITOR_API FMGAAttributeReferencerPayload
{
	/** UObject class default object container of referencer */
	TWeakObjectPtr<UObject> DefaultObject;

	/** UObject class default object container of referencer */
	TWeakObjectPtr<UBlueprint> ReferencerBlueprint;
	
	/** Original package name of owner AttributeSet (from which an attribute got renamed) */
	FString PackageName;

	/** Value of previous attribute name */
	FString OldPropertyName;
	
	/** Value of new attribute name */
	FString NewPropertyName;

	FMGAAttributeReferencerPayload() = default;

	FString ToString() const
	{
		return FString::Printf(
			TEXT("DefaultObject: %s, PackageName: %s, OldPropertyName: %s, NewPropertyName: %s"),
			*GetNameSafe(DefaultObject.Get()),
			*PackageName,
			*OldPropertyName,
			*NewPropertyName
		);
	}
};

/**
 * Interface for custom referencer handler for a given UObject CDO
 */
class MODULARGAMEPLAYABILITIESEDITOR_API IMGAAttributeReferencerHandler : public TSharedFromThis<IMGAAttributeReferencerHandler>
{
public:
	virtual ~IMGAAttributeReferencerHandler() = default;

	virtual void OnPreCompile(const FString& InPackageName) = 0;
	virtual void OnPostCompile(const FString& InPackageName) = 0;
	
	virtual bool HandlePreCompile(const FAssetIdentifier& InAssetIdentifier, const FMGAAttributeReferencerPayload& InPayload) = 0;
	virtual bool HandleAttributeRename(const FAssetIdentifier& InAssetIdentifier, const FMGAAttributeReferencerPayload& InPayload, TArray<TSharedRef<FTokenizedMessage>>& OutMessages) = 0;
};
