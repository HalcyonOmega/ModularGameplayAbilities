// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGAHeaderViewListItem.h"
#include "Attributes/ModularAttributeSetBase.h"

class FMGAAttributeSetWizardViewModel;
struct FMGAClampedAttributeData;

/** A header view list item that displays a constructor declaration */
struct FMGASourceViewConstructorListItem : public FMGAHeaderViewListItem
{
	explicit FMGASourceViewConstructorListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel);
	
	static FMGAHeaderViewListItemPtr Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel);

	//~ FHeaderViewListItem Interface
	virtual void ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset) override;
	//~ End FHeaderViewListItem Interface

protected:
	static void GetCodeTextsForClampedProperty(const FProperty* InProperty, FString& OutRawItemString, FString& OutRichTextString);

	static void GetCodeTextsForClampDefinition(const FMGAAttributeClampDefinition& InDefinition, const FString& PropertyName, const FString& ClampDefinitionPropertyName, FString& OutRawItemString, FString& OutRichTextString);
};
