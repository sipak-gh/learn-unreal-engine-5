# Appendix B — Project Reference

This appendix puts the finished GridSnake (version 1.0.0, `project/ch20/`) in one place: how
to play and build it, how the code fits together, and where each idea from the course appears.
File paths are relative to `project/ch20/Source/GridSnake/` unless they start with `Config/`,
`Content/` or `/Game/`. The binary assets (`.uasset`, `.umap`) are not in the repository;
[Appendix C](appendix-c-editor-asset-checklist.md) lists every one in the order you create it.

Previous: [Appendix A — Cheat Sheet](appendix-a-cheatsheet.md). Next: [Appendix C — Editor Asset Checklist](appendix-c-editor-asset-checklist.md).

## Playing GridSnake

You steer a snake around a walled board. It moves one cell at a time and never stops. Eat the
three gems to open the exit, then drive into the exit. A wall, the closed exit or your own body
ends the attempt. Every gem makes the snake two segments longer.

| Action | Keyboard | Gamepad |
|---|---|---|
| Steer | W A S D or the arrow keys | Left stick or D-pad |
| Pause / resume | P or Escape | Special Right (Start / Menu) |
| Menus | Mouse, or arrow keys and Enter | D-pad and the bottom face button (UMG navigation) |
| Console (development builds) | `` ` `` | — |

In Play In Editor, **Escape** stops the session before the game sees it, so use **P** to test
the pause menu.

### Screens

- **Main menu** (`L_MainMenu`): Continue (only when you have progress), Play, Level Select,
  Options, Quit. The version number sits in the bottom-right corner.
- **Level select**: one button per level in `DA_LevelList`. Each button shows the level's best
  score or "Locked".
- **Options**: fullscreen, VSync, music and effects volume sliders, Reset Progress, Back.
- **HUD**: level name, gem counter with icons, score, and a banner for the countdown numbers,
  "Level complete!" and "Ouch!".
- **Pause menu**: Resume, Restart, Main Menu.
- **Level end**: "Level Complete!", "You Crashed!" or "You Win!", with the level score, best
  score and run total, plus Next Level / Play Again, Retry and Main Menu.

### Levels

The levels and their tuning live in `DA_LevelList` (chapter 13).

| # | Map | Display name | Step interval | Points per gem | Speed curve | Run total after 3 gems |
|--:|---|---|---|--:|---|--:|
| 0 | `L_Level01` | First Steps | 0.18 s | 100 | none | 300 |
| 1 | `L_Level02` | The Maze | 0.15 s | 150 | none | 750 |
| 2 | `L_Level03` | Long Snake | 0.15 s | 200 | `C_SpeedUp`: ×1.0 at length 3 down to ×0.5 at length 40 | 1350 |

Each level map holds a `GSBoard` with its tile map (`TM_Level0N`; `TM_Level01` is 24 × 14 cells
of 16 units), one Player Start, three `BP_GSGem` and one `BP_GSExit`. Cell (x, y) is centred at
(16x, 0, −16y). Finishing the last level ends the run with "You Win!", and Play Again starts
a new run at level 0.

## Building and running

1. Install Unreal Engine 5.8 and a C++ toolchain (see [README](README.md#requirements)).
2. Create the project as in chapter 1, copy `project/ch20/Source`, `Config`, `Tools` and
   `GridSnake.uproject` into it, and create the assets with
   [Appendix C](appendix-c-editor-asset-checklist.md).
3. Regenerate project files, build **GridSnakeEditor** (Development Editor) and open the editor.
4. Play `L_MainMenu` to go through the menus, or any `L_LevelNN` to start that level directly.
5. Package with **Platforms > (your platform) > Package Project**, or with the scripts in
   `examples/ch20/` (chapter 20).

Runtime files, in the project's `Saved/` folder when you run from the editor. A Shipping build
on Windows uses `%LOCALAPPDATA%\GridSnake\Saved\` instead (chapters 18 and 20).

| Path | Contents |
|---|---|
| `Saved/SaveGames/GridSnake.sav` | Progress and volumes (slot `GridSnake`, user index 0) |
| `Saved/Logs/GridSnake.log` | The log of the last run; filter for `LogGridSnake` |
| `Saved/Config/<Platform>/GameUserSettings.ini` | Fullscreen and VSync from the Options screen |

## Architecture

```text
 UGSGameInstance  (lives for the whole session)
 ├── UGSSaveSubsystem       UGSSaveGame: unlocked levels, best scores, volumes
 ├── UGSAudioSubsystem      persistent music component, sound mix, MetaSound "Intensity"
 └── UGSLevelFlowSubsystem  run state: level index, run score; fades, loading screen, travel
           ▲ asks for level data
           │
 UGSSettings (Project Settings > Game > GridSnake, DefaultGame.ini)
 └── LevelList ─► DA_LevelList (UGSLevelList) ─► FGSLevelInfo per level (soft map reference)

 Menu world (L_MainMenu)                 Level world (L_Level01..03)
 AGSMenuGameMode                         AGSGameMode ──────────── rules, countdown, scoring
 AGSMenuPlayerController                 ├── AGSGameState ─────── phase, gems, score + delegates
 └── W_MainMenu                          ├── AGSPlayerController  input, view target, HUD, pause, level end
     ├── W_LevelSelect ─ W_LevelButton   │   ├── W_HUD            listens to the game state
     └── W_Options                       │   ├── W_PauseMenu
                                         │   └── W_LevelEnd
                                         └── AGSSnake (pawn) ──── grid movement, body, input
                                         AGSBoard ─────────────── tile map, camera, cell maths, wall queries
                                         AGSGem, AGSExit ──────── IGSSnakeInteractable
                                         AGSFlipbookEffect ────── one-shot gem pop
