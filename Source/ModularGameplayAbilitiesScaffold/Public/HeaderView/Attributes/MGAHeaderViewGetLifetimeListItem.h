// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGAHeaderViewListItem.h"

class FMGAAttributeSetWizardViewModel;

/** A header view list item that displays a GetLifetimeReplicatedProps method declaration */
struct FMGAHeaderViewGetLifetimeListItem : public FMGAHeaderViewListItem
{
	explicit FMGAHeaderViewGetLifetimeListItem(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel);
	
	/** Creates a list item for the Header view representing a declaration for the given blueprint */
	static FMGAHeaderViewListItemPtr Create(const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel);

	//~ FHeaderViewListItem Interface
	virtual void ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset) override;
	//~ End FHeaderViewListItem Interface
};
