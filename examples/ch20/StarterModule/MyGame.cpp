// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#include "MyGame.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogMyGame);

// The module name here must match the folder, the Build.cs class and the .uproject entry.
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, MyGame, "MyGame");