```

Two rules hold the design together:

- **Actors report, the game mode decides.** A gem tells the game mode "the snake ate me"; the
  game mode adds score through the game state and grows the snake. Widgets and the exit listen
  to the game state's delegates instead of polling it.
- **Anything that must survive a map change lives in the game instance.** Each `OpenLevel`
  destroys the world, including the game mode, game state, controller and widgets. The run
  score, the save and the music live in game instance subsystems.

### Classes

| Class | Base | Role | Ch | Reuse it in your game? |
|---|---|---|--:|---|
| `AGSGameMode` (`Core/`) | `AGameModeBase` | Level rules: countdown, gems, exit, death, level end | 2 | Pattern yes, rules no |
| `UGSSettings` (`Core/`) | `UDeveloperSettings` | Project-wide tuning and asset references, in Project Settings | 3 | Yes, rename |
| `UGSGameInstance` (`Core/`) | `UGameInstance` | Session object; the `GSDumpClass` console command | 3 | Optional |
| `AGSGem` (`Gameplay/`) | `AActor`, `IGSSnakeInteractable` | Bobbing pickup; reports, pops, plays a sound, destroys itself | 5 | As a pickup template |
| `AGSExit` (`Gameplay/`) | `AActor`, `IGSSnakeInteractable` | Door: blocks the Grid channel while closed, opens on all gems | 5 | As a door template |
| `AGSBoard` (`Gameplay/`) | `AActor` | Tile map + orthographic camera; cell ↔ world, `IsCellBlocked`, `SnapActorToCell`, debug grid | 6 | Yes, for any grid game |
| `AGSGameState` (`Core/`) | `AGameStateBase` | Phase, gems, score, countdown; five dynamic delegates | 7 | Pattern yes |
| `AGSPlayerController` (`Core/`) | `APlayerController` | Mapping context, board as view target, HUD, pause, level-end screen, cheat class | 7 | Yes, adapt |
| `EGSDirection`, `GSGrid` (`Snake/GSGridTypes.h`) | enum, namespace | Directions, offsets, opposites, sprite rotations | 7 | Yes |
| `AGSSnake` (`Snake/`) | `APawn` | Fixed-step grid movement, turn queue, body sprites, head animation, death | 7 | The step/queue idea |
| `IGSSnakeInteractable` (`Gameplay/`) | `UInterface` | `OnSnakeEntered(AGSSnake*)`, a BlueprintNativeEvent | 11 | Pattern yes |
| `AGSFlipbookEffect` (`Gameplay/`) | `AActor` | Plays one flipbook once, then destroys itself | 12 | Yes, as is |
| `UGSLevelList`, `FGSLevelInfo` (`Data/`) | `UPrimaryDataAsset`, struct | Ordered levels with per-level tuning | 13 | Yes, change the fields |
| `UGSLevelFlowSubsystem` (`Core/`) | `UGameInstanceSubsystem` | Runs: start, complete, next, restart, main menu; fade and loading screen | 14 | Yes |
| `UGSHUDWidget` (`UI/`) | `UUserWidget` | HUD logic for `W_HUD` | 15 | Pattern yes |
| `AGSMenuGameMode` (`Core/`) | `AGameModeBase` | Menu world: no pawn, starts music, fades in | 16 | Yes |
| `AGSMenuPlayerController` (`Core/`) | `APlayerController` | UI-only input, cursor, creates `W_MainMenu` | 16 | Yes |
| `UGSMenuPanelWidget` (`UI/`) | `UUserWidget` | Base for sub-panels: Back button, native `OnBackRequested` | 16 | Yes |
| `UGSMainMenuWidget` (`UI/`) | `UUserWidget` | Widget switcher between main panel, level select and options; version label | 16 | Yes |
| `UGSLevelSelectWidget`, `UGSLevelButtonWidget` (`UI/`) | panel / `UUserWidget` | Buttons built at runtime from the level list and the save | 16 | Yes |
| `UGSOptionsWidget` (`UI/`) | panel | Display settings, volumes, reset progress | 16 | Yes |
| `UGSPauseMenuWidget`, `UGSLevelEndWidget` (`UI/`) | `UUserWidget` | Pause and level-end screens | 16 | Yes |
| `UGSAudioSubsystem` (`Core/`) | `UGameInstanceSubsystem` | Persistent music, intensity, volumes through a sound mix | 17 | Yes |
| `UGSSaveGame` (`Save/`) | `USaveGame` | Saved fields and `SaveVersion` | 18 | Change the fields |
| `UGSSaveSubsystem` (`Save/`) | `UGameInstanceSubsystem` | Load at start, migrate, async save, progress queries | 18 | Yes |
| `UGSCheatManager` (`Core/`) | `UCheatManager` | `GSGrow`, `GSCollectAll`, `GSWin`, `GSUnlockAll` | 19 | Pattern yes |

The module itself (`GridSnake.h/.cpp`) declares the log category `LogGridSnake`, the Grid
collision channel alias `ECC_GSGrid` (= `ECC_GameTraceChannel1`) and the stat group
`STATGROUP_GridSnake`.

### The game state's phases

```text
Countdown ──(TickCountdown reaches 0)──► Playing ──(exit reached)──► Completed
                                            └──────(snake died)────► Failed
