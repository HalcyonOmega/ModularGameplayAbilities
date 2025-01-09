// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGAHeaderViewListItem.h"

class FMGAAttributeSetWizardViewModel;

/** A header view list item that displays a gameplay attribute data variable declaration */
struct FMGASourceViewOnRepListItem : public FMGAHeaderViewListItem
{
	FMGASourceViewOnRepListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel, const FProperty& InProperty);
	
	/** Creates a list item for the Header view representing a variable declaration for the given blueprint variable */
	static FMGAHeaderViewListItemPtr Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel, const FProperty& InProperty);

	//~ FHeaderViewListItem Interface
	virtual void ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset) override;
	//~ End FHeaderViewListItem Interface

protected:

	/** None if the name is legal, else holds the name of the variable */
	FName IllegalName = NAME_None;
};
