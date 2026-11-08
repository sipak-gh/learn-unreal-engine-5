// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".
// A starting point for your own 2D game. Copy this folder to Source/MyGame/ in a new
// C++ project, then rename "MyGame" and the "MG" prefix to your own (see the chapter).

using UnrealBuildTool;

public class MyGame : ModuleRules
{
	public MyGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"DeveloperSettings",
			"Paper2D"
		});

		// Add these when you build menus with UMG (chapter 13 onwards):
		// PublicDependencyModuleNames.Add("UMG");
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Include headers relative to the module root: #include "Core/MGGameMode.h".
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