```

`AGSGameState::SetPhase` broadcasts `OnPhaseChanged`. The game mode ignores gems and the exit
outside `Playing`, and the player controller refuses to pause outside `Countdown` and `Playing`.

## Data flow

### Starting a run from the menu

1. `L_MainMenu` loads. `AGSMenuGameMode::StartPlay` calls
   `UGSLevelFlowSubsystem::NotifyLevelStarted` (fade in) and starts the music through
   `UGSAudioSubsystem::HandleLevelStarted`.
2. `AGSMenuPlayerController::BeginPlay` sets `FInputModeUIOnly`, shows the cursor and adds
   `W_MainMenu`, which focuses its first button.
3. **Play** calls `StartRun(0)`. **Continue** calls `StartRun` with the last unlocked level.
   A level button calls `StartRun(LevelIndex)`. `StartRun` resets the run score to 0.
4. `OpenLevelWithFade` fades the camera to black over 0.4 s and holds it there. A game instance
   timer then calls `OpenLevelBySoftObjectPtr`. If the game is paused, it skips the fade and
   opens the map at once.
5. `FCoreUObjectDelegates::PreLoadMap` fires `HandlePreLoadMap`, which shows the Slate loading
   screen in standalone and packaged builds (minimum 1 s).

### Inside a level

1. `AGSGameMode::StartPlay` calls `NotifyLevelStarted`. The subsystem finds the level index from
   the map name, so a level opened directly in the editor still knows which level it is. Then
   `Super::StartPlay` runs every actor's `BeginPlay`.
   - The gems and the exit snap to their cells.
   - The exit binds to `OnAllGemsCollected`.
   - The controller makes the board the view target, adds `IMC_Snake` and creates `W_HUD`,
     which binds to the game state's delegates.
   - The snake snaps to the Player Start's cell.
2. The game mode tells the audio subsystem the level started (the music keeps playing), reads
   `PointsPerGem`, `StepInterval` and `SpeedCurve` from the level's `FGSLevelInfo`, binds
   `OnDied` and starts the 3-2-1 countdown timer.
3. When the countdown ends, the phase becomes `Playing` and the snake starts stepping.
4. The snake overlaps a gem. `NotifyActorBeginOverlap` calls
   `IGSSnakeInteractable::Execute_OnSnakeEntered`, the gem calls `OnGemCollected`, and the game
   mode updates `AddGem` (score, gems, delegates), `Grow(2)` and the music intensity.
5. The third gem fires `OnAllGemsCollected`. The exit plays its opening animation and sound and
   stops blocking the Grid channel.
6. The snake enters the open exit, and `OnExitReached` runs:
   - It stops the snake, sets the phase to `Completed` and plays the level-complete sound.
   - It calls `UGSLevelFlowSubsystem::CompleteLevel`, which adds the level score to the run
     score and records progress (see below).
   - After **Level End Delay** (1.5 s), the controller shows `W_LevelEnd` in UI-only input mode.
7. If the next cell is a wall, the closed exit or the body, `AGSSnake::Die` plays the dead
   animation and blinks the body. It then broadcasts `OnDied`. The game mode sets the phase to
   `Failed` and shows `W_LevelEnd` with Retry after the same delay.
8. **Next Level** calls `OpenNextLevel` (index + 1, fade). **Retry** calls `RestartLevel`.
   **Main Menu** calls `ReturnToMainMenu`. **Play Again** calls `StartRun(0)`.

### Pausing

`IA_Pause` (Trigger when Paused) is bound with `ETriggerEvent::Started` in
`AGSPlayerController::SetupInputComponent`. `PauseGame` calls `SetPause(true)`, adds
`W_PauseMenu` at Z-order 10 and switches to `FInputModeGameAndUI`, so the same key can resume.
`ResumeGame` reverses that. Restart and Main Menu travel without a fade because the world is
paused.

### Saving

- `UGSSaveSubsystem::Initialize` loads slot `GridSnake` synchronously if it exists. Otherwise it
  creates a fresh `UGSSaveGame` and sets `SaveVersion = CurrentVersion` (1). An older version is
  migrated there.
- `UGSAudioSubsystem::Initialize` declares `InitializeDependency<UGSSaveSubsystem>()`, so the
  saved volumes exist before it applies them.
- `CompleteLevel` is the one place that records progress:
  - `SubmitLevelScore(LevelId, Score)`, keyed by the map's asset name.
  - `UnlockLevel(Index + 1)`, or `SubmitRunScore` after the last level.
  - `Save()`, which runs `AsyncSaveGameToSlot`.
- The Options screen changes volumes in memory and saves once, when you press Back. Reset
  Progress clears unlocks and best scores, keeps the volumes, and saves.
- The level select, the Continue button and the level-end screen read from the save subsystem.

## Settings and config

| File / section | Holds | Ch |
|---|---|--:|
| `GridSnake.uproject` | Engine association 5.8, the `GridSnake` module, Paper2D plugin | 1–2 |
| `Config/DefaultEngine.ini` `GameMapsSettings` | `GameDefaultMap=L_MainMenu`, `EditorStartupMap=L_Level01`, `GlobalDefaultGameMode=BP_GSGameMode`, `GameInstanceClass=GSGameInstance` | 1–3, 16 |
| `Config/DefaultEngine.ini` `RendererSettings` | 2D-friendly rendering: no Lumen, VSM, bloom, auto exposure, motion blur or AA | 4 |
| `Config/DefaultEngine.ini` `CollisionProfile` | Trace channel `Grid` (`ECC_GameTraceChannel1`), default response **Ignore** | 10 |
| `Config/DefaultInput.ini` | Enhanced Input player input and input component classes, console keys | 1, 8 |
| `Config/DefaultGame.ini` `GSSettings` | `GemsPerLevel=3`, `DefaultStepInterval=0.15`, `PointsPerGem=100`, `LevelList`, `MainMenuMap`, `Music`, `VolumeMix`, `MusicClass`, `SfxClass` | 3, 13, 16, 17 |
| `Config/DefaultGame.ini` `GeneralProjectSettings` | Project name, description, `ProjectVersion=1.0.0` | 1, 20 |
| `Config/DefaultGame.ini` `ProjectPackagingSettings` | Shipping, four maps to cook, `Data`/`Audio`/`UI` always cooked, pak + IoStore, no debug files | 20 |
| `Config/DefaultEditor.ini` | Template editor-only keys | 1 |
| `Source/GridSnake/GridSnake.Build.cs` | Public: Core, CoreUObject, Engine, InputCore, EnhancedInput, DeveloperSettings, Paper2D, UMG. Private: MoviePlayer, Slate, SlateCore | 2–16 |

Blueprint defaults that matter (set in the editor, listed in each `ASSETS.md`):

| Asset | Settings |
|---|---|
| `BP_GSGameMode` | Default Pawn `BP_GSSnake`, Player Controller `BP_GSPlayerController`, Countdown Seconds 3, Growth Per Gem 2, Level End Delay 1.5, Level Complete Sound `SW_LevelComplete` |
| `BP_GSPlayerController` | Default Mapping Context `IMC_Snake`, Pause Action `IA_Pause`, HUD `W_HUD`, Pause Menu `W_PauseMenu`, Level End `W_LevelEnd` |
| `BP_GSSnake` | Steer Action `IA_Steer`, Body Sprite `S_SnakeBody`, four head flipbooks, Death Sound `SW_Die`, Initial Length 3, Max Queued Turns 2, Death Blinks 4 |
| `BP_GSGem` | Flipbook `FB_Gem_Idle`, Pickup Effect Class `BP_GSGemPop`, Pickup Sound `SW_Eat` |
| `BP_GSExit` | Closed/Opening/Open flipbooks, Open Sound `SW_ExitOpen` |
| `BP_GSMenuGameMode` / `BP_GSMenuPlayerController` | Menu controller; Main Menu Widget Class `W_MainMenu` |

## Content layout

```text
Content/GridSnake/
├── Art/
│   ├── Textures/   T_Tiles, T_SnakeHead_{Idle,Move,Eat,Dead}, T_SnakeBody, T_Gem_{Idle,Pop}, T_Exit_{Closed,Opening,Open}
│   ├── Sprites/    S_<Name>_<n>, S_SnakeBody
│   ├── Flipbooks/  FB_SnakeHead_{Idle,Move,Eat,Dead}, FB_Gem_{Idle,Pop}, FB_Exit_{Closed,Opening,Open}
│   └── Tiles/      TS_Dungeon, TM_Level01..03
├── Audio/          SW_Eat, SW_Die, SW_ExitOpen, SW_LevelComplete, SW_UIClick, SW_Music, MSS_Music, SC_Music, SC_SFX, SMix_Volumes
├── Blueprints/     BP_GSGameMode, BP_GSPlayerController, BP_GSSnake, BP_GSGem, BP_GSExit, BP_GSGemPop, BP_GSMenuGameMode, BP_GSMenuPlayerController
├── Data/           DA_LevelList, C_SpeedUp
├── Input/          IA_Steer, IA_Pause, IMC_Snake
├── Maps/           L_MainMenu, L_Level01, L_Level02, L_Level03
└── UI/             W_HUD, W_MainMenu, W_LevelSelect, W_LevelButton, W_Options, W_PauseMenu, W_LevelEnd
```

## File map

```text
project/ch20/
├── GridSnake.uproject
├── ASSETS.md                     the last chapter's asset changes (all of them: Appendix C)
├── Config/                       DefaultEngine.ini, DefaultGame.ini, DefaultInput.ini, DefaultEditor.ini
├── Tools/make_placeholder_art.py writes RawAssets/Textures/*.png and RawAssets/Audio/*.wav
└── Source/
    ├── GridSnake.Target.cs, GridSnakeEditor.Target.cs
    └── GridSnake/
        ├── GridSnake.Build.cs, GridSnake.h, GridSnake.cpp
        ├── Core/       game modes, game state, controllers, settings, game instance,
        │               level flow and audio subsystems, cheat manager
        ├── Data/       GSLevelList
        ├── Gameplay/   GSBoard, GSGem, GSExit, GSFlipbookEffect, GSSnakeInteractable
        ├── Save/       GSSaveGame, GSSaveSubsystem
        ├── Snake/      GSSnake, GSGridTypes
        └── UI/         GSHUDWidget and the menu widgets
```

`examples/chNN/` holds standalone classes and scripts that each chapter uses to show one idea
in isolation (prefix `Ex`). `examples/ch20/StarterModule/` is the starting point for your own
game (prefix `MG`).

## Milestones

What each chapter added to GridSnake (snapshots in `project/chNN/`):

| Ch | GridSnake gained |
|---:|---|
| 1 | A Blank project with Paper2D and an empty `L_Level01` |
| 2 | The C++ module, `LogGridSnake`, `AGSGameMode` and `BP_GSGameMode` |
| 3 | `UGSSettings` in Project Settings, `UGSGameInstance` with `GSDumpClass` |
| 4 | Placeholder art, sprites, flipbooks, 2D render settings |
| 5 | `AGSGem` (bobbing) and `AGSExit` (closed/open) |
| 6 | `AGSBoard`: tile map, cell maths, orthographic camera framing |
| 7 | Game state, player controller, the snake pawn on the Player Start |
| 8 | Steering with Enhanced Input (`IA_Steer`, `IMC_Snake`) |
| 9 | Fixed-step movement, turn queue, body, countdown, death on walls |
| 10 | The Grid channel, wall queries, gem and exit triggers, `SnapActorToCell` |
| 11 | The interface, delegates and rules: gems, growth, exit, score |
| 12 | Head animation states, exit opening animation, gem pop, body gradient, death blink |
| 13 | `DA_LevelList`, three levels, per-level speed and points, `C_SpeedUp` |
| 14 | Runs across levels: level flow subsystem, fades, loading screen |
| 15 | The HUD |
| 16 | Main menu, level select, options, pause and level-end screens |
| 17 | Sound effects, persistent adaptive music, volume control |
| 18 | Saved progress, unlocks, best scores, Continue, Reset Progress |
| 19 | Stats, Insights scopes, Visual Logger, `GS.Debug.Grid`, cheat manager |
| 20 | Packaging settings, version 1.0.0 on the main menu |

## Concept → code map

Where each idea from the course appears in the finished code. Paths are relative to
`project/ch20/Source/GridSnake/` unless noted.

| Ch | Concept | Where |
|---:|---|---|
| 1 | Project file pins the engine, enables Paper2D | `GridSnake.uproject` (`EngineAssociation`, `Plugins`) |
| 1 | Startup maps in config | `Config/DefaultEngine.ini` `[/Script/EngineSettings.GameMapsSettings]` |
| 2 | Module rules and dependencies | `GridSnake.Build.cs` |
| 2 | Project log category | `GridSnake.h` `DECLARE_LOG_CATEGORY_EXTERN`, `GridSnake.cpp` `DEFINE_LOG_CATEGORY` |
| 2 | C++ base class, Blueprint child | `Core/GSGameMode.h` → `BP_GSGameMode` |
| 3 | Developer settings, config properties, the CDO | `Core/GSSettings.h` (`UCLASS(Config = Game, DefaultConfig)`, `UGSSettings::Get`) |
| 3 | Reflection walk and an `Exec` command | `UGSGameInstance::GSDumpClass` (`TFieldIterator<FProperty>`) |
| 4 | Placeholder art generated from code | `project/ch20/Tools/make_placeholder_art.py` |
| 5 | Components created in the constructor, scene root | `AGSGem::AGSGem` (`CreateDefaultSubobject`, `SetupAttachment`) |
| 5 | Ticking only where needed | `AGSGem::Tick` (bob); `AGSExit` and `AGSFlipbookEffect` never tick; `AGSBoard::Tick` only draws debug shapes |
| 5 | Construction script in C++ | `AGSExit::OnConstruction` → `UpdateVisuals` |
| 6 | Tile map data: size, cell size, cell ↔ world | `AGSBoard::GetGridSize`, `GetCellSize`, `CellToWorld`, `WorldToCell` |
| 6 | Orthographic camera that frames the map | `AGSBoard::FrameCamera` (`OrthoWidth`, `TargetAspectRatio`) |
| 7 | Game mode picks the framework classes | `AGSGameMode::AGSGameMode` (`GameStateClass`, `PlayerControllerClass`, `DefaultPawnClass`) |
| 7 | Fixed camera as view target | `AGSPlayerController::BeginPlay` (`bAutoManageActiveCameraTarget = false`, `SetViewTarget`) |
| 7 | Pawn snapped to the grid | `AGSSnake::SnapToBoard` |
| 8 | Mapping context and action binding | `AGSPlayerController::BeginPlay` (`AddMappingContext`), `AGSSnake::SetupPlayerInputComponent` |
| 8 | 2D input vector → grid direction | `DirectionFromInput` in `Snake/GSSnake.cpp` |
| 9 | Fixed-step movement with an accumulator | `AGSSnake::Tick`, `AGSSnake::Step` |
| 9 | Input buffered between steps | `AGSSnake::QueueDirection`, `MaxQueuedTurns` |
| 9 | Many sprites, one component | `AGSSnake::UpdateVisuals` (`UPaperGroupedSpriteComponent`) |
| 9 | Looping timer | `AGSGameMode::StartPlay` / `TickCountdown` |
| 10 | Custom trace channel | `GridSnake.h` `ECC_GSGrid`; `Config/DefaultEngine.ini` `+DefaultChannelResponses` |
| 10 | Overlap query instead of tile data | `AGSBoard::IsCellBlocked` (`OverlapBlockingTestByChannel`) |
| 10 | Trigger boxes and overlap events | `AGSGem` / `AGSExit` `Trigger`, `AGSSnake::NotifyActorBeginOverlap` |
| 10 | Collision that switches with state | `AGSExit::UpdateCollision` (`Blocker`) |
| 11 | Interface with a BlueprintNativeEvent | `Gameplay/GSSnakeInteractable.h`, `Execute_OnSnakeEntered` |
| 11 | Dynamic multicast delegates | `AGSGameState` (`OnGemsChanged` …), `AGSSnake::OnDied` |
| 11 | Rules in the game mode | `AGSGameMode::OnGemCollected`, `OnExitReached`, `HandleSnakeDied` |
| 12 | Animation state chosen in one place | `AGSSnake::PlayHeadAnim`, `HandleHeadAnimFinished` |
| 12 | One-shot effect actor | `AGSFlipbookEffect` (`OnFinishedPlaying` → `Destroy`) |
| 12 | Per-instance sprite colour | `AGSSnake::GetSegmentColor` (`FLinearColor::LerpUsingHSV`) |
| 13 | Primary data asset with structs | `Data/GSLevelList.h` (`UGSLevelList`, `FGSLevelInfo`) |
| 13 | Soft references | `FGSLevelInfo::Map` (`TSoftObjectPtr<UWorld>`), `UGSSettings::LevelList` |
| 13 | Curve-driven tuning | `AGSSnake::ConfigureSpeed`, `SpeedCurve->GetFloatValue` |
| 14 | State that survives `OpenLevel` | `Core/GSLevelFlowSubsystem.h` (`UGameInstanceSubsystem`) |
| 14 | Fade out, travel, fade in | `OpenLevelWithFade`, `OpenPendingLevel`, `NotifyLevelStarted` |
| 14 | Loading screen | `HandlePreLoadMap` (`GetMoviePlayer()->SetupLoadingScreen`) |
| 15 | C++ widget parent, `BindWidget` | `UI/GSHUDWidget.h` |
| 15 | Event-driven UI with unbinding | `UGSHUDWidget::NativeConstruct` / `NativeDestruct` |
| 15 | Widgets built at runtime | `UGSHUDWidget::BuildGemIcons` (`WidgetTree->ConstructWidget<UImage>`) |
| 16 | Menu map with its own game mode | `Core/GSMenuGameMode.cpp`, `Core/GSMenuPlayerController.cpp` |
| 16 | Input modes and focus | `AGSPlayerController::EnterMenuInputMode`, `NativeConstruct` focus calls in `UI/` |
| 16 | Pause | `AGSPlayerController::PauseGame` / `ResumeGame` (`SetPause`) |
| 16 | Widget switcher and native delegates | `UGSMainMenuWidget::ShowPanel`, `UGSMenuPanelWidget::OnBackRequested` |
| 16 | Timer delegate with a payload | `AGSGameMode::OnExitReached` (`FTimerDelegate::CreateUObject(…, Result)`) |
| 17 | 2D one-shot sounds | `UGameplayStatics::PlaySound2D` in `AGSGem`, `AGSExit`, `AGSSnake`, `AGSGameMode` |
| 17 | Music that survives level changes | `UGSAudioSubsystem::HandleLevelStarted` (`CreateSound2D(…, true, false)`) |
| 17 | MetaSound parameter from C++ | `UGSAudioSubsystem::SetMusicIntensity` (`SetFloatParameter("Intensity", …)`) |
| 17 | Volumes through a sound mix | `UGSAudioSubsystem::ApplyVolumes` (`SetSoundMixClassOverride`, `PushSoundMixModifier`) |
| 18 | Save game object and version | `Save/GSSaveGame.h` (`SaveVersion`, `CurrentVersion`) |
| 18 | Load, migrate, async save | `UGSSaveSubsystem::Initialize`, `Save` (`AsyncSaveGameToSlot`) |
| 18 | Subsystem dependency | `UGSAudioSubsystem::Initialize` (`InitializeDependency<UGSSaveSubsystem>`) |
| 19 | Stats and Insights scopes | `GridSnake.h` `DECLARE_STATS_GROUP`; `AGSSnake::Step` (`SCOPE_CYCLE_COUNTER`, `TRACE_CPUPROFILER_EVENT_SCOPE`) |
| 19 | Assertions and the Visual Logger | `AGSSnake::Step` (`check`, `UE_VLOG_LOCATION`), `AGSSnake::Die` (`UE_VLOG`) |
| 19 | Console variable and debug drawing | `Gameplay/GSBoard.cpp` `CVarGSDebugGrid` (`GS.Debug.Grid`), `DrawDebugBox` |
| 19 | Cheat manager | `Core/GSCheatManager.h`, `CheatClass` in `AGSPlayerController::AGSPlayerController` |
| 20 | Packaging settings and cooking soft references | `Config/DefaultGame.ini` `[/Script/UnrealEd.ProjectPackagingSettings]` |
| 20 | Version label from config | `UGSMainMenuWidget::NativeOnInitialized` (`GConfig->GetString(…ProjectVersion…)`) |
| 20 | Command-line packaging | `examples/ch20/package-*.{bat,sh}` (`RunUAT BuildCookRun`) |


---

Sources for this appendix: [REFERENCES.md](REFERENCES.md#appendix-b--project-reference)
