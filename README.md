# Learn Unreal Engine 5 by Building `GridSnake`

This course teaches Unreal Engine 5 from first principles while you build one real game:
**GridSnake**, a top-down, grid-based Snake made with Paper2D, Unreal's 2D plugin. The snake eats
three gems to open the exit, and the exit leads to the next level. By the end the game has menus,
a HUD, music, saved progress and a packaged build you can give to other people.

It borrows from two classics. As in *Go by Example*, every idea comes with a small, focused C++
example and the output it should print. As in *The Rust Programming Language*, the chapters are a
narrative that explains why the engine works the way it does, and each one adds a feature to the
project.

The goal is that after chapter 20 you can start your own 2D game in Unreal with confidence. You
will know the editor workflow, the split between C++ and Blueprints, UObjects and reflection, the
Gameplay Framework, Enhanced Input, Paper2D sprites, flipbooks and tile maps, 2D collision,
cameras, game rules, UMG, audio, saving, data-driven design, level flow, debugging and packaging.

## Requirements

- **Exactly Unreal Engine 5.8.** 5.8 was released on June 23, 2026; Epic calls it the last planned
  major UE5 release before UE6. Install it from the Epic Games Launcher (Windows and macOS). On
  Linux, use Epic's Linux download or a source build from GitHub, which needs a GitHub account
  linked to your Epic account. Other versions rename headers, settings and editor labels, and some
  of the code *will* break.
- **A C++ toolchain for your platform:**

  | Platform | OS | IDE and toolchain |
  |---|---|---|
  | Windows | Windows 10 or 11, 64-bit | **Visual Studio 2026** (recommended) or **Visual Studio 2022 17.14+**, with the workloads "Game development with C++" and ".NET desktop development"; MSVC **14.50** (minimum 14.38); Windows SDK **10.0.26100** (minimum 10.0.22621). **JetBrains Rider** works too and uses the same MSVC toolchain. |
  | macOS | macOS Sonoma 14 or later, Apple silicon or Intel | **Xcode 15.4+** (minimum 15.2) |
  | Linux | A recent 64-bit distribution (see Epic's Linux requirements page) | Epic's bundled **clang 20.1.8** toolchain, installed with the engine; any editor (VS Code, Rider, CLion) |

- **Hardware:** a GPU with DirectX 12 (Windows), Metal (macOS) or Vulkan (Linux) support;
  16 GB of RAM minimum, 32 GB recommended; about **100 GB of free disk** for the engine, its
  derived data and your projects. The first launch compiles shaders and takes a while.
- **Python 3.8+** to run `Tools/make_placeholder_art.py`. It uses only the standard library and
  generates every texture and sound the game uses, so there is nothing to download or buy.
- **C++ basics.** You should be comfortable with classes, inheritance, virtual functions,
  references and pointers, and headers versus source files. Unreal's own C++ dialect (macros,
  reflection, garbage collection, its containers and strings) is taught from scratch.
- **Optional:** Git with Git LFS for your own projects (chapter 20).

Chapter 1 walks through installing the engine; chapter 2 sets up the IDE.

## How this course was checked

The environment that produced this course cannot run Unreal Engine: it has no GPU, no Epic account,
and not the 100+ GB the engine needs. **The C++ targets UE 5.8, but none of it was compiled or run
by the authors.** Every API, macro specifier, module name, header path and signature was checked
against Epic's UE 5.8 documentation and C++ API reference on dev.epicgames.com, the 5.8 release
notes, and the engine's Paper2D and core source code for behaviour details. The Python art
generator is the one program that was run, and its real output is shown in chapter 4.

What that means for you:

- Where a book would show program output, this course shows a block labelled **Expected Output
  Log**: the lines the code's `UE_LOG` calls will print, with variable parts marked like `<this>`.
- There are no screenshots. The text describes what you should see instead.
- Editor labels drift between versions. Where a label could not be confirmed, the text says "the
  label may differ slightly in your version".
- Some engine behaviours are flagged as uncertain where they matter: overlap events when the snake
  moves without sweeping, experimental tile map collision, the loading screen (Standalone and
  packaged builds only), MetaSound node names, music surviving a level change, and the order of
  input mode and focus changes. The chapters explain the fallback for each.

If something does not compile on 5.8, prints different output, or a menu path does not match your
editor, trust the editor and **please report it** (an issue or a pull request with the chapter,
the snippet and the error is ideal).

## How to use this course

- **Read in order.** Each chapter uses only what earlier chapters taught, and each one ends with
  a working game.
- **Type the code.** You learn more by typing the examples and project steps than by copying them.
  Unreal will make you restart the editor more often than you expect; each chapter says when.
- **Try the examples.** `examples/chNN/` holds the small example classes from chapter NN. Each is
  an `.h`/`.cpp` pair that compiles in *any* UE 5.8 C++ project: copy it into `Source/<YourModule>/`,
  add the modules it lists to your `.Build.cs`, build, and follow the chapter's steps (usually
  "drag it into a level and press Play").
