# Chapter 2 — Projects, Modules and the Build

GridSnake is still a Blueprint-only project. This chapter turns it into a C++ project and explains
what that means: the `.uproject` and its modules, the `Build.cs` and `Target.cs` files,
UnrealBuildTool and UnrealHeaderTool, IDE project files, build configurations and Live Coding. Then
you write your first log line and learn Unreal's string types. GridSnake gains its `GridSnake` code
module, a log category, and a C++ game mode, `AGSGameMode`, configured through a Blueprint child
`BP_GSGameMode`. Previous: [Chapter 1](01-hello-unreal.md). Next:
[Chapter 3](03-objects-and-reflection.md) (UObject and reflection).

## Adding C++ to a Blueprint project

A Blueprint project has no `Source/` folder and no compiled code of its own. The editor converts it
the first time you add a C++ class. Close any IDE you have open, then:

1. **Tools > New C++ Class...**. The **Add C++ Class** wizard opens on **Choose Parent Class**.
2. Under **Common Classes**, select **Game Mode Base** and click **Next**.
3. On the **Name Your New Game Mode Base** page:
   - **Name**: `GSGameMode`.
   - **Path**: the wizard proposes `.../GridSnake/Source/GridSnake/`. Append `Core/` so it reads
     `.../GridSnake/Source/GridSnake/Core/`.
   - Leave the **Public**/**Private** class-type toggle unselected if your version shows one. With
     it, the wizard splits headers into `Public/` and sources into `Private/`; GridSnake keeps each
     `.h` next to its `.cpp`.
4. Click **Create Class**.

The wizard then does a lot at once (the exact dialogs vary a little between versions):

- It creates `Source/` with two target files (`GridSnake.Target.cs`, `GridSnakeEditor.Target.cs`),
  a module folder `Source/GridSnake/` with `GridSnake.Build.cs`, `GridSnake.h` and `GridSnake.cpp`,
  and your class files `Core/GSGameMode.h` and `Core/GSGameMode.cpp`.
- It adds a `Modules` entry to `GridSnake.uproject`.
- It generates IDE project files (a Visual Studio solution on Windows).
- It compiles the new module, which is why it needs the toolchain from chapter 1. The first
  compile takes a minute or two. On some versions it instead tells you the module must be compiled
  and asks to open the IDE.

If the compile fails here, the usual cause is a missing toolchain component. Check the Visual
Studio workloads from chapter 1, then try again from the IDE.

### Replace the wizard's files with the course's

The wizard's files work, but the course uses a slightly different layout for the whole project:

- **No export macros.** The wizard writes `class GRIDSNAKE_API AGSGameMode`. `GRIDSNAKE_API`
  exports the class from the module's DLL so *other* modules can use it. GridSnake has a single
  module, so the course drops it everywhere. (If you later split your game into several modules,
  add it back to classes other modules use.)
- **Module-root includes.** The course adds the module folder to the include path, so every file
  writes `#include "Core/GSGameMode.h"` regardless of where it lives.
- **A header comment and Unreal 5.8 include settings** that match the snapshots exactly.

So:

1. **Close the editor.**
2. Delete the project's `Source/` folder and copy `project/ch02/Source/` in its place.
3. Make the `Modules` array of `GridSnake.uproject` match the snapshot (it should already).
4. Regenerate the IDE project files (see [Generating project files and opening the
   IDE](#generating-project-files-and-opening-the-ide)).
5. Build the **GridSnakeEditor** target in **Development Editor** from your IDE and start the editor
   from there.

The rest of this chapter explains each of those files.

## Anatomy of a C++ project

After this chapter the project looks like this (generated folders omitted):

```
GridSnake/
  GridSnake.uproject
  Config/
  Content/
  Source/
    GridSnake.Target.cs           the game target
    GridSnakeEditor.Target.cs     the editor target
    GridSnake/                    the GridSnake module
      GridSnake.Build.cs          how to build the module
      GridSnake.h                 module-wide declarations (the log category)
      GridSnake.cpp               module registration
      Core/
        GSGameMode.h
        GSGameMode.cpp
```

Two words matter here:

- A **module** is a unit of C++ code with its own folder and `Build.cs`. It compiles to one library
  (a DLL in the editor). The engine itself is hundreds of modules: `Core`, `CoreUObject`, `Engine`,
  `Paper2D`, `UMG` and so on. Your game is one more.
- A **target** is something you can build and run: the editor with your modules loaded, or the
  standalone game. A target lists the modules it includes.

### The .uproject Modules array

The wizard added this to `GridSnake.uproject`:

`project/ch02/GridSnake.uproject`

```json
{
	"FileVersion": 3,
	"EngineAssociation": "5.8",
	"Category": "",
	"Description": "",
	"Modules": [
		{
			"Name": "GridSnake",
			"Type": "Runtime",
			"LoadingPhase": "Default",
			"AdditionalDependencies": [
				"Engine"
			]
		}
	],
	"Plugins": [
		{
			"Name": "ModelingToolsEditorMode",
			"Enabled": true,
			"TargetAllowList": [
				"Editor"
			]
		},
		{
			"Name": "Paper2D",
			"Enabled": true
		}
	]
}
```

- `Name` must match the module folder `Source/GridSnake/` and its `GridSnake.Build.cs`.
- `Type: Runtime` means the module is loaded in every target: editor, game and server. An
  `Editor` module would load only in the editor, which is where editor tools live.
- `LoadingPhase: Default` loads it at the usual point during engine start-up, after the engine
  modules. Earlier phases exist for modules that must hook in before the engine's own content
  loads; a game rarely needs them.
- `AdditionalDependencies` lists modules that must be loaded before this one. The wizard always adds
  `Engine`; leave it.

When the editor opens a project with a `Modules` array, it looks for the compiled modules in
`Binaries/`. If they are missing or out of date, it asks:

> The following modules are missing or built with a different engine version: GridSnake.
> Would you like to rebuild them now?

**Yes** builds them with UnrealBuildTool, which works for small changes. When the build fails it
only tells you to build from source manually, so get used to building from the IDE instead.

### Target.cs: what to build

`project/ch02/Source/GridSnakeEditor.Target.cs`

```csharp
// GridSnake - course project for "Learn Unreal Engine 5".

using UnrealBuildTool;
using System.Collections.Generic;

public class GridSnakeEditorTarget : TargetRules
{
	public GridSnakeEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("GridSnake");
	}
}
```

`project/ch02/Source/GridSnake.Target.cs`

```csharp
// GridSnake - course project for "Learn Unreal Engine 5".

using UnrealBuildTool;
using System.Collections.Generic;

public class GridSnakeTarget : TargetRules
{
	public GridSnakeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("GridSnake");
	}
}
```

These are C# files. UnrealBuildTool compiles and runs them to learn what to build; they are not
part of your game.

- The **file name** is the target name: `GridSnakeEditor.Target.cs` defines the target
  `GridSnakeEditor`. The class name is the target name plus `Target`.
- `Type` is the kind of program. `TargetType.Editor` builds your modules as DLLs for the editor
  (`UnrealEditor`). `TargetType.Game` builds a standalone game executable, used for packaging in
  chapter 20. (`Client` and `Server` exist for multiplayer.)
- `DefaultBuildSettings` pins a set of build defaults (warning levels, C++ standard, include
  behaviour) to a known version. Without it, new engine releases could change how your code
  compiles. `V7` is the newest set in 5.8.
- `IncludeOrderVersion` pins the order and set of headers the engine's own headers include. Epic
  trims those includes over time, which can break code that relied on a header pulling in another.
  `Unreal5_8` opts in to 5.8's include set.
- `ExtraModuleNames` adds your module to the target.

The two version settings are the most version-specific lines in the project. The 5.8 *Target
reference* documents both properties; `V7` and `Unreal5_8` are the newest values of their enums in
5.8 (each enum also has a `Latest` value that moves with every engine release, which is exactly what
pinning avoids). If your build reports that `V7` or `Unreal5_8` does not exist, you are not on 5.8:
use the values your own wizard wrote.

### Build.cs: how to build the module

`project/ch02/Source/GridSnake/GridSnake.Build.cs`

```csharp
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
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Lets every file include headers relative to the module root,
		// e.g. #include "Core/GSGameMode.h", no matter which folder it lives in.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
```

This is the file you will edit most often as the project grows, because every engine feature you
use lives in a module you must list here.

**Dependencies.** A dependency does two things: it adds the other module's public headers to your
include path, and it links your module against that module's library.

- `Core`: containers, strings, math, logging.
- `CoreUObject`: the object system and reflection (chapter 3).
- `Engine`: actors, worlds, components, the Gameplay Framework.
- `InputCore` and `EnhancedInput`: keys and the input system. The wizard adds them; GridSnake uses
  them from chapter 8.

Later chapters add `Paper2D`, `UMG`, `DeveloperSettings` and a few more, each when it is first
needed.

**Public or private?** `PublicDependencyModuleNames` is for modules whose types appear in *your
public headers*. Anyone who depends on your module then gets those include paths too, because they
will include your headers. `PrivateDependencyModuleNames` is for modules used only inside your
`.cpp` files. In a single-module game the difference rarely matters, but the rule keeps build times
down in larger projects. GridSnake lists the basic modules as public, as the wizard does, and adds
later modules where they belong.

**PCHUsage.** A *precompiled header* (PCH) is a large set of headers compiled once and reused by
every `.cpp` file, which saves a lot of build time. `UseExplicitOrSharedPCHs` means: use this
module's own PCH if it names one (`PrivatePCHHeaderFile`), otherwise reuse a *shared* PCH built for
the engine modules you depend on. It also means every file should include what it uses (the
*IWYU*, "include what you use", style), because no single header pulls in everything.

The shared PCH sometimes hides a missing include. For example, `GSGameMode.cpp` below calls
`GetWorld()->GetMapName()`, which needs the full `UWorld` type from `Engine/World.h`. Without that
include the file would still compile today, because the shared Engine PCH already contains
`UWorld`, but it would break the day the PCH changes or someone builds without PCHs (the error is
*use of undefined type 'UWorld'*). So GridSnake includes `Engine/World.h` explicitly, as do the
chapter's examples. When the compiler complains about an undefined or incomplete type, the fix is
almost always a missing include.

**PublicIncludePaths.** `ModuleDirectory` is the folder of this `Build.cs`. Adding it to the
include path lets `Snake/GSSnake.cpp` include `Core/GSGameMode.h` without `../` paths. The wizard's
default instead relies on `Public/`/`Private/` folders, which GridSnake does not use.

### The module source files

`project/ch02/Source/GridSnake/GridSnake.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"

/** Log category for all GridSnake gameplay code. Filter the Output Log with "LogGridSnake". */
DECLARE_LOG_CATEGORY_EXTERN(LogGridSnake, Log, All);
```

`project/ch02/Source/GridSnake/GridSnake.cpp`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "GridSnake.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, GridSnake, "GridSnake");

DEFINE_LOG_CATEGORY(LogGridSnake);
```

