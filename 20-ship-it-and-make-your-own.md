# Chapter 20 — Ship It, Then Make Your Own

The last chapter has two halves. In the first, GridSnake becomes a real game: a program you can
copy to another computer and run without the editor. You will learn what cooking and packaging
do, set the packaging options GridSnake needs, and package a Shipping build from the editor and
from the command line. You will also put a version number on the main menu and test the package
the way a player would run it. In the second half, everything from the course becomes a plan for
*your* 2D game: which template to start from, how to lay out the project, what to take from
GridSnake (with a starter module to copy), how to keep the scope small, and where to go next.
GridSnake gains its packaging settings, version 1.0.0, and a version label on the main menu.

Previous: [Chapter 19](19-debugging-and-profiling.md). Next:
[Appendix A — Cheat Sheet](appendix-a-cheatsheet.md).

## From editor to executable

In the editor, GridSnake runs inside `UnrealEditor`. Assets are loaded from `.uasset` files in a
format meant for editing, and your code is a DLL the editor loads. A player has none of that. To
turn the project into a game, Unreal runs a pipeline, usually all at once:

1. **Build**: compile the game target (`GridSnake.Target.cs`, chapter 2) for the target platform
   and build configuration. The result is a standalone executable with the engine and your module
   linked in, without any editor code.
2. **Cook**: convert every asset the game needs into the platform's runtime format. Textures are
   compressed for the target GPU, editor-only data (thumbnails, source art paths, Blueprint graphs
   that were compiled) is dropped, and shaders are compiled for the target. Anything not cooked
   is **not in the game**.
3. **Stage**: copy the executable, the cooked content, config files and the runtime libraries into
   a staging folder with the layout the game expects.
4. **Package**: put the staged content into container files and do whatever the platform needs
   (an `.app` bundle on macOS, for example).
5. **Archive**: copy the finished result to the folder you chose.

The editor's **Package Project** and the command-line `RunUAT BuildCookRun` both run this pipeline
with Unreal Automation Tool (UAT).

### Containers: pak and IoStore

A cooked game has thousands of small files. Packaging bundles them into a few containers:

- A **`.pak`** file holds the content, like an archive. It loads faster than loose files and is
  harder to tamper with.
- **IoStore** (`.utoc` and `.ucas` files) is the newer container format the engine loads from by
  default. It is faster again. A `.pak` file still comes along with it for files that IoStore does
  not hold.

You do not have to understand the formats. Leave both on, and expect files with these extensions in
`Content/Paks/` of the packaged game.

### Development or Shipping

Chapter 2 listed the build configurations. For packaged games, two matter:

| | Development | Shipping |
|---|---|---|
| Optimized | yes | yes, plus more |
| Console (backtick key) | yes | **no** |
| Cheat manager, `GS*` cheats | yes | **no** (`UE_WITH_CHEAT_MANAGER` is 0) |
| `ECVF_Cheat` CVars | settable | hidden |
| `check()` | active | **removed** |
| Debug drawing (`ENABLE_DRAW_DEBUG`) | yes | **no** |
| Stats, Visual Logger | yes | **no** |
| Log file | written | none by default |
| Use it for | testing, profiling, finding bugs in a package | the build you give to players |

A third, **Test**, is Shipping with a few tools, such as stats, left in for profiling. You will
rarely need it for a small game. Package Development while you test, and Shipping for release.

## Packaging settings

Project Settings > **Project > Packaging** holds the options. They are stored in
`Config/DefaultGame.ini`, in a section whose name still says `UnrealEd`, for historical reasons.
Here is GridSnake's, new in this chapter:

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
; GridSnake (ch20): Project Settings > Project > Packaging.
; Shipping removes logging, stats, console and cheats; use Development for test builds.
BuildConfiguration=PPBC_Shipping
; The maps the cooker starts from. Anything they reference is cooked too.
+MapsToCook=(FilePath="/Game/GridSnake/Maps/L_MainMenu")
+MapsToCook=(FilePath="/Game/GridSnake/Maps/L_Level01")
+MapsToCook=(FilePath="/Game/GridSnake/Maps/L_Level02")
+MapsToCook=(FilePath="/Game/GridSnake/Maps/L_Level03")
; Assets only reached through config soft references (GSSettings) or the level list
; are not found by following map references, so always cook these folders.
+DirectoriesToAlwaysCook=(Path="/Game/GridSnake/Data")
+DirectoriesToAlwaysCook=(Path="/Game/GridSnake/Audio")
+DirectoriesToAlwaysCook=(Path="/Game/GridSnake/UI")
; One pak/IoStore container for the whole game.
UsePakFile=True
bUseIoStore=True
IncludeDebugFiles=False
ForDistribution=False
```

Key by key, with the Project Settings label (some are in the section's **advanced** part; click
the small arrow at the bottom of a group to show them):

| Key | Label | Meaning |
|---|---|---|
| `BuildConfiguration` | **Build Configuration** | the configuration **Package Project** uses. `PPBC_Shipping` is the value the editor writes for Shipping; `PPBC_Development` for Development |
| `MapsToCook` | **List of maps to include in a packaged build** | the maps the cooker starts from |
| `DirectoriesToAlwaysCook` | **Additional Asset Directories to Cook** | folders cooked whole, referenced or not |
| `UsePakFile` | **Use Pak File** | put content in a `.pak` instead of loose files |
| `bUseIoStore` | **Use Io Store** | also use IoStore containers |
| `IncludeDebugFiles` | **Include Debug Files** | copy debug symbols (`.pdb`) into the package; useful for reading crash callstacks from testers, but large |
| `ForDistribution` | **For Distribution** | marks a build for store submission. Some platforms require it, and it changes signing. Leave it off until you submit to a store |

Set the values in the editor rather than typing the ini, then compare the section with the
snapshot. The `+` before a key adds an element to an array, as in chapter 13. If you are unsure of
a value's spelling, change the setting in the editor and read what it wrote. `PPBC_Shipping` is
the widely used spelling, but the editor's own output is the authority.

### Why soft references need explicit cooking

The cooker decides what goes into the game by following references. It starts from the maps (and
the default maps and game mode in **Maps & Modes**) and cooks everything they reference, everything
*those* assets reference, and so on. If the map list is empty, it cooks every map in the project,
including test maps you never meant to ship. GridSnake lists its four maps, so only those and what
they need are cooked.

The catch is what the cooker *cannot* see. GridSnake reaches several assets only through
`UGSSettings`, a developer settings class whose values are lines in `DefaultGame.ini` (chapter 3).
`LevelList`, `Music`, `VolumeMix`, `MusicClass` and `SfxClass` are soft references written in a
config file, not in any asset, so following references from the maps never reaches them. In the
editor everything is on disk, so the game works. In the package those assets would be missing:
`UGSSettings::GetLevelList()` returns null, the main menu shows no levels, and the music never
starts. This is the classic "works in the editor, missing in the build" bug.

**Additional Asset Directories to Cook** fixes it by cooking the folders whole: `Data` (the level
list and speed curves), `Audio` (music, sound classes, the mix) and `UI` (widget Blueprints, some
of which are only loaded by class reference). The general rule:

> Anything your code loads by path, by config, or by soft reference from a settings class must be
> in a map's reference chain, in **Additional Asset Directories to Cook**, or in the **Primary
> Asset** setup of the Asset Manager.

The Asset Manager (Project Settings > Game > **Asset Manager**) is the scalable answer for big
games: it groups assets into types and cooks them by rule. For a game the size of GridSnake, the
directory list is simpler and enough.

## Packaging from the editor

Packaging uses the platform's real compiler, so the toolchain from chapter 1 must be installed. On
Windows that means Visual Studio's C++ workload with a Windows SDK. Packaging for Linux *from*
Windows needs Epic's Linux cross-compile toolchain, a separate download described in the Linux
development requirements page of the docs. On Linux, the engine's own setup provides the
toolchain. A macOS build needs a Mac with Xcode.

1. Save everything (**File > Save All**).
2. On the main toolbar, open the **Platforms** menu. Choose your platform, for example **Windows**.
   Its submenu has:
   - **Package Project**: the full pipeline.
   - **Cook Content**: cooking only. A quick way to find missing or broken content without
     waiting for a full package.
   - A build configuration choice (labelled **Binary Configuration** or similar in recent
     versions). Leave it on the project setting to use **Build Configuration** from the packaging
     settings, or pick one here for a single build.
3. Choose **Package Project**. A folder picker opens. Pick a folder **outside the project**, for
   example `C:\Builds\GridSnake`. The course's `.gitignore` also ignores a folder named `Packaged/`
   if you prefer to keep builds next to the project.
4. A notification in the bottom right shows progress, with a **Show Output Log** link. Packaging
   the first time takes several minutes, mainly for shader compilation. Later builds are faster.
5. When it finishes, the notification says it succeeded or failed. On failure, open the Output Log
   (and **Window > Message Log**, page **Packaging Results** or similar) and scroll to the **first**
   `Error:` line. Later errors are often consequences of the first.

The **Platforms** menu also has **Packaging Settings...** at the bottom, a shortcut to the Project
Settings page above.

The result for Windows looks roughly like this (the names depend on the configuration and version):

```text
C:\Builds\GridSnake\Windows\
  GridSnake.exe                      a small launcher: run this one
  GridSnake\
    Binaries\Win64\GridSnake-Win64-Shipping.exe     the real game (Development: GridSnake.exe)
    Content\Paks\GridSnake-Windows.pak, .ucas, .utoc
  Engine\                            engine content and runtime libraries