- **Use the snapshots when stuck.** `project/chNN/` is the complete GridSnake project at the end of
  chapter NN. Unreal assets (`.uasset`, `.umap`) are binary, so a snapshot holds only the text
  files: `GridSnake.uproject`, `Config/`, `Source/` (from chapter 2), `Tools/` (from chapter 4),
  and an `ASSETS.md` that lists every asset to create in the editor, with exact names, paths and
  settings. To rebuild a snapshot:
  1. Create a blank project named `GridSnake` (chapter 1), or use your own.
  2. With the editor closed, copy the snapshot's `GridSnake.uproject`, `Config/`, `Source/` and
     `Tools/` over it. Keep your own `ProjectID` line in `Config/DefaultGame.ini`.
  3. Run `python Tools/make_placeholder_art.py` to generate `RawAssets/`.
  4. Generate project files, build `GridSnakeEditor` in **Development Editor**, and open the
     project.
  5. Create the assets from that snapshot's `ASSETS.md` (each one lists what it carries over from
     the previous chapter), or follow
     [Appendix C](appendix-c-editor-asset-checklist.md), which merges every chapter's steps into
     one ordered checklist.
- **Compare with a diff.** For example `diff -r MyGridSnake/Source project/ch09/Source`.
- **Do the exercises** at the end of each chapter before opening the collapsed solutions.

## Table of contents

| # | Chapter | You'll learn | GridSnake milestone |
|---|---------|--------------|---------------------|
| 1 | [Hello, Unreal](01-hello-unreal.md) | Install, Project Browser, editor tour, levels, Play In Editor, the Paper2D plugin | Blank project, `L_Level01`, project settings |
| 2 | [Projects, Modules and the Build](02-projects-modules-and-the-build.md) | `.uproject`, modules, `Build.cs`, `Target.cs`, UBT and UHT, IDE setup, Live Coding, logging | C++ module, `AGSGameMode`, `BP_GSGameMode` |
| 3 | [Objects and Reflection](03-objects-and-reflection.md) | UObject, `UCLASS`/`UPROPERTY`/`UFUNCTION`, the CDO, garbage collection, `TObjectPtr`, config, developer settings, Exec | `UGSSettings`, `UGSGameInstance` with `GSDumpClass` |
| 4 | [Pixel Art with Paper2D](04-pixel-art-with-paper2d.md) | Textures, sprite import settings, sprites, flipbooks, sprite materials, 2D rendering settings | Generated art imported; sprites and flipbooks |
| 5 | [Actors and Components](05-actors-and-components.md) | `AActor`, components, constructor vs `BeginPlay`, `OnConstruction`, `Tick`, Blueprint subclasses | `AGSGem`, `AGSExit`, `BP_GSGem`, `BP_GSExit` |
| 6 | [Tile Maps and the Orthographic Camera](06-tile-maps-and-the-orthographic-camera.md) | Tile sets, tile maps, layers, tile collision, orthographic cameras, grid and world math | `AGSBoard` with `TM_Level01` and a framing camera |
| 7 | [The Gameplay Framework](07-the-gameplay-framework.md) | GameMode, GameState, PlayerController, Pawn, Player Start, spawning and possession, view targets | `AGSSnake` pawn, `AGSGameState`, `AGSPlayerController` |
| 8 | [Enhanced Input](08-enhanced-input.md) | Input actions, mapping contexts, modifiers, triggers, binding in C++ | Steering with keyboard and gamepad |
| 9 | [Time: Ticks and Timers](09-time-ticks-and-timers.md) | Tick, delta time, fixed steps, the timer manager, input buffering, grouped sprites | The snake moves and has a body; countdown |
| 10 | [Collision in 2D](10-collision-in-2d.md) | Collision channels and profiles, overlaps, queries, custom trace channels, tile collision | Walls and self-bites kill; gems and exit are triggers |
| 11 | [Interfaces, Delegates and Game Rules](11-interfaces-delegates-and-game-rules.md) | `UINTERFACE`, `BlueprintNativeEvent`, delegates of every kind, game phases | Gems → exit opens → level complete; death → restart |
| 12 | [Flipbook Animation from C++](12-flipbook-animation-from-cpp.md) | Flipbook components, state-driven animation, one-shot effects, colour and visibility | Animated head, gem pops, opening exit, death blink |
| 13 | [Data-Driven Levels](13-data-driven-levels.md) | Data assets, soft references, curves, config-driven asset paths | `DA_LevelList`, three levels, speed curves |
| 14 | [Level Transitions and Persistence](14-level-transitions-and-persistence.md) | `OpenLevel`, travel, game instance subsystems, camera fades, loading screens | Fade, loading screen, run score across levels |
| 15 | [A HUD with UMG](15-a-hud-with-umg.md) | `UUserWidget`, `BindWidget`, widget trees, creating widgets from C++, delegates → UI | `W_HUD` with gems, icons, score, banner |
| 16 | [Menus, Pause and Input Modes](16-menus-pause-and-input-modes.md) | Menu level, input modes, focus, pausing, widget switchers, game user settings | Main menu, level select, options, pause, level end |
| 17 | [Sound and Music](17-sound-and-music.md) | Sound waves, `PlaySound2D`, audio components, MetaSounds, sound classes and mixes | SFX, adaptive music, volume sliders |
| 18 | [Saving Progress](18-saving-progress.md) | `USaveGame`, slots, async saving, versioning, subsystem dependencies | Unlocks, best scores, Continue, reset |
| 19 | [Debugging and Profiling](19-debugging-and-profiling.md) | Logging, asserts, CVars, debug drawing, the cheat manager, Visual Logger, stats, Unreal Insights | Debug grid, cheats, stats, traces |
| 20 | [Ship It, Then Make Your Own](20-ship-it-and-make-your-own.md) | Packaging, cooking, build configurations, versioning; starting your own 2D game | Packaged Shipping build; your next project |
| A | [Cheat Sheet](appendix-a-cheatsheet.md) | Macros, specifiers, modules, common APIs, console commands, shortcuts | |
| B | [Project Reference](appendix-b-project-reference.md) | How to play, architecture, classes, assets, data flow, concept → code map | |
| C | [Editor Asset Checklist](appendix-c-editor-asset-checklist.md) | Every asset and editor setting of the final game, in build order | |

