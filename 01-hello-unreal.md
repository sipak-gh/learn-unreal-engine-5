# Chapter 1 — Hello, Unreal

Unreal Engine 5.8 is a 3D engine, yet it is also a strong choice for 2D games. It ships an official
2D toolkit (Paper2D), a mature editor, two ways to program it (C++ and Blueprints), and one-click
packaging for many platforms. This chapter installs the engine and a C++ toolchain, creates the
`GridSnake` project, and tours the editor panels the rest of the course refers to by name. You will
also learn levels, Play In Editor, Project Settings and the config files behind them, and a handful
of console commands. The project gains its first files: a `.uproject` with Paper2D enabled, an
empty level `L_Level01`, and the project settings that open it. Next:
[Chapter 2](02-projects-modules-and-the-build.md) (turning the project into a C++ project).

## What you will build

Over twenty chapters you will build **GridSnake**, a top-down Snake game drawn with Paper2D. Each
level is a tile map with walls. The snake moves one grid cell per step and buffers your turns, so
quick double-taps work. Each level holds three gems; collecting all three opens the exit, and
entering the exit fades to a loading screen and opens the next level. Around that core you will add
a main menu, a pause menu, level-complete and game-over screens, a HUD, music that reacts to your
progress, volume sliders, saved progress, console cheats, debug drawing, and finally a packaged
Shipping build.

Snake is small enough to finish, yet it touches every system a 2D game needs. Because it lives on a
grid, there is no physics to tune. That leaves room to study Unreal's architecture: framework
classes, reflection, assets, level flow and UI. The grid logic itself is plain C++ you can reason
about.

The course has five parts:

| Part | Chapters | What it covers |
|---|---|---|
| Foundations | 1–3 | The editor, projects and modules, the build, UObject and reflection |
| The 2D world | 4–6 | Paper2D art, actors and components, tile maps and an orthographic camera |
| Gameplay | 7–12 | The Gameplay Framework, Enhanced Input, ticks and timers, collision, game rules, animation |
| Game structure | 13–18 | Data-driven levels, level transitions, HUD, menus, sound, saving |
| Craft | 19–20 | Debugging and profiling, packaging, and starting your own game |

### How to read this course

Each chapter explains ideas with small examples in `examples/chNN/` and then grows the project. The
complete project at the end of chapter NN is in `project/chNN/`. Unreal assets (`.uasset`,
`.umap`) are binary, so the snapshots contain only text files: config files, C++ source, and an
`ASSETS.md` that lists every asset to create in the editor, with exact names and settings.

One honesty note, repeated in the README. The environment that produced this course cannot run
Unreal Engine: it has no GPU, no Epic account, and not the 100+ GB the engine needs. **No code in
this course was compiled or run by the authors.** Every API, macro, module name, header and editor
path was checked against Epic's UE 5.8 documentation and API reference. Where a book would show
program output, this course shows a block labelled **Expected Output Log**: the lines the code's log
calls will print, with variable parts marked like `<this>`. Where an editor label is uncertain, the
text says so. If something does not match your 5.8 editor, trust the editor and please report it.

This chapter has no C++ yet, so it has no expected logs either.

## Installing Unreal Engine 5.8

Unreal Engine 5.8 was released on June 23, 2026. Epic describes it as the last planned major UE5
release before UE6, so it is a stable target: what you learn here carries over.

### Windows and macOS: the Epic Games Launcher

1. Download the **Epic Games Launcher** from Epic's site and sign in with an Epic account.
2. Click **Unreal Engine** in the left sidebar, then the **Library** tab at the top.
3. Next to **Engine Versions**, click **+**. A new engine slot appears.
4. Click the version number on the slot and pick **5.8.x** (the latest 5.8 patch).
5. Click **Install**. Choose an install location on a fast drive with plenty of free space.
6. Before the download starts, open **Options** on the slot (the small arrow next to Launch, or
   the dialog that appears during install). Recommended choices:
   - **Core Components**: required.
   - **Starter Content** and **Templates and Feature Packs**: keep them. The Project Browser needs
     the templates; starter content is small.
   - **Editor symbols for debugging**: optional and large (tens of GB). They let your debugger show
     engine call stacks with function names. Worth it once you write C++; you can add them later
     from the same Options dialog.
   - **Target platforms**: you need only your own desktop platform for this course. Leave Android,
     iOS and consoles unchecked.

A screenshot of the Library tab with one 5.8 slot and its Options dialog would help here; the
labels above are what to look for.

### Linux

Epic publishes Linux binaries as a zip on its Linux download page (an Epic account is required).
Unzip it to a folder you own and run `Engine/Binaries/Linux/UnrealEditor`. The alternative is a
source build: link your GitHub account to your Epic account, clone the `EpicGames/UnrealEngine`
repository at the 5.8 release branch, and run `Setup.sh`, `GenerateProjectFiles.sh` and `make`.
The exact file names and steps change between releases, so follow Epic's Linux page for 5.8 rather
than a blog post.

### What to expect

