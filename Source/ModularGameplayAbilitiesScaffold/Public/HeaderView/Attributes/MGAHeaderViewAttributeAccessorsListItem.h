// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGAHeaderViewListItem.h"

/** A header view list item that displays a list of include directives */
struct FMGAHeaderViewAttributesAccessorsListItem : public FMGAHeaderViewListItem
{
	FMGAHeaderViewAttributesAccessorsListItem();

	/** Creates a list item for the Header view representing a declaration for the given blueprint */
	static FMGAHeaderViewListItemPtr Create();

	//~ FHeaderViewListItem Interface
	virtual void ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset) override;
	//~ End FHeaderViewListItem Interface
};
