// Copyright Chronicler.

using UnrealBuildTool;

public class ModularGameplayAbilities : ModuleRules
{
	public ModularGameplayAbilities(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameFeatures",
				"GameplayAbilities",
				"ModalCamera",
				"ModularGameplay",
				"ModularGameplayExperiences",
				"NetCore"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"EnhancedInput",
				"GameplayMessageRuntime",
				"GameplayTags",
				"GameplayTasks",
				"ModularGameplayActors",
				"ModularGameplayData",
				"ModularGameplayExperiences",
				"NetCore",
				"Slate",
				"SlateCore",
			}
			);
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"BlueprintGraph",
					"EditorFramework",
					"RigVMDeveloper",
					"UnrealEd"
				}
			);
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}
