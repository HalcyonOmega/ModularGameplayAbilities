// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UGameplayAbility;

struct MODULARGAMEPLAYABILITIES_API FMGADelegates
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FMGAOnVariableAddedOrRemoved, const FName&);
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FMGAOnVariableRenamed, const FName&, const FName&, const FName&);
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FMGAOnVariableTypeChanged, const FName&, FString, UObject*);
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FMGAOnPostCompile, const FName&);
	DECLARE_MULTICAST_DELEGATE_OneParam(FMGAOnPreCompile, const FName&);

	DECLARE_MULTICAST_DELEGATE(FMGAOnRequestDetailsRefresh)

	static FMGAOnVariableAddedOrRemoved OnVariableAdded;
	static FMGAOnVariableAddedOrRemoved OnVariableRemoved;
	static FMGAOnVariableRenamed OnVariableRenamed;
	static FMGAOnVariableTypeChanged OnVariableTypeChanged;
	
	static FMGAOnPreCompile OnPreCompile;
	static FMGAOnPostCompile OnPostCompile;
	
	static FMGAOnRequestDetailsRefresh OnRequestDetailsRefresh;
};
