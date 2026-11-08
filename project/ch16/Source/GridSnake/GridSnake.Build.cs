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
			"DeveloperSettings",
			"Paper2D",
			"UMG"
		});

		// Used only inside our .cpp files: the loading screen (MoviePlayer) and
		// Slate, the UI framework that UMG and the loading screen are built on.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"MoviePlayer",
			"Slate",
			"SlateCore"
		});

		// Lets every file include headers relative to the module root,
		// e.g. #include "Core/GSGameMode.h", no matter which folder it lives in.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
