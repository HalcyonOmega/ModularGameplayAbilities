// Copyright Halcyonyx Studios.

using UnrealBuildTool;
using System.IO;

public class ModularGameplayAbilitiesEditor : ModuleRules
{
    public ModularGameplayAbilitiesEditor(ReadOnlyTargetRules Target) : base(Target)
    {
		// PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		ShortName = "MGAEditor";
		
		var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

		// NOTE: General rule is not to access the private folder of another module,
		// but to use SPathPicker (used in SMGANewDataTableWindowContent), we need to include some private headers
		PrivateIncludePaths.AddRange(
			new string[]
			{
				// For access to SPathPicker (used in SMGANewDataTableWindowContent)
				Path.Combine(EngineDir, @"Source/Editor/ContentBrowser/Private")
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetTools",
				"ModularGameplayAbilities",
				"Core",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ModularGameplayAbilitiesEditorCommon",
				"ModularGameplayAbilitiesScaffold",
				"BlueprintGraph",
				"ContentBrowserData",
				"CoreUObject",
				"DataTableEditor",
				"DeveloperSettings",
				"EditorFramework",
				"EditorSubsystem",
				"Engine",
				"GameplayAbilities",
				"GameplayTags",
				"GraphEditor",
				"InputCore",
				"Kismet",
				"KismetWidgets",
				"MessageLog",
				"RigVMDeveloper",
				"Slate",
				"SlateCore",
				"ToolWidgets",
				"UnrealEd",
			}
		);
	}
}