- **Disk:** plan for about 100 GB for the engine, its derived data and your projects.
- **Memory:** 16 GB is the minimum; 32 GB is noticeably smoother, especially while compiling C++.
- **GPU:** DirectX 12 (Windows), Metal (macOS) or Vulkan (Linux) capable. Check
  *Hardware and Software Specifications* in Epic's docs for 5.8.
- **First launch is slow.** The editor compiles shaders for your GPU the first time it opens a
  project. A progress counter shows in the lower right ("Compiling Shaders" with a number). This can
  take many minutes; later launches reuse the results from the Derived Data Cache (DDC).

## Your C++ toolchain

GridSnake starts as a Blueprint project and becomes a C++ project in chapter 2. Install the C++
toolchain now, though, for two reasons. It is a large download that is better done once while the
engine installs. And when the editor first adds C++ to a project, it immediately tries to compile;
if no compiler is found, you get an error dialog instead of a working project.

| Platform | Toolchain for UE 5.8 |
|---|---|
| Windows | **Visual Studio 2026** (recommended) or **Visual Studio 2022 17.14+**. In the Visual Studio Installer select the workloads **Game development with C++** and **.NET desktop development**. Under the C++ game workload's optional components, make sure the **Unreal Engine installer** item, an MSVC toolset (14.50 recommended, 14.38 minimum) and a Windows SDK (10.0.26100 recommended, 10.0.22621 minimum) are ticked. |
| Windows (alternative) | **JetBrains Rider** opens Unreal projects directly. It still uses the MSVC compiler and Windows SDK, so install the Visual Studio Build Tools with the same components. |
| macOS | **Xcode 15.4 or later** (15.2 minimum) from the App Store, then run it once to accept the license. |
| Linux | Epic's bundled **clang 20.1.8** cross-toolchain, installed by `Setup.sh` or included with the binary package. Any editor works: VS Code, Rider or CLion. |

Why .NET? UnrealBuildTool, the program that builds every Unreal C++ project, is written in C#.
Chapter 2 explains what it does.

Epic's guides *Setting Up Visual Studio for Unreal Engine* and the *Rider* setup guide walk
through the installer screens; both are linked in [REFERENCES.md](REFERENCES.md). You can verify
the Visual Studio install later from the editor: when a C++ project is open, **Tools > Open Visual
Studio** must work.

## Creating the GridSnake project

Launch 5.8 from the Library tab (**Launch** on the 5.8 slot). The **Unreal Project Browser** opens.
On the left are categories; on the right, templates for the selected category and a **Project
Defaults** panel.

1. Select **Games** on the left, then the **Blank** template.
2. In **Project Defaults** set:
   - **Blueprint** (not C++). Chapter 2 adds C++ through the editor, which is the realistic way:
     many projects start as Blueprint prototypes.
   - **Target Platform**: **Desktop**.
   - **Quality Preset**: **Maximum**.
   - **Starter Content**: **off**. GridSnake generates its own art; starter content would add
     hundreds of unrelated assets.
   - **Raytracing**: **off**. A 2D game gains nothing from it.
3. **Project Location**: a short path on a fast drive, such as `D:\Projects` or `~/Projects`.
   Long paths can break builds on Windows, which has path length limits.
4. **Project Name**: `GridSnake`.
5. Click **Create**.

Why **Blank**? Unreal 4 shipped a "2D Side Scroller" template built on Paper2D. The 5.x Project
Browser has no Paper2D template: the side-scroller and platformer templates use 3D characters. Check
the **Games** templates in your version, but Blank is the right start either way. You will build
every 2D piece yourself, which is the point of the course.

The editor opens on a default level with a floor, a sky and lights. You will replace it in a moment.

### The project on disk

Open the project folder in your file manager. You will see:

```
GridSnake/
  GridSnake.uproject      the project file: engine version, modules, plugins
  Config/                 your settings: DefaultEngine.ini, DefaultGame.ini, ...
  Content/                your assets (.uasset, .umap)
  Saved/                  logs, autosaves, user settings, screenshots   (generated)
  Intermediate/           build and temporary files                     (generated)
  DerivedDataCache/       cached compiled data, if the project keeps one (generated)
```

The first three are the project. Everything marked *generated* is rebuilt by the editor and is
safe to delete when the editor is closed. That is why the course's `.gitignore` excludes
`Saved/`, `Intermediate/`, `DerivedDataCache/` and, from chapter 2 on, `Binaries/` and the IDE
files. Committing them would bloat a repository with gigabytes of files that differ on every
machine. Depending on settings, the DDC may live in a shared folder outside the project instead;
either way you never edit it.

`GridSnake.uproject` is a small JSON file. Double-clicking it opens the project in the engine
version named in `EngineAssociation`. You will read it in a moment.

## Enabling Paper2D

Paper2D is a first-party plugin: it ships with the engine and Epic maintains it. It provides
sprites, flipbooks (frame-by-frame animations), tile sets and tile maps. In 5.x it is enabled by
default. Verify it:

1. **Edit > Plugins**. The Plugins window opens.
2. Type `Paper2D` in the search box. The plugin appears in the **2D** category.
3. Its **Enabled** checkbox must be ticked. If you had to tick it, click **Restart Now** at the
   bottom of the window.

