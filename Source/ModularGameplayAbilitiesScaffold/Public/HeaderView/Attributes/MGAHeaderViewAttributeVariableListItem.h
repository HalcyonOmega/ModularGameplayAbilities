// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MGAHeaderViewListItem.h"
#include "Types/SlateEnums.h"

struct FGameplayAttributeData;
class FMGAAttributeSetWizardViewModel;
struct FBPVariableDescription;

/** A header view list item that displays a gameplay attribute data variable declaration */
struct FMGAHeaderViewAttributeVariableListItem : public FMGAHeaderViewListItem
{
	FMGAHeaderViewAttributeVariableListItem(const FProperty& InVarProperty, const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel);

	/** Creates a list item for the Header view representing a variable declaration for the given blueprint variable */
	static FMGAHeaderViewListItemPtr Create(const FProperty& InVarProperty, const TSharedPtr<FMGAAttributeSetWizardViewModel>& InViewModel);

	//~ FHeaderViewListItem Interface
	virtual void ExtendContextMenu(FMenuBuilder& InMenuBuilder, TWeakObjectPtr<UObject> InAsset) override;
	//~ End FHeaderViewListItem Interface

protected:
	/** Returns a string containing the specifiers for the UPROPERTY line */
	static FString GetConditionalUPropertySpecifiers(const FProperty& VarProperty);

	bool OnVerifyRenameTextChanged(const FText& InNewName, FText& OutErrorText, TWeakObjectPtr<UObject> WeakAsset) const;
	void OnRenameTextCommitted(const FText& CommittedText, ETextCommit::Type TextCommitType, TWeakObjectPtr<UObject> WeakAsset) const;

	/** None if the name is legal, else holds the name of the variable */
	FName IllegalName = NAME_None;
};