```

The whole folder is the game. Zip `Windows\` and anyone with a 64-bit Windows PC can unzip and run
`GridSnake.exe`.

## Packaging from the command line

The editor's packaging button runs UAT with a long command line. Running it yourself lets you
package from a script or a build server, without opening the editor. The command is
`BuildCookRun`. The scripts below are this chapter's examples. Set the three paths at the top and
run them.

A tip before the scripts: when you package from the editor, the Output Log prints the exact UAT
command line it ran, near the start of the packaging output. It is the most reliable reference for
your engine version. Copy flags from it when in doubt.

`examples/ch20/package-windows.bat`

```bat
@echo off
rem Example for chapter 20 of "Learn Unreal Engine 5".
rem Packages a project for Windows with RunUAT BuildCookRun.
rem
rem Usage:   package-windows.bat [Shipping^|Development]
rem Edit ENGINE_DIR, PROJECT and OUTPUT_DIR below first.
rem
rem The flags, in order:
rem   BuildCookRun         the UAT command that does build + cook + stage + package
rem   -project=...         the .uproject to package (absolute path)
rem   -noP4                do not talk to Perforce source control
rem   -unattended          never wait for a dialog; fail instead
rem   -utf8output          UTF-8 log output, so non-ASCII paths print correctly
rem   -platform=Win64      the target platform
rem   -clientconfig=...    the build configuration: Shipping or Development
rem   -build               compile the game target for that platform and configuration
rem   -cook                convert the content to the platform's runtime format
rem   -stage               copy binaries and cooked content into a staging folder
rem   -pak                 put the staged content into .pak container files
rem   -iostore             ...and into IoStore containers (.utoc/.ucas)
rem   -compressed          compress the containers (smaller, a little slower to load)
rem   -prereqs             include the prerequisites installer (Visual C++ runtime)
rem   -package             run the platform's packaging step
rem   -archive             copy the finished build to -archivedirectory
rem   -archivedirectory=   where the finished build goes; a "Windows" folder is created inside

setlocal

set "ENGINE_DIR=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=C:\Projects\GridSnake\GridSnake.uproject"
set "OUTPUT_DIR=C:\Builds\GridSnake"

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Shipping"

if not exist "%ENGINE_DIR%\Engine\Build\BatchFiles\RunUAT.bat" (
  echo RunUAT.bat not found - set ENGINE_DIR in this script.
  exit /b 1
)
if not exist "%PROJECT%" (
  echo %PROJECT% not found - set PROJECT in this script.
  exit /b 1
)

call "%ENGINE_DIR%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="%PROJECT%" ^
  -noP4 ^
  -unattended ^
  -utf8output ^
  -platform=Win64 ^
  -clientconfig=%CONFIG% ^
  -build ^
  -cook ^
  -stage ^
  -pak ^
  -iostore ^
  -compressed ^
  -prereqs ^
  -package ^
  -archive ^
  -archivedirectory="%OUTPUT_DIR%"

if errorlevel 1 (
  echo Packaging FAILED - scroll up to the first error.
  exit /b 1
)
echo Packaged %CONFIG% build in %OUTPUT_DIR%\Windows
```

`examples/ch20/package-linux.sh`

```bash
#!/usr/bin/env bash
# Example for chapter 20 of "Learn Unreal Engine 5".
# Packages a project for Linux with RunUAT BuildCookRun.
#
# Usage:   ./package-linux.sh [Shipping|Development]
# Edit ENGINE_DIR, PROJECT and OUTPUT_DIR below first (or set them in the environment).
# The flags are explained in package-windows.bat; only -platform differs, and Linux
# has no prerequisites installer, so -prereqs is left out.

set -euo pipefail

ENGINE_DIR="${ENGINE_DIR:-$HOME/UnrealEngine-5.8}"
PROJECT="${PROJECT:-$HOME/Projects/GridSnake/GridSnake.uproject}"
OUTPUT_DIR="${OUTPUT_DIR:-$HOME/Builds/GridSnake}"
CONFIG="${1:-Shipping}"

UAT="$ENGINE_DIR/Engine/Build/BatchFiles/RunUAT.sh"

if [[ ! -x "$UAT" ]]; then
  echo "RunUAT.sh not found at $UAT - set ENGINE_DIR." >&2
  exit 1
fi
if [[ ! -f "$PROJECT" ]]; then
  echo "$PROJECT not found - set PROJECT." >&2
  exit 1