References: [REFERENCES.md](REFERENCES.md) lists the sources behind each chapter, with an archived
copy of every link.

## What you'll build

GridSnake starts in a main menu with **Play**, **Continue**, **Select Level**, **Options** and
**Quit**, usable with mouse, keyboard or gamepad. Each level is a Paper2D tile map seen through an
orthographic camera:

- The snake moves one cell per fixed step. Steer with WASD, the arrow keys, the left stick or the
  D-pad; turns are buffered, so quick double turns work.
- A three-second countdown starts each level. The HUD shows the level name, `Gems 0/3` with three
  gem icons that light up, and the score.
- Each gem makes the snake grow, plays a pop animation and a sound, and makes the music brighter.
  The third gem opens the exit with an animation and a fanfare.
- Entering the exit completes the level: a level-end screen shows the level score, your best and
  the run total, then **Next Level** fades to black, shows a loading screen (in Standalone and
  packaged builds) and opens the next map. Hitting a wall or yourself ends the attempt with a death
  animation and offers **Retry**.
- There are three levels, *First Steps*, *The Maze* and *Long Snake*. They are described by one
  data asset, and the last one speeds up as the snake grows.
- **P**, **Escape** or the gamepad's Start/Menu button pauses the game. Options has fullscreen, VSync,
  music and effects volume, and Reset Progress.
- Unlocked levels, best scores and volumes are saved between sessions.
- Development builds have console cheats (`GSGrow`, `GSCollectAll`, `GSWin`, `GSUnlockAll`), a
  debug grid (`GS.Debug.Grid 1`), a stat group and Visual Logger entries. The Shipping build has
  none of them and shows `v1.0.0` on the main menu.

Playing the first level from the main menu, on a fresh save, prints this in the Output Log
(filter: `LogGridSnake`). This is an expected log, not a captured one. Lines that come from the
same moment may appear in a different order, and `...` stands for repeated lines:

```text
LogGridSnake: No save game in slot GridSnake - starting fresh
LogGridSnake: Level flow subsystem initialised
LogGridSnake: GSGameInstance initialised: 3 gems per level, step 0.15s
LogGridSnake: Level L_MainMenu started (index -1, run score 0)
LogGridSnake: Music started: MSS_Music
LogGridSnake: Opening /Game/GridSnake/Maps/L_Level01.L_Level01
LogGridSnake: GameState ready: 0/3 gems
LogGridSnake: Level L_Level01 started (index 0, run score 0)
LogGridSnake: Board GSBoard_0: 24x14 cells of 16 units
LogGridSnake: Snake placed at cell X=12 Y=7 facing EGSDirection::Right, length 3
LogGridSnake: BP_GSExit_C_0 starts closed
LogGridSnake: View target: GSBoard_0
LogGridSnake: Countdown: 3
LogGridSnake: Countdown: 2
LogGridSnake: Countdown: 1
LogGridSnake: Phase: EGSLevelPhase::Playing
LogGridSnake: Snake starts moving EGSDirection::Right, one step every 0.18s
LogGridSnake: Gem 1/3
LogGridSnake: Gem 2/3
LogGridSnake: Gem 3/3
LogGridSnake: All gems collected - the exit opens
LogGridSnake: BP_GSExit_C_0 is now open
LogGridSnake: Phase: EGSLevelPhase::Completed
LogGridSnake: Level 0 complete: +300, run score 300
LogGridSnake: Save to slot GridSnake succeeded
```

[Appendix B](appendix-b-project-reference.md) has the controls, the architecture, every class and
asset, and a map from each chapter's ideas to the code.
