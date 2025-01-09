using UnrealBuildTool;

public class ModularGameplayAbilitiesEditorCommon : ModuleRules
{
    public ModularGameplayAbilitiesEditorCommon(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        ShortName = "MGAEditorCommon";

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "EditorStyle",
                "Engine",
                "GameplayAbilities",
                "Projects",
                "Slate",
                "SlateCore",
            }
        );
    }
}