- `CoreMinimal.h` is the small set of basics nearly every Unreal file includes: integer types,
  `FString`, `TArray`, math types, logging macros.
- `IMPLEMENT_PRIMARY_GAME_MODULE` registers the module with the engine's module manager.
  `FDefaultGameModuleImpl` is a default module class that does nothing special on start-up and
  shut-down. The second argument is the module name; the third is the game name. Every project has
  exactly one *primary* game module; additional game modules would use `IMPLEMENT_GAME_MODULE`.
- The log category is declared in the header and defined in the `.cpp`, like any global in C++.
  The next sections explain it.

## UnrealBuildTool and UnrealHeaderTool

Building an Unreal project is not just running a compiler. Two Epic tools run first.

**UnrealBuildTool (UBT)** is the build system. It is a C# program, which is why the Visual Studio
installer needed the .NET workload. When you build `GridSnakeEditor Win64 Development`, UBT:

1. Compiles and runs the `Target.cs` and every relevant `Build.cs` to learn the modules, their
   dependencies, include paths and settings.
2. Runs UnrealHeaderTool on your module's headers.
3. Invokes the platform compiler (MSVC, clang or Apple clang) on every changed `.cpp` and the
   generated code, then the linker.
4. Writes the result to `Binaries/<Platform>/`. For the editor target on Windows that is
   `Binaries/Win64/UnrealEditor-GridSnake.dll`.

UBT also *generates the IDE project files*: a Visual Studio solution or Xcode project is a view of
what UBT knows, not the source of truth. That is why you regenerate project files after adding
files or changing a `Build.cs`, and why the course's `.gitignore` excludes them.

**UnrealHeaderTool (UHT)** reads your headers looking for Unreal's macros: `UCLASS`, `USTRUCT`,
`UENUM`, `UPROPERTY`, `UFUNCTION`, `GENERATED_BODY` and others. For each header that uses them it
writes:

- `GSGameMode.generated.h`, which your header includes. It defines what `GENERATED_BODY()` expands
  to: constructors, the `StaticClass()` function, type information.
