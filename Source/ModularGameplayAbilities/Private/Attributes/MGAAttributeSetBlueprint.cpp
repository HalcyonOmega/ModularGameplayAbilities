// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Attributes/MGAAttributeSetBlueprint.h"

#include "MGADelegates.h"
#include "ModularGameplayAbilitiesLogChannels.h"
#include "Misc/EngineVersionComparison.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "RigVMDeveloperTypeUtils.h"
#endif

UMGAAttributeSetBlueprint::~UMGAAttributeSetBlueprint()
{
	MGA_LOG(VeryVerbose, TEXT("UMGAAttributeSetBlueprint::~UMGAAttributeSetBlueprint - Destructor"))
#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectModified.RemoveAll(this);
	OnChanged().RemoveAll(this);
	OnCompiled().RemoveAll(this);
#endif
}

void UMGAAttributeSetBlueprint::PostLoad()
{
	Super::PostLoad();
#if WITH_EDITOR
	RegisterDelegates();
#endif
}

#if WITH_EDITOR

void UMGAAttributeSetBlueprint::RegisterDelegates()
{
	if (IsTemplate())
	{
		return;
	}
	
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::RegisterDelegates - Setup delegates for %s"), *GetName())

	FCoreUObjectDelegates::OnObjectModified.RemoveAll(this);
	OnChanged().RemoveAll(this);
	OnCompiled().RemoveAll(this);

	FCoreUObjectDelegates::OnObjectModified.AddUObject(this, &UMGAAttributeSetBlueprint::OnPreVariableChange);
	OnChanged().AddUObject(this, &UMGAAttributeSetBlueprint::OnPostVariableChange);
	OnCompiled().AddUObject(this, &UMGAAttributeSetBlueprint::OnPostCompiled);
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UMGAAttributeSetBlueprint::OnPreVariableChange(UObject* InObject)
{
	if (InObject != this)
	{
		return;
	}

	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnPreVariableChange %s"), *GetNameSafe(InObject))

	LastNewVariables = NewVariables;
}

void UMGAAttributeSetBlueprint::HandleVariableChanges(const UBlueprint* InBlueprint)
{
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnPostVariableChange %s"), *GetNameSafe(InBlueprint))

	TMap<FGuid, int32> NewVariablesByGuid;
	for (int32 VarIndex = 0; VarIndex < NewVariables.Num(); VarIndex++)
	{
		NewVariablesByGuid.Add(NewVariables[VarIndex].VarGuid, VarIndex);
	}

	TMap<FGuid, int32> OldVariablesByGuid;
	for (int32 VarIndex = 0; VarIndex < LastNewVariables.Num(); VarIndex++)
	{
		OldVariablesByGuid.Add(LastNewVariables[VarIndex].VarGuid, VarIndex);
	}

	for (const FBPVariableDescription& OldVariable : LastNewVariables)
	{
		if (!NewVariablesByGuid.Contains(OldVariable.VarGuid))
		{
			OnVariableRemoved(OldVariable.VarName);
		}
	}

	for (const FBPVariableDescription& NewVariable : NewVariables)
	{
		if (!OldVariablesByGuid.Contains(NewVariable.VarGuid))
		{
			OnVariableAdded(NewVariable.VarName);
			continue;
		}

		const int32 OldVarIndex = OldVariablesByGuid.FindChecked(NewVariable.VarGuid);
		const FBPVariableDescription& OldVariable = LastNewVariables[OldVarIndex];
		if (OldVariable.VarName != NewVariable.VarName)
		{
			OnVariableRenamed(OldVariable.VarName, NewVariable.VarName);
		}

		if (OldVariable.VarType != NewVariable.VarType)
		{
			OnVariableTypeChanged(NewVariable.VarName, OldVariable.VarType, NewVariable.VarType);
		}
	}

	LastNewVariables = NewVariables;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UMGAAttributeSetBlueprint::OnPostVariableChange(UBlueprint* InBlueprint) const
{
	if (InBlueprint != this)
	{
		return;
	}

	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnPostVariableChange - %s"), *GetNameSafe(InBlueprint))
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnPostVariableChange - IsPossiblyDirty: %s"), IsPossiblyDirty() ? TEXT("true") : TEXT("false"))
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnPostVariableChange - IsUpToDate: %s"), IsUpToDate() ? TEXT("true") : TEXT("false"))
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UMGAAttributeSetBlueprint::OnPostCompiled(UBlueprint* InBlueprint)
{
	if (InBlueprint != this)
	{
		return;
	}


	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnPostCompiled - %s"), *GetNameSafe(InBlueprint))
	HandleVariableChanges(InBlueprint);

	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnPostCompiled - IsPossiblyDirty: %s"), IsPossiblyDirty() ? TEXT("true") : TEXT("false"))
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnPostCompiled - IsUpToDate: %s"), IsUpToDate() ? TEXT("true") : TEXT("false"))

	if (const UPackage* Package = GetPackage())
	{
		FMGADelegates::OnPostCompile.Broadcast(Package->GetFName());
	}
}

void UMGAAttributeSetBlueprint::OnVariableAdded(const FName& Name)
{
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnVariableAdded Name: %s"), *Name.ToString())
	FMGADelegates::OnVariableAdded.Broadcast(Name);
}

void UMGAAttributeSetBlueprint::OnVariableRemoved(const FName& Name)
{
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnVariableRemoved Name: %s"), *Name.ToString())
	FMGADelegates::OnVariableRemoved.Broadcast(Name);
}

void UMGAAttributeSetBlueprint::OnVariableRenamed(const FName& InOldVarName, const FName& InNewVarName) const
{
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnVariableRenamed InOldVarName: %s, InNewVarName: %s"), *InOldVarName.ToString(), *InNewVarName.ToString())

	const UPackage* Package = GetPackage();
	if (!Package)
	{
		MGA_LOG(Warning, TEXT("UMGAAttributeSetBlueprint::OnVariableRenamed SelfPackage nullptr"))
		return;
	}

	// Broadcast renamed event, the subsystem will handle the rest
	FMGADelegates::OnVariableRenamed.Broadcast(Package->GetFName(), InOldVarName, InNewVarName);
}

void UMGAAttributeSetBlueprint::OnVariableTypeChanged(const FName& InVarName, const FEdGraphPinType& InOldPinType, const FEdGraphPinType& InNewPinType) const
{
	const FString OldVarTypeStr = FString::Printf(TEXT("Category: %s, SubCategory: %s"), *InOldPinType.PinCategory.ToString(), *InOldPinType.PinSubCategory.ToString());
	const FString NewVarTypeStr = FString::Printf(TEXT("Category: %s, SubCategory: %s"), *InNewPinType.PinCategory.ToString(), *InNewPinType.PinSubCategory.ToString());

	FString CPPType;
	UObject* CPPTypeObject = nullptr;
	RigVMTypeUtils::CPPTypeFromPinType(InNewPinType, CPPType, &CPPTypeObject);

	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnVariableTypeChanged InVarName: %s || InOldPinType: %s || InNewPinType: %s"), *InVarName.ToString(), *OldVarTypeStr, *NewVarTypeStr)
	MGA_LOG(Verbose, TEXT("UMGAAttributeSetBlueprint::OnVariableTypeChanged ==> CPPType: %s || CPPTypeObject: %s"), *CPPType, *GetNameSafe(CPPTypeObject))

	FMGADelegates::OnVariableTypeChanged.Broadcast(InVarName, CPPType, CPPTypeObject);
}

#endif