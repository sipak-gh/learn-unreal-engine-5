# Chapter 16 assets: Menus, pause and input modes

Carried over: everything in `../ch15/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Maps/L_MainMenu` | Level | **File > New Level > Empty Level** | World Settings > **GameMode Override** = `BP_GSMenuGameMode` |
| `/Game/GridSnake/Blueprints/BP_GSMenuGameMode` | Blueprint Class | `GSMenuGameMode` | Classes > Player Controller Class = `BP_GSMenuPlayerController`; Default Pawn Class stays None |
| `/Game/GridSnake/Blueprints/BP_GSMenuPlayerController` | Blueprint Class | `GSMenuPlayerController` | UI > Main Menu Widget Class = `W_MainMenu` |
| `/Game/GridSnake/Input/IA_Pause` | Input Action | - | Value Type **Digital (bool)**; **Trigger when Paused** = on |
| `IMC_Snake` (changed) | Input Mapping Context | - | `IA_Pause`: Escape, P, Gamepad Special Right (Start/Menu) |
| `BP_GSPlayerController` (changed) | Blueprint Class | `GSPlayerController` | Input > Pause Action = `IA_Pause`; UI > Pause Menu Widget Class = `W_PauseMenu`, Level End Widget Class = `W_LevelEnd` |
| `/Game/GridSnake/UI/W_LevelButton` | Widget Blueprint | `GSLevelButtonWidget` | Layout below |
| `/Game/GridSnake/UI/W_LevelSelect` | Widget Blueprint | `GSLevelSelectWidget` | Level Button Class = `W_LevelButton` |
| `/Game/GridSnake/UI/W_Options` | Widget Blueprint | `GSOptionsWidget` | Layout below |
| `/Game/GridSnake/UI/W_MainMenu` | Widget Blueprint | `GSMainMenuWidget` | Contains `W_LevelSelect` and `W_Options` instances |
| `/Game/GridSnake/UI/W_PauseMenu` | Widget Blueprint | `GSPauseMenuWidget` | Layout below |
| `/Game/GridSnake/UI/W_LevelEnd` | Widget Blueprint | `GSLevelEndWidget` | Layout below |

## Widget layouts

Bold names are `BindWidget` names and must match exactly. Labels in quotes go in
each button's child **Text** widget. Those text widgets need no special name,
except `NextButtonText`.

```
W_LevelButton                        (Designer toolbar: Fill Screen -> Desired)
[Button] Button
└─ [Vertical Box]
   ├─ [Text] NameText     "1. Level"
   └─ [Text] DetailText   "Best 0"     (optional, used from chapter 18)

W_LevelSelect
[Vertical Box]
├─ [Text] "Select Level"   font size 36
├─ [Vertical Box] LevelButtonPanel     (any panel works: Wrap Box, Uniform Grid...)
└─ [Button] BackButton  "Back"

W_Options
[Vertical Box]
├─ [Text] "Options"   font size 36
├─ [Horizontal Box] [Text] "Fullscreen"  [Check Box] FullscreenCheckBox
├─ [Horizontal Box] [Text] "VSync"       [Check Box] VSyncCheckBox
└─ [Button] BackButton  "Back"

W_MainMenu
[Canvas Panel]
├─ [Image] full-screen anchors, Color and Opacity dark (background)
└─ [Widget Switcher] PanelSwitcher   anchored centre, Alignment (0.5, 0.5), Size To Content
   ├─ [Vertical Box] MainPanel       (child 0)
   │  ├─ [Text] "GridSnake"  font size 64
   │  ├─ [Button] PlayButton         "Play"
   │  ├─ [Button] LevelSelectButton  "Select Level"
   │  ├─ [Button] OptionsButton      "Options"
   │  └─ [Button] QuitButton         "Quit"
   ├─ [W_LevelSelect] LevelSelectPanel   (from the Palette's User Created section)
   └─ [W_Options] OptionsPanel

W_PauseMenu
[Overlay]
├─ [Image] fill, Color (0, 0, 0, 0.6)
└─ [Vertical Box] centred
   ├─ [Text] "Paused"  font size 48
   ├─ [Button] ResumeButton    "Resume"
   ├─ [Button] RestartButton   "Restart Level"
   └─ [Button] MainMenuButton  "Main Menu"

W_LevelEnd
[Overlay]
├─ [Image] fill, Color (0, 0, 0, 0.6)
└─ [Vertical Box] centred
   ├─ [Text] TitleText  "Level Complete!"  font size 48
   ├─ [Text] ScoreText  "Level score 0   Total 0"
   ├─ [Button] NextButton      [Text] NextButtonText "Next Level"
   ├─ [Button] RetryButton     "Retry"
   └─ [Button] MainMenuButton  "Main Menu"
```

Keep every button's **Is Focusable** on (the default), so keyboard and gamepad
navigation works. The focused widget gets a dashed outline when you navigate with
keys or a gamepad. That is **Project Settings > Engine > User Interface > Render
Focus Rule**, default *Navigation Only*. Give the buttons a **Style > Hovered**
tint that differs from Normal for mouse users. Chapter 17 adds a click sound to the
same style.

## Editor steps

1. Build.
2. Create the input action:
   - Create `IA_Pause`: Value Type **Digital (bool)**.
   - Tick **Trigger when Paused**. Without it the game could pause but never
     unpause.
   - In `IMC_Snake`, add a mapping for `IA_Pause` with **Escape**, **P** and
     **Gamepad Special Right**.
   - In PIE, Escape also stops the session: the editor's Stop shortcut. Test pause
     with **P**, or remap Stop in **Editor Preferences > Keyboard Shortcuts**.
3. Build the six Widget Blueprints as above. Each one's parent is its C++ class:
   **Graph > Class Settings > Parent Class**.
   - In `W_LevelSelect`, set Class Defaults > **UI > Level Button Class** =
     `W_LevelButton`.
4. Create `BP_GSMenuPlayerController` and set **Main Menu Widget Class** =
   `W_MainMenu`.
5. Create `BP_GSMenuGameMode` and set its **Player Controller Class** =
   `BP_GSMenuPlayerController`.
6. Create the main menu level:
   - **File > New Level > Empty Level**, saved as `GridSnake/Maps/L_MainMenu`.
   - **Window > World Settings > Game Mode > GameMode Override** =
     `BP_GSMenuGameMode`.
   - The level needs no actors. The menu draws over a black view.
7. In `BP_GSPlayerController`, set **Pause Action**, **Pause Menu Widget Class** and
   **Level End Widget Class**.
8. **Project Settings > Game > GridSnake > Levels > Main Menu Map** = `L_MainMenu`,
   and **Maps & Modes > Game Default Map** = `L_MainMenu`. Keep Editor Startup Map
   on `L_Level01`, or switch it, as you prefer.
9. Test:
   - Open `L_MainMenu` and Play. The mouse cursor is visible and Play / Select
     Level / Options / Quit work with mouse, keyboard (arrows + Enter) and gamepad.
   - Quit ends PIE.
   - In a level, **P** pauses: the game freezes and the pause menu has focus.
   - Finishing or failing a level shows `W_LevelEnd` instead of moving on by
     itself.
