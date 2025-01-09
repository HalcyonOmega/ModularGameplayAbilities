// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGAHeaderViewListItem.h"

class FMGAAttributeSetWizardViewModel;

/** A header view list item that displays a gameplay attribute data variable declaration */
struct FMGAHeaderViewOnRepListItem : public FMGAHeaderViewListItem
{
	/** Creates a list item for the Header view representing a variable declaration for the given blueprint variable */
	static FMGAHeaderViewListItemPtr Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel, const FProperty& InVarProperty);

	//~ FHeaderViewListItem Interface
	virtual void ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset) override;
	//~ End FHeaderViewListItem Interface

	explicit FMGAHeaderViewOnRepListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel, const FProperty& VarProperty);

protected:
	/** None if the name is legal, else holds the name of the variable */
	FName IllegalName = NAME_None;
};