A plugin that is enabled by default does not have to appear in the `.uproject`. GridSnake lists
it anyway, so the project states its dependency explicitly and keeps working even if a later engine
changes the default. Close the editor and make the `Plugins` array of `GridSnake.uproject` match the
chapter 1 snapshot:

`project/ch01/GridSnake.uproject`

```json
{
	"FileVersion": 3,
	"EngineAssociation": "5.8",
	"Category": "",
	"Description": "",
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

- `EngineAssociation` names the engine version that opens the project. Launcher installs use the
  version number; source builds write a GUID instead.
- `ModelingToolsEditorMode` was written by the Blank template. `TargetAllowList: ["Editor"]` loads
  it only in the editor, never in the packaged game.
- `Paper2D` is the line GridSnake adds. Unticking the plugin in **Edit > Plugins** would turn this
  into `"Enabled": false`.
- The file uses tabs. The editor rewrites it whenever you change plugins, so keep it valid JSON: a
  stray comma stops the project from opening.

Reopen the project by double-clicking `GridSnake.uproject`.

### What is new for 2D in 5.8

Paper2D itself has no new features in 5.8. The 5.8 release notes do list **Orthographic Viewport
Improvements**: the top, side and front views draw with less visual noise and keep editing tools
visible while you work. They matter to you because a 2D game is edited mostly in orthographic
views. Skim that section of the release notes once you finish the editor tour below.
Third-party 2D add-ons such as PaperZD (animation state machines for Paper2D) are popular, but the
course uses only first-party plugins. Chapter 20 points you to them.

## A tour of the editor

The level editor is one window made of dockable panels. Every panel can be moved, tabbed or closed;
**Window > Load Layout > Default Editor Layout** restores the default if you lose something. A
labelled screenshot of the default layout would help here. In words, from the top:

### Menu bar and main toolbar

- **Menu bar**: **File**, **Edit**, **Window**, **Tools**, **Build**, **Select**, **Actor**,
  **Help**. You will mostly use File (levels), Edit (Project Settings, Plugins, Editor
  Preferences), Window (panels) and Tools (C++ classes, from chapter 2).
- **Main toolbar**, under the menu bar, left to right: a **Save** button, the **mode** selector
  (Selection Mode is the default), **Quick Add** (the cube-with-plus icon: add actors), the
  **Blueprints** and **Cinematics** menus, the **Play** controls in the middle, then **Platforms**
  (packaging, chapter 20) and **Settings** (editor and project settings shortcuts) on the right.

### The viewport

The large central area is the **level viewport**: a live view of the level you are editing.

- **Navigation in perspective view.** Hold the right mouse button and use **W A S D** to fly, **Q**
  and **E** to go down and up; the mouse wheel while flying changes speed. **F** focuses the
  selected actor. **Alt + left drag** orbits around it.
- **View type.** The drop-down at the top-left of the viewport (it reads **Perspective**) switches
  to orthographic views: **Top**, **Bottom**, **Left**, **Right**, **Front**, **Back**. An
  orthographic view has no perspective: parallel lines stay parallel and size does not shrink with
  distance. In those views, right-drag pans and the wheel zooms. Paper2D sprites lie in the X-Z
  plane, so you will edit levels in a side view that looks along the Y axis: **Left** or
  **Right**. **Front** and **Back** look along X and see sprites edge-on. Which of Left and Right
  shows +X to the right is checked in chapter 4.
- **View mode.** The next drop-down (it reads **Lit**) switches between **Lit**, **Unlit**,
  **Wireframe** and more. **Unlit** shows colours without lighting, which is how 2D sprites look.
- **Show flags.** The **Show** menu turns categories of things on and off, such as the grid,
  collision or bounds.

### Outliner and Details

- The **Outliner** (top right) lists every actor in the level. Click an actor to select it, rename
  it with F2, and organise actors in folders.
- The **Details** panel (below it) shows the selected actor's properties: its transform (Location,
  Rotation, Scale), its components, and every property its class exposes. Chapter 3 explains how a
  C++ property appears there.

### Content Browser and Content Drawer

The **Content Browser** shows the project's assets as a folder tree under **Content** (in paths:
`/Game/`). You can open it two ways:

- The **Content Drawer**: click **Content Drawer** at the bottom-left of the window, or press
  **Ctrl+Space**. It slides up and hides again when you click elsewhere. Good for a quick drag.
- A docked browser: **Window > Content Browser > Content Browser 1**, or **Dock in Layout** at the
  top-right of the drawer.

Right-click in the asset area to create assets and folders; right-click an asset for its actions.
Assets are moved and renamed *inside* the Content Browser, never in the file manager, because other
assets reference them by path and the editor fixes those references only when it does the move.

### Place Actors

**Window > Place Actors** opens a panel of actor types: **Basic** (cubes, spheres, cameras, Player
Start), **Lights**, **Shapes**, **Visual Effects** and **All Classes**. Drag an entry into the
viewport to place it. The toolbar's **Quick Add** button offers the same list as a menu.

### Output Log

**Window > Output Log** (or **Output Log** in the bottom status bar) shows everything the engine
logs. In this course it plays the role of a terminal: every "expected output" is a set of lines
in this panel.

- Each line starts with a **category**, such as `LogTemp:` or, from chapter 2, `LogGridSnake:`.
- Warnings are yellow and errors are red. A plain line is a `Log` message; other verbosities are
  named after the category (`LogTemp: Warning: ...`).
- The **search box** filters lines by text. The **Filters** drop-down hides message, warning or
  error lines and has a **Categories** list for showing only selected categories.
- Right-click in the log and choose **Clear Log** before a test run, so you see only new lines.

The same lines go to a file on disk, which survives a crash (see exercise 2).

### The console

The console runs commands that query or change the engine at run time.

- In the editor: the **Cmd** box in the bottom status bar, next to Output Log.
- While playing: the backtick key (**`**, left of **1** on a US keyboard). Press it once for a
  one-line console, twice for a larger one that shows output. On other keyboard layouts the key may
  differ; the key is configured as `ConsoleKeys` in `Config/DefaultInput.ini`.