- `GSGameMode.gen.cpp`, compiled into your module. It builds the *reflection data*: a runtime
  description of your class, its properties and functions.

They land under `Intermediate/Build/<Platform>/UnrealEditor/Inc/GridSnake/` (in recent versions, in
a `UHT` subfolder). You never edit them. Reflection is what lets the editor show your class in menus,
Blueprints derive from it, the garbage collector track its pointers and the config system fill its
properties. Chapter 3 is all about it.

Now read the game mode's header with that in mind:

`project/ch02/Source/GridSnake/Core/GSGameMode.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameMode.generated.h"

/**
 * The rules of a GridSnake level. Only the server (in single player: the game)
 * has a game mode. BP_GSGameMode derives from this class to pick assets.
 */
UCLASS()
class AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	virtual void StartPlay() override;
};
```

- `#include "GSGameMode.generated.h"` must be the **last** include. UHT checks this.
- `UCLASS()` marks the class for UHT. The empty parentheses take *specifiers*; chapter 3 covers
  them. A `UCLASS` inherits most specifiers from its parent; `AGameModeBase` is `Blueprintable`, so
  `AGSGameMode` is too.
- `GENERATED_BODY()` is replaced by the generated code. It must be the first thing in the class body.
- The `A` prefix is required by Unreal's naming rules for classes derived from `AActor` (`U` for
  other `UObject` classes, `F` for plain structs and classes, `E` for enums, `I` for interfaces, `T`
  for templates). UHT rejects a `UCLASS` whose prefix does not match its base. `GS` is GridSnake's own
  prefix after that letter.

**Unity builds.** To compile faster, UBT may glue several `.cpp` files of a module into one *unity*
file before compiling. A side effect: two `static` helper functions with the same name in different
`.cpp` files of one module can clash. Give file-local helpers distinctive names. The examples in
this chapter do.

### Live Coding and when to restart

Building in the IDE while the editor is closed always works. During the day, though, you want to
change code while the editor stays open. Unreal offers **Live Coding**: it compiles your changes
and patches the running editor (or a running PIE session) in place.

- Trigger it with **Ctrl+Alt+F11**, in the editor or in your IDE, or with the editor's **Compile**
  button (a small icon near the bottom right of the editor window in recent versions; its menu has
  the Live Coding options).
- A separate **Live Coding** console window shows the compiler output; it is not the Output Log. A
  notification in the lower-right corner shows the build's status, ending with "Live coding
  succeeded" on success.
- It is on by default. The settings are in **Edit > Editor Preferences > General > Live Coding**:
  **Enable Live Coding**, **Enable Reinstancing**, and **Automatically Compile Newly Added C++
  Classes** (which is why the class wizard may compile through Live Coding instead of asking you
  to build).
- While the editor runs with Live Coding enabled, building from the IDE is blocked: UBT stops with
  a message telling you to exit the editor or press Ctrl+Alt+F11. That is a safety net, not an
  error in your code.

Live Coding patches function bodies reliably. With Reinstancing it can also handle many header
changes, such as a new `UPROPERTY` on an existing class, but not all of them. One trap: a default
value you change in a *constructor* in the `.cpp` does not update objects that already exist. **Close
the editor and build from the IDE when:**

- you added a new class or a new source file,
- you changed a class's layout in ways Live Coding refuses (it says so),
- you changed a `Build.cs`, a `Target.cs` or the `.uproject`,
- or anything behaves strangely after a patch. Restart when in doubt; it costs a minute, while
  chasing a stale-patch bug can cost an hour.

The older mechanism, **Hot Reload**, rebuilt the whole DLL and reloaded it, and was known for
corrupting Blueprints. It is not used when Live Coding is enabled; leave it that way.

Live Coding has long been a Windows feature. If your macOS or Linux editor has no Live Coding
settings, close the editor, build, and reopen; it starts quickly once the shaders are compiled.

## Generating project files and opening the IDE

### Windows: Visual Studio

1. In the file manager, right-click `GridSnake.uproject` and choose **Generate Visual Studio project
   files** (on Windows 11, first **Show more options**). This writes `GridSnake.sln` (or a `.slnx`
   with newer Visual Studio versions). From inside the editor, **Tools > Refresh Visual Studio
   Project** does the same, and **Tools > Open Visual Studio** opens it.
2. Open the solution. The **Solution Explorer** has an `Engine` folder with `UE5` and a `Games`
   folder with `GridSnake`. Right-click **GridSnake** and choose **Set as Startup Project**.
3. In the toolbar, set the solution configuration to **Development Editor** and the platform to
   **Win64**.
4. Right-click **GridSnake** in the Solution Explorer and choose **Build**. Watch the **Output**
   window (**Show output from: Build**).
5. **Debug > Start Debugging** (F5) starts the editor with your project and the debugger attached.
   **Ctrl+F5** starts it without the debugger, which loads faster.

### Windows: Rider

Rider opens `GridSnake.uproject` directly (**File > Open**), with no solution needed. In the
toolbar pick the **GridSnake** run configuration with **DevelopmentEditor** and your platform, then
build and run from there (labels may differ slightly between Rider versions). Epic's Rider setup
guide lists the plugins Rider installs into the project.

### macOS: Xcode

In the editor, **Tools > Refresh Xcode Project** (or **Generate Xcode Project** if none exists yet)
writes an Xcode workspace and opens it. In Xcode, select the **GridSnakeEditor** scheme and **My
Mac**, then **Product > Build** and **Product > Run**. The scheme's Run configuration chooses
Development or DebugGame.

### Linux, and any platform from the command line

You can always build without an IDE. Replace `<UE>` with your engine folder and use your project
path:

```console
$ <UE>/Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh -project="$HOME/Projects/GridSnake/GridSnake.uproject" -game
$ <UE>/Engine/Build/BatchFiles/Linux/Build.sh GridSnakeEditor Linux Development -project="$HOME/Projects/GridSnake/GridSnake.uproject"
```

The first command writes project files (pass `-vscode` for a VS Code workspace); the second builds
the editor target. On macOS the scripts are under `Engine/Build/BatchFiles/Mac/` with platform
`Mac`. On Windows:

```console
> "<UE>\Engine\Build\BatchFiles\Build.bat" GridSnakeEditor Win64 Development -Project="D:\Projects\GridSnake\GridSnake.uproject"
```

These commands were not run here; their output is UBT's progress (one line per compiled file, then
the link step) and ends with the total time on success.

The build arguments are always **target, platform, configuration**. The configurations you will
meet:

