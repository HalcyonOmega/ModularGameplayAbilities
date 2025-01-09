using UnrealBuildTool;

public class ModularGameplayAbilitiesScaffold : ModuleRules
{
    public ModularGameplayAbilitiesScaffold(ReadOnlyTargetRules Target) : base(Target)
    {
        // PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        ShortName = "MGAScaffold";
		
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AppFramework",
                "ApplicationCore",
                "AssetTools",
                "ModularGameplayAbilities",
                "ModularGameplayAbilitiesEditorCommon",
                "BlueprintGraph",
                "CoreUObject",
                "DeveloperSettings",
                "DesktopPlatform",
                "Engine",
                "EngineSettings",
                "GameplayAbilities",
                "GameProjectGeneration",
                "MainFrame",
                "InputCore",
                "Projects",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "ToolWidgets",
                "UnrealEd",
                "WorkspaceMenuStructure",
            }
        );
        		
        if (Target.bWithLiveCoding)
        {
            PrivateIncludePathModuleNames.Add("LiveCoding");
        }
    }
}