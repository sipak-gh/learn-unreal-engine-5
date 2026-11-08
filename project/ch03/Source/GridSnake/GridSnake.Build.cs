// GridSnake - course project for "Learn Unreal Engine 5".

using UnrealBuildTool;

public class GridSnake : ModuleRules
{
	public GridSnake(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Lets every file include headers relative to the module root,
		// e.g. #include "Core/GSGameMode.h", no matter which folder it lives in.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