| IDE configuration | Command-line configuration | Use |
|---|---|---|
| **Development Editor** | `GridSnakeEditor ... Development` | The default for daily work. Engine and game optimised, debuggable enough. |
| **DebugGame Editor** | `GridSnakeEditor ... DebugGame` | Your game module unoptimised, the engine optimised. Use it when stepping through your code shows "optimised away" variables. |
| **Debug Editor** | `GridSnakeEditor ... Debug` | Everything unoptimised. Needs a source-built engine; the launcher engine has no Debug binaries. |
| **Development** / **Shipping** | `GridSnake ... Development` / `Shipping` | The standalone game target. Chapter 20 packages a Shipping build. |

DebugGame and Development produce different DLL names, so the editor must be started with the same
configuration you built. If you build DebugGame but double-click the `.uproject`, the editor looks
for the Development DLL and asks to rebuild.

## The first log line

With the module built, you can log. The game mode does it on start:

`project/ch02/Source/GridSnake/Core/GSGameMode.cpp`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameMode.h"
#include "GridSnake.h"
#include "Engine/World.h"

AGSGameMode::AGSGameMode()
{
}

void AGSGameMode::StartPlay()
{
	Super::StartPlay();

	UE_LOG(LogGridSnake, Log, TEXT("GridSnake game mode started on map %s"), *GetWorld()->GetMapName());
}
```

`UE_LOG(Category, Verbosity, Format, Args...)` is the workhorse:

- **Category** (`LogGridSnake`) appears at the start of each line and lets you filter. Declare one
  per module or system: `DECLARE_LOG_CATEGORY_EXTERN` in a header everyone can include, plus one
  `DEFINE_LOG_CATEGORY` in a `.cpp`. For a category used in one file only,
  `DEFINE_LOG_CATEGORY_STATIC` does both. `LogTemp` is a built-in category for quick experiments.
- **Verbosity** is how important the message is (table below).
- **Format** must be wrapped in `TEXT(...)`. Unreal strings are made of `TCHAR`, a wide character
  type on most platforms, and `TEXT` turns a literal into a `TCHAR` literal.
- **Args** follow `printf` rules: `%d` for `int32`, `%f` for `float` and `double`, `%s` for a
  string. `%s` needs a `const TCHAR*`, so an `FString` is passed with a leading `*`:
  `*GetWorld()->GetMapName()`. Passing the `FString` itself is a bug. Recent versions usually
  catch it at compile time with an error about the format arguments; older ones printed garbage or
  crashed.

`Super::StartPlay()` calls the parent's version first. `Super` is a type alias that
`GENERATED_BODY` defines for the parent class; always call it when you override engine functions,
or engine behaviour silently stops. For `AGameModeBase`, `StartPlay` is called once when the world
starts play, and its parent version starts all actors' `BeginPlay`. So this line prints *after* the
`BeginPlay` lines of actors already in the level.

### Verbosity levels

The second argument of `DECLARE_LOG_CATEGORY_EXTERN(LogGridSnake, Log, All)` is the category's
default verbosity at run time: messages up to that level are shown. The third is the compile-time
maximum: messages above it are removed from the build entirely. `All` keeps them all.

| Verbosity | Shown by default (category at `Log`) | Use |
|---|---|---|
| `Fatal` | always, then **crashes** the program | unrecoverable states; prefer `check` (chapter 19) |
| `Error` | yes, red | something failed |
| `Warning` | yes, yellow | something is wrong but play continues |
| `Display` | yes | important messages; also printed to a game's console window |
| `Log` | yes | normal messages; this is GridSnake's default |
| `Verbose` | no | detail for when you are debugging one system |
| `VeryVerbose` | no | floods: per-frame or per-item detail |

You raise a category's verbosity at run time with the console command `Log <Category>
<Verbosity>`, for example `Log LogGridSnake Verbose`. Exercise 1 shows two permanent ways.

Unreal 5.2 and later also offer `UE_LOGFMT` (header `Logging/StructuredLog.h`), which uses named
`{Placeholders}` instead of `printf` specifiers. The course sticks to `UE_LOG`, which you will see in
nearly all existing code.

### Example: logging at every verbosity

The examples in `examples/chNN/` are small classes you drop into *any* UE 5.8 C++ project. To keep
GridSnake identical to the snapshots, the simplest setup is a second project: in the Project
Browser choose **Games > Blank > C++**, name it `Sandbox`, and copy example files into
`Sandbox/Source/Sandbox/`. Alternatively, put them in `GridSnake/Source/GridSnake/Examples/` and
delete them before the next chapter. Either way, close the editor, add the files, build, and start
the editor again: new classes need a full build.

`examples/ch02/ExHelloActor.h`

```cpp
// Chapter 2 example: logging at every verbosity from an actor.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExHelloActor.generated.h"

/** Drop into a level and press Play: logs on BeginPlay and EndPlay. */
UCLASS()
class AExHelloActor : public AActor
{
	GENERATED_BODY()

public:
	AExHelloActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
```

`examples/ch02/ExHelloActor.cpp`

```cpp
// Chapter 2 example: logging at every verbosity from an actor.

#include "ExHelloActor.h"
#include "Engine/World.h"

// A log category visible only in this .cpp file. Default verbosity Log:
// Verbose and VeryVerbose lines are hidden until you raise it.
DEFINE_LOG_CATEGORY_STATIC(LogExHello, Log, All);

AExHelloActor::AExHelloActor()
{
	// This actor only logs; it never needs a per-frame Tick.
	PrimaryActorTick.bCanEverTick = false;
}

void AExHelloActor::BeginPlay()
{
	Super::BeginPlay();

	// %s needs a TCHAR pointer: the * operator turns an FString into one.
	UE_LOG(LogExHello, Log, TEXT("BeginPlay on %s in world %s"), *GetName(), *GetWorld()->GetMapName());
	UE_LOG(LogExHello, Display, TEXT("Display also goes to the console window of a game"));
	UE_LOG(LogExHello, Warning, TEXT("Warnings are yellow"));
	UE_LOG(LogExHello, Error, TEXT("Errors are red"));
	UE_LOG(LogExHello, Verbose, TEXT("Verbose lines are hidden until you raise the verbosity"));
}

void AExHelloActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// UEnum::GetValueAsString finds the reflected enum and returns "Type::Name".
	UE_LOG(LogExHello, Log, TEXT("EndPlay: %s"), *UEnum::GetValueAsString(EndPlayReason));

