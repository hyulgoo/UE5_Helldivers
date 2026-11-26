// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Helldivers : ModuleRules
{
	public Helldivers(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add("Helldivers");

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Niagara", "PhysicsCore", "AIModule", "NavigationSystem", "DeveloperSettings" });

		PrivateDependencyModuleNames.AddRange(new string[] {
            "GameplayAbilities",
            "GameplayTasks",
            "GameplayTags",
            "SlateCore"
        });

        // 런타임에 읽는 표(HDStratagemCatalog·HDUpgradeCatalog). 패키징은 Config 에서 *.ini 만 넣으므로 따로 pak 에 넣는다
        RuntimeDependencies.Add("$(ProjectDir)/Config/HDStratagems.json", StagedFileType.UFS);
        RuntimeDependencies.Add("$(ProjectDir)/Config/HDUpgrades.json", StagedFileType.UFS);

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
