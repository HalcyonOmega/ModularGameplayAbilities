// Copyright Halcyonyx Studios

#include "MGADeveloperModule.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#define LOCTEXT_NAMESPACE "FMGADeveloperModule"

void FMGADeveloperModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FMGADeveloperModule::HandlePostEngineInit);
}

void FMGADeveloperModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	
	if (GEditor)
	{
		UMGAEditorSubsystem::Get().UnregisterReferencerHandler(TEXT("MGAK2Node_SwitchGameplayAttribute"));
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void FMGADeveloperModule::HandlePostEngineInit()
{
	if (GEditor)
	{
		UMGAEditorSubsystem::Get().RegisterReferencerHandler(TEXT("MGAK2Node_SwitchGameplayAttribute"), FMGASwitchNodeReferencerHandler::Create());
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMGADeveloperModule, ModularGameplayAbilitiesDeveloper)