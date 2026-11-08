# Appendix C — Editor Asset Checklist

Unreal assets (`.uasset`, `.umap`) are binary, so the repository ships only text: C++ sources,
config files and one `ASSETS.md` per chapter snapshot. This appendix merges all twenty
`ASSETS.md` files into one ordered checklist. It builds the finished GridSnake from an empty
folder with the final `project/ch20/` sources. Use it to rebuild the game on a new machine,
to recover from a broken asset, or to check your project against the course.

Each step names the chapter that explains it. Menu labels are those of Unreal Engine 5.8. A
label marked *(label may differ)* is one the course could not confirm exactly. Look for the
closest match.

**Names matter.** Config files and C++ refer to assets by path, and `BindWidget` properties
need widgets with exactly the right names. Use the names and folders below.

Previous: [Appendix B — Project Reference](appendix-b-project-reference.md). Next: the [README](README.md).

## 0. Before you start

- [ ] Unreal Engine 5.8 and a C++ toolchain are installed ([README](README.md#requirements)).
- [ ] Python 3.8 or newer is on your `PATH`, for the placeholder art.

## 1. Project and module (chapters 1–3)

- [ ] **Epic Games Launcher > Unreal Engine > Library**, launch **5.8**. In the Project Browser
      choose **Games > Blank**, **Blueprint**, Desktop, Maximum, **Starter Content off**, name
      `GridSnake`, **Create**. (ch1)
- [ ] **Edit > Plugins**: make sure **Paper2D** is enabled. (ch1)
- [ ] **Tools > New C++ Class... > Game Mode Base**, name `GSGameMode`, path
      `Source/GridSnake/Core/`. This creates the module and both targets. Close the editor
      when it has compiled. (ch2)
- [ ] Replace `Source/`, `Config/` and `GridSnake.uproject` with the ones from `project/ch20/`
      and copy `project/ch20/Tools/`. Regenerate project files: on Windows, right-click
      `GridSnake.uproject` > **Generate Visual Studio project files**. On Linux and macOS, see
      chapter 2. (ch2)
- [ ] Build **GridSnakeEditor** (Development Editor) from your IDE and start the editor from
      there. The config already names assets that do not exist yet. Until you create them,
      expect warnings about missing maps and soft references in the Output Log.
- [ ] **Project Settings > Project > Maps & Modes > Game Instance Class** shows
      `GSGameInstance`. It comes from the copied `DefaultEngine.ini`. (ch3)

## 2. Art (chapter 4)

- [ ] From the project folder run `python Tools/make_placeholder_art.py`. It writes
      `RawAssets/Textures/*.png` (11 strips) and `RawAssets/Audio/*.wav` (6 files).
- [ ] **Project Settings > Editor > Paper2D - Import** *(label may differ)*: Default Pixels per
      Unreal Unit `1`, Default Sprite Texture Group `2D Pixels (unfiltered)`, Default Material
      `MaskedUnlitSpriteMaterial`. These are not in the copied config: set them by hand.
- [ ] The 2D rendering settings (no Lumen, distance fields, virtual shadow maps, auto exposure,
      bloom, motion blur; Anti-Aliasing None) come from the copied `DefaultEngine.ini`. Restart
      if the editor asks.
- [ ] Create the folders `Content/GridSnake/{Maps,Blueprints,Art/Textures,Art/Sprites,Art/Flipbooks,Art/Tiles,Input,Data,Audio,UI}`.
- [ ] Drag every PNG from `RawAssets/Textures` into `Art/Textures`. Select them all,
      right-click > **Sprite Actions > Apply Paper2D Texture Settings**.
- [ ] For every texture except `T_Tiles`: right-click > **Sprite Actions > Extract Sprites**,
      Grid mode, cell 16 × 16, margins 0. Name the sprites `S_<Name>_<n>` (`S_Gem_Idle_0` …),
      or `S_SnakeBody` for the single body sprite. Move them to `Art/Sprites`. Pivot stays
      **Center Center**.
- [ ] Create the nine flipbooks in `Art/Flipbooks`: select one strip's sprites in order,
      right-click > **Create Flipbook**. Set Frames Per Second:

  | Flipbook | FPS | Flipbook | FPS |
  |---|--:|---|--:|
  | `FB_SnakeHead_Idle` | 4 | `FB_Gem_Idle` | 8 |
  | `FB_SnakeHead_Move` | 8 | `FB_Gem_Pop` | 16 |
  | `FB_SnakeHead_Eat` | 16 | `FB_Exit_Closed` (1 frame) | any |
  | `FB_SnakeHead_Dead` | 8 | `FB_Exit_Opening` | 10 |
  |  |  | `FB_Exit_Open` | 8 |

## 3. Tiles and the first level (chapters 1, 6, 10)

- [ ] Right-click `T_Tiles` > **Sprite Actions > Create Tile Set**, name `TS_Dungeon`, move it to
      `Art/Tiles`. Tile Size 16 × 16. Row 0 (tiles 0–3) is floor, row 1 (tiles 4–7) is wall.
- [ ] In `TS_Dungeon`, select each of tiles 4–7 and click **Add Box** in the Single Tile Editor.
      Save.
- [ ] **Add > Paper2D > Tile Map** `TM_Level01` in `Art/Tiles`: Map Width 24, Map Height 14,
      Tile Width/Height 16, Projection **Orthogonal**.
- [ ] Layers: rename the default layer `Floor` and **untick Layer Collides**. Add a layer
      `Walls` above it, with Layer Collides **ticked**. Fill `Floor` with floor tiles. Paint a
      wall border plus some inner walls on `Walls`. Keep cell (12, 7) and the cells to its left
      free, plus three free cells for gems and one border cell for the exit. Optionally raise
      **Collision Thickness** to 50 or more. (ch6, ch9, ch10)
- [ ] **File > New Level > Empty Level**, save as `Maps/L_Level01`.
- [ ] From **C++ Classes/GridSnake/Gameplay**, drag `GSBoard` into `L_Level01` at (0, 0, 0). Set its
      TileMap component's **Tile Map** = `TM_Level01` and **Level > Start Direction** = Right.
- [ ] Check **Project Settings > Engine > Collision**: the trace channel **Grid** exists with
      Default Response **Ignore**. It comes from the copied `DefaultEngine.ini`. If you create it
      by hand, it must be `ECC_GameTraceChannel1`. (ch10)

## 4. Gameplay Blueprints (chapters 5, 7, 12, 17)

Create each with right-click on the C++ class > **Create Blueprint class based on …**, in
`Content/GridSnake/Blueprints`. The sound assets come in section 7. Come back then to fill in
the sound fields.

| Blueprint | Parent | Settings |
|---|---|---|
| `BP_GSGemPop` | `GSFlipbookEffect` | Flipbook > Source Flipbook `FB_Gem_Pop` |
| `BP_GSGem` | `GSGem` | Flipbook > Source Flipbook `FB_Gem_Idle`; Gem > Pickup Effect Class `BP_GSGemPop`, Pickup Sound `SW_Eat` |
| `BP_GSExit` | `GSExit` | Exit > Closed `FB_Exit_Closed`, Opening `FB_Exit_Opening`, Open `FB_Exit_Open`, Open Sound `SW_ExitOpen` |
| `BP_GSSnake` | `GSSnake` | Head > Source Flipbook `FB_SnakeHead_Idle`; Snake > Body Sprite `S_SnakeBody`, Initial Length 3, Max Queued Turns 2; Animation > Idle/Move/Eat/Dead flipbooks `FB_SnakeHead_*`, Death Blinks 4; Input > Steer Action `IA_Steer`; Audio > Death Sound `SW_Die` |
| `BP_GSPlayerController` | `GSPlayerController` | Input > Default Mapping Context `IMC_Snake`, Pause Action `IA_Pause`; UI > HUD Widget Class `W_HUD`, Pause Menu Widget Class `W_PauseMenu`, Level End Widget Class `W_LevelEnd` |
| `BP_GSGameMode` | `GSGameMode` | Classes > Default Pawn Class `BP_GSSnake`, Player Controller Class `BP_GSPlayerController` (Game State stays `GSGameState`); Rules > Countdown Seconds 3, Growth Per Gem 2, Level End Delay 1.5; Audio > Level Complete Sound `SW_LevelComplete` |
| `BP_GSMenuPlayerController` | `GSMenuPlayerController` | UI > Main Menu Widget Class `W_MainMenu` |
| `BP_GSMenuGameMode` | `GSMenuGameMode` | Classes > Player Controller Class `BP_GSMenuPlayerController`; Default Pawn Class stays None |

- [ ] Create the eight Blueprints. Fields that point at assets you have not made yet (input,
      UI, audio) can wait until those sections.
- [ ] **Project Settings > Maps & Modes > Default GameMode** = `BP_GSGameMode`. The copied
      config already names it. (ch2)

## 5. Input (chapters 8, 16)

- [ ] In `Input/`: **Input > Input Action** `IA_Steer`, Value Type **Axis2D (Vector2D)**.
- [ ] **Input > Input Action** `IA_Pause`, Value Type **Digital (bool)**, **Trigger when Paused**
      ticked.
- [ ] **Input > Input Mapping Context** `IMC_Snake`:

  | Action | Key | Modifiers (in order) |
  |---|---|---|
  | `IA_Steer` | D, Right, Gamepad D-pad Right | none |
  | `IA_Steer` | A, Left, Gamepad D-pad Left | Negate |
  | `IA_Steer` | W, Up, Gamepad D-pad Up | Swizzle Input Axis Values (YXZ) |
  | `IA_Steer` | S, Down, Gamepad D-pad Down | Swizzle Input Axis Values (YXZ), then Negate |
  | `IA_Steer` | Gamepad Left Thumbstick 2D-Axis | Dead Zone (Radial, Lower Threshold 0.25) |
  | `IA_Pause` | Escape, P, Gamepad Special Right | none |

- [ ] Fill in the input fields of `BP_GSSnake` and `BP_GSPlayerController`.

## 6. Level contents (chapters 5, 7, 11)

- [ ] In `L_Level01`, place **exactly three** `BP_GSGem`, one `BP_GSExit` (Is Open **off**) and
      one **Player Start** (Place Actors > Basic). Put each on a free cell centre. Cell
      (x, y) is at (16x, 0, −16y), for example the Player Start at cell (12, 7) =
      (192, 0, −112). Leave room to the left of the Player Start. Gems and the exit snap to the
      nearest cell when play starts.

## 7. Audio (chapter 17)

- [ ] Drag the six `.wav` files from `RawAssets/Audio` into `Audio/`: `SW_Eat`, `SW_Die`,
      `SW_ExitOpen`, `SW_LevelComplete`, `SW_UIClick`, `SW_Music`. Tick **Looping** on
      `SW_Music`.
- [ ] **Audio > Classes > Sound Class** `SC_Music` and `SC_SFX` (volume 1). **Audio > Mix > Sound
      Mix** `SMix_Volumes` with no adjusters.
- [ ] Set **Sound > Class** = `SC_SFX` on the five effect waves. **Asset Actions > Edit Selection
      in Property Matrix** does them together.
- [ ] **Audio > MetaSound Source** `MSS_Music`, Output Format **Mono**, Sound > Class `SC_Music`.
      Add a float input `Intensity` (default 0). Then build the graph *(node names may differ)*:

  ```text
  On Play ──► Wave Player (Mono)  Wave Asset = SW_Music, Loop = true
                   │ Out Mono
                   ▼
             One-Pole Low Pass Filter ──► Out Mono
                   ▲ Cutoff Frequency
  Intensity ──► Map Range (In 0..1 → Out 800..18000)
  ```

- [ ] **Project Settings > Game > GridSnake > Audio**: Music `MSS_Music`, Volume Mix
      `SMix_Volumes`, Music Class `SC_Music`, Sfx Class `SC_SFX`.
- [ ] Fill in the sound fields of `BP_GSGem`, `BP_GSExit`, `BP_GSSnake` and `BP_GSGameMode`
      (section 4).

## 8. Widgets (chapters 15, 16, 17, 18, 20)

In `UI/`, create each with **User Interface > Widget Blueprint**, then **All Classes** and the
C++ parent. The widget names in the layouts (`LevelText`, `PlayButton` …) are `BindWidget` names
and must match exactly; the ones marked *optional* are `BindWidgetOptional`. Unnamed widgets can
keep any name. Text in quotes is preview text or a button label. Keep **Is Focusable** on
for every button. Give every button **Style > Pressed Sound** = `SW_UIClick` and a Hovered tint
that differs from Normal.

- [ ] `W_HUD` (parent `GSHUDWidget`). Class Defaults > HUD: Gem Icon Sprite `S_Gem_Idle_0`,
      Gem Icon Size (32, 32), Missing Color dark grey with alpha 0.6. (ch15)

  ```text
  [Canvas Panel]
  ├─ [Horizontal Box]  top-left, offset (24, 16), Size To Content
  │  ├─ [Text] LevelText "Level"          font size 24
  │  ├─ [Spacer] 32 x 1
  │  ├─ [Text] GemsText "Gems 0/3"        font size 24
  │  ├─ [Horizontal Box] GemIconBox       optional
  │  ├─ [Spacer] 32 x 1
  │  └─ [Text] ScoreText "Score 0"        font size 24
  └─ [Text] BannerText   centre, Alignment (0.5, 0.5), Size To Content, size 48, centred, empty
  ```

- [ ] `W_LevelButton` (parent `GSLevelButtonWidget`; Designer toolbar: Desired size). (ch16, ch18)

  ```text
  [Button] Button
  └─ [Vertical Box]
     ├─ [Text] NameText   "1. Level"
     └─ [Text] DetailText "Best 0"     optional
  ```

- [ ] `W_LevelSelect` (parent `GSLevelSelectWidget`). UI > Level Button Class = `W_LevelButton`.

  ```text
  [Vertical Box]
  ├─ [Text] "Select Level"  size 36
  ├─ [Vertical Box] LevelButtonPanel   (any panel)
  └─ [Button] BackButton "Back"
  ```

- [ ] `W_Options` (parent `GSOptionsWidget`). (ch16, ch17, ch18)

  ```text
  [Vertical Box]
  ├─ [Text] "Options"  size 36
  ├─ [Horizontal Box] [Text] "Fullscreen"   [Check Box] FullscreenCheckBox
  ├─ [Horizontal Box] [Text] "VSync"        [Check Box] VSyncCheckBox
  ├─ [Horizontal Box] [Text] "Music"        [Slider] MusicVolumeSlider  Min 0, Max 1, Step Size 0.05
  ├─ [Horizontal Box] [Text] "Effects"      [Slider] SfxVolumeSlider    Min 0, Max 1, Step Size 0.05
  ├─ [Button] ResetProgressButton "Reset Progress"
  └─ [Button] BackButton "Back"
  ```

- [ ] `W_MainMenu` (parent `GSMainMenuWidget`). (ch16, ch18, ch20)

  ```text
  [Canvas Panel]
  ├─ [Image] full-screen anchors, dark colour (background)
  ├─ [Widget Switcher] PanelSwitcher   centre, Alignment (0.5, 0.5), Size To Content
  │  ├─ [Vertical Box] MainPanel       child 0
  │  │  ├─ [Text] "GridSnake"  size 64
  │  │  ├─ [Button] ContinueButton    "Continue"
  │  │  ├─ [Button] PlayButton        "Play"
  │  │  ├─ [Button] LevelSelectButton "Select Level"
  │  │  ├─ [Button] OptionsButton     "Options"
  │  │  └─ [Button] QuitButton        "Quit"
  │  ├─ [W_LevelSelect] LevelSelectPanel   (Palette > User Created)
  │  └─ [W_Options] OptionsPanel
  └─ [Text] VersionText  optional, anchored bottom-right, size 14
  ```

- [ ] `W_PauseMenu` (parent `GSPauseMenuWidget`). (ch16)

  ```text
  [Overlay]
  ├─ [Image] fill, Color (0, 0, 0, 0.6)
  └─ [Vertical Box] centred
     ├─ [Text] "Paused"  size 48
     ├─ [Button] ResumeButton   "Resume"
     ├─ [Button] RestartButton  "Restart Level"
     └─ [Button] MainMenuButton "Main Menu"
  ```

- [ ] `W_LevelEnd` (parent `GSLevelEndWidget`). (ch16)

  ```text
  [Overlay]
  ├─ [Image] fill, Color (0, 0, 0, 0.6)
  └─ [Vertical Box] centred
     ├─ [Text] TitleText "Level Complete!"  size 48
     ├─ [Text] ScoreText "Level score 0   Best 0   Total 0"
     ├─ [Button] NextButton     [Text] NextButtonText "Next Level"
     ├─ [Button] RetryButton    "Retry"
     └─ [Button] MainMenuButton "Main Menu"
  ```

- [ ] Fill in the UI fields of `BP_GSPlayerController` and `BP_GSMenuPlayerController`.

## 9. Data and more levels (chapter 13)

- [ ] In `Data/`: **Miscellaneous > Curve > CurveFloat** `C_SpeedUp` with keys (3, 1.0) and
      (40, 0.5).
- [ ] Duplicate `TM_Level01` as `TM_Level02` and `TM_Level03`, and repaint their `Walls`
      layers.
- [ ] Duplicate `L_Level01` as `L_Level02` and `L_Level03`. In each, point the board's
      **Tile Map** at the new tile map. Move the gems, exit and Player Start onto free cells,
      keeping three gems.
- [ ] **Miscellaneous > Data Asset**, class `GSLevelList`, named `DA_LevelList`, with these
      **Levels**:

  | # | Map | Display Name | Step Interval | Points Per Gem | Speed Curve |
  |---|---|---|---|---|---|
  | 0 | `L_Level01` | First Steps | 0.18 | 100 | none |
  | 1 | `L_Level02` | The Maze | 0.15 | 150 | none |
  | 2 | `L_Level03` | Long Snake | 0.15 | 200 | `C_SpeedUp` |

- [ ] **Project Settings > Game > GridSnake > Levels > Level List** = `DA_LevelList`.

## 10. Main menu map (chapter 16)

- [ ] **File > New Level > Empty Level**, save as `Maps/L_MainMenu`. **Window > World
      Settings > GameMode Override** = `BP_GSMenuGameMode`. It needs no actors.
- [ ] **Project Settings > Game > GridSnake > Levels > Main Menu Map** = `L_MainMenu`.
      **Maps & Modes > Game Default Map** = `L_MainMenu`, **Editor Startup Map** = `L_Level01`
      (or the menu, as you prefer).

## 11. Check it

- [ ] Compare **Project Settings > Game > GridSnake** with `project/ch20/Config/DefaultGame.ini`:
      Gems Per Level 3, Default Step Interval 0.15, Points Per Gem 100, and the five asset fields.
- [ ] Every Blueprint and Widget Blueprint compiles without errors. A missing `BindWidget`
      name is the usual cause.
- [ ] Play `L_MainMenu`. Music starts. Play opens `L_Level01` with a fade, and the HUD shows
      `Gems 0/3`. After the countdown the snake moves. Three gems open the exit, and the exit
      shows the level-end screen. Next Level goes to `L_Level02`. **P** pauses.
- [ ] Back in the menu, Continue appears, level 2 is unlocked, and level 1 shows `Best 300`.
- [ ] In the console: `GS.Debug.Grid 1` draws red walls and green free cells. `GSWin` finishes
      the level. (ch19)

## 12. Shipping settings (chapter 20)

- [ ] **Project Settings > Project > Description > Project Version** = `1.0.0`.
- [ ] **Project Settings > Project > Packaging**: Build Configuration Shipping (or Development
      for test builds). Maps to include: `L_MainMenu`, `L_Level01`, `L_Level02`, `L_Level03`.
      Additional Asset Directories to Cook: `/Game/GridSnake/Data`, `/Game/GridSnake/Audio`,
      `/Game/GridSnake/UI`. Use Pak File and Use Io Store on. The copied `DefaultGame.ini`
      already holds these.
- [ ] **Platforms > *your platform* > Package Project** into a folder outside the project. Run
      it. The menu shows `v1.0.0` and progress survives a restart.


---

Sources for this appendix: [REFERENCES.md](REFERENCES.md#appendix-c--editor-asset-checklist)
