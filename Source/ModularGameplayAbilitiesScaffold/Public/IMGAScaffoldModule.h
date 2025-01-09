// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SWindow.h"

struct FMGAAttributeWindowArgs
{
	FText Title = NSLOCTEXT("IMGAScaffoldModule", "DefaultWindowTitle", "Create new C++ Attribute Set");
	FVector2d Size = FVector2D(1280, 720);
	ESizingRule SizingRule = ESizingRule::UserSized;
	bool bSupportsMinimize = true;
	bool bSupportsMaximize = true;
	bool bIsModal = false;

	FMGAAttributeWindowArgs() = default;
};

/** Module interface for scaffold module */
class IMGAScaffoldModule : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to this module's interface. This is just for convenience!
	 * Beware of calling this during the shutdown phase, though. Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static IMGAScaffoldModule& Get()
	{
		static const FName ModuleName = "ModularGameplayAbilitiesScaffold";
		return FModuleManager::LoadModuleChecked<IMGAScaffoldModule>(ModuleName);
	}

	/**
	 * Checks to see if this module is loaded and ready. It is only valid to call Get() during shutdown if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		static const FName ModuleName = "ModularGameplayAbilitiesScaffold";
		return FModuleManager::Get().IsModuleLoaded(ModuleName);
	}

	/** Creates and returns a new wizard widget to create attribute C++ class from a ModularAttributeSet. */
	virtual TSharedRef<SWindow> CreateAttributeWizard(const FAssetData& InAssetData, const FMGAAttributeWindowArgs& InArgs) const = 0;
};