	Super::EndPlay(EndPlayReason);
}
```

To see it:

1. Build and start the editor. In the Content Browser, open **C++ Classes > Sandbox** (or
   **GridSnake**); `ExHelloActor` is listed there. If the **C++ Classes** folder is missing, enable
   **Settings > Show C++ Classes** in the Content Browser.
2. Drag `ExHelloActor` into the level. It has no components, so it shows only as a small icon and a
   line in the Outliner.
3. Clear the Output Log, set its search box to `LogExHello`, click **Play**, then press **Esc**.

Expected Output Log (filter: `LogExHello`), in PIE on `L_Level01`:

```text
LogExHello: BeginPlay on <ExHelloActor_N> in world UEDPIE_0_L_Level01
LogExHello: Display: Display also goes to the console window of a game
LogExHello: Warning: Warnings are yellow
LogExHello: Error: Errors are red
LogExHello: EndPlay: EEndPlayReason::EndPlayInEditor
```

- `<ExHelloActor_N>` is the actor's object name, assigned when you placed it. The name in the
  Outliner is its *label*, which can differ.
- `Log` lines have no verbosity word; every other level names itself after the category.
- The `Verbose` line is missing. Play again after typing `Log LogExHello Verbose` in the console,
  and it appears as `LogExHello: Verbose: Verbose lines are hidden until you raise the verbosity`.
- `UEDPIE_0_` is a prefix PIE adds to the world name; the next section explains it.
- `EndPlay` runs whenever an actor leaves play. The reason tells you why: `EndPlayInEditor` when PIE
  stops, `Destroyed` when the actor is destroyed, `LevelTransition` when another level opens, and
  `Quit` when the game exits. `UEnum::GetValueAsString` works for any enum that reflection knows,
  because UHT generated its names. Chapter 3 explains how.

### Example: FString, FName and FText

Unreal has three string types, and the log calls above already used two of them. Choosing the right
one is a decision you make constantly:

| Type | What it is | Use it for |
|---|---|---|
| `FString` | A mutable, heap-allocated array of `TCHAR`. Supports `+`, `Printf`, searching, slicing. | Building and manipulating text, logs, file paths, debugging. |
| `FName` | An immutable, case-insensitive identifier. Stored once in a global table; each `FName` is a small index into it. Comparing two is as cheap as comparing integers. | Identifiers: asset and object names, tags, socket names, map keys. |
| `FText` | Display text for players. Localizable (it carries a key into the translation tables) and formats numbers, dates and plurals for the player's culture. | Anything a player reads: HUD, menus, messages. |

`examples/ch02/ExStringTypes.h`

```cpp
// Chapter 2 example: FString, FName and FText, and converting between them.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExStringTypes.generated.h"

/** Drop into a level and press Play: logs one line per string operation. */
UCLASS()
class AExStringTypes : public AActor
{
	GENERATED_BODY()

public:
	AExStringTypes();

protected:
	virtual void BeginPlay() override;
};
```

`examples/ch02/ExStringTypes.cpp`

```cpp
// Chapter 2 example: FString, FName and FText, and converting between them.

#include "ExStringTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogExStrings, Log, All);

// LOCTEXT needs a namespace for the localization keys it creates.
#define LOCTEXT_NAMESPACE "ExStringTypes"

static const TCHAR* ExStrBool(bool bValue)
{
	return bValue ? TEXT("true") : TEXT("false");
}

AExStringTypes::AExStringTypes()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AExStringTypes::BeginPlay()
{
	Super::BeginPlay();

	// FString: a mutable, heap-allocated string for building and editing text.
	const FString Level = TEXT("L_Level01");
	const FString Status = FString::Printf(TEXT("%s has %d gems"), *Level, 3);
	const FString Joined = Level + TEXT(" / ") + FString::FromInt(42);
	UE_LOG(LogExStrings, Log, TEXT("FString: '%s' (Len %d)"), *Status, Status.Len());
	UE_LOG(LogExStrings, Log, TEXT("FString: '%s'"), *Joined);

	// FString == ignores case. Ask for case-sensitive comparison explicitly.
	const FString Upper = Level.ToUpper();
	UE_LOG(LogExStrings, Log, TEXT("FString: == %s, Equals CaseSensitive %s"),
		ExStrBool(Level == Upper), ExStrBool(Level.Equals(Upper, ESearchCase::CaseSensitive)));

	// FName: an immutable, case-insensitive identifier. Comparing two FNames
	// compares two small integers, not the characters.
	const FName GemName(TEXT("Gem"));
	const FName ShoutedName(TEXT("GEM"));
	UE_LOG(LogExStrings, Log, TEXT("FName: Gem == GEM %s, default FName is None %s"),
		ExStrBool(GemName == ShoutedName), ExStrBool(FName().IsNone()));

	// FText: display text for players. It is localizable and formats numbers
	// for the current culture.
	const FText Score = FText::Format(
		LOCTEXT("ScoreFormat", "Score {0}, gems {1}/{2}"),
		FText::AsNumber(12345), FText::AsNumber(2), FText::AsNumber(3));
	UE_LOG(LogExStrings, Log, TEXT("FText: %s"), *Score.ToString());

	// Conversions. FText -> FName is deliberately missing: go through FString.
	const FName NameFromString(*Level);                   // FString -> FName
	const FString StringFromName = GemName.ToString();      // FName -> FString
	const FText TextFromString = FText::FromString(Level);  // FString -> FText
	const FText TextFromName = FText::FromName(GemName);    // FName -> FText
	const FString StringFromText = Score.ToString();        // FText -> FString
	const int32 Parsed = FCString::Atoi(TEXT("42"));        // FString -> int32
	UE_LOG(LogExStrings, Log, TEXT("Convert: %s %s %s %s %d"),
		*NameFromString.ToString(), *StringFromName, *TextFromString.ToString(),
		*TextFromName.ToString(), Parsed);
	UE_LOG(LogExStrings, Log, TEXT("Convert: FText back to FString has Len %d"), StringFromText.Len());
}

