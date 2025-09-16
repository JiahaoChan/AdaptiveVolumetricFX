// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VolumetricFXRendering : ModuleRules
{
	public VolumetricFXRendering(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
        
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
        
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"MaterialShaderQualitySettings",
			}
			);
        
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Renderer",
				"RenderCore",
				"RHI",
				"Projects",
			}
			);
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("TargetPlatform");
			
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"UnrealEd",
					"MaterialUtilities",
					"SlateCore",
					"Slate"
				}
			);
			
			CircularlyReferencedDependentModules.AddRange(
				new string[] {
					"UnrealEd",
					"MaterialUtilities",
				}
			);
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}