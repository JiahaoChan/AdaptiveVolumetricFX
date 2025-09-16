// Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.

using UnrealBuildTool;

public class AdaptiveVolumetricFX : ModuleRules
{
	public AdaptiveVolumetricFX(ReadOnlyTargetRules Target) : base(Target)
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
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"VolumetricFXRendering",
			}
			);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				
			}
			);
	}
}