#undef LOCTEXT_NAMESPACE
```

Drag `ExStringTypes` into a level and press Play.

Expected Output Log (filter: `LogExStrings`), with the editor's language set to English:

```text
LogExStrings: FString: 'L_Level01 has 3 gems' (Len 20)
LogExStrings: FString: 'L_Level01 / 42'
LogExStrings: FString: == true, Equals CaseSensitive false
LogExStrings: FName: Gem == GEM true, default FName is None true
LogExStrings: FText: Score 12,345, gems 2/3
LogExStrings: Convert: L_Level01 Gem L_Level01 Gem 42
LogExStrings: Convert: FText back to FString has Len 22
```

- **`FString::Printf`** builds a string with the same specifiers as `UE_LOG`. `+` concatenates;
  `FString::FromInt` converts a number. `Len()` counts characters, not bytes.
- **`FString` equality ignores case.** `"L_Level01" == "L_LEVEL01"` is true. This surprises
  everyone once. Use `Equals(Other, ESearchCase::CaseSensitive)` when case matters.
- **`FName` ignores case too**, by design: names identify things, and `Gem` and `GEM` name the same
  thing. A default-constructed `FName` is `NAME_None`, which prints as `None`; Unreal uses it as
  "no name" everywhere.
- **`FText::Format`** takes a pattern with numbered (or named) arguments. `FText::AsNumber` formats
  `12345` for the current culture: `12,345` in English, `12.345` in German. That is why player-facing
  numbers go through `FText`.
- **`LOCTEXT(Key, Text)`** creates a localizable `FText` in the namespace defined by
  `LOCTEXT_NAMESPACE`. The localization tools collect these. Define the namespace before use and
  `#undef` it at the end of the file, because unity builds can merge your `.cpp` with others.
  `FText::FromString` creates *culture-invariant* text that is never translated; use it for text
  that comes from outside, such as a player's name.
- **Conversions** between the three types cost something: building an `FName` from a string looks
  it up in the global table, and `FText` to `FString` loses the localization information. There is no
  direct `FText` to `FName`, on purpose: display text is not an identifier.
- **`ExStrBool`** is a helper for `%s`. It has an unusual name because a `static` function called
  `Bool` in another `.cpp` of the same module could clash in a unity build.

### Filtering the Output Log

Logs become noisy quickly: the engine logs hundreds of lines while starting PIE. Three ways to keep
your lines in view:

- Type the category name (`LogGridSnake`) in the Output Log's search box. This is how the course
  labels every expected log block: *Expected Output Log (filter: `LogGridSnake`)*.
- **Filters > Categories**: untick **Show All**, then tick only your categories.
- **Filters**: untick **Messages** to see only warnings and errors.

Right-click and **Clear Log** before each test run.

## A game mode in C++, configured in a Blueprint

A **game mode** defines the rules of a game: which classes to spawn for players, how a match starts
and ends, what happens when a player dies. There is exactly one game mode per running world, and in
multiplayer it exists only on the server, which is why the header comment mentions it. Chapter 7
covers the whole Gameplay Framework; here the game mode only proves your code runs.

### The "C++ base, Blueprint child" pattern

You could set `AGSGameMode` itself as the project's game mode. GridSnake instead creates a Blueprint
class that derives from it, `BP_GSGameMode`, and uses that. Every gameplay class in the course
follows the same pattern:

- **C++ holds the logic**: rules, movement, state. It is fast, testable, diff-able in version
  control, and easy to search.
- **The Blueprint child holds assets and tuning**: which sprite, which sound, which widget class,
  how fast. Those are references to assets in `Content/`, and hard-coding asset paths in C++ is
  brittle (a rename breaks the build or crashes at run time). In a Blueprint, references are fixed
  up automatically when assets move, and a designer can tweak values without a compiler.
- The C++ class sets sensible **defaults** in its constructor; the Blueprint overrides only what it
  must.

`BP_GSGameMode` has nothing to override yet. It exists from the start so that every later chapter
can configure it, and so that the project settings never have to change again.

### Create BP_GSGameMode

1. In the Content Browser, create the folder `Content/GridSnake/Blueprints`.
2. Right-click inside it and choose **Blueprint Class**. The **Pick Parent Class** dialog opens.
3. Expand **All Classes** at the bottom and type `GSGameMode` in its search box. Select
   **GSGameMode** and click **Select**.
4. Name the new asset `BP_GSGameMode`.
5. Double-click it to open the Blueprint editor. The **Class Defaults** panel shows the game mode's
   properties: **Default Pawn Class**, **HUD Class**, **Player Controller Class** and more. Leave
   them. In the top-right corner, **Parent class: GSGameMode** confirms the C++ parent. Close the
   editor with **Compile** and **Save**.

If `GSGameMode` does not appear in step 3, the editor is not running your newest build. Close it,
build **Development Editor** in your IDE, and start it again.

### Project default or World Settings

There are two places to choose a game mode:

- **Project default**: **Edit > Project Settings > Project > Maps & Modes > Default Modes > Default
  GameMode**. Used by every level that does not override it.
- **Per level**: **Window > World Settings**, section **Game Mode**, **GameMode Override**. Takes
  precedence for that one level. Useful for a menu level with different rules (chapter 16 uses it).

Set the project default: **Default GameMode** = `BP_GSGameMode`. The **Selected GameMode** section
below it now lists the Blueprint's class defaults; editing them there edits the Blueprint.

The editor writes one new line into `Config/DefaultEngine.ini`:

`project/ch02/Config/DefaultEngine.ini` (excerpt)

```ini
[/Script/EngineSettings.GameMapsSettings]
; GridSnake (ch01): Project Settings > Project > Maps & Modes
GameDefaultMap=/Game/GridSnake/Maps/L_Level01.L_Level01
EditorStartupMap=/Game/GridSnake/Maps/L_Level01.L_Level01
; GridSnake (ch02): Default GameMode
GlobalDefaultGameMode=/Game/GridSnake/Blueprints/BP_GSGameMode.BP_GSGameMode_C
```

Note the `_C` at the end. A Blueprint asset `BP_GSGameMode` contains a *generated class*
`BP_GSGameMode_C`; the asset is the editable graph, the `_C` class is what the engine instantiates.
Any setting that wants a class rather than an asset uses the `_C` name. (A C++ class would be
written as a script path instead, such as `/Script/GridSnake.GSGameMode`.)

### Play and read the log

Clear the Output Log, filter it by `LogGridSnake`, and click **Play**.

Expected Output Log (filter: `LogGridSnake`), in PIE:

```text
LogGridSnake: GridSnake game mode started on map UEDPIE_0_L_Level01
```

Now choose **Standalone Game** from the Play options. The separate game process does not write to
the editor's Output Log. It writes its own log file in `Saved/Logs/`: `GridSnake.log`, or
`GridSnake_2.log` when the editor still holds the first name open. Open it after closing the game.