Commands are not case-sensitive, and typing the first letters lists matching commands.
[Console commands worth knowing](#console-commands-worth-knowing) below has a list to try.

## Levels, actors and Play In Editor

A **level** (or *map*; the file extension is `.umap`) is a collection of **actors**: objects that
exist in the world, have a transform and can be placed, such as a mesh, a light, a camera or, later,
the snake. Playing the game means loading a level into a **world** and starting its actors.

### Create L_Level01

The level the editor opened with comes from the template. GridSnake starts from nothing.

1. In the Content Browser, select **Content**, right-click and choose **New Folder**. Name it
   `GridSnake`. Inside it create a folder `Maps`.
2. **File > New Level**. A dialog offers several templates (**Basic**, **Empty Level**, **Open
   World** and so on). Choose **Empty Level** and click **Create**.
3. The viewport turns into an empty void with a grid. The Outliner is empty.
4. **File > Save Current Level As...**. Select `Content/GridSnake/Maps`, type `L_Level01` as the
   name, and click **Save**.

The asset path is now `/Game/GridSnake/Maps/L_Level01`. `/Game/` is the editor's name for the
`Content/` folder. The course keeps all its assets under `/Game/GridSnake/` with these folders,
created as the chapters need them:

| Folder | Holds | Prefixes |
|---|---|---|
| `Maps` | levels | `L_` |
| `Blueprints` | Blueprint classes | `BP_` |
| `Art/Textures`, `Art/Sprites`, `Art/Flipbooks`, `Art/Tiles` | Paper2D art | `T_`, `S_`, `FB_`, `TS_`/`TM_` |
| `Input` | input actions and mapping contexts | `IA_`, `IMC_` |
| `Data` | data assets and curves | `DA_`, `C_` |
| `UI` | widgets | `W_` |
| `Audio` | sounds, MetaSounds, sound classes and mixes | `SW_`, `MSS_`, `SC_`, `SMix_` |

The prefixes are a common Unreal convention. They make assets of different types easy to tell
apart in searches and asset pickers, where only names are shown.

**Why an Empty Level and not Basic?** Basic adds a floor, a sky, a sun and fog. A 2D game shows
flat sprites in front of an orthographic camera and needs none of it. Starting empty also means
everything in the level is something you placed on purpose.

### Actors: place a few shapes

Place some actors to see how levels work. You will delete them afterwards.

1. **Quick Add > Shapes > Cube** (or drag **Cube** from **Place Actors > Basic**). A cube appears
   in front of the camera and is selected.
2. In **Details**, set **Location** to `0, 0, 0`. Look at the Outliner: it lists `Cube`.
3. Add a **Sphere** and set its Location to `200, 0, 0`.
4. The shapes look black. An empty level has no lights, and the **Lit** view mode shows what a
   camera would see. Switch the view mode to **Unlit** and the shapes appear in their flat grey.
   That is your first lesson for a 2D game: Paper2D sprites are usually *unlit*, so they look the
   same with no lights at all.
5. Select the cube, press **W**, **E** and **R** to switch the gizmo between move, rotate and scale,
   and drag the coloured handles. Red is X, green is Y, blue is Z. Unreal is Z-up.
6. Switch the viewport to **Left** and select the cube. If the gizmo shows the red (X) handle
   across and the blue (Z) handle up, you are looking along the Y axis at the X-Z plane: the plane
   Paper2D draws in. If you see the green (Y) handle across instead, try **Right**, **Front** or
   **Back**; view names have shifted between engine versions, and the gizmo colours never lie.

Each placed actor is an *instance* of a *class*: `StaticMeshActor` here, with a cube mesh assigned.
Chapter 5 writes your own actor classes.

### Play In Editor

The **Play** button in the main toolbar starts the game inside the editor. This is **Play In Editor
(PIE)**: the editor makes a copy of the level in a separate world and plays it. The editor's level
is not touched, so changes made while playing are lost when you stop.

The vertical ellipsis button next to Play opens the **Play Options** menu, which chooses
*where* to play:

- **Selected Viewport**: in the level viewport. Fastest; the default.
- **New Editor Window (PIE)**: in a separate window, closer to the real game resolution.
- **Standalone Game**: launches a separate game process from the editor. Slower to start, but it
  behaves like a packaged game in ways PIE cannot (chapter 14's loading screen only shows there).
- **Mobile Preview** and **VR Preview**: for mobile and VR projects; GridSnake does not use them.

The same menu offers **Simulate** (shortcut **Alt+S**, **Option+S** on macOS). It runs the level's
logic without spawning a player: you keep the editor camera and can select actors while they move.
Useful for watching gameplay from outside.

Try it: click **Play**. The editor has no GameMode configured for GridSnake yet, so the engine's
default spawns a free-flying default pawn: move the mouse to look and use WASD to fly around the
shapes. Press **Esc** to stop. While playing, **Shift+F1** releases the mouse cursor so you can
click editor panels; click back into the viewport to recapture it. **F8** detaches from the player
("ejects") so you can select actors in the playing world.

Now clean up: select the cube and the sphere in the Outliner, press **Delete**, switch the view mode
back to **Lit** if you like, and save with **Ctrl+S**. `L_Level01` is empty again.

## Project Settings and config files

**Edit > Project Settings** opens a window of every project-wide setting, grouped in a list on the
left: **Project**, **Game**, **Engine**, **Editor**, **Platforms** and **Plugins**. The search box at
the top finds a setting by name, which is often faster than browsing.

### Description

**Project > Description** holds the project's identity. Set:

| Field (section) | Value |
|---|---|
| **Project Name** (About) | `GridSnake` |
| **Description** (About) | `A grid-based 2D snake game built with Paper2D - course project for "Learn Unreal Engine 5".` |
| **Project Version** (About) | `0.1.0` |
| **Copyright Notice** (Legal) | `GridSnake - course project for "Learn Unreal Engine 5".` |

The copyright notice is not just decoration: the C++ class wizard in chapter 2 stamps it at the top
of new files.

### Maps & Modes

**Project > Maps & Modes** chooses which level opens and which game rules apply.

1. Expand **Default Maps**.
2. Set **Editor Startup Map** to `L_Level01`: the level the editor opens with.
3. Set **Game Default Map** to `L_Level01`: the level a packaged or standalone game starts with.

The **Default Modes** section below sets the **Default GameMode**; chapter 2 points it at GridSnake's
own game mode.

### Where settings go: config files

Project Settings has no save button. Every change is written at once into an `.ini` file in
`Config/`. After the steps above, `Config/DefaultEngine.ini` contains this section (the chapter 1
snapshot shows the lines the course relies on):

`project/ch01/Config/DefaultEngine.ini`

```ini
; GridSnake - DefaultEngine.ini
; The Blank template writes most of this file; Project Settings edits it for you.
; Lines that the course adds or changes are marked "GridSnake:". If your generated
; file contains extra template lines not shown here, keep them.

[/Script/EngineSettings.GameMapsSettings]
; GridSnake (ch01): Project Settings > Project > Maps & Modes
GameDefaultMap=/Game/GridSnake/Maps/L_Level01.L_Level01
EditorStartupMap=/Game/GridSnake/Maps/L_Level01.L_Level01

[/Script/Engine.RendererSettings]
r.ReflectionMethod=1
r.GenerateMeshDistanceFields=True
r.DynamicGlobalIlluminationMethod=1
r.Shadow.Virtual.Enable=1
r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange=True
r.AllowStaticLighting=False

[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_DX12

[/Script/HardwareTargeting.HardwareTargetingSettings]
TargetedHardwareClass=Desktop
AppliedTargetedHardwareClass=Desktop
DefaultGraphicsPerformance=Maximum
AppliedDefaultGraphicsPerformance=Maximum
```

How to read it:

- **Section names are class paths.** `[/Script/EngineSettings.GameMapsSettings]` means "the class
  `GameMapsSettings` in the C++ module `EngineSettings`". Each key is a property of that class. The
  Maps & Modes page is simply an editor for that class's properties, and chapter 3 shows how your
  own C++ classes get the same treatment.
- **Asset paths have two parts.** `/Game/GridSnake/Maps/L_Level01.L_Level01` is the *package* (the
  file, `/Game/GridSnake/Maps/L_Level01`) followed by `.` and the *object* inside it (the level,
  `L_Level01`). You will see this form in every asset reference.
- **The renderer and hardware sections** came from the template and the project defaults you picked
  (Desktop, Maximum, no ray tracing). Lumen (`r.DynamicGlobalIlluminationMethod=1`) and virtual
  shadow maps are UE5's defaults; they cost little in an unlit 2D game.
- **Comments start with `;`.** The editor does not write comments; the course's snapshots add them
  to mark which lines matter. When the editor saves this file it may reorder sections or drop the
  comments. That is fine: compare keys and values, not formatting.

The Description fields land in `Config/DefaultGame.ini`:

`project/ch01/Config/DefaultGame.ini`

```ini
; GridSnake - DefaultGame.ini
; The template also writes a ProjectID=<GUID> line here; keep yours.

[/Script/EngineSettings.GeneralProjectSettings]
; GridSnake (ch01): Project Settings > Project > Description
ProjectName=GridSnake
Description=A grid-based 2D snake game built with Paper2D - course project for "Learn Unreal Engine 5".
CopyrightNotice=GridSnake - course project for "Learn Unreal Engine 5".
ProjectVersion=0.1.0
```

The chapter's example file annotates each Description field with the key it becomes:

`examples/ch01/DefaultGame-snippet.ini`

```ini
; Chapter 1 example: the keys that Edit > Project Settings > Project > Description
; writes into Config/DefaultGame.ini. Do not paste this file into a project; the
; editor writes these lines for you. It shows what each field turns into.

[/Script/EngineSettings.GeneralProjectSettings]
; ProjectID=<32 hex digits>
;   Written by the Project Browser when the project is created: a GUID that
;   identifies the project. Keep yours; never copy it between projects.
; About > Project Name. The display name of the project.
ProjectName=GridSnake
; About > Description. Free text.
Description=A grid-based 2D snake game built with Paper2D - course project for "Learn Unreal Engine 5".
; Legal > Copyright Notice. Also stamped into new C++ files by the class wizard.
CopyrightNotice=GridSnake - course project for "Learn Unreal Engine 5".
; About > Project Version. Free text; code can read it (chapter 20 does).
ProjectVersion=0.1.0
; Optional fields GridSnake leaves empty. The editor writes a key only once you
; set it, so they are commented out here:
; CompanyName=Your Studio            (Publisher > Company Name)
; Homepage=https://example.com       (Publisher > Homepage)
; SupportContact=you@example.com     (Publisher > Support Contact)
```

- An `.ini` file has `[Section]` headers and `Key=Value` lines. Values are not quoted; everything
  after `=` is the value, quotes included.
- The editor writes a key only when its value differs from the engine default. That is why a fresh
  project's ini files are short, and why `CompanyName` does not appear until you set it.

The other two files came from the template unchanged:

`project/ch01/Config/DefaultInput.ini`

```ini
; GridSnake - DefaultInput.ini
; Written by the template. Enhanced Input is the default input system in UE5.

[/Script/Engine.InputSettings]
bAltEnterTogglesFullscreen=True
bF11TogglesFullscreen=True
bEnableLegacyInputScales=True
DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput
DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent
DefaultTouchInterface=None
-ConsoleKeys=Tilde
+ConsoleKeys=Tilde
```

`project/ch01/Config/DefaultEditor.ini`

```ini
; GridSnake - DefaultEditor.ini
; Written by the template. The course never edits it by hand; chapter 4's Paper2D import
; settings are saved here by the editor.

[EditoronlyBP]
bAllowClassAndBlueprintPinMatching=true
bReplaceBlueprintWithClass= true
bDontLoadBlueprintOutsideEditor= true
bBlueprintIsNotBlueprintType= true
```

- `DefaultPlayerInputClass` and `DefaultInputComponentClass` select **Enhanced Input**, UE5's input
  system. Chapter 8 builds on it. Values like `/Script/EnhancedInput.EnhancedPlayerInput` are class
  paths: the C++ class `EnhancedPlayerInput` in the module `EnhancedInput`.
- `ConsoleKeys` is an *array*. A leading `+` adds an element and a leading `-` removes one. The
  template first removes `Tilde` (so it is never listed twice) and then adds it. `Tilde` is the key
  that types `~` and `` ` `` on a US keyboard.
- `[EditoronlyBP]` holds editor-only Blueprint options the template sets. You will not touch them.
  The editor itself adds sections to this file when you change editor-side project settings; in
  chapter 4 that happens for the Paper2D import defaults.

### The config hierarchy

`DefaultEngine.ini` is not the whole story. Unreal builds each configuration (Engine, Game, Input,
Editor and others) by reading a stack of files, where each later file overrides the earlier ones:

1. **Engine base**: `Engine/Config/BaseEngine.ini` in the engine install. The engine's defaults.
   Never edit it.
2. **Engine platform**: for example `Engine/Config/Windows/BaseWindowsEngine.ini`.
3. **Project default**: `Config/DefaultEngine.ini`. Your project's settings; commit this.
4. **Project platform**: for example `Config/Windows/WindowsEngine.ini`, if it exists. Overrides
   for one platform.
5. **Saved (user)**: `Saved/Config/<Platform>/Engine.ini`. Settings changed at run time on this
   machine, such as graphics options a player picks. Not committed; deleting it resets them.

The rule to remember: **the project's `Default*.ini` files are the ones you edit and commit.** The
editor writes them when you change Project Settings; you can also edit them by hand while the editor
is closed. Chapter 3 adds a section of GridSnake's own.

## Console commands worth knowing

The console is the quickest way to measure and inspect a running game. Here is the chapter's
example list. Press **Play**, open the console with **`**, and try each command:

`examples/ch01/console-commands.txt`

```text
# Console commands to try in Play In Editor (chapter 1)
#
# Press Play, then open the console with the backtick key (`, left of 1 on a US
# keyboard). Type a command and press Enter. Commands are not case-sensitive.
# The Cmd box at the bottom of the editor window runs the same commands without
# Play. Lines starting with # are comments; do not type them.

stat fps
# A frame-rate counter appears in the top-right of the viewport.
# Run it again to hide it. Every "stat" command toggles.

stat unit
# Shows Frame, Game, Draw, GPU and RHIT times in milliseconds (and a few more
# rows in some versions). Frame is the total; the largest of the others is
# the bottleneck. In an empty level all of them are small.

t.MaxFPS 30
# Caps the frame rate at 30. Watch "stat fps" settle near 30 and Frame in
# "stat unit" rise to about 33 ms. "t.MaxFPS 0" removes the cap.

r.ScreenPercentage 50
# Renders the 3D scene at half resolution and scales it up. Edges get blurry.
# "r.ScreenPercentage 100" restores it. UI is not affected.

HighResShot 2
# Writes a screenshot at twice the viewport resolution. The Output Log prints
# the file path; it is under Saved/Screenshots/ in the project folder (in a
# per-platform subfolder such as WindowsEditor).

ShowFlag.Bounds 1
# Draws every primitive's bounding box and sphere. "ShowFlag.Bounds 0" hides
# them. Other show flags: ShowFlag.Collision, ShowFlag.Wireframe.

help
# Prints help text to the console and the log. For the full list of commands
# and variables, use DumpConsoleCommands (writes them to the Output Log).
# Typing the first letters of a command in the console also shows matches;
# Tab completes the highlighted one.
```

The empty level makes some of these hard to see. Place a cube again for `ShowFlag.Bounds` and
`r.ScreenPercentage`, and delete it afterwards.

Two kinds of things are typed into the console:

- **Commands** do something: `stat fps`, `HighResShot 2`, `help`. Chapter 3 writes your own with
  the `Exec` specifier, and chapter 19 adds cheats.
- **Console variables (CVars)** hold a value: `t.MaxFPS`, `r.ScreenPercentage`. Type the name alone
  to print the current value, or the name and a value to set it. The prefix tells you the system:
  `r.` rendering, `t.` time, `p.` physics, `au.` audio. Chapter 19 defines GridSnake's own CVars.

Reading `stat unit` is a skill you will use in chapter 19:

- **Frame** is the total time of one frame. 16.7 ms means 60 FPS; 33.3 ms means 30 FPS.
- **Game** is the game thread: your gameplay code, ticking actors and Blueprints.
- **Draw** is the render thread preparing draw calls.
- **GPU** is the graphics card's time.
- **RHIT** is the RHI thread submitting commands to the graphics API.

The threads work in parallel, so Frame is roughly the *largest* of them (plus waiting), not their
sum. When Frame is much larger than all of them, something is capping the rate, such as
`t.MaxFPS` or vertical sync. Note that the editor itself also renders, so PIE numbers are a little
worse than a packaged game's.

CVars set in the console last until you stop playing or close the editor. To make one permanent,
put it in a config file. For example, this in `Config/DefaultEngine.ini` caps the frame rate for
everyone who opens the project:

```ini
[SystemSettings]
t.MaxFPS=60
```

GridSnake does not do that; it is shown so you know where such lines go.

## Project: GridSnake's first files

Everything in this chapter's project section you have already done while reading. Here it is in
one place, as a checklist. The chapter 1 snapshot is `project/ch01/`; its `ASSETS.md` lists the
assets the snapshot cannot contain.

| Path | Type | Created from | Key settings |
|---|---|---|---|
| `GridSnake.uproject` | Project | Project Browser > **Games** > **Blank** | **Blueprint**, Target Platform **Desktop**, Quality **Maximum**, **Starter Content off**, Raytracing off |
| `/Game/GridSnake/Maps/L_Level01` | Level | **File > New Level > Empty Level** | Saved as `L_Level01` |

Editor steps:

1. In the Epic Games Launcher, open **Unreal Engine > Library** and launch **5.8**.
2. In the Project Browser choose **Games > Blank**. Set **Blueprint**, **Desktop**, **Maximum**,
   **Starter Content** off and **Raytracing** off. Name the project `GridSnake` and click
   **Create**.
3. **Edit > Plugins**: search for `Paper2D` and make sure it is enabled. Close the editor and make
   sure the `.uproject` lists `Paper2D` as shown in [Enabling Paper2D](#enabling-paper2d).
4. In the Content Browser create the folders `Content/GridSnake/Maps`.
5. **File > New Level > Empty Level**, then **File > Save Current Level As...** into
   `GridSnake/Maps` as `L_Level01`.
6. **Edit > Project Settings > Project > Maps & Modes**: set both **Editor Startup Map** and **Game
   Default Map** to `L_Level01`.
7. **Edit > Project Settings > Project > Description**: set Project Name, Description, Project
   Version `0.1.0` and Copyright Notice as in the table above.
8. Close the editor and compare your `Config/` files with `project/ch01/Config/`. The keys from the
   snapshot must be present with the same values. Extra lines from the template are fine.

The snapshot's text files, in full:

```
project/ch01/
  ASSETS.md
  GridSnake.uproject
  Config/
    DefaultEditor.ini
    DefaultEngine.ini
    DefaultGame.ini
    DefaultInput.ini
```

**Expected result.** Open the project: the editor starts on `L_Level01` (its name shows in the
tab at the top-left of the editor window). Click **Play**: the viewport shows a black or grey empty
world, because the level has nothing in it. Press Esc. Nothing is logged yet by GridSnake: the Output Log shows
only the engine's own lines.

Two checks that confirm the settings work:

- **File > Open Level** something else (any level from the template, if you kept it), then close
  and reopen the editor. It starts on `L_Level01` again: Editor Startup Map works.
- Choose **Standalone Game** from the Play options. A separate game window opens on `L_Level01`:
  Game Default Map works. Close it with the window's close button, or open its console and type
  `exit`.

You can delete the template's starting level if the Content Browser still shows one. GridSnake
never uses it.

## Summary

- Unreal Engine 5.8 is installed from the Epic Games Launcher (Windows, macOS) or Epic's Linux
  package or source. Install a C++ toolchain now: Visual Studio 2026 or 2022 17.14+ (or Rider),
  Xcode 15.4+, or the bundled clang on Linux.
- GridSnake is a **Games > Blank** Blueprint project with Starter Content off. 5.x has no Paper2D
  template; Paper2D is a first-party plugin, enabled by default, which GridSnake lists explicitly
  in its `.uproject`.
- A project is `GridSnake.uproject`, `Config/` and `Content/`. `Saved/`, `Intermediate/` and the
  DDC are generated and ignored by version control.
- The editor's panels: viewport (perspective and orthographic views, Lit/Unlit), Outliner,
  Details, Content Browser and Content Drawer (Ctrl+Space), Place Actors, Output Log and console.
- A level (`.umap`) holds actors. Play In Editor plays a copy of the level; changes during play are
  lost. Play modes: Selected Viewport, New Editor Window, Standalone Game, Simulate.
- Project Settings write into `Config/Default*.ini`. Section names are class paths; asset paths
  are `Package.Object`. Config files stack: engine base, platform, project default, project
  platform, saved user settings.
- The console runs commands (`stat fps`, `stat unit`, `HighResShot`) and sets CVars (`t.MaxFPS`,
  `r.ScreenPercentage`).

## Exercises

1. **A second startup level.** Create a second empty level, `/Game/GridSnake/Maps/L_Scratch`, and
   make the editor open with it. Which line of which file changes? Then set it back to `L_Level01`.

   <details><summary>Solution</summary>

   1. **File > New Level > Empty Level**, then **File > Save Current Level As...** into
      `GridSnake/Maps` as `L_Scratch`.
   2. **Edit > Project Settings > Project > Maps & Modes > Default Maps > Editor Startup Map** =
      `L_Scratch`.
   3. Open `Config/DefaultEngine.ini` in a text editor. Under
      `[/Script/EngineSettings.GameMapsSettings]` the line is now:

      ```ini
      EditorStartupMap=/Game/GridSnake/Maps/L_Scratch.L_Scratch
      ```

   4. Restart the editor: it opens `L_Scratch`. `GameDefaultMap` did not change, so Standalone Game
      still starts on `L_Level01`. The two settings are independent: one is for your editing
      session, the other for players.
   5. Set **Editor Startup Map** back to `L_Level01`. Delete `L_Scratch` from the Content Browser
      (right-click > **Delete**) if you do not want it.

   </details>

2. **Logs on disk.** The Output Log panel is cleared when the editor closes. Find where Unreal writes
   the same log to disk, and find the log of the session *before* the current one.

   <details><summary>Solution</summary>

   The current session's log is `Saved/Logs/GridSnake.log` in the project folder. When the editor
   starts, it renames the previous log to a backup with a timestamp in its name, in the same folder,
   for example `GridSnake-backup-2026.11.07-21.39.22.log`. Each line in the file starts with a
   timestamp and a frame number in brackets, then the category:

   ```text
   [2026.11.07-21.39.22:600][  0]LogInit: ...
   ```

   This file is what you send along with a bug report, and it survives a crash. Packaged games write
   their logs to a per-user folder instead (chapter 20).

   </details>

3. **Cap the frame rate.** In PIE, run `stat unit`, note the Frame time, then cap the game at
   20 FPS. What happens to Frame, Game and Draw, and why?

   <details><summary>Solution</summary>

   ```text
   stat unit
   t.MaxFPS 20
   ```

   Frame rises to about 50 ms (1000 ms / 20). Game and Draw stay small, as before, because the
   threads do the same small amount of work; they simply wait longer between frames. This is how
   you recognise a cap, as opposed to real load: Frame is far above every individual thread time.
   `t.MaxFPS 0` removes the cap. The editor itself may also throttle the frame rate when its window
   is not focused, which shows the same pattern.

   </details>

4. **Project version.** Change the project version to `0.0.1` in the editor and find it in the
   ini file. Then set it back to `0.1.0`.

   <details><summary>Solution</summary>

   **Edit > Project Settings > Project > Description > About > Project Version** = `0.0.1`. The
   change is saved at once into `Config/DefaultGame.ini`:

   ```ini
   [/Script/EngineSettings.GeneralProjectSettings]
   ...
   ProjectVersion=0.0.1
   ```

   The section name says which class owns the key: `GeneralProjectSettings` in the module
   `EngineSettings`. Set it back to `0.1.0` so your file matches the snapshot. Chapter 20 bumps the
   version for release and shows it in the main menu.

   </details>

---

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-1--hello-unreal)
