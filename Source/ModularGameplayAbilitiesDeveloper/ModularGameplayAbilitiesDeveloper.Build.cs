// Copyright Halcyonyx Studios.

using UnrealBuildTool;

public class ModularGameplayAbilitiesDeveloper : ModuleRules
{
    public ModularGameplayAbilitiesDeveloper(ReadOnlyTargetRules Target) : base(Target)
    {
        ShortName = "MGADeveloper";

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
                "Engine",
                "GameplayAbilities",
                "ModularGameplayAbilities",
                "Projects",
                "Slate",
                "SlateCore",
            }
        );

        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "BlueprintGraph",
                    "UnrealEd",
                    "ModularGameplayAbilitiesEditor",
                }
            );
        }
    }
}