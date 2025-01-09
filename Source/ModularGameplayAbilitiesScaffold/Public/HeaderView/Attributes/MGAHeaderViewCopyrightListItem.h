// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGAHeaderViewListItem.h"

class FMGAAttributeSetWizardViewModel;

/** A header view list item that displays a GetLifetimeReplicatedProps method declaration */
struct FMGAHeaderViewCopyrightListItem : public FMGAHeaderViewListItem
{
	FMGAHeaderViewCopyrightListItem();
	
	/** Creates a list item for the Header view representing a declaration for the given blueprint */
	static FMGAHeaderViewListItemPtr Create();

	//~ FHeaderViewListItem Interface
	virtual void ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset) override;
	//~ End FHeaderViewListItem Interface

protected:

	/** None if the name is legal, else holds the name of the variable */
	FName IllegalName = NAME_None;
};