fi
case "$CONFIG" in
  Shipping|Development|DebugGame|Test) ;;
  *) echo "Unknown configuration '$CONFIG'." >&2; exit 1 ;;
esac

"$UAT" BuildCookRun \
  -project="$PROJECT" \
  -noP4 \
  -unattended \
  -utf8output \
  -platform=Linux \
  -clientconfig="$CONFIG" \
  -build \
  -cook \
  -stage \
  -pak \
  -iostore \
  -compressed \
  -package \
  -archive \
  -archivedirectory="$OUTPUT_DIR"

echo "Packaged $CONFIG build in $OUTPUT_DIR/Linux"
```

`examples/ch20/package-mac.sh` is the same script apart from its header comments and three
lines: the default `ENGINE_DIR` is `/Users/Shared/Epic Games/UE_5.8` (the launcher's default
install location on macOS), the platform is `-platform=Mac`, and the final message names the
`Mac` folder. Its header notes that Xcode must be installed, and that a `.app` you give to other
people must be code-signed and notarized, which the script does not do.

```bash
ENGINE_DIR="${ENGINE_DIR:-/Users/Shared/Epic Games/UE_5.8}"
# ...
  -platform=Mac \
# ...
echo "Packaged $CONFIG build in $OUTPUT_DIR/Mac"
```

The shell scripts were syntax-checked with `bash -n`, and their guard clauses were run. The UAT
calls themselves could not be run in the environment that wrote this course. To run them:

```console
$ chmod +x package-linux.sh
$ ENGINE_DIR=~/UnrealEngine-5.8 PROJECT=~/Projects/GridSnake/GridSnake.uproject ./package-linux.sh Development
```

On Windows, open a terminal in the folder and run `package-windows.bat` or
`package-windows.bat Development`.

UAT prints a lot: the build, then the cook (one line per package in verbose phases), then staging
and packaging. The end of a successful run should look like this:

Expected final lines of UAT output:

```text
BUILD SUCCESSFUL
AutomationTool exiting with ExitCode=0 (Success)
```

On failure the exit code is not 0, and the script prints its `FAILED` line. Search the output
upwards for the first `Error:` line. UAT also writes full logs under the engine's
`Engine/Programs/AutomationTool/Saved/Logs` folder (or your user folder's equivalent for an
installed engine), whose path it prints when something fails.

## Versioning and the main menu

Every build you hand out should carry a version, so that a bug report can say which build it is
about. Unreal has a field for it: **Project Settings > Project > Description > Project Version**,
stored as `ProjectVersion` in `DefaultGame.ini` (chapter 1 set it to `0.1.0`). GridSnake 1.0 bumps
it:

```ini
[/Script/EngineSettings.GeneralProjectSettings]
...
ProjectVersion=1.0.0
; GridSnake (ch20): bump before every build you hand out; the main menu shows it.
```

A common scheme is *major.minor.patch*: patch for fixes, minor for new content, major for big
changes. What matters most is that it changes with every build you give away.

The main menu shows it. `UI/GSMainMenuWidget.h` gains an optional text widget:

```cpp
class UTextBlock;

	/** Optional: shows "v" + Project Settings > Project > Description > Project Version. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VersionText;
```

`BindWidgetOptional` (chapter 15) means the Widget Blueprint *may* have a `TextBlock` named
`VersionText`. If it does, the pointer is set. If not, it stays null and the Blueprint still
compiles. That suits a cosmetic element.

`UI/GSMainMenuWidget.cpp` adds two includes and fills it in `NativeOnInitialized`:

```cpp
#include "Components/TextBlock.h"
#include "Misc/ConfigCacheIni.h"

void UGSMainMenuWidget::NativeOnInitialized()
{
	// ... (button bindings as before)

	if (VersionText)
	{
		// Read straight from the game config (DefaultGame.ini, staged with the packaged game).
		FString Version;
		GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GGameIni);
		VersionText->SetText(FText::FromString(FString::Printf(TEXT("v%s"), *Version)));
	}
}
```

`GConfig` is the engine's config cache: every ini file, merged from the engine's defaults, your
`Default*.ini` files and the user's saved settings. `GetString(Section, Key, Out, File)` reads one
value. `GGameIni` names the game config (`DefaultGame.ini` and the layers under it). Config files
are staged with the packaged game, so this works there too.

Why not `GetDefault<UGeneralProjectSettings>()->ProjectVersion`, the way GridSnake reads its own
settings? That class lives in the `EngineSettings` module, which GridSnake does not depend on.
Reading the ini directly avoids a Build.cs change for one string. Both are fine. If you prefer the
typed way, add `"EngineSettings"` to the private dependencies and include
`GeneralProjectSettings.h`.

## Testing the package

A package can fail in ways the editor never shows. Test it as a player would:

1. **Run the right file**: `Windows\GridSnake.exe`. The game starts in the main menu (the Game
   Default Map, `L_MainMenu`) with `v1.0.0` in the corner. If it starts in a black screen or the
   wrong map, check **Maps & Modes > Game Default Map** and the cooked map list.
2. **Play every level**: all three, through the loading screen (chapter 14), which only shows in
   Standalone and packaged builds. Listen for the music and sounds; missing audio usually means a
   folder was not cooked.
3. **Quit and restart**: progress must persist. The packaged game does **not** share the editor's
   saves. Editor saves are in the project's `Saved/SaveGames/`. A Shipping build on Windows saves
   per user, in `%LOCALAPPDATA%\GridSnake\Saved\SaveGames\`. A Development package keeps its
   `Saved` folder next to the game, in `Windows\GridSnake\Saved\`. Other platforms use their own
   per-user folders. If progress is not kept, find the `.sav` file first. To test a first run
   again, delete that file.
4. **Try the debug tools**: in a Development package, the backtick key opens the console and
   `GSUnlockAll` works. In Shipping, the key does nothing, and the cheats and `GS.Debug.Grid` do
   not exist. That is the point of this chapter's build configuration table.
5. **Read the log** (Development): `Windows\GridSnake\Saved\Logs\GridSnake.log`, the same lines as
   the editor's Output Log, from startup to quit. Shipping writes no log by default. To debug a
   Shipping-only problem, package Development first. Most bugs show up in both.
6. **Window and display**: Alt+Enter toggles fullscreen (`DefaultInput.ini`,
   `bAltEnterTogglesFullscreen`). The Options menu's settings (chapter 16) should survive a
   restart.
7. **Test on another computer**: a machine without Unreal or Visual Studio. This catches missing
   runtime libraries: run the prerequisites installer included with `-prereqs` (in the package's
   `Engine\Extras\Redist` folder) if the game will not start. It also catches absolute paths and
   anything that only worked because the editor had it loaded.

If something is missing in the package only, use **Platforms > *your platform* > Cook Content**
and read the cook's warnings in the Output Log. Lines about packages that could not be found or
loaded point straight at the asset.

## Project: GridSnake 1.0

The chapter 20 snapshot is `project/ch20/`. Compared with `project/ch19/`:

| File | Change |
|---|---|
| `Config/DefaultGame.ini` | `ProjectVersion=1.0.0`; the `[/Script/UnrealEd.ProjectPackagingSettings]` section |
| `Source/GridSnake/UI/GSMainMenuWidget.h` | optional `VersionText` |
| `Source/GridSnake/UI/GSMainMenuWidget.cpp` | reads `ProjectVersion` with `GConfig` and shows it |

Both code changes and the ini are shown in full above. No Build.cs change.

Editor steps:

1. Open `W_MainMenu` (in `/Game/GridSnake/UI`). From the Palette, drag a **Text** widget onto the
   canvas and name it exactly `VersionText` (top of the Details panel). Anchor it bottom-right,
   with alignment 1, 1 and a small offset from the corner, and set the font **Size** to 14. Its
   default text does not matter; the code replaces it. **Compile** and **Save**.
2. **Edit > Project Settings > Project > Description > Project Version** = `1.0.0`. Optionally fill
   in **Project Displayed Title**, which becomes the window title.
3. **Project Settings > Project > Packaging**:
   - **Build Configuration**: **Development** for test builds (it keeps the console, logs and
     cheats), **Shipping** for the build you hand out. The snapshot has Shipping.
   - **List of maps to include in a packaged build** (advanced): add `L_MainMenu`, `L_Level01`,
     `L_Level02` and `L_Level03` with the **+** button.
   - **Additional Asset Directories to Cook**: `/Game/GridSnake/Data`, `/Game/GridSnake/Audio`
     and `/Game/GridSnake/UI`.
   - **Use Pak File** on, **Use Io Store** on.
   - Close the editor and compare the `[/Script/UnrealEd.ProjectPackagingSettings]` section of
     `Config/DefaultGame.ini` with the snapshot.
4. Optional: **Project Settings > Platforms > Windows**: **Game Icon** and the **Splash** images
   (exercise 2).
5. **Platforms > *your platform* > Package Project**, choosing an output folder outside the
   project. Or run `examples/ch20/package-windows.bat` (or the `.sh` for your platform).
6. Run the packaged game and go through [Testing the package](#testing-the-package).

**Expected result.** The package boots into the main menu with `v1.0.0` in the bottom-right corner.
**Play** starts level 1 after the countdown, and all three levels play through with the loading
screen between them. Progress and volume settings survive a restart. In the Shipping build, the
console and the `GS*` cheats are gone. Packaging ends with UAT's `BUILD SUCCESSFUL`, as shown
above.

There are no new log lines in this chapter. In a Development package, the log in
`Saved/Logs/GridSnake.log` shows the same GridSnake lines as the editor for the same actions,
starting with the level flow and save subsystems initialising.

GridSnake is finished. The rest of the chapter is about the next game: yours.

## Making your own 2D game in UE5

You now know every piece a small 2D game needs: sprites, flipbooks and tile maps; actors,
components and the gameplay framework; input, time and collision; interfaces, delegates and data
assets; level flow; UMG menus and a HUD; sound; saving; debugging; and packaging. This section
turns that into a plan.

### Pick a template

In the Project Browser:

- **Games > Blank**, with **C++** selected, **Desktop**, **Starter Content** off. This is what
  GridSnake became after chapter 2, and the recommended start for a 2D game. It contains nothing to
  delete. If you start with Blueprint and add C++ later, chapter 2 shows how.
- **Quality**: **Maximum** or **Scalable**. For unlit pixel art it barely matters, because you turn
  the expensive features off anyway (next section).

There is no Paper2D template in the UE5 Project Browser. UE4 had a 2D side-scroller template, but
UE5's game templates are all 3D. Recent versions add gameplay *variants* to some of them, so check
yours. That is no loss: Blank plus this course's settings is a better 2D starting point than a
template you would have to take apart.

There is one other road, sometimes called **2.5D**: a 3D game seen from the side or from above
through an orthographic (or narrow perspective) camera. It uses either 3D models or Paper2D
sprites with a lit material placed in a 3D world. A 3D template such as Top Down or Third Person
is a fair start for that, with the camera changed. Choose 2.5D if you want real lighting, depth
and 3D physics. Choose pure Paper2D, as in GridSnake, for pixel art and a flat, exact look.

### Set up the project

For a Paper2D game called, say, `DungeonPusher`:

1. Create the project: **Games > Blank > C++**, Desktop, Starter Content off, name `DungeonPusher`.
2. **Edit > Plugins**: make sure **Paper2D** is enabled (chapter 1).
3. Copy the rendering settings from `project/ch20/Config/DefaultEngine.ini`
   (`[/Script/Engine.RendererSettings]`, chapter 4) into your `DefaultEngine.ini`: Lumen, virtual
   shadows, auto exposure, bloom, motion blur and anti-aliasing off.
4. **Project Settings > Editor > Paper2D - Import**: set the pixels per unit your art uses
   (chapter 4).
5. Replace the wizard's module with the starter module below, renamed (see the next section).
6. Create the folder layout below in the Content Browser, then the input assets, the Blueprint
   children and the first map, in the same order as chapters 6 to 8.
7. Set up Git with LFS before the first asset is saved ([Source control](#source-control)).

### Lay out the project

GridSnake's layout scales well. Keep it:

```text
DungeonPusher/
  DungeonPusher.uproject
  Config/                    DefaultEngine.ini (2D rendering), DefaultGame.ini, DefaultInput.ini
  Content/DungeonPusher/     one top folder for your assets, so plugins and imports stay apart
    Art/Textures/  Art/Sprites/  Art/Flipbooks/  Art/Tiles/
    Audio/  Blueprints/  Data/  Input/  Maps/  UI/
  RawAssets/                 source art (.aseprite, .png sheets), outside Content/
  Source/
    DungeonPusher.Target.cs  DungeonPusherEditor.Target.cs
    DungeonPusher/
      DungeonPusher.Build.cs  DungeonPusher.h/.cpp
      Core/  Player/  Gameplay/  Data/  UI/  Save/
  Tools/                     scripts (art generation, batch jobs)
```

Rules that GridSnake followed and that will keep saving you time:

- **One runtime module** until you have a real reason for more. Folders by feature, not by type.
  `PublicIncludePaths.Add(ModuleDirectory)` so includes read `Core/DPGameMode.h`.
- **One class prefix** (`GS`, here `DP`) and asset prefixes (`BP_`, `W_`, `S_`, `FB_`, `T_`,
  `IA_`, `IMC_`, `DA_`, `L_`).
- **C++ for logic, Blueprint children for data**: assets, tuning numbers and layout live in
  Blueprint defaults and data assets. Code never hard-codes an asset path.
- **A developer settings class** (`UDPSettings`) for project-wide values and the asset references
  code needs (chapter 3). Remember that its soft references must be cooked explicitly.
- **Game instance subsystems** for anything that outlives a level: flow, save, audio (chapters 14,
  17 and 18).
- **A data asset for the level list** once levels need more than a map: names, speeds, scores
  (chapter 13).

### The starter module

`examples/ch20/StarterModule/` is that structure, distilled from GridSnake and ready to copy. It is
called `MyGame` with the prefix `MG`. Every API in it is one GridSnake already uses, except the
`UFloatingPawnMovement` free movement, which is new. It contains:

| File | Role | GridSnake original |
|---|---|---|
| `MyGame.Build.cs` | modules: Core, CoreUObject, Engine, InputCore, EnhancedInput, DeveloperSettings, Paper2D | `GridSnake.Build.cs` |
| `MyGame.h/.cpp` | `LogMyGame`, the primary game module | `GridSnake.h/.cpp` |
| `Core/MGSettings.h` | main menu map, level list, fade time | `GSSettings` |
| `Core/MGFlowSubsystem.h/.cpp` | open level by index, next, restart, main menu, with fades | `GSLevelFlowSubsystem` |
| `Core/MGGameMode.h/.cpp` | default classes; tells the flow a level started | `GSGameMode` |
| `Core/MGPlayerController.h/.cpp` | adds the mapping context; optional fixed camera | `GSPlayerController` |
| `Player/MGPawn2D.h/.cpp` | a free-moving top-down pawn: box, flipbook, ortho camera | (new) |

The settings class is short and shows the pattern:

`examples/ch20/StarterModule/Core/MGSettings.h`

```cpp
// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MGSettings.generated.h"

/**
 * Project-wide settings. Appears under Project Settings > Game > My Game and is saved
 * to Config/DefaultGame.ini. Read it anywhere with UMGSettings::Get().
 *
 * Soft pointers do not load anything by themselves, and the cooker does not follow
 * config files: list every map here in Project Settings > Packaging > "List of maps to
 * include in a packaged build" as well (chapter 20).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "My Game"))
class UMGSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** The map with the main menu. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<UWorld> MainMenuMap;

	/** The playable levels, in order. Index 0 is the first level. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TArray<TSoftObjectPtr<UWorld>> Levels;

	/** Seconds for the fade to and from black around a level change. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels", meta = (ClampMin = "0", Units = "s"))
	float FadeSeconds = 0.4f;

	/** The class default object holds the values loaded from the .ini files. */
	static const UMGSettings* Get() { return GetDefault<UMGSettings>(); }
};
```

The flow subsystem is `UGSLevelFlowSubsystem` without the run score, save and loading-screen
parts. It finds the current level's index by comparing the map's name with the list
(`TSoftObjectPtr::GetAssetName()`), so Play In Editor on any listed map works. The game mode calls
`NotifyLevelStarted` before `Super::StartPlay()`, exactly as in GridSnake. Read both in the folder;
there is nothing in them you have not seen.

The pawn is the one new piece. GridSnake's snake moves cell by cell, but most games move freely:

`examples/ch20/StarterModule/Player/MGPawn2D.h`

```cpp
// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MGPawn2D.generated.h"

class UBoxComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class UInputAction;
class UPaperFlipbook;
class UPaperFlipbookComponent;
struct FInputActionValue;

/**
 * A free-moving top-down 2D pawn: a box for collision, a flipbook for looks, an
 * orthographic camera, and simple movement locked to the X-Z plane. Make a Blueprint
 * child (BP_MGPawn2D) to set the flipbooks and the move action.
 *
 * For a platformer (gravity, jumping) start from APaperCharacter instead (chapter 20).
 */
UCLASS()
class AMGPawn2D : public APawn
{
	GENERATED_BODY()

public:
	AMGPawn2D();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> Movement;

	/** An Axis2D input action (IA_Move): X is right, Y is up. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> IdleFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> RunFlipbook;

private:
	void OnMove(const FInputActionValue& Value);
};
```

`examples/ch20/StarterModule/Player/MGPawn2D.cpp`

```cpp
// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#include "Player/MGPawn2D.h"
#include "MyGame.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "InputActionValue.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"

AMGPawn2D::AMGPawn2D()
{
	PrimaryActorTick.bCanEverTick = true;

	// The root is what moves and what collides. 16x16 units = one 16x16 pixel sprite
	// at 1 pixel per unit; deep in Y so it always meets walls, whatever their depth.
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(8.0f, 16.0f, 8.0f));
	Box->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(Box);

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Box);
	Flipbook->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Paper2D draws in the X-Z plane. Yaw -90 looks down -Y: +X is right, +Z is up.
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Box);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(512.0f);
	Camera->SetRelativeLocation(FVector(0.0f, 500.0f, 0.0f));
	Camera->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	// Simple acceleration-based movement that sweeps the root against walls.
	// The plane constraint stops it ever drifting in Y (towards or away from the camera).
	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 200.0f;
	Movement->SetPlaneConstraintEnabled(true);
	Movement->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);
}

void AMGPawn2D::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMGPawn2D::OnMove);
	}
	else
	{
		UE_LOG(LogMyGame, Warning, TEXT("%s has no MoveAction - set it in the Blueprint child"), *GetName());
	}
}

void AMGPawn2D::OnMove(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddMovementInput(FVector::ForwardVector, Input.X); // +X: screen right
	AddMovementInput(FVector::UpVector, Input.Y);      // +Z: screen up
}

void AMGPawn2D::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Pick the animation from the velocity instead of from the input, so it also looks
	// right when something else pushes the pawn.
	const FVector Velocity = GetVelocity();
	UPaperFlipbook* Wanted = Velocity.SizeSquared() > 1.0f ? RunFlipbook : IdleFlipbook;
	if (Wanted && Flipbook->GetFlipbook() != Wanted)
	{
		Flipbook->SetFlipbook(Wanted);
	}

	// Face the way we move: mirror the sprite with a negative X scale.
	if (FMath::Abs(Velocity.X) > 1.0f)
	{
		Flipbook->SetRelativeScale3D(FVector(Velocity.X < 0.0f ? -1.0f : 1.0f, 1.0f, 1.0f));
	}
}
```

Modules: the starter's Build.cs already has them (`Paper2D` for the flipbook, `EnhancedInput`
for the input).

Notes on the pawn:

- `UFloatingPawnMovement` (in `GameFramework/FloatingPawnMovement.h`, Engine module) is the
  simplest movement component that accelerates, brakes and sweeps its root against blocking
  geometry. `AddMovementInput` feeds it a direction each frame. No gravity, so it suits top-down
  games.
- `SetPlaneConstraintEnabled(true)` with `EPlaneConstraintAxisSetting::Y` locks movement on the Y
  axis, so the pawn never drifts towards or away from the camera. These functions come from
  `UMovementComponent` and work the same on the character movement component.
- The camera is a child of the pawn, so it follows. For a fixed single-screen view, set **Fixed
  View Target Class** on the player controller to a class placed in every level, the way
  GridSnake's board camera works.
- The move action is GridSnake's `IA_Steer` setup (chapter 8): an **Axis2D** action with WASD and
  the arrow keys mapped through **Swizzle Input Axis Values** and **Negate** modifiers. Unlike
  steering, movement wants the **Triggered** event every frame the key is held, which is what an
  Axis2D action without extra triggers gives.

To use the starter module in a new C++ project called `MyGame`:

1. Close the editor. Delete the wizard's `Source/MyGame/` files and copy the contents of
   `examples/ch20/StarterModule/` into `Source/MyGame/`.
2. Check that `MyGame.uproject` lists the module `MyGame` and the plugin `Paper2D`, and that
   `Source/MyGame.Target.cs` has `ExtraModuleNames.Add("MyGame")`.
3. Right-click the `.uproject` > **Generate Visual Studio project files** (or your platform's
   equivalent, chapter 2), then build **Development Editor** and open the project.
4. Create `BP_MGGameMode`, `BP_MGPlayerController` and `BP_MGPawn2D` (Blueprint children). Set the
   pawn's flipbooks and move action, the controller's mapping context, and the game mode's
   controller and pawn classes. Then pick `BP_MGGameMode` in **Maps & Modes**.
5. **Project Settings > Game > My Game**: set the main menu map and the levels.

For a project with another name, rename *before* you create any Blueprint children. Blueprint and
config references store class names, and renaming later needs redirects. Rename:

- the folder `Source/MyGame/`, the file `MyGame.Build.cs` and the class name inside it;
- `MyGame.h/.cpp`: the log category and `IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl,
  DungeonPusher, "DungeonPusher")`;
- the `MG` prefix in every class and file name, with a search and replace (`AMGPawn2D` becomes
  `ADPPawn2D`, `MGPawn2D.h` becomes `DPPawn2D.h`), including the `.generated.h` includes;
- the `.uproject`'s module name and `Target.cs` `ExtraModuleNames`, if they differ.

The wizard already generated the module files with your project's name, so the quickest way is
often to keep its `.Build.cs`, `.h` and `.cpp`, add the missing modules and lines from the
starter, and copy only the `Core/` and `Player/` folders, renaming as you go.

### What to take from GridSnake

The starter module is the skeleton. The rest of GridSnake is a parts bin. [Appendix B](appendix-b-project-reference.md) maps every
class to its role. These are the systems worth copying whole, with the prefix renamed:

| System | GridSnake files | Chapter | Take it when |
|---|---|---|---|
| Save game and save subsystem | `Save/GSSaveGame`, `Save/GSSaveSubsystem` | 18 | always: progress, unlocks, best scores |
| Audio subsystem | `Core/GSAudioSubsystem` | 17 | music that survives level changes, volume settings |
| Loading screen | `HandlePreLoadMap` in `GSLevelFlowSubsystem` | 14 | levels take more than a moment to load |
| Level list data asset | `Data/GSLevelList` | 13 | per-level names, rules or difficulty |
| Menu panels | `UI/GSMenuPanelWidget`, `GSMainMenuWidget`, `GSOptionsWidget`, `GSPauseMenuWidget` | 16 | always: menu, options, pause |
| HUD bound to game state delegates | `UI/GSHUDWidget`, `Core/GSGameState` | 11, 15 | score, lives, timers on screen |
| Interaction interface | `Gameplay/GSSnakeInteractable` | 11 | pickups, doors, switches, hazards |
| One-shot flipbook effect | `Gameplay/GSFlipbookEffect` | 12 | puffs, sparkles, explosions |
| Board with tile map and ortho camera | `Gameplay/GSBoard` | 6, 10 | grid or single-screen games |
| Fixed-step movement and turn buffer | `Snake/GSSnake` (`Tick`, `Step`, `DirectionQueue`) | 9 | grid movement, turn-based or tile-step games |
| Cheat manager and debug CVars | `Core/GSCheatManager`, `GS.Debug.Grid` | 19 | always, from day one |
| Packaging settings, version label | `DefaultGame.ini`, `GSMainMenuWidget` | 20 | always |
| Placeholder art script | `Tools/make_placeholder_art.py` | 4 | until real art exists |

Exercise 4 walks through copying the save subsystem.

### Keep the scope small

The most common way a first game fails is that it never ends. GridSnake is deliberately small:
**one mechanic** (steer and grow), **three levels**, **menus, a HUD, sound, saving and a
package**. That is the right size for your first own game too. Concretely:

- **One core mechanic** you can explain in one sentence: "push crates onto switches to open the
  door". Prototype it with placeholder art first. If it is not fun with squares, art will not
  save it.
- **Three levels**, not thirty. Three levels prove the mechanic, the progression and the level
  flow. More levels are content, and content is easy to add to a finished game.
- **Everything around it**: main menu, pause, game over, options, saved progress and a package.
  This "boring" half takes as long as the mechanic, and it is what makes it a game rather than a
  prototype. You have built all of it once already.
- **Cut, don't add**. Keep a list of ideas for "version 2" and do not touch it until version 1 is
  packaged.
- **Ship it to someone**. A package played by one friend teaches you more than another month of
  features.

### A checklist from the course

Use the course as a checklist for your game. Each line is a chapter you can reread when you get
there.

| Step | Chapter |
|---|---|
| Project created, Paper2D on, startup and default maps set, version `0.1.0` | 1 |
| C++ module, `Build.cs`, log category, game mode with a Blueprint child | 2 |
| Developer settings class; game instance | 3 |
| Art imported with Paper2D texture settings; sprites, flipbooks; 2D rendering settings | 4 |
| Actors with components; spawning | 5 |
| Tile set, tile maps for the levels, orthographic camera | 6 |
| Game mode, game state, player controller, pawn: who owns what | 7 |
| Input actions and mapping context; rebindable keys | 8 |
| Movement and timing: tick, timers, fixed steps if needed | 9 |
| Collision channels and profiles; overlaps for pickups | 10 |
| Interactable interface; delegates for game events; win and lose rules | 11 |
| Animation state from code; one-shot effects | 12 |
| Level data asset; per-level settings | 13 |
| Level flow subsystem with fades and a loading screen | 14 |
| HUD | 15 |
| Main menu, pause menu, options, input modes | 16 |
| Sounds, music, volume settings | 17 |
| Save game: progress and settings | 18 |
| Cheats, debug draw, stats; profile once | 19 |
| Packaging settings, version, Shipping package tested on another PC | 20 |

### Other kinds of 2D games

GridSnake moves on a grid. Two other common shapes:

**Top-down, free movement** (twin-stick, adventure, shooters): use `MGPawn2D` above. Its
`UFloatingPawnMovement` handles acceleration and sliding along walls.

**Platformers** (gravity, jumping, slopes): use **`APaperCharacter`** (`PaperCharacter.h`, Paper2D
module). It is an `ACharacter` with a flipbook component as its visible part. It brings a capsule
and the full `UCharacterMovementComponent`, which already does walking, falling, jumping, air
control and slopes. Constrain it to the X-Z plane, as with the pawn above:

```cpp
#include "PaperCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

ADPPlayerCharacter::ADPPlayerCharacter()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->SetPlaneConstraintEnabled(true);
	Move->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);
	Move->bOrientRotationToMovement = false; // flip the sprite yourself, as in MGPawn2D
	Move->JumpZVelocity = 600.0f;
	Move->GravityScale = 2.0f;               // snappier than real-world gravity
}
```

Bind a jump action to `ACharacter::Jump` and `StopJumping`, and movement to
`AddMovementInput(FVector::ForwardVector, Value)`. The flipbook component is `GetSprite()`. Check
the class page for `APaperCharacter` in your engine version before relying on details; it has been
stable for years, but Paper2D is updated rarely. The character movement component is large and
built for 3D. Most platformer feel comes from tuning its numbers (gravity, air control, braking),
so expose them in the Blueprint child and iterate.

**Physics-driven** games (bouncing, stacking) can use sprite collision shapes (the Sprite editor's
**Collision** mode, chapter 10) with **Simulate Physics** on, constrained to the plane with the
body's **Constraints > Lock Position Y** option, or similar, in the physics settings.

### Art workflow

- Draw in a pixel-art editor such as **Aseprite** (commercial) or **LibreSprite** (a free fork of an
  older Aseprite). Keep the source files in `RawAssets/`, outside `Content/`.
- Export each animation as a **sprite sheet** PNG. Import it with the Paper2D texture settings
  (chapter 4), then use **Sprite Actions > Extract Sprites** in grid mode and **Create Flipbook**.
- Paper2D can also import a sprite sheet **JSON** file (the TexturePacker "JSON Hash/Array" style
  that Aseprite can export too). Dragging the `.json` into the Content Browser creates the texture
  and sprites in one go, saving the manual extraction. This importer is a small part of Paper2D
  that is rarely documented. If the import does nothing, check that Paper2D's sprite sheet import
  is available in your version and fall back to Extract Sprites.
- Keep one pixel size throughout (GridSnake: 16 pixels per cell, 1 pixel per unit). Mixed pixel
  scales are the most visible sign of an unfinished 2D game.
- Placeholder first: `Tools/make_placeholder_art.py` from chapter 4 is a template for generating
  coloured squares with the right sizes and frame counts.

### Source control

Use source control from the first day, even alone. It is your undo history across days, and your
backup.

- **Git with LFS** is the simple choice for one person or a small team. Start from the course's
  `.gitignore`: it ignores `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, IDE files
  and packaged builds, and keeps the platform icons under `Build/`. It also ignores `RawAssets/`,
  because the course only generates placeholder art there. **Delete that line** in your game,
  where `RawAssets/` holds source art you must not lose. Unreal's `.uasset` and `.umap`
  files are binary and large, so store them with [Git LFS](https://git-lfs.com/). A
  `.gitattributes` at the repository root:

  ```text
  *.uasset filter=lfs diff=lfs merge=lfs -text
  *.umap   filter=lfs diff=lfs merge=lfs -text
  *.png    filter=lfs diff=lfs merge=lfs -text
  *.wav    filter=lfs diff=lfs merge=lfs -text
  *.aseprite filter=lfs diff=lfs merge=lfs -text
  ```

  Run `git lfs install` once per machine before the first commit of assets.
- **Binary files cannot be merged.** If two people change the same Blueprint or map, one change is
  lost. Agree who edits what, or use file locking (Git LFS supports locks with the `lockable`
  attribute and `git lfs lock`).
- **Perforce** (Helix Core) is what Epic recommends for teams, and has first-class editor
  integration with exclusive checkout. The editor's source control integration (the
  **Revision Control** button in the status bar, called **Source Control** in older versions) also
  has a Git provider.

## Where to go next

The course covered the core. These are the next topics, roughly in the order a 2D developer meets
them. All Epic links are in [REFERENCES.md](REFERENCES.md#chapter-20--ship-it-then-make-your-own).

- **PaperZD**: a third-party plugin, available on Fab, Epic's marketplace. It adds animation
  graphs, state machines and animation notifies to Paper2D flipbooks, much like the Animation
  Blueprints 3D characters use. When your character has more than a handful of animations,
  `Tick`-based switching (chapter 12, `MGPawn2D`) gets unwieldy, and PaperZD is the usual answer.
  Search for it on Fab and check that it supports your engine version.
- **CommonUI**: Epic's plugin for menus that work with gamepad, keyboard and mouse alike. It
  handles input routing, focus, button prompts that change with the device, and stacks of screens.
  Use it once you want controller support in your menus.
- **Gameplay Ability System** (GAS): a framework for abilities, cooldowns, costs, attributes such as
  health and mana, and effects such as buffs and damage over time. It is complex to set up and
  meant for games with many abilities, such as RPGs and action games. A small game does not need
  it, but it is worth knowing it exists before you write your own.
- **Networking and multiplayer**: replication, RPCs, server authority. The gameplay framework you
  learned (game mode on the server, game state and player state replicated) was designed for it.
  Multiplayer is a big step; ship a single-player game first.
- **Steam**: to sell on Steam you need a Steamworks partner account and an app ID. Valve's
  Steamworks documentation covers the store side. Unreal's **Online Subsystem Steam** plugin
  connects the game to achievements, leaderboards, cloud saves and matchmaking. Enable the plugin,
  add the configuration from Epic's page to `DefaultEngine.ini`, and package as in this chapter.
  You upload the packaged folder with Valve's tools. Achievements and cloud saves are what a
  small single-player game usually wants first.
- **World Partition** and **PCG** (Procedural Content Generation): for large, open or generated
  3D worlds. Less relevant to a 2D game, but good to know if you move to 2.5D.
- **Lyra**: Epic's large sample game, built on CommonUI, GAS and modular gameplay. It is too big to
  copy, but a reference for how Epic structures a full game.
- **Coming from Unity?** Epic's *Unreal Engine for Unity Developers* page maps the concepts
  (GameObject → Actor, MonoBehaviour → component, prefab → Blueprint class).
- **Tom Looman's C++ guide**: a long, practical community guide to Unreal C++, with many of this
  course's topics in more depth.
- **The engine source**: once you link your Epic and GitHub accounts, you can read the engine's
  source. It is the final answer to "what does this function really do?" Reading
  `UPaperFlipbookComponent` or `UCharacterMovementComponent` teaches more than any page.

And the documentation itself: the API reference has a page for every class used in this course.
When the editor and a page disagree, trust the editor, as this course has asked from chapter 1.

## Summary

This chapter:

- Packaging runs **build, cook, stage, package, archive**. Cooking converts assets for the
  platform and includes only what is referenced from the cooked maps, plus the folders you list.
- `[/Script/UnrealEd.ProjectPackagingSettings]` holds the Build Configuration, the maps to cook,
  the always-cooked folders and the container options. Anything reached only through config or
  developer settings must be cooked explicitly.
- Package from **Platforms > *platform* > Package Project**, or with `RunUAT BuildCookRun`
  (`-build -cook -stage -pak -iostore -package -archive`). Success ends with `BUILD SUCCESSFUL`.
- Shipping has no console, cheats, `check`, debug drawing, stats or log. Development keeps them.
- `ProjectVersion` in `GeneralProjectSettings`, read with `GConfig->GetString(..., GGameIni)`,
  labels every build.
- Test the package as a player: fresh saves in the per-user folder, every level, a restart, a
  second machine.

The course:

- **Foundations** (chapters 1 to 3): the editor, config files, C++ modules and the build, the
  UObject system and reflection, developer settings.
- **2D on screen** (4 to 6): Paper2D textures, sprites, flipbooks, tile maps and the orthographic
  camera, with rendering settings for crisp pixel art.
- **Gameplay** (7 to 13): the gameplay framework, Enhanced Input, ticks and timers, collision
  channels, interfaces and delegates, flipbook animation from code, data assets.
- **A complete game** (14 to 18): level flow with fades and loading screens, UMG HUD and menus,
  sound and music, saving.
- **Finishing** (19 and 20): debugging and profiling tools, packaging, and the plan for your own
  game.

You can now start a 2D game in Unreal Engine 5 from an empty project, structure it so it keeps
growing, and ship it. Keep the first one small.

## Exercises

1. **Development versus Shipping.** Package a Development build next to the Shipping one. Compare
   their sizes, and check which one has a console and a log.

   <details><summary>Solution</summary>

   Run `package-windows.bat Development` with a different `OUTPUT_DIR` (or choose **Development**
   in the Platforms menu's build configuration and package into a second folder). UAT receives
   `-clientconfig=Development`.

   - The Development game's executable is `Binaries\Win64\GridSnake.exe`; Shipping's is
     `GridSnake-Win64-Shipping.exe`. The Development executable is usually a bit larger, because
     it carries the console, stats and debug code. The content containers are about the same size.
   - In the Development build, the backtick key opens the console; `stat fps`, `GS.Debug.Grid 1`
     and `GSCollectAll` work. In Shipping nothing happens.
   - The Development build writes `Windows\GridSnake\Saved\Logs\GridSnake.log`. Shipping writes
     none, and keeps its saves in `%LOCALAPPDATA%\GridSnake\Saved\SaveGames\`. Development keeps
     its saves next to the game. The two builds do **not** share progress.

   Give testers Development builds when you want logs from them, and players Shipping builds.

   </details>

2. **An icon and a splash screen.** Give the Windows build its own icon and startup splash image.

   <details><summary>Solution</summary>

   1. Make a 256×256 `.ico` file (most image editors and free converters export `.ico`) and a splash
      image (a `.bmp` or `.png`, depending on what the field accepts in your version).
   2. **Edit > Project Settings > Platforms > Windows**: under the icon setting (**Game Icon**),
      click the file picker and choose the `.ico`. Under **Splash**, set the **Game Splash** image.
      The editor copies the files into the project, under `Build/Windows/` (and
      `Build/Windows/Resources/` in some versions).
   3. Commit them. The course's `.gitignore` ignores everything in `Build/` *except* `.ico`,
      `.png` and `.bmp` files, so these are kept while staging output is not.
   4. Package again. The executable and the window use the new icon, and the splash shows while
      the game loads. In the editor the splash for **Editor Splash** is a separate setting.

   </details>

3. **Plan your own game on one page.** Use the course's checklist to plan a small game of your
   own, and compare with the example plan for a "top-down dungeon pusher".

   <details><summary>Solution</summary>

   An example plan, one page, written before any code:

   ```text
   DUNGEON PUSHER (prefix DP) - push crates onto switches; all switches pressed opens the exit.

   Mechanic   Grid movement, one cell per key press (GridSnake's Step without the timer).
              Moving into a crate pushes it if the cell behind it is free.
   Levels     3 tile maps, 10x8 cells. Level 1 teaches pushing, 2 adds a wall maze,
              3 needs pushing a crate twice.
   Win/lose   Win: all switches pressed, walk into the exit. No lose state; "R" restarts.
   Rules     Moves counter; best move count per level is saved.

   Reuse      StarterModule (settings, flow, game mode, controller with fixed view target).
              GSBoard -> DPBoard (cells, IsCellBlocked, camera framing).
              IGSSnakeInteractable -> IDPPushable / IDPSteppable (crate, switch, exit).
              GSSaveSubsystem -> DPSaveSubsystem (unlocks + best moves).
              GSAudioSubsystem, menu widgets, HUD (moves, switches 0/3).
   New        DPPlayer pawn (grid steps on input), DPCrate (can it move? then move),
              DPSwitch (pressed while a crate is on it), DPExit (opens when all pressed).

   Chapters   1-4 setup and art (placeholder script: player, crate, switch, exit, tiles).
              6 tile maps, 7 framework, 8 input (four discrete actions), 10 collision
              (Grid channel for walls and crates), 11 interfaces and the "all switches"
              delegate, 12 push/press animations, 13 level list with par moves,
              14-18 flow, HUD, menus, sound, save, 19 cheats (DPSolve, DPWin), 20 ship.

   Not in v1  Undo, level editor, more than 3 levels, controller support, Steam.
   Done when  A Shipping package with 3 levels, menu, save, sound runs on a friend's PC.
   ```

   The important parts are the one-sentence mechanic, the short level list, the explicit "not in
   v1" list, and the "done when" line. Each row of the course checklist should be covered by a
   line of the plan, or deliberately skipped.

   </details>

4. **Port a system.** Copy GridSnake's save subsystem into a project made from the starter module.

   <details><summary>Solution</summary>

   1. Copy `project/ch20/Source/GridSnake/Save/GSSaveGame.h/.cpp` and `GSSaveSubsystem.h/.cpp`
      into `Source/MyGame/Save/`.
   2. Rename the files and classes: `GSSaveGame` → `MGSaveGame`, `UGSSaveGame` → `UMGSaveGame`,
      `UGSSaveSubsystem` → `UMGSaveSubsystem`, and the includes, including
      `"MGSaveGame.generated.h"` and `"MGSaveSubsystem.generated.h"`.
   3. Replace `#include "GridSnake.h"` with `#include "MyGame.h"` and `LogGridSnake` with
      `LogMyGame`. Change the slot name (`TEXT("GridSnake")`) to your game's.
   4. Remove what your game does not have: GridSnake's save stores per-level best scores, unlocked
      levels and the volume settings. Keep the fields you need. Any function that uses
      `UGSSettings::GetLevelList()` can use `UMGSettings::Get()->Levels.Num()` instead.
   5. Build.cs: the save system needs only `Engine` (`USaveGame`, `UGameplayStatics`), which the
      starter already has. Close the editor and build, because there are new classes.
   6. Call it where GridSnake did: `GetGameInstance()->GetSubsystem<UMGSaveSubsystem>()` from the
      flow subsystem when a level is completed (`UnlockLevel`, `Save`), and from the menu to show
      unlocked levels.

   Expected Output Log (filter: `LogMyGame`) on the first Play, as the subsystem initialises with
   no save file (with the slot renamed to `MyGame`):

   ```text
   LogMyGame: No save game in slot MyGame - starting fresh
   ```

   After you complete a level and the save finishes:

   ```text
   LogMyGame: Save to slot MyGame succeeded
   ```

   </details>

---

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-20--ship-it-then-make-your-own)