Expected log line (Standalone Game, in the game's log file):

```text
LogGridSnake: GridSnake game mode started on map L_Level01
```

The difference is a classic surprise. PIE does not play the level you are editing: it *duplicates*
it into a new world, and renames the copy so the two cannot be confused. The prefix is
`UEDPIE_<n>_`, where `<n>` is the PIE instance (0 for the first player; multiplayer testing in PIE
runs several instances). `UWorld::GetMapName()` returns the copy's name. In Standalone and packaged
games there is no copy and no prefix.

So never compare map names directly in gameplay code. When you need the clean name, use
`UGameplayStatics::GetCurrentLevelName(this)`, which removes the prefix by default, or
`UWorld::RemovePIEPrefix`. Chapter 13 identifies levels through data assets instead, which avoids
the problem altogether.

## Reading compiler errors

C++ build errors in Unreal look intimidating because UBT, UHT, the compiler and the linker all
report in the same window. Two habits help:

- **Read the Output window, not just the Error List.** In Visual Studio, the Error List mixes real
  build errors with IntelliSense guesses, which are often wrong about Unreal macros. Set the Error
  List's filter to **Build Only**, or read the **Output** window (Show output from: **Build**).
- **Fix the first error first.** One mistake causes a cascade: a missing header makes every later
  use of its types fail. Scroll up to the first line containing `error`, fix that, and rebuild.
  Errors from UHT appear *before* any compiler output, since UHT runs first.

Each error line names a file and line number, then a code: `C` codes (such as `C2065`) come from the
MSVC compiler, `LNK` codes from the linker, and clang errors read `error:` followed by a message.

Two mistakes you will make sooner or later:

**1. The `.generated.h` include is not last, or missing.** Suppose you add an include *below*
`#include "GSGameMode.generated.h"`, or forget the generated include altogether. UHT stops before the
compiler runs, with an error in your header saying that the `.generated.h` file must be the last
include (or that it expected one). The fix is always the same: `#include "<FileName>.generated.h"`
as the final include of every header that contains `UCLASS`, `USTRUCT`, `UENUM` or `UINTERFACE`,
with exactly the header's own file name. After renaming a header, rename its generated include too.

**2. A missing module dependency.** Suppose a later chapter's code uses `UUserWidget` but you forgot
to add `"UMG"` to `GridSnake.Build.cs`. One of two errors appears, depending on whether the header can
be found through some other path:

- **The header is not found.** The compiler stops at the `#include` line: *cannot open include file*
  (MSVC `C1083`) or *file not found* (clang). Your module does not see that module's headers because
  it does not depend on it.
- **The header is found but the code is not linked.** Everything compiles, then the linker fails with
  *unresolved external symbol* (MSVC `LNK2019`/`LNK2001`) or *undefined symbol* (clang), naming a
  function such as `UUserWidget::StaticClass`. Your code calls functions that live in a DLL your
  module is not linked against. This happens when another dependency exposes the headers but your
  module never lists the module itself.

The fix for both: find which module the type lives in (the API reference page of every class shows
its **Module** and **Header**), add that module to `PublicDependencyModuleNames` or
`PrivateDependencyModuleNames`, regenerate project files, and rebuild with the editor closed.

The API reference is worth bookmarking for this alone. Search for `AGameModeBase` and the page
shows **Module: Engine** and a **Header** path ending in
`Engine/Classes/GameFramework/GameModeBase.h`.
The include path you write starts after `Classes/` or `Public/`: `GameFramework/GameModeBase.h`.

## Project: the GridSnake module and game mode

Here is the chapter's project work in one place. The changes since `project/ch01/` are:

| File | Change |
|---|---|
| `GridSnake.uproject` | new `Modules` array with the `GridSnake` runtime module |
| `Config/DefaultEngine.ini` | new `GlobalDefaultGameMode` line |
| `Source/GridSnake.Target.cs`, `Source/GridSnakeEditor.Target.cs` | new: game and editor targets |
| `Source/GridSnake/GridSnake.Build.cs` | new: module rules |
| `Source/GridSnake/GridSnake.h`, `GridSnake.cpp` | new: module registration and `LogGridSnake` |
| `Source/GridSnake/Core/GSGameMode.h`, `GSGameMode.cpp` | new: `AGSGameMode` |

All files are shown in full above. The assets, from `project/ch02/ASSETS.md`:

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `Source/GridSnake/...` | C++ module | **Tools > New C++ Class...** | Creates the module, targets and `.Build.cs` (replaced by this snapshot's files) |
| `/Game/GridSnake/Blueprints/BP_GSGameMode` | Blueprint Class | `GSGameMode` (C++) | No changes yet |

Editor steps:

1. **Tools > New C++ Class... > Game Mode Base**, name `GSGameMode`, path
   `Source/GridSnake/Core/`. The editor generates the module, both `Target.cs` files and the
   project files, then compiles.
2. Close the editor. Replace the generated `Source/` folder with the snapshot's. Regenerate project
   files (Windows: right-click `GridSnake.uproject` > **Generate Visual Studio project files**;
   other platforms: see [Generating project files and opening the
   IDE](#generating-project-files-and-opening-the-ide)).
3. Build **GridSnakeEditor** in **Development Editor** from your IDE and start the editor from
   there.
4. Create `Content/GridSnake/Blueprints`. Right-click > **Blueprint Class** > **All Classes** >
   `GSGameMode`; name it `BP_GSGameMode`.
5. **Project Settings > Project > Maps & Modes > Default GameMode** = `BP_GSGameMode`.
6. Play.

Expected Output Log (filter: `LogGridSnake`), in PIE:

```text
LogGridSnake: GridSnake game mode started on map UEDPIE_0_L_Level01
```

In a Standalone Game the same line reads `... started on map L_Level01`. The viewport is still
empty: the default pawn from `AGameModeBase` flies around a void. Chapter 3 adds GridSnake's settings
and game instance; chapter 7 replaces the default pawn with the snake.

## Summary

- Adding a C++ class through **Tools > New C++ Class...** converts a Blueprint project: it creates
  `Source/` with two targets and one module, adds the module to the `.uproject`, and compiles.
  GridSnake then replaces the wizard's files with its own layout (no `_API` macros, module-root
  includes).
- A **module** is a folder with a `Build.cs`; it compiles to one library. A **target**
  (`Target.cs`) is a program to build: `Editor` or `Game`. `DefaultBuildSettings = V7` and
  `IncludeOrderVersion = Unreal5_8` pin 5.8's build defaults.
- `Build.cs` lists dependencies: public for types in your public headers, private for `.cpp`-only
  use. A missing dependency causes *cannot open include file* or *unresolved external symbol*.
- **UBT** builds everything and generates IDE project files; **UHT** reads `UCLASS`/`UPROPERTY`
  macros and writes `.generated.h` and `.gen.cpp` files. The `.generated.h` include comes last.
- **Live Coding** (Ctrl+Alt+F11) patches code into the running editor. Restart and
  build from the IDE after adding classes or changing `Build.cs`, and whenever in doubt.
- Build configurations: Development Editor for daily work, DebugGame Editor for stepping through
  your code, Shipping for release.
- `UE_LOG(Category, Verbosity, TEXT("..."), Args)` logs; `%s` takes `*FString`. Declare categories
  with `DECLARE_LOG_CATEGORY_EXTERN` + `DEFINE_LOG_CATEGORY`. Change verbosity with `Log <Category>
  <Verbosity>`.
- `FString` is for building text, `FName` for identifiers, `FText` for what players read. `FString`
  and `FName` comparisons ignore case.
- Gameplay classes are C++ bases with Blueprint children (`BP_GSGameMode`): logic in C++, assets and
  tuning in Blueprint. Blueprint classes are referenced with a `_C` suffix.
- In PIE, `GetMapName()` returns `UEDPIE_0_L_Level01`; in Standalone, `L_Level01`.

## Exercises

1. **Verbose logging.** Add a `Verbose` log line to `AGSGameMode::StartPlay` and make it visible.
   Find three ways: one for this session, one for one launch, and one permanent.

   <details><summary>Solution</summary>

   Add after the existing line:

   ```cpp
   UE_LOG(LogGridSnake, Verbose, TEXT("StartPlay finished in world %s"), *GetWorld()->GetName());
   ```

   Build (or Live Code) and play: the line does not appear, because `LogGridSnake` defaults to `Log`.

   - **This session:** type `Log LogGridSnake Verbose` in the console (the editor's **Cmd** box or
     the PIE console), then play again. `Log LogGridSnake Log` restores the default.
   - **One launch:** start the editor or game with the command-line argument
     `-LogCmds="LogGridSnake Verbose"`. In Visual Studio, put it in the GridSnake project's
     **Properties > Debugging > Command Arguments** after the existing project path.
   - **Permanent:** add to `Config/DefaultEngine.ini`:

     ```ini
     [Core.Log]
     LogGridSnake=Verbose
     ```

   Expected Output Log (filter: `LogGridSnake`), in PIE:

   ```text
   LogGridSnake: GridSnake game mode started on map UEDPIE_0_L_Level01
   LogGridSnake: Verbose: StartPlay finished in world UEDPIE_0_L_Level01
   ```

   Remove the line and the ini section afterwards so your project matches the snapshot. A `Verbose`
   line is a good place for detail you want available but not always visible; chapter 19 uses them.

   </details>

2. **Break the build on purpose.** Remove `"Engine"` from `PublicDependencyModuleNames` in
   `GridSnake.Build.cs` and build with the editor closed. Does it fail? Then also remove
   `"EnhancedInput"` and build again. Explain both results.

   <details><summary>Solution</summary>

   With only `"Engine"` removed, the build will most likely still **succeed**. `EnhancedInput`'s own
   public headers use engine types, so it lists `Engine` as a *public* dependency, and public
   dependencies are passed on: depending on `EnhancedInput` gives you `Engine`'s include paths and
   library too. This is legal but fragile. If a later chapter removed `EnhancedInput`, unrelated code
   would break. List every module you use directly.

   With both removed, `GSGameMode.h` can no longer find `GameFramework/GameModeBase.h`, which lives in
   the `Engine` module. The compiler stops at that `#include` with *cannot open include file*
   (MSVC) or *file not found* (clang). If the header were still reachable another way, you would get
   the link-time variant instead: *unresolved external symbol* for functions such as
   `AGameModeBase::StartPlay`, which `Super::StartPlay()` calls.

   Restore both lines, regenerate project files and rebuild.

   </details>

3. **Live Coding round trip.** With the editor open and PIE stopped, change the log text
   in `AGSGameMode::StartPlay`, recompile with Live Coding, and play again. Then try the same while
   the IDE builds, and read what UBT says.

   <details><summary>Solution</summary>

   1. Change the format string, for example to `TEXT("GridSnake game mode ready on map %s")`, and
      save the file.
   2. In the editor press **Ctrl+Alt+F11**. The Live Coding window compiles `GSGameMode.cpp` and
      patches it in; a "Live coding succeeded" notification appears.
   3. Play. Expected Output Log (filter: `LogGridSnake`):

      ```text
      LogGridSnake: GridSnake game mode ready on map UEDPIE_0_L_Level01
      ```

   4. Now build from Visual Studio while the editor is open. UBT refuses, telling you that Live
      Coding is active and to exit the editor or press Ctrl+Alt+F11. That is expected.

   Changing a function body is the ideal Live Coding case. Revert the text afterwards (Live Code
   again, or close the editor and build).

   </details>

4. **Count the actors.** Make the game mode also log how many actors exist in the world when play
   starts. Is the "empty" level really empty?

   <details><summary>Solution</summary>

   `TActorIterator` (header `EngineUtils.h`) iterates every actor of a class in a world:

   ```cpp
   #include "Core/GSGameMode.h"
   #include "GridSnake.h"
   #include "EngineUtils.h"

   // ...

   void AGSGameMode::StartPlay()
   {
   	Super::StartPlay();

   	UE_LOG(LogGridSnake, Log, TEXT("GridSnake game mode started on map %s"), *GetWorld()->GetMapName());

   	int32 Count = 0;
   	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
   	{
   		UE_LOG(LogGridSnake, Verbose, TEXT("  actor %s (%s)"), *It->GetName(), *It->GetClass()->GetName());
   		++Count;
   	}
   	UE_LOG(LogGridSnake, Log, TEXT("The world has %d actors"), Count);
   }
   ```

   An alternative is `UGameplayStatics::GetAllActorsOfClass(this, AActor::StaticClass(), Actors)`
   from `Kismet/GameplayStatics.h`, which fills a `TArray<AActor*>`; the iterator avoids building the
   array.

   Expected Output Log (filter: `LogGridSnake`), in PIE:

   ```text
   LogGridSnake: GridSnake game mode started on map UEDPIE_0_L_Level01
   LogGridSnake: The world has <N> actors
   ```

   `<N>` is around ten, not zero. Turn on `Log LogGridSnake Verbose` to see the list: the
   `WorldSettings` actor, the game mode itself, a game state, a player controller with its player
   state and camera manager, a HUD, the default pawn, and a few engine helpers. The Gameplay
   Framework spawns all of these for every level; chapter 7 explains each one. Remove the code
   afterwards.

   </details>

---

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-2--projects-modules-and-the-build)
