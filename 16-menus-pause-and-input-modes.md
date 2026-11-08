# Chapter 16 — Menus, Pause and Input Modes

A game is more than its levels. Before the first level there is a main menu. During a level the
player can pause. After a level the game says how it went and asks what to do next. This
chapter builds all three screens. On the way it covers the parts of Unreal that menus depend on
and that are easy to get subtly wrong:

- a **main menu level** with its own game mode and player controller, and no pawn;
- the three **input modes**, which decide whether keys and the mouse go to the game, the UI or
  both;
- **focus**: which widget receives the keyboard and gamepad, and how arrow keys and the D-pad move
  between buttons;
- **pausing**: what stops, what keeps running, and how the pause key can still unpause;
- **widget switchers** for menus with several panels, and native delegates to connect them;
- an options screen backed by **`UGameUserSettings`**, and quitting the game.

GridSnake gains `L_MainMenu` with Play / Select Level / Options / Quit, a pause menu (Resume /
Restart Level / Main Menu) and a level-end screen (Next Level or Retry / Main Menu). The game now
boots into the menu.

Previous: [Chapter 15](15-a-hud-with-umg.md). Next: [Chapter 17](17-sound-and-music.md) (sound and
music).

## A menu is a level

You could draw a main menu on top of level 1 and hide it when the player presses Play. Many
small games do. GridSnake gives the menu **its own level** instead, `L_MainMenu`, for three
reasons:

- **Nothing runs behind it.** No snake, no countdown, no timers. The menu level has no actors at
  all, so there is nothing to freeze, hide or reset.
- **Its own rules.** A level can override the game mode in its **World Settings**. The menu's game
  mode spawns no pawn and uses a player controller that only shows widgets. The gameplay game mode
  from chapter 7 stays exactly as it was.
- **A clean start.** "Main Menu" from the pause menu just opens `L_MainMenu`. Opening a level
  destroys the old world with every actor and widget in it (chapter 14), so the menu can never
  find leftovers from the game.

The cost is a level load when entering and leaving the menu. Chapter 14's fade and loading screen
already hide that.

### The menu's game mode

`Source/GridSnake/Core/GSMenuGameMode.h` (new) declares `AGSMenuGameMode : AGameModeBase` with a
constructor, a `StartPlay` override and a protected `PlayerCanRestart_Implementation` override.

`Source/GridSnake/Core/GSMenuGameMode.cpp` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSMenuGameMode.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Core/GSMenuPlayerController.h"
#include "Engine/GameInstance.h"

AGSMenuGameMode::AGSMenuGameMode()
{
	// BP_GSMenuGameMode replaces this with BP_GSMenuPlayerController, which knows W_MainMenu.
	PlayerControllerClass = AGSMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void AGSMenuGameMode::StartPlay()
{
	// Clears the "transitioning" flag and fades in, as in every level.
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->NotifyLevelStarted(GetWorld());

	Super::StartPlay();
}

bool AGSMenuGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return false;
}
```

- It derives from `AGameModeBase`, not from `AGSGameMode`. The menu has no gems, snake or
  countdown, so it inherits none of that.
- **No pawn, two ways.** `DefaultPawnClass = nullptr` says there is no class to spawn.
  `PlayerCanRestart` returning `false` tells the game mode that this player should not be
  (re)started at all, so it never even tries to spawn one. Without a pawn the controller views the
  world from its own position: an empty, black level, which is all the menu needs.
- **`StartPlay` calls `NotifyLevelStarted`.** Chapter 14's `UGSLevelFlowSubsystem` sets a
  `bTransitioning` flag when it starts loading a level, and ignores further requests until
  `NotifyLevelStarted` clears it. `AGSGameMode` calls it in every gameplay level. The menu must call
  it too: otherwise, after "Main Menu" from a level, the flag would still be set and the Play
  button would silently do nothing. The call also fades the view in from black.

### The menu's player controller

The header (`Core/GSMenuPlayerController.h`, new) declares a `BeginPlay` override, an
`EditDefaultsOnly` property `TSubclassOf<UGSMainMenuWidget> MainMenuWidgetClass` (set to `W_MainMenu`
in the Blueprint) and a `Transient` pointer `MainMenuWidget`, the same pattern as chapter 15's
HUD.

`Source/GridSnake/Core/GSMenuPlayerController.cpp` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSMenuPlayerController.h"
#include "GridSnake.h"
#include "UI/GSMainMenuWidget.h"

void AGSMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s has no MainMenuWidgetClass - set it in BP_GSMenuPlayerController"), *GetName());
		return;
	}

	// UI only: game input is ignored and the cursor is free. Set it before
	// adding the menu, which then focuses its Play button so arrow keys and
	// the D-pad can move between buttons.
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	MainMenuWidget = CreateWidget<UGSMainMenuWidget>(this, MainMenuWidgetClass);
	MainMenuWidget->AddToViewport();
}
```

- `IsLocalController()` is true for a controller that belongs to a player on this machine. In a
  single-player game it always is. On a multiplayer server, the controllers of remote players are
  not local, and they must not create widgets. The check costs nothing and keeps the habit.
- The input mode and cursor lines are the subject of the next section. Note the order: input mode
  first, then the widget.

### Booting into the menu

Two settings point at the new map. The game's start map is in `DefaultEngine.ini`:

`Config/DefaultEngine.ini` (changed)

```ini
[/Script/EngineSettings.GameMapsSettings]
; GridSnake (ch01): Project Settings > Project > Maps & Modes
; GridSnake (ch16): the game now starts in the main menu.
GameDefaultMap=/Game/GridSnake/Maps/L_MainMenu.L_MainMenu
EditorStartupMap=/Game/GridSnake/Maps/L_Level01.L_Level01
```

- **Game Default Map** is the map a packaged game (or Standalone Game) opens first. It is now the
  menu.
- **Editor Startup Map** is the map the editor opens. It stays `L_Level01`, so you can keep
  pressing Play on a level while you work on it. Chapter 14's subsystem works out which level is
  running, so playing a level directly still works.

The "Main Menu" buttons need to know the menu map too. Like the level list, it goes in the
project settings class from chapter 13, `UGSSettings`:

`Source/GridSnake/Core/GSSettings.h` (added)

```cpp
	/** The map with the main menu, loaded by "Main Menu" buttons. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<UWorld> MainMenuMap;
```

`Config/DefaultGame.ini` (added, at the end of the `GSSettings` section)

```ini
; GridSnake (ch16)
MainMenuMap=/Game/GridSnake/Maps/L_MainMenu.L_MainMenu
```

A soft pointer, as for the level list: the settings object exists from engine start, and it must
not load a whole map into memory just by naming it. The level flow subsystem gets one more entry
point:

`Source/GridSnake/Core/GSLevelFlowSubsystem.h` (added)

```cpp
	/** Ends the run and loads the main menu map. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	void ReturnToMainMenu();
```

`Source/GridSnake/Core/GSLevelFlowSubsystem.cpp` (added)

```cpp
void UGSLevelFlowSubsystem::ReturnToMainMenu()
{
	CurrentLevelIndex = INDEX_NONE;
	OpenLevelWithFade(UGSSettings::Get()->MainMenuMap);
}
```

`OpenLevelWithFade` is chapter 14's fade-out-then-open function. `CurrentLevelIndex` goes back to
`INDEX_NONE` (-1): no level of the run is current. The run score is left alone; the next
`StartRun` resets it.

## Input modes

Unreal sends every key press, mouse move and gamepad button to **Slate**, the UI layer under UMG
(chapter 15). Slate decides where it goes. The game viewport, the area the world is drawn in, is
itself a Slate widget, and the game's input (Enhanced Input, chapter 8) only sees what Slate
passes to the viewport. The player controller's **input mode** sets the rules for that.

An input mode is a small bundle of settings, applied all at once:

- **Does the game receive input at all?** In UI only mode the viewport ignores input, so the
  player controller sees nothing.
- **Which widget has keyboard focus?** Key presses go to the focused widget first.
- **Is the mouse locked to the viewport, and captured?** A captured mouse sends all movement to
  the game (for camera look) and is usually hidden.

The mouse **cursor** is separate: `bShowMouseCursor`, or the `SetShowMouseCursor(bool)` setter,
on the player controller. Change it together with the input mode.

### The three modes

```cpp
// GameFramework/PlayerController.h
void SetInputMode(const FInputModeDataBase& InData);

// The three modes, each a small struct with setters:
FInputModeGameOnly InputMode;
FInputModeUIOnly InputMode;            // SetWidgetToFocus, SetLockMouseToViewportBehavior
FInputModeGameAndUI InputMode;         // the same, plus SetHideCursorDuringCapture
```

| Mode | Game input | Keyboard focus | Mouse | Use it for |
|---|---|---|---|---|
| `FInputModeGameOnly` | Yes | The game viewport | Captured by the game | Playing |
| `FInputModeUIOnly` | No | `SetWidgetToFocus`, if given | Free (if not locked) | Menus that replace the game |
| `FInputModeGameAndUI` | Keys the UI does not handle | `SetWidgetToFocus`, if given | Free; captured only while a button is held | Menus over a game that still listens |

- **`FInputModeGameOnly`** gives keyboard focus back to the game viewport and lets the game
  capture the mouse. Use it whenever a menu closes. It has no widget to focus.
- **`FInputModeUIOnly`** stops all input to the game. Widgets get the mouse and keys. The game
  can't even see Escape or the pause key. `SetWidgetToFocus(SomeWidget->TakeWidget())` focuses a
  widget as part of the mode change. (`TakeWidget()` returns the UMG widget's underlying Slate
  widget, which is what the input mode works with.)
- **`FInputModeGameAndUI`** is the in-between mode. Widgets get the mouse and keyboard first.
  Anything they don't handle continues to the game. `SetHideCursorDuringCapture(false)` keeps
  the cursor visible while a mouse button is held over the game. The default hides it.
- **`SetLockMouseToViewportBehavior`** takes an `EMouseLockMode`: `DoNotLock`, `LockOnCapture`,
  `LockAlways` or `LockInFullscreen`. With `DoNotLock` the cursor can leave the window, which is
  what players expect in a menu. The default differs between the modes, so the GridSnake code
  always sets it explicitly.

In Blueprints, the same three modes are the nodes **Set Input Mode Game Only**, **Set Input Mode UI
Only** and **Set Input Mode Game And UI**.

### How a key press travels

The difference between UI only and Game and UI is easiest to see by following one key press. Say
the pause menu is open and its Resume button has focus, and the player presses P:

```
Key press
  │
  ▼
Slate: the focused widget gets OnKeyDown first ......... ResumeButton (does not use P)
  │ unhandled: bubble up to the parent
  ▼
... each parent widget gets a chance .................... the pause menu, then the viewport's layers
  │ unhandled
  ▼
The game viewport widget
  ├─ UI only:     ignores it. Nothing happens.
  └─ Game and UI: passes it to the player controller ─▶ Enhanced Input ─▶ IA_Pause fires
  │ still unhandled
  ▼
Slate navigation: arrow keys, Tab and the D-pad move focus between widgets
```

Three things follow from this picture:

- In **UI only** mode a menu must handle its own close key, for example in `NativeOnKeyDown`, as
  the confirm dialog below does. The game's input actions never fire.
- In **Game and UI** mode a key that the UI doesn't use reaches the game. That is why GridSnake's
  pause menu uses it: the same `IA_Pause` key that opened the menu also closes it.
- Navigation comes **last**. If something earlier handles an arrow key, focus does not move. That
  becomes important in [Pausing](#pausing).

### Focus and the order of calls

`SetInputMode` doesn't change focus immediately. It records the change, and the engine applies it
shortly afterwards, once the current input event or frame is processed. `UWidget::SetFocus()`, on
the other hand, takes effect straight away if the widget is already on screen (if it isn't yet,
the request is remembered until it is). Mixing them in the wrong order gives a menu that looks
right but ignores the keyboard:

```cpp
// Wrong: the menu focuses its button in NativeConstruct, during AddToViewport...
Menu->AddToViewport();
// ...then GameOnly's queued focus change hands focus back to the viewport.
SetInputMode(FInputModeGameOnly());
```

The exact timing has varied between engine versions, so don't rely on it. Follow one of two rules
that always work:

1. **Set the input mode first, then show the widget and focus inside it.** GridSnake does this
   everywhere. `FInputModeUIOnly` and `FInputModeGameAndUI` without `SetWidgetToFocus` don't
   move the focus anywhere, so the widget's own `SetFocus` in `NativeConstruct` stands.
2. **Or let the input mode do the focusing** with `SetWidgetToFocus`. Because the input mode is
   applied later, it wins over any `SetFocus` made earlier in the same frame. The example below
   uses this.

### Example: a menu controller

`ExMenuController` is a player controller that shows one menu at a time. `ShowMenu` gives the
input to the UI; `HideMenu` gives it back to the game. It works in any project. Its partner,
`ExConfirmDialog`, is a yes/no dialog that reports its answer through a native delegate. Together
they show both focus rules and both ways of closing a UI-only menu: a button and a key.

`examples/ch16/ExConfirmDialog.h` (excerpt)

```cpp
UCLASS(Abstract)
class UExConfirmDialog : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMessage(const FText& InMessage);

	/**
	 * A native single-cast delegate: true for Yes, false for No.
	 * DECLARE_DELEGATE_OneParam(FMyName, bool) declares this same type with a name.
	 */
	TDelegate<void(bool)> OnResult;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// YesButton and NoButton (UButton), Message (UTextBlock): all BindWidget.
	// Private: UFUNCTION() HandleYesClicked / HandleNoClicked, and Finish(bool).
};
```

`examples/ch16/ExConfirmDialog.cpp`

```cpp
// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, InputCore, UMG, Slate, SlateCore.

#include "ExConfirmDialog.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogExConfirm, Log, All);

void UExConfirmDialog::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Once per widget object, so the bindings can never be added twice.
	YesButton->OnClicked.AddDynamic(this, &UExConfirmDialog::HandleYesClicked);
	NoButton->OnClicked.AddDynamic(this, &UExConfirmDialog::HandleNoClicked);
}

void UExConfirmDialog::NativeConstruct()
{
	Super::NativeConstruct();

	// The safe answer gets the focus, so a stray Enter or A press does no harm.
	NoButton->SetFocus();
}

void UExConfirmDialog::SetMessage(const FText& InMessage)
{
	Message->SetText(InMessage);
}

FReply UExConfirmDialog::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Keys the focused button does not use bubble up to us. Escape or the
	// gamepad's right face button (B on Xbox) mean "no". In Play In Editor,
	// Escape stops the session before the game sees it.
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		Finish(false);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UExConfirmDialog::HandleYesClicked()
{
	Finish(true);
}

void UExConfirmDialog::HandleNoClicked()
{
	Finish(false);
}

void UExConfirmDialog::Finish(bool bConfirmed)
{
	UE_LOG(LogExConfirm, Log, TEXT("Confirm result: %s"), bConfirmed ? TEXT("true") : TEXT("false"));

	// Off the screen first: the listener may open another menu.
	RemoveFromParent();
	OnResult.ExecuteIfBound(bConfirmed);
}
```

- **`NativeOnKeyDown`** is the C++ side of a widget's **On Key Down** event. It receives keys
  that the focused widget inside it did not handle (the bubbling from the diagram). Return
  `FReply::Handled()` to stop the key there, or call `Super` to let it continue. This is how a
  UI-only menu closes itself on Escape or B, since the game can't see those keys.
- **`TDelegate<void(bool)>`** is a native, single-cast delegate: exactly one listener, called
  with `ExecuteIfBound` (or `Execute`, which asserts a listener is bound). Native delegates bind
  to plain C++ functions and lambdas and cost less than dynamic ones, but Blueprints can't see
  them. The dialog's only client is C++, so that is fine.
- `RemoveFromParent()` comes **before** the broadcast. A listener might show another menu, and the
  old one should be off the screen by then.
- `NativeOnInitialized` binds the buttons and `NativeConstruct` sets the focus. The difference
  matters for every menu in this chapter, so it gets its own subsection in
  [Panels with a widget switcher](#panels-with-a-widget-switcher).

`examples/ch16/ExMenuController.h` declares `AExMenuController : APlayerController` with:

- `UUserWidget* ShowMenu(TSubclassOf<UUserWidget> MenuClass, FName FocusWidgetName = NAME_None)`
  and `void HideMenu()`, both `BlueprintCallable`;
- two console commands, `UFUNCTION(Exec) void ExAskConfirm()` and `void ExCloseMenu()`;
- `UPROPERTY(EditDefaultsOnly) TSubclassOf<UExConfirmDialog> ConfirmDialogClass` (`W_ExConfirm`);
- a private `HandleConfirmResult(bool)` and a `UPROPERTY(Transient)` pointer `CurrentMenu`.

`examples/ch16/ExMenuController.cpp`

```cpp
// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, InputCore, UMG, Slate, SlateCore.

#include "ExMenuController.h"
#include "Blueprint/UserWidget.h"
#include "ExConfirmDialog.h"

DEFINE_LOG_CATEGORY_STATIC(LogExMenu, Log, All);

UUserWidget* AExMenuController::ShowMenu(TSubclassOf<UUserWidget> MenuClass, FName FocusWidgetName)
{
	if (!MenuClass)
	{
		UE_LOG(LogExMenu, Warning, TEXT("ShowMenu: no menu class"));
		return nullptr;
	}

	// One menu at a time.
	if (CurrentMenu)
	{
		CurrentMenu->RemoveFromParent();
	}

	CurrentMenu = CreateWidget<UUserWidget>(this, MenuClass);
	CurrentMenu->AddToViewport(/*ZOrder=*/10);

	// UI only: keys and buttons go to widgets, the game sees nothing.
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	// The input mode can move focus too. It is applied after this function, so
	// it wins over any SetFocus the menu did in NativeConstruct.
	UWidget* FocusTarget = FocusWidgetName.IsNone() ? nullptr : CurrentMenu->GetWidgetFromName(FocusWidgetName);
	if (FocusTarget)
	{
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
	}

	SetInputMode(InputMode);
	SetShowMouseCursor(true);

	UE_LOG(LogExMenu, Log, TEXT("Menu shown: %s (input: UI only, focus %s)"),
		*CurrentMenu->GetName(), FocusTarget ? *FocusTarget->GetName() : TEXT("left to the menu"));
	return CurrentMenu;
}

void AExMenuController::HideMenu()
{
	if (!CurrentMenu)
	{
		return;
	}

	CurrentMenu->RemoveFromParent();
	CurrentMenu = nullptr;

	// Game only also gives keyboard focus back to the game viewport.
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	UE_LOG(LogExMenu, Log, TEXT("Menu hidden (input: game only)"));
}

void AExMenuController::ExAskConfirm()
{
	UExConfirmDialog* Dialog = Cast<UExConfirmDialog>(ShowMenu(ConfirmDialogClass, TEXT("NoButton")));
	if (!Dialog)
	{
		return;
	}

	Dialog->SetMessage(NSLOCTEXT("ExMenu", "Question", "Is this the right answer?"));
	// Native delegates bind to plain member functions; BindUObject also checks
	// that this controller still exists before calling it.
	Dialog->OnResult.BindUObject(this, &AExMenuController::HandleConfirmResult);
}

void AExMenuController::ExCloseMenu()
{
	HideMenu();
}

void AExMenuController::HandleConfirmResult(bool bConfirmed)
{
	UE_LOG(LogExMenu, Log, TEXT("Controller heard: %s"), bConfirmed ? TEXT("yes") : TEXT("no"));
	HideMenu();
}
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`, `InputCore`, `UMG`, `Slate`, `SlateCore`.

- **`ShowMenu`** uses focus rule 2. It adds the menu first and then names the widget to focus
  in the input mode. `GetWidgetFromName` finds a widget in a user widget's tree by its designer
  name, the same name `BindWidget` uses. It returns `nullptr` if there is none, so the log says
  `left to the menu` in that case.
- **`HideMenu`** removes the menu and switches to `FInputModeGameOnly`, which also returns focus
  to the viewport. Without that, the game would get no keys after a UI-only menu.
- **`UFUNCTION(Exec)`** makes a function a console command (chapter 9). Exec functions work on a
  player controller, its pawn, the game mode, the HUD and a few other classes, but not on
  arbitrary actors.
- **`BindUObject`** binds a native delegate to a member function of a `UObject`. The delegate
  holds a weak reference: if the controller is destroyed, `ExecuteIfBound` simply does nothing.
  (`BindLambda` and `BindRaw` would not check, so prefer `BindUObject` for `UObject`s.)

To try it:

1. Copy the four files into your module, check the modules above are in its `Build.cs`, and build.
2. Create a Widget Blueprint with parent class `ExConfirmDialog` (as in chapter 15: right-click >
   **User Interface > Widget Blueprint**, then pick the parent under **All Classes**). Name it
   `W_ExConfirm`, and build this hierarchy (bold names must match exactly):

   ```
   [Overlay]
   ├─ [Image]              Horizontal and Vertical Alignment: Fill; Color and Opacity (0, 0, 0, 0.6)
   └─ [Vertical Box]       Horizontal and Vertical Alignment: Center
      ├─ [Text] Message    "Question?"
      └─ [Horizontal Box]
         ├─ [Button] YesButton  ─ [Text] "Yes"
         └─ [Button] NoButton   ─ [Text] "No"
   ```

   The full-screen Image does two jobs. It darkens what is behind, and, being visible to the
   mouse, it stops clicks from reaching anything underneath. Compile and save.
3. Create a Blueprint Class with parent `ExMenuController`, named `BP_ExMenuController`. In its
   Class Defaults set **Example > Confirm Dialog Class** = `W_ExConfirm`.
4. Create a Blueprint Class with parent **Game Mode Base**, named `BP_ExMenuGameMode`, and set its
   **Player Controller Class** = `BP_ExMenuController`. Open a test level and set **World Settings
   > GameMode Override** = `BP_ExMenuGameMode`.
5. Press **Play**. Open the console with the backtick key (`` ` ``, the key above Tab on a US
   keyboard) and type `ExAskConfirm`. The dialog appears with **No** focused. Press the left
   arrow key, then Enter, to answer Yes. Run it again and click **No** with the mouse.
6. Run `ExAskConfirm` once more, then `ExCloseMenu` from the console. The console still works in
   UI only mode: it belongs to the viewport client, which keeps feeding it keys.

Expected Output Log (filter: `LogExMenu` and `LogExConfirm`):

```
LogExMenu: Menu shown: W_ExConfirm_C_0 (input: UI only, focus NoButton)
LogExConfirm: Confirm result: true
LogExMenu: Controller heard: yes
LogExMenu: Menu hidden (input: game only)
LogExMenu: Menu shown: W_ExConfirm_C_1 (input: UI only, focus NoButton)
LogExConfirm: Confirm result: false
LogExMenu: Controller heard: no
LogExMenu: Menu hidden (input: game only)
LogExMenu: Menu shown: W_ExConfirm_C_2 (input: UI only, focus NoButton)
LogExMenu: Menu hidden (input: game only)
```

- The number after `_C_` counts the widget objects created; it may differ for you.
- The answer is logged by the dialog first, then heard by the controller. The listener runs inside
  `ExecuteIfBound`, so `Controller heard` always comes right after `Confirm result`.
- If the arrow key doesn't move the focus, the dialog probably has no focus at all: check that
  the buttons are named exactly `YesButton` and `NoButton` (the log shows `left to the menu` if
  `NoButton` isn't found).
- Escape closes the dialog only outside Play In Editor (see [Pausing](#pausing)). In PIE, press
  the gamepad's B button, or try it in **Standalone Game** (the drop-down next to Play).

### GridSnake's input modes

GridSnake has four screens and picks a mode for each:

| Screen | Mode | Cursor | Why |
|---|---|---|---|
| Playing (HUD) | Game only | Hidden | The HUD only displays; it takes no input. |
| Main menu | UI only | Shown | There is no game to send input to. |
| Pause menu | Game and UI | Shown | The pause key must reach `IA_Pause` to unpause. |
| Level end | UI only | Shown | The level is over: the game must not react to anything. |

The player controller does the menu side in one helper:

`Source/GridSnake/Core/GSPlayerController.cpp` (added)

```cpp
void AGSPlayerController::EnterMenuInputMode(bool bGameKeepsInput)
{
	if (bGameKeepsInput)
	{
		// Game and UI: widgets get the mouse and focus, but keys they do not use
		// (Escape) still reach the player's input, so the pause key can unpause.
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		// UI only: the game sees no input at all.
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	bShowMouseCursor = true;
}
```

Its header comment states the rule: "Call it before adding the widget: the widget then moves
focus to its first button." Neither mode sets `SetWidgetToFocus`, so the widgets' own `SetFocus`
calls decide the focus (rule 1).

And `BeginPlay` resets the mode for every gameplay level:

`Source/GridSnake/Core/GSPlayerController.cpp` (added to `BeginPlay`, after the view target is set)

```cpp
	// Coming from a menu, the UI may still own the keyboard: give it back to the game.
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
```

Why, when the new level has a brand-new player controller? Because the input mode isn't stored in
the controller. It is applied to the **game viewport** and to Slate's focus, and those survive a
level change. Coming from the menu or the level-end screen (both UI only), the viewport would still
ignore game input and the snake would not steer. The bug hides if you always test by pressing Play
on `L_Level01`, where the viewport starts in its default state.

## Panels with a widget switcher

The main menu has three "pages": the main buttons, level select and options. They could be three
separate widgets added and removed in turn. GridSnake puts them all in one `W_MainMenu` and uses
a **Widget Switcher** to show one at a time.

A **Widget Switcher** (`UWidgetSwitcher`, in the Palette's **Panel** section) is a panel that
holds any number of children but displays only one, its **active widget**. The others still exist
with all their state; they just are not drawn, can't be clicked and can't take focus. You pick
the active child by position with `SetActiveWidgetIndex(int32)` (0 is the first child) or by
pointer with `SetActiveWidget(UWidget*)`, and read it back with `GetActiveWidgetIndex()` or
`GetActiveWidget()`.

GridSnake switches by pointer. Indexes break silently when someone reorders the children in the
designer; pointers to `BindWidget` properties do not. In the designer, the switcher's **Active
Widget Index** (in its Details panel, under **Switcher**; the label may differ slightly in your
version) chooses which child you see while editing. It is also the starting child at runtime.

### A base class for panels

Level select and options have something in common: a Back button that returns to the main
buttons, and a moment when they are shown and must fill themselves in. That goes in a base class:

`Source/GridSnake/UI/GSMenuPanelWidget.h` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSMenuPanelWidget.generated.h"

class UButton;

/** A native (C++-only) delegate: cheaper than a dynamic one, but invisible to Blueprints. */
DECLARE_MULTICAST_DELEGATE(FGSBackRequestedSignature);

/**
 * Base class for a sub-screen of the main menu (level select, options).
 * The main menu shows a panel, calls Refresh, and listens for OnBackRequested.
 */
UCLASS(Abstract)
class UGSMenuPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Brings the panel up to date and moves focus into it. */
	virtual void Refresh() {}

	FGSBackRequestedSignature OnBackRequested;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

private:
	UFUNCTION()
	void HandleBackClicked();
};
```

`Source/GridSnake/UI/GSMenuPanelWidget.cpp` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSMenuPanelWidget.h"
#include "Components/Button.h"

void UGSMenuPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// NativeOnInitialized runs once per widget object, so bindings made here
	// are never doubled (NativeConstruct runs each time it is added to a parent).
	BackButton->OnClicked.AddDynamic(this, &UGSMenuPanelWidget::HandleBackClicked);
}

void UGSMenuPanelWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}
```

- **`BindWidget` in a base class** works for every Widget Blueprint whose parent is a subclass:
  `W_LevelSelect` and `W_Options` must both contain a Button named `BackButton`.
- **The panel doesn't know who shows it.** It only announces "back was requested". The main menu
  listens and decides what back means. The same panel could sit in a pause menu, where back would
  mean something else.
- `Refresh` is virtual with an empty body, so a panel that has nothing to refresh needs no code.

### Native and dynamic delegates

`FGSBackRequestedSignature` is a **native** multicast delegate. Chapter 11 used **dynamic**
delegates, and both kinds appear in this chapter, so here is the difference:

| | Native: `DECLARE_MULTICAST_DELEGATE...`, `TDelegate` | Dynamic: `DECLARE_DYNAMIC_MULTICAST_DELEGATE...` |
|---|---|---|
| Bind with | `AddUObject`, `AddLambda`, `AddRaw`, `AddSP`... (`Bind...` for single-cast) | `AddDynamic` (C++), or a Blueprint event |
| Handler | Any C++ function or lambda | Must be a `UFUNCTION` |
| Blueprints | Can't see it; can't be a `UPROPERTY` | `UPROPERTY(BlueprintAssignable)` exposes it |
| Cost | A direct call | A call by function name through reflection |
| Examples here | `OnBackRequested`, `ExConfirmDialog::OnResult` | `UButton::OnClicked`, `UCheckBox::OnCheckStateChanged` |

Use dynamic delegates when Blueprints must bind (all UMG widget events are dynamic for that
reason). Use native ones for C++-to-C++ plumbing.

`AddUObject(this, &Class::Function)` stores a weak reference to `this`. If the object is destroyed,
the binding is skipped at broadcast time rather than crashing. Lambdas (`AddLambda`) and raw
pointers (`AddRaw`) get no such check, so prefer `AddUObject` when the listener is a `UObject`.

### NativeOnInitialized or NativeConstruct?

Chapter 15's HUD bound to the game state in `NativeConstruct` and unbound in `NativeDestruct`.
The menus bind their buttons in `NativeOnInitialized` instead. Both are right, for different
kinds of binding:

| Function | Runs | Bind here |
|---|---|---|
| `NativeOnInitialized` | **Once**, when the widget object is created (`CreateWidget`, or as part of its parent) | Delegates on your **own child widgets**: they live and die with you. Never unbind. |
| `NativeConstruct` | **Every time** the widget is added to the screen or to a parent | Delegates on **outside objects** (the game state). Unbind in `NativeDestruct`. Also: set the focus. |
| `NativeDestruct` | Every time the widget is removed | Undo what `NativeConstruct` did. |

GridSnake's pause menu is created once and added and removed every time the player pauses. Had it
bound `ResumeButton->OnClicked` in `NativeConstruct` with `AddDynamic`, the second pause would bind
again, and Resume would run twice per click (with an ensure in the log, as chapter 15's exercise 4
described). Focus, on the other hand, must be set every time the menu appears, so `SetFocus` goes
in `NativeConstruct`.

### The main menu

`Source/GridSnake/UI/GSMainMenuWidget.h` (new) declares `UCLASS(Abstract) class
UGSMainMenuWidget : public UUserWidget` with these `BindWidget` properties: `UWidgetSwitcher
PanelSwitcher`; `UWidget MainPanel` (the switcher's first child, the column of main buttons);
`UGSLevelSelectWidget LevelSelectPanel`; `UGSOptionsWidget OptionsPanel`; and four `UButton`s,
`PlayButton`, `LevelSelectButton`, `OptionsButton` and `QuitButton`. It overrides
`NativeOnInitialized` and `NativeConstruct`, and declares `ShowMainPanel()`,
`ShowPanel(UGSMenuPanelWidget*)` and a `UFUNCTION()` click handler per button.

`Source/GridSnake/UI/GSMainMenuWidget.cpp` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSMainMenuWidget.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/GSLevelSelectWidget.h"
#include "UI/GSOptionsWidget.h"

void UGSMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PlayButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandlePlayClicked);
	LevelSelectButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleLevelSelectClicked);
	OptionsButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleOptionsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleQuitClicked);

	// Native delegates bind with AddUObject: the binding is skipped safely if we are destroyed.
	LevelSelectPanel->OnBackRequested.AddUObject(this, &UGSMainMenuWidget::ShowMainPanel);
	OptionsPanel->OnBackRequested.AddUObject(this, &UGSMainMenuWidget::ShowMainPanel);
}

void UGSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ShowMainPanel();
}

void UGSMainMenuWidget::ShowMainPanel()
{
	PanelSwitcher->SetActiveWidget(MainPanel);
	PlayButton->SetFocus();
}

void UGSMainMenuWidget::ShowPanel(UGSMenuPanelWidget* Panel)
{
	PanelSwitcher->SetActiveWidget(Panel);
	Panel->Refresh();
}

void UGSMainMenuWidget::HandlePlayClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->StartRun(0);
}

void UGSMainMenuWidget::HandleLevelSelectClicked()
{
	ShowPanel(LevelSelectPanel);
}

void UGSMainMenuWidget::HandleOptionsClicked()
{
	ShowPanel(OptionsPanel);
}

void UGSMainMenuWidget::HandleQuitClicked()
{
	// In Play In Editor this ends the session; in a packaged game it closes the window.
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
}
```

- **Widgets inside widgets.** `LevelSelectPanel` is not a Button or a Text but a whole user widget:
  an instance of `W_LevelSelect`, dragged into `W_MainMenu` from the Palette's **User Created**
  section. `BindWidget` binds it like any other widget, as long as the instance's name matches and
  its class is a `UGSLevelSelectWidget`. That is composition: each panel is designed, compiled and
  tested in its own asset.
- **`MainPanel` is a plain `UWidget`.** The main buttons sit in a Vertical Box, and the code only
  needs it to pass to `SetActiveWidget`, so the most general type will do. `BindWidget` accepts
  any widget class derived from the property's type.
- **Switch, then refresh.** `ShowPanel` makes the panel visible before `Refresh` moves focus into
  it. Focus can't go to a widget the switcher is hiding.
- **`ShowMainPanel` on construct.** The menu always opens on the main buttons with Play focused,
  even if someone left the switcher's Active Widget Index on another panel in the designer.
- **Play calls `StartRun(0)`.** The level flow subsystem from chapter 14 resets the run score and
  opens the first level with a fade. The menu only asks; it knows nothing about maps.

### Level select: buttons built at runtime

Level select shows one button per entry in chapter 13's level list (`DA_LevelList`). The number
of levels is data, so the buttons are created in code, like chapter 15's gem icons. This time each
button is a small user widget of its own, `W_LevelButton`, so it can be styled in the designer.

`Source/GridSnake/UI/GSLevelButtonWidget.cpp` (new; the header binds `UButton Button` and
`UTextBlock NameText` with `BindWidget`, `UTextBlock DetailText` with `BindWidgetOptional`, and
stores an `int32 LevelIndex`. `NativeOnInitialized` binds `Button->OnClicked` to `HandleClicked`.)

```cpp
void UGSLevelButtonWidget::Setup(int32 InLevelIndex, const FGSLevelInfo& Level)
{
	LevelIndex = InLevelIndex;
	NameText->SetText(FText::Format(LOCTEXT("LevelButton", "{0}. {1}"), LevelIndex + 1, Level.DisplayName));

	if (DetailText)
	{
		DetailText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGSLevelButtonWidget::FocusButton()
{
	Button->SetFocus();
}

void UGSLevelButtonWidget::HandleClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->StartRun(LevelIndex);
}
```

- **Each button remembers its level.** A `UButton`'s `OnClicked` has no parameters, so a list of
  plain buttons sharing one handler could not tell which was clicked. Wrapping each button in a
  user widget with a `LevelIndex` member solves that neatly.
- **`FocusButton`.** Focus must go to the `UButton` inside, not to the user widget around it:
  the button is the focusable part. A user widget can be made focusable itself (**Is Focusable** in
  its Class Defaults), but then it would need its own key handling.
- `DetailText` is optional and collapsed for now. Chapter 18 shows the best score there.

`Source/GridSnake/UI/GSLevelSelectWidget.cpp` (new; the header declares `Refresh` as an override,
`UPanelWidget* LevelButtonPanel` as `BindWidget` and
`TSubclassOf<UGSLevelButtonWidget> LevelButtonClass` as `EditDefaultsOnly`)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSLevelSelectWidget.h"
#include "GridSnake.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
#include "UI/GSLevelButtonWidget.h"

void UGSLevelSelectWidget::Refresh()
{
	LevelButtonPanel->ClearChildren();

	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	if (!LevelList || !LevelButtonClass)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("Level select needs a level list and a LevelButtonClass"));
		BackButton->SetFocus();
		return;
	}

	UGSLevelButtonWidget* FirstButton = nullptr;
	for (int32 Index = 0; Index < LevelList->Num(); ++Index)
	{
		// A widget created with another widget as its owner shares its owning player.
		UGSLevelButtonWidget* LevelButton = CreateWidget<UGSLevelButtonWidget>(this, LevelButtonClass);
		LevelButton->Setup(Index, *LevelList->GetLevel(Index));
		LevelButtonPanel->AddChild(LevelButton);

		if (!FirstButton)
		{
			FirstButton = LevelButton;
		}
	}

	if (FirstButton)
	{
		FirstButton->FocusButton();
	}
	else
	{
		BackButton->SetFocus();
	}
}
```

- **`UPanelWidget`** is the base class of every panel: Vertical Box, Wrap Box, Uniform Grid Panel,
  Scroll Box and the rest. Binding to the base type lets the designer choose the layout without a
  code change. `AddChild` and `ClearChildren` work on any panel. `AddChild` returns the generic
  slot (`UPanelSlot*`); cast it to `UVerticalBoxSlot` and so on to set padding in code.
- **Rebuilt on every Refresh.** The list could be built once in `NativeOnInitialized`, but
  rebuilding is cheap for a handful of buttons and stays correct when chapter 18 adds best scores
  that change between visits.
- **`CreateWidget(this, ...)`** with a widget as owner: the new widget gets the same owning player
  as `this`. (Chapter 15 used the player controller as owner; both are fine.)
- **Focus always goes somewhere.** If there are no levels, focus moves to Back. A panel that takes
  no focus would leave focus on a button in the now hidden main panel, and keyboard and gamepad
  players would be stuck.

## Focus and gamepad navigation

A mouse user points at what they want. Keyboard and gamepad users can't point: they move a
**focus** from widget to widget and press a key to activate it. Slate handles that for you as
long as one thing is true: **some focusable widget inside your menu has focus.**

- **Who has focus.** Slate tracks focus per *user*: each local player (each gamepad in
  split-screen) has its own focused widget. `UWidget::SetFocus()` focuses the widget for its owning
  player, one more reason to create widgets with a player as owner.
- **What can have focus.** Buttons, check boxes, sliders and text boxes have an **Is Focusable**
  option (on by default for buttons; leave it on). Texts, images and panels can't take focus.
- **Moving focus.** The arrow keys, Tab and Shift+Tab, the gamepad's D-pad and left stick move
  focus to the nearest focusable widget in that direction.
- **Activating.** Enter or Space, or the gamepad's bottom face button (A on Xbox), clicks the
  focused button. The same `OnClicked` fires as for a mouse click, so the menus need no extra code
  for keyboards and gamepads.
- **Seeing it.** The focused widget is drawn with a dashed outline. **Project Settings > Engine >
  User Interface > Render Focus Rule** controls when. The default, *Navigation Only*, shows it
  after the player navigates with keys or a gamepad, not after a mouse click. Many games also give
  buttons a **Style > Hovered** look; set one that differs from **Normal** so mouse users see what
  is under the cursor.

### Where each screen puts the focus

The one rule for every screen: whenever it appears, or its content changes, it moves focus to a
sensible widget.

| When | Code | Focused |
|---|---|---|
| Main menu appears | `UGSMainMenuWidget::NativeConstruct` → `ShowMainPanel` | Play |
| Back from a panel | `OnBackRequested` → `ShowMainPanel` | Play |
| Level select opens | `UGSLevelSelectWidget::Refresh` | First level button (or Back) |
| Options open | `UGSOptionsWidget::Refresh` | Fullscreen check box |
| Pause menu appears | `UGSPauseMenuWidget::NativeConstruct` | Resume |
| Level end appears | `UGSLevelEndWidget::Setup` | Next, or Retry after a crash |

If focus ends up on a hidden widget, or on nothing, the keyboard and gamepad do nothing until the
player clicks with the mouse. On a console that means a stuck player. Go through each screen with
the keyboard only before calling it done.

### Navigation rules

Every widget has a **Navigation** section in its Details panel with one rule per direction (Left,
Right, Up, Down, Next, Previous). The labels may differ slightly in your version, but the common
rules are:

- **Escape** (the default): focus may leave in that direction, to the nearest focusable widget.
- **Stop**: focus can't move that way.
- **Wrap**: focus wraps to the other side of this widget's children. Set **Up** and **Down** to
  Wrap on a vertical button list, and Down from the last button goes to the first.
- **Explicit**: focus goes to a named widget. **Custom** and **CustomBoundary**: a function
  decides.

The default spatial navigation suits GridSnake's simple columns of buttons. Wrap or Stop on a
dialog's root keeps focus from wandering out of the dialog to the menu behind it.

### Beyond UMG: Common UI

Unreal also ships **Common UI**, a plugin built on UMG for games with many layered menus and
several input devices. Its menu stacks manage input modes and focus for you, and its button
prompts switch between keyboard and gamepad icons. It automates exactly the problems in this
chapter (who has focus, which input mode, how Back works), but takes more setup and is beyond this
course. See the Common UI documentation in this chapter's sources.

## Pausing

Pausing a game in Unreal is one call on the player controller:

```cpp
// GameFramework/PlayerController.h
virtual bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause());
virtual bool IsPaused() const;

// Kismet/GameplayStatics.h, the same from anywhere with a world context
static bool SetGamePaused(const UObject* WorldContextObject, bool bPaused);
static bool IsGamePaused(const UObject* WorldContextObject);
```

`SetPause` doesn't pause by itself. It asks the **game mode**, which has the final say: the game
mode's `AllowPausing` must agree. By default it does in a single-player game (and in multiplayer
only if the game mode's `bPauseable` is set). If the game mode refuses, `SetPause` returns `false`
and nothing changes, so check the result. A paused world remembers which player paused it.
`IsPaused()` is true while any player has the world paused.

### What stops and what keeps running

Pausing pauses the **world**: game time stops. Things that run on game time stop; things that
run on real time, and the UI, keep going.

| Stops while paused | Keeps running |
|---|---|
| Actor and component `Tick` (unless marked to tick when paused) | Slate and UMG: widgets draw, tick, animate and take clicks |
| World timers (`GetWorldTimerManager()`), so the countdown freezes | Player controllers (they tick when paused by default) |
| Physics and movement | Enhanced Input actions with **Trigger when Paused** on |
| `GetWorld()->GetTimeSeconds()` (game time) | `GetWorld()->GetRealTimeSeconds()` (real time) |
| Enhanced Input actions without Trigger when Paused | The console, and the editor around PIE |

To make an actor keep ticking while paused, call `SetTickableWhenPaused(true)` on it (or set
`PrimaryActorTick.bTickEvenWhenPaused = true` in its constructor). That affects `Tick` only, not
timers. `GetWorld()->GetUnpausedTimeSeconds()` is a third clock: it keeps running during a pause,
like real time, but follows time dilation (`slomo`), like game time. Chapter 17 covers sound
during a pause.

The editor toolbar's **Pause** button during Play In Editor is a different thing: a debug pause
of the whole simulation, including things a game pause keeps running. To test your pause menu,
use the game's own pause key.

### The pause key: Trigger when Paused

The pause key is an Enhanced Input action, `IA_Pause`. Enhanced Input checks each action while
the world is paused: an action fires during a pause only if its **Trigger when Paused** option
is on. Steering doesn't have it, which is exactly right: no turning the snake while paused. But
`IA_Pause` needs it, or the game could pause and never unpause.

```
IA_Pause      Value Type: Digital (bool)      Trigger when Paused: on
IMC_Snake     IA_Pause ← Escape, P, Gamepad Special Right (the Start / Menu button)
```

> **Escape in Play In Editor.** In PIE, the editor's own shortcut for **Stop** is Escape, and the
> editor sees the key before the game does. Pressing Escape to pause ends the session instead.
> Test pausing with **P** or the gamepad's Start button. Alternatively, remap **Stop** in **Editor
> Preferences > General > Keyboard Shortcuts** (search for "Stop"; the label may differ slightly
> in your version), or test in **Standalone Game**. In a packaged game, Escape pauses as expected.

### GridSnake's pause

The pause binding lives on the **player controller**, not on the snake:

`Source/GridSnake/Core/GSPlayerController.h` (added) gains public `PauseGame()`, `ResumeGame()`
and `ShowLevelEnd(EGSLevelResult, int32 LevelScore, int32 RunScore)`; a `SetupInputComponent`
override; `EditDefaultsOnly` properties `UInputAction* PauseAction` (category Input) and
`PauseMenuWidgetClass` and `LevelEndWidgetClass` (category UI); `Transient` pointers to the two
created widgets; and private `HandlePausePressed()` and `EnterMenuInputMode(bool)`. It now
includes `Core/GSGameState.h` for `EGSLevelResult`.

`Source/GridSnake/Core/GSPlayerController.cpp` (added)

```cpp
#include "EnhancedInputComponent.h"
#include "UI/GSLevelEndWidget.h"
#include "UI/GSPauseMenuWidget.h"

void AGSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// The pause binding lives on the controller, not the snake: pausing is about
	// the player, not the pawn, and must work even when the snake is dead.
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AGSPlayerController::HandlePausePressed);
		}
	}
}

void AGSPlayerController::HandlePausePressed()
{
	if (IsPaused())
	{
		ResumeGame();
	}
	else
	{
		PauseGame();
	}
}

void AGSPlayerController::PauseGame()
{
	// Only a level in progress can be paused, not one that has just ended.
	const AGSGameState* State = GetWorld()->GetGameState<AGSGameState>();
	const bool bLevelRunning = State && (State->GetPhase() == EGSLevelPhase::Countdown || State->GetPhase() == EGSLevelPhase::Playing);
	if (!bLevelRunning || !PauseMenuWidgetClass || IsPaused())
	{
		return;
	}

	if (!SetPause(true))
	{
		return;
	}

	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UGSPauseMenuWidget>(this, PauseMenuWidgetClass);
	}
	EnterMenuInputMode(/*bGameKeepsInput=*/true);
	// Z-order 10 draws the menu above the HUD (0).
	PauseMenuWidget->AddToViewport(10);
}

void AGSPlayerController::ResumeGame()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
	}

	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}
```

- **`SetupInputComponent`** is the player controller's equivalent of the pawn's
  `SetupPlayerInputComponent` (chapter 8). The controller's input component is an
  `UEnhancedInputComponent` because the project's default input component class is Enhanced
  Input's (chapter 8). The mapping context `IMC_Snake`, added in `BeginPlay` since chapter 8,
  provides the keys for both the snake's and the controller's actions.
- **Why the controller?** Pausing is about the *player*, not the pawn. A binding on the snake
  would stop working whenever the player has no pawn or a different one, and it would be one more
  thing for a dead snake to get right. The controller is always there.
- **`ETriggerEvent::Started`** fires once per press. `Triggered` would fire every frame while
  the key is held and toggle the pause on and off.
- **`PauseGame` guards.** Pausing during the countdown or play is fine. After the level has
  ended, the level-end screen owns the input, and a pause menu on top of it would only confuse.
- **The menu widget is created once** and reused. That is why its buttons are bound in
  `NativeOnInitialized` (see the table earlier). Each `AddToViewport` runs `NativeConstruct`
  again, which focuses Resume.
- **`ResumeGame` is the mirror image:** off the screen, unpause, back to game-only input, cursor
  off. It's public because the pause menu's Resume button calls it too.

### The pause menu widget

`Source/GridSnake/UI/GSPauseMenuWidget.cpp` (new; the header binds three Buttons,
`ResumeButton`, `RestartButton` and `MainMenuButton`, and `NativeOnInitialized` binds their
`OnClicked` to the three handlers below)

```cpp
void UGSPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResumeButton->SetFocus();
}

void UGSPauseMenuWidget::HandleResumeClicked()
{
	if (AGSPlayerController* PlayerController = GetOwningPlayer<AGSPlayerController>())
	{
		PlayerController->ResumeGame();
	}
}

void UGSPauseMenuWidget::HandleRestartClicked()
{
	// The level stays paused; the subsystem skips its fade while paused.
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->RestartLevel();
}

void UGSPauseMenuWidget::HandleMainMenuClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->ReturnToMainMenu();
}
```

`GetOwningPlayer<AGSPlayerController>()` returns the owning player already cast to the given type,
or `nullptr` if it's another class.

### Leaving a level while paused

Restart Level and Main Menu leave the level **without unpausing**. Chapter 14's
`OpenLevelWithFade` fades the camera out and opens the level from a timer, and while paused a
timer-driven fade might never get to its end. The pause menu covers the level anyway, so the
subsystem skips the fade:

`Source/GridSnake/Core/GSLevelFlowSubsystem.cpp` (added to `OpenLevelWithFade`, after
`PendingMap = Map;`)

```cpp
	// The pause menu can restart or quit the level while the game is paused.
	// The pause menu already hides the level, so skip the fade and go now.
	if (UGameplayStatics::IsGamePaused(GetGameInstance()))
	{
		OpenPendingLevel();
		return;
	}
```

The new level starts **unpaused**: the pause belongs to the old world, which is destroyed. Its
player controller sets game-only input in `BeginPlay`, as shown earlier, so the mouse cursor goes
away and the snake steers again.

### Example: pause and the two clocks

`ExPauseToggle` makes the table above visible. A console command toggles the pause, and a
one-second world timer logs both clocks.

`examples/ch16/ExPauseToggle.h` declares `AExPauseToggleController : APlayerController` with
a public `UFUNCTION(Exec) void ExTogglePause();`, a `BeginPlay` override, a private `LogClocks()`,
an `FTimerHandle ClockTimer` and an `int32 TimerCount = 0`.

`examples/ch16/ExPauseToggle.cpp`

```cpp
// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExPauseToggle.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExPause, Log, All);

void AExPauseToggleController::BeginPlay()
{
	Super::BeginPlay();

	// A world timer runs on game time, which stands still while the game is paused.
	GetWorldTimerManager().SetTimer(ClockTimer, this, &AExPauseToggleController::LogClocks, 1.0f, /*bLoop=*/true);
}

void AExPauseToggleController::ExTogglePause()
{
	const bool bWantPaused = !IsPaused();

	// The game mode decides: SetPause returns false if it refuses.
	const bool bAccepted = SetPause(bWantPaused);

	UE_LOG(LogExPause, Log, TEXT("SetPause(%s) returned %s; IsGamePaused = %s"),
		bWantPaused ? TEXT("true") : TEXT("false"),
		bAccepted ? TEXT("true") : TEXT("false"),
		UGameplayStatics::IsGamePaused(this) ? TEXT("true") : TEXT("false"));
}

void AExPauseToggleController::LogClocks()
{
	++TimerCount;

	// GetTimeSeconds stops while paused; GetRealTimeSeconds never stops.
	const UWorld* World = GetWorld();
	UE_LOG(LogExPause, Log, TEXT("Timer %d: game time %.1f s, real time %.1f s"),
		TimerCount, World->GetTimeSeconds(), World->GetRealTimeSeconds());
}
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`.

To try it:

1. Copy both files into your module and build. (In a unity build, every example's log category
   needs its own name, which is why each example defines its own `LogEx...` category.)
2. Create a Blueprint Class with parent **Game Mode Base**, named `BP_ExPauseGameMode`, and set
   its **Player Controller Class** to `ExPauseToggleController` (a C++ class works here directly).
   In a test level, set **World Settings > GameMode Override** = `BP_ExPauseGameMode`.
3. Press **Play**. Wait three seconds, open the console and type `ExTogglePause`. Wait about five
   seconds, then type `ExTogglePause` again.

Expected Output Log (filter: `LogExPause`). The exact numbers depend on how fast you type; these
are illustrative:

```
LogExPause: Timer 1: game time 1.0 s, real time 1.0 s
LogExPause: Timer 2: game time 2.0 s, real time 2.0 s
LogExPause: Timer 3: game time 3.0 s, real time 3.0 s
LogExPause: SetPause(true) returned true; IsGamePaused = true
LogExPause: SetPause(false) returned true; IsGamePaused = false
LogExPause: Timer 4: game time 4.0 s, real time 9.4 s
LogExPause: Timer 5: game time 5.0 s, real time 10.4 s
```

- **No timer lines appear while paused.** The world timer manager doesn't advance during a pause.
- **After the pause, game time carries on from where it stopped**, while real time has moved on
  by the length of the pause. Timer 4 fires one *game* second after timer 3.
- The console still works while paused: it is not part of the world.
- `ExTogglePause` reads `IsPaused()` on the controller, which is true whoever paused the world.

### When arrow keys don't move the focus in the pause menu

Remember [how a key press travels](#how-a-key-press-travels): in Game and UI mode, a key the
focused button does not use goes to the **game** before Slate's navigation sees it. GridSnake's
arrow keys, D-pad and left stick are mapped to `IA_Steer` in `IMC_Snake`. Depending on your engine
version, the game's input may report such a mapped key as handled even though `IA_Steer` doesn't
fire while paused. Navigation then never happens: the mouse works in the pause menu, but arrow
keys and the D-pad don't move the focus.

Test it: pause with P, then press the down arrow. If focus moves from Resume to Restart Level,
nothing needs to change. If it doesn't, there are two good fixes:

- **Take the steering keys away while paused.** Enhanced Input mapping contexts can be added and
  removed at run time (chapter 8). Create `IMC_Menu` containing only `IA_Pause` (Escape, P,
  Gamepad Special Right). In `PauseGame`, remove `IMC_Snake` and add `IMC_Menu`; in
  `ResumeGame`, swap them back. While paused, only the pause keys reach the game.
- **Or use UI only mode for the pause menu**, so no key reaches the game, and let the menu close
  itself, as `ExConfirmDialog` does:

  ```cpp
  // In GSPlayerController.cpp, PauseGame: the game gets no input while paused.
  EnterMenuInputMode(/*bGameKeepsInput=*/false);

  // In GSPauseMenuWidget.h (protected), and add #include "InputCoreTypes.h" to the .cpp:
  virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

  // In GSPauseMenuWidget.cpp:
  FReply UGSPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
  {
  	const FKey Key = InKeyEvent.GetKey();
  	if (Key == EKeys::P || Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right)
  	{
  		HandleResumeClicked();
  		return FReply::Handled();
  	}
  	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
  }
  ```

  The cost is that the pause keys are now listed twice: in `IMC_Snake` (to pause) and in the
  widget (to resume).

The main menu and the level-end screen use UI only mode, so they are not affected.

## Level end and game over

Until now, GridSnake moved on by itself: 1.5 seconds after a crash the level restarted, and after
the exit the next level loaded. Now a **level-end screen** appears and the player chooses. One
widget, `W_LevelEnd`, serves three outcomes, described by a new enum next to the level phase:

`Source/GridSnake/Core/GSGameState.h` (added)

```cpp
/** How a level ended, for the level-end screen. */
UENUM(BlueprintType)
enum class EGSLevelResult : uint8
{
	Completed,
	Failed,
	/** The last level in the list was completed. */
	RunComplete
};
```

| Result | Title | Buttons (focused first) |
|---|---|---|
| `Completed` | Level Complete! | **Next Level**, Main Menu |
| `Failed` | You Crashed! | **Retry**, Main Menu |
| `RunComplete` | You Win! | **Play Again**, Main Menu |

### The game mode asks for the screen

The game mode's timers used to call `RestartLevel` and `LoadNextLevel`. Both functions are gone;
the timers now call one function with the result as an argument:

`Source/GridSnake/Core/GSGameMode.cpp` (changed)

```cpp
	// ... in OnExitReached:
	Snake->StopMoving();
	State->SetPhase(EGSLevelPhase::Completed);
	const bool bHasNextLevel = GetLevelFlow()->CompleteLevel(State->GetScore());

	// CreateUObject can carry extra arguments ("payload") for the bound function.
	const EGSLevelResult Result = bHasNextLevel ? EGSLevelResult::Completed : EGSLevelResult::RunComplete;
	GetWorldTimerManager().SetTimer(LevelEndTimer,
		FTimerDelegate::CreateUObject(this, &AGSGameMode::ShowLevelEnd, Result), LevelEndDelay, /*bLoop=*/false);
}

void AGSGameMode::HandleSnakeDied(AGSSnake* Snake)
{
	GetGSGameState()->SetPhase(EGSLevelPhase::Failed);
	GetWorldTimerManager().SetTimer(LevelEndTimer,
		FTimerDelegate::CreateUObject(this, &AGSGameMode::ShowLevelEnd, EGSLevelResult::Failed), LevelEndDelay, /*bLoop=*/false);
}

void AGSGameMode::ShowLevelEnd(EGSLevelResult Result)
{
	if (AGSPlayerController* PlayerController = Cast<AGSPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->ShowLevelEnd(Result, GetGSGameState()->GetScore(), GetLevelFlow()->GetRunScore());
	}
}
```

- **Timer payloads.** `SetTimer(Handle, this, &Function, ...)` from chapter 9 only calls functions
  with no parameters. `FTimerDelegate::CreateUObject(this, &Function, Args...)` builds a delegate
  that stores extra arguments and passes them when the timer fires. The bound function's
  parameters are the payload, in order.
- **`CompleteLevel` banks the score first** and returns whether another level follows, so the
  run score shown on the screen already includes this level.
- The header now includes `Core/GSGameState.h` instead of forward-declaring `AGSGameState`:
  `ShowLevelEnd` takes an `EGSLevelResult` by value, and an enum used like that must be fully
  known.
- The 1.5-second `LevelEndDelay` stays. It gives the player a moment to see the crash or the
  "Level complete!" banner from chapter 15 before the screen covers them.

### The controller shows it

`Source/GridSnake/Core/GSPlayerController.cpp` (added)

```cpp
void AGSPlayerController::ShowLevelEnd(EGSLevelResult Result, int32 LevelScore, int32 RunScore)
{
	if (!LevelEndWidgetClass)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s has no LevelEndWidgetClass"), *GetName());
		return;
	}

	EnterMenuInputMode(/*bGameKeepsInput=*/false);
	LevelEndWidget = CreateWidget<UGSLevelEndWidget>(this, LevelEndWidgetClass);
	LevelEndWidget->AddToViewport(10);
	LevelEndWidget->Setup(Result, LevelScore, RunScore);
}
```

The game is **not paused** here. The snake has stopped, so nothing is moving anyway, and UI only
input makes sure nothing reacts. Each level shows the screen at most once, so it is simply
created each time; the level change destroys it.

### The level-end widget

`Source/GridSnake/UI/GSLevelEndWidget.cpp` (new; the header binds `TitleText`, `ScoreText`,
`NextButton`, `NextButtonText`, `RetryButton` and `MainMenuButton`, and stores the `Result`;
`NativeOnInitialized` binds the three buttons as in the other menus)

```cpp
void UGSLevelEndWidget::Setup(EGSLevelResult InResult, int32 LevelScore, int32 RunScore)
{
	Result = InResult;

	switch (Result)
	{
	case EGSLevelResult::Completed:
		TitleText->SetText(LOCTEXT("Completed", "Level Complete!"));
		NextButtonText->SetText(LOCTEXT("NextLevel", "Next Level"));
		break;
	case EGSLevelResult::Failed:
		TitleText->SetText(LOCTEXT("Failed", "You Crashed!"));
		break;
	case EGSLevelResult::RunComplete:
		TitleText->SetText(LOCTEXT("RunComplete", "You Win!"));
		NextButtonText->SetText(LOCTEXT("PlayAgain", "Play Again"));
		break;
	}

	ScoreText->SetText(FText::Format(LOCTEXT("EndScore", "Level score {0}   Total {1}"), LevelScore, RunScore));

	// A completed level's score is already banked, so only a crash offers Retry.
	const bool bFailed = Result == EGSLevelResult::Failed;
	NextButton->SetVisibility(bFailed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	RetryButton->SetVisibility(bFailed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	(bFailed ? RetryButton : NextButton)->SetFocus();
}

void UGSLevelEndWidget::HandleNextClicked()
{
	UGSLevelFlowSubsystem* LevelFlow = GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
	if (Result == EGSLevelResult::RunComplete)
	{
		LevelFlow->StartRun(0);
	}
	else
	{
		LevelFlow->OpenNextLevel();
	}
}

void UGSLevelEndWidget::HandleRetryClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->RestartLevel();
}

void UGSLevelEndWidget::HandleMainMenuClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->ReturnToMainMenu();
}
```

- **One widget, three moods.** Texts and visibility set from data beat three nearly identical
  layouts. `Collapsed` (chapter 15) also removes the hidden button from navigation.
- **Focus goes in `Setup`, not `NativeConstruct`.** The right button depends on the result, which
  isn't known until `Setup`. `AddToViewport` comes first, so the button is already on screen.
- **No Retry after completing a level**, because `CompleteLevel` has already banked the level's
  score; replaying would count it twice.
- **The subsystem does the work.** Every button is one call into `UGSLevelFlowSubsystem`, so the
  fade, the loading screen and the run score stay in one place.

## Options and UGameUserSettings

Players expect to change at least the window mode and VSync. Unreal stores such per-machine
settings in a **`UGameUserSettings`** object. The engine creates one at startup, loads it from
`GameUserSettings.ini` in the project's `Saved/Config/<platform>` folder (for a packaged game,
in the user's local app data), and applies it.

`UGameUserSettings::GetGameUserSettings()` returns the engine's instance. It has a getter and a
setter per setting: `Get/SetFullscreenMode` (`EWindowMode::Fullscreen`, `WindowedFullscreen` or
`Windowed`), `IsVSyncEnabled`/`SetVSyncEnabled`, `SetScreenResolution(FIntPoint)`,
`SetFrameRateLimit(float)` and `SetOverallScalabilityLevel(int32)` (0 Low to 3 Epic, 4 Cinematic).

The setters only **store** a value. Nothing changes on screen until you apply it.
`ApplySettings` applies everything (resolution, window mode, VSync, scalability) and then saves
the file; there are also narrower `ApplyResolutionSettings` and `ApplyNonResolutionSettings`. The
`bCheckForCommandLineOverrides` argument lets command-line options such as `-windowed` win;
`false` means "the player's choice wins". Some versions might apply without saving, so if a
setting doesn't survive a restart, add a `SaveSettings()` call after `ApplySettings`.

`SetOverallScalabilityLevel` sets all the scalability groups (view distance, shadows, effects
and so on) at once. Each group is also a console variable, such as `sg.ShadowQuality`. The
Scalability reference in this chapter's sources lists them. A 2D game like GridSnake barely
notices, so the options screen leaves them out.

### GridSnake's options panel

`Source/GridSnake/UI/GSOptionsWidget.cpp` (new; the header derives from `UGSMenuPanelWidget`,
overrides `Refresh`, binds `FullscreenCheckBox` and `VSyncCheckBox`, and declares the two
handlers as `UFUNCTION()`s with a `bool` parameter)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSOptionsWidget.h"
#include "Components/CheckBox.h"
#include "GameFramework/GameUserSettings.h"

void UGSOptionsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	FullscreenCheckBox->OnCheckStateChanged.AddDynamic(this, &UGSOptionsWidget::HandleFullscreenChanged);
	VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &UGSOptionsWidget::HandleVSyncChanged);
}

void UGSOptionsWidget::Refresh()
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	// Showing the current values: if this fires a change event, it only writes the same value back.
	FullscreenCheckBox->SetIsChecked(Settings->GetFullscreenMode() != EWindowMode::Windowed);
	VSyncCheckBox->SetIsChecked(Settings->IsVSyncEnabled());
	FullscreenCheckBox->SetFocus();
}

void UGSOptionsWidget::HandleFullscreenChanged(bool bIsChecked)
{
	// Window mode changes have no effect in Play In Editor; try Standalone Game.
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	Settings->SetFullscreenMode(bIsChecked ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
	Settings->ApplySettings(/*bCheckForCommandLineOverrides=*/false);
}

void UGSOptionsWidget::HandleVSyncChanged(bool bIsChecked)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	Settings->SetVSyncEnabled(bIsChecked);
	Settings->ApplySettings(/*bCheckForCommandLineOverrides=*/false);
}
```

- **`Super::NativeOnInitialized()` first.** The base class binds the Back button there; skipping
  `Super` would silently break Back.
- **Refresh reads, the handlers write.** The panel never keeps its own copy of the settings, so it
  can't get out of step with the engine's.
- **`OnCheckStateChanged`** passes the new state as a `bool`, so the handlers take one. Whether
  `SetIsChecked` from code also fires the event has varied; the comment in `Refresh` notes that
  it doesn't matter here, because it would only apply the same value again.
- **Windowed fullscreen**, a borderless window the size of the screen, is the friendliest kind of
  fullscreen: switching to it is instant and alt-tab works. `EWindowMode::Fullscreen` is exclusive
  fullscreen.
- **Test in Standalone Game.** In Play In Editor the game draws into an editor viewport, so
  window mode changes do nothing. VSync changes are hard to see anywhere; compare the console
  command `stat fps` with your monitor's refresh rate.

Chapter 17 adds volume sliders to this panel.

## Quitting

`UGSMainMenuWidget::HandleQuitClicked` calls `UKismetSystemLibrary::QuitGame(WorldContext,
Player, QuitPreference, bIgnorePlatformRestrictions)` with `EQuitPreference::Quit` (the other value,
`Background`, sends the app to the background on mobile platforms). The last argument respects
platform rules: some platforms don't allow a game to quit itself, and with `false` the call
does nothing there. That is what their store requirements usually expect.

- **In Play In Editor** Quit ends the session, like pressing Stop. That is how you test it.
- **In a packaged game** it closes the window and the process ends normally, so the game
  instance's `Shutdown` runs and logs `GSGameInstance shutting down`.

Exercise 3 adds an "Are you sure?" question before quitting.

## Building the screens

The C++ is done. The editor side follows `project/ch16/ASSETS.md`. As before, a misnamed
`BindWidget` widget fails the Widget Blueprint's compile and names the property, so typos are
caught early.

### Input

1. Build the project from your IDE (new classes need a full build) and open the editor.
2. In `Content/GridSnake/Input`, right-click > **Input > Input Action**, name it `IA_Pause`. Set
   **Value Type** = **Digital (bool)** and tick **Trigger when Paused**.
3. Open `IMC_Snake` and add a mapping for `IA_Pause` with three keys: **Escape**, **P** and
   **Gamepad Special Right** (the Start or Menu button).

### The Widget Blueprints

Create six Widget Blueprints in `Content/GridSnake/UI`, each with its C++ parent: right-click >
**User Interface > Widget Blueprint**, then pick the parent under **All Classes**. If you created
one with the wrong parent, change it in **Graph > Class Settings > Parent Class**.

| Asset | Parent class |
|---|---|
| `W_LevelButton` | `GSLevelButtonWidget` |
| `W_LevelSelect` | `GSLevelSelectWidget` |
| `W_Options` | `GSOptionsWidget` |
| `W_MainMenu` | `GSMainMenuWidget` |
| `W_PauseMenu` | `GSPauseMenuWidget` |
| `W_LevelEnd` | `GSLevelEndWidget` |

Build them in this order, because `W_LevelSelect` uses `W_LevelButton` and `W_MainMenu` uses the
two panels. Bold-style names after a widget type are `BindWidget` names and must match exactly.
Quoted labels go in the button's child **Text** widget; those Text widgets need no special name,
except `NextButtonText`.

```
W_LevelButton                        (Designer toolbar: Fill Screen -> Desired)
[Button] Button
└─ [Vertical Box]
   ├─ [Text] NameText     "1. Level"
   └─ [Text] DetailText   "Best 0"     (optional, used from chapter 18)

W_LevelSelect                        (Class Defaults: UI > Level Button Class = W_LevelButton)
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

Notes on the layouts:

- **Fill Screen -> Desired.** In the Designer toolbar's screen-size drop-down, **Desired** previews
  a small widget like `W_LevelButton` at its own size. It only affects the preview.
- **The switcher.** Drag children onto `PanelSwitcher` in the Hierarchy as onto any panel; the
  first (`MainPanel`) is index 0. `W_LevelSelect` and `W_Options` appear in the Palette under
  **User Created** once compiled; drag each in and rename the instances `LevelSelectPanel` and
  `OptionsPanel`.
- **The darkened background.** An Image with no brush draws a plain rectangle in its **Color and
  Opacity**; in an Overlay, set its slot's alignments to Fill. It also blocks clicks to the HUD.
- **Buttons.** Keep **Is Focusable** ticked (the default). Give each button a **Style > Hovered**
  tint that differs from Normal. Chapter 17 adds a click sound to the same style.

### Blueprints, map and settings

4. Right-click > **Blueprint Class**, expand **All Classes**, pick `GSMenuPlayerController`, and
   name it `BP_GSMenuPlayerController` (in `Content/GridSnake/Blueprints`). In Class Defaults set
   **UI > Main Menu Widget Class** = `W_MainMenu`.
5. Do the same with parent `GSMenuGameMode`, named `BP_GSMenuGameMode`. Set **Classes > Player
   Controller Class** = `BP_GSMenuPlayerController`. Leave **Default Pawn Class** at None.
6. **File > New Level > Empty Level**, then save it as `Content/GridSnake/Maps/L_MainMenu`. Open
   **Window > World Settings** and set **Game Mode > GameMode Override** = `BP_GSMenuGameMode`.
   The level needs no actors: the menu draws over a black view.
7. Open `BP_GSPlayerController` and set **Input > Pause Action** = `IA_Pause`, **UI > Pause Menu
   Widget Class** = `W_PauseMenu` and **UI > Level End Widget Class** = `W_LevelEnd`.
8. In **Edit > Project Settings > Game > GridSnake**, set **Levels > Main Menu Map** =
   `L_MainMenu`. In **Project > Maps & Modes**, set **Game Default Map** = `L_MainMenu`. Keep
   **Editor Startup Map** on `L_Level01`, or switch it, as you prefer. (The project's `.ini` files
   already contain both values if you copied the snapshot's `Config` folder.)

### Trying it

Open `L_MainMenu` and press **Play**.

- The menu appears with the mouse cursor and a dark background. Press the down arrow: the dashed
  focus outline moves from Play to Select Level. Enter opens level select, with level 1 focused.
  **Back** returns with Play focused. A gamepad works the same with the D-pad and A.
- **Options** shows two check boxes ticked to match the current settings.
- **Play** fades into level 1. The cursor disappears and the snake steers with the keyboard: the
  game-only reset in `BeginPlay` is working.
- **P** during the countdown or play pauses. The countdown stops, the pause menu appears with
  Resume focused, and **P** again resumes.
- Pause and choose **Main Menu**: the menu appears at once, without a fade.
- Finish level 1: after 1.5 seconds, **Level Complete!** shows the level and total score, with
  **Next Level** focused. Crash into a wall: **You Crashed!** with **Retry**.
- From the menu, **Quit** ends the session.

Expected Output Log (filter: `LogGridSnake`) for: start in the menu, Play, collect the gems, reach
the exit. Lines from earlier chapters are shortened to `...`:

```
LogGridSnake: Level flow subsystem initialised
...
LogGridSnake: Level L_MainMenu started (index -1, run score 0)
LogGridSnake: Opening /Game/GridSnake/Maps/L_Level01.L_Level01
LogGridSnake: GameState ready: 0/3 gems
...
LogGridSnake: Level L_Level01 started (index 0, run score 0)
LogGridSnake: View target: GSBoard_0
LogGridSnake: Countdown: 3
...
LogGridSnake: All gems collected - the exit opens
...
LogGridSnake: Phase: EGSLevelPhase::Completed
LogGridSnake: Level 0 complete: +300, run score 300
```

- `index -1` is correct for the menu: `L_MainMenu` is not in the level list.
- Each new world logs `GameState ready` before `Level ... started`, because the game state is
  spawned with the game mode, before `StartPlay`. The board's name and the score depend on your
  level.
- Choosing **Main Menu** on the level-end screen adds
  `Opening /Game/GridSnake/Maps/L_MainMenu.L_MainMenu` and then
  `Level L_MainMenu started (index -1, run score 300)`. The run score is kept until the next
  `StartRun`.
- The menus themselves log nothing unless something is missing. Watch for these warnings:
  `BP_GSMenuPlayerController_C_0 has no MainMenuWidgetClass ...`,
  `... has no LevelEndWidgetClass`, and `Level select needs a level list and a LevelButtonClass`.

If something is wrong:

- **Play shows a level, not the menu.** You pressed Play in `L_Level01`. Open `L_MainMenu` first.
  Game Default Map only affects Standalone and packaged games.
- **Black screen, no menu.** Check `L_MainMenu`'s **GameMode Override**, the game mode's **Player
  Controller Class**, and the controller's **Main Menu Widget Class**, in that order. The warning
  above covers the last one.
- **The menu shows but keys do nothing until you click.** Focus is lost. Check the button names
  (`PlayButton` and so on) and that **Is Focusable** is on.
- **Play does nothing after returning to the menu.** `AGSMenuGameMode::StartPlay` isn't calling
  `NotifyLevelStarted`, or `L_MainMenu` isn't using `BP_GSMenuGameMode`.
- **The snake doesn't steer after Play.** The game-only reset is missing from
  `AGSPlayerController::BeginPlay`.
- **P does nothing.** `BP_GSPlayerController` has no **Pause Action**, or `IA_Pause` isn't in
  `IMC_Snake`. **P pauses but doesn't unpause:** tick **Trigger when Paused** on `IA_Pause`.
- **Escape stops PIE.** That is the editor's Stop shortcut; see [Pausing](#pausing).
- **Arrow keys don't move the focus in the pause menu** but do in the main menu: see
  [When arrow keys don't move the focus in the pause menu](#when-arrow-keys-dont-move-the-focus-in-the-pause-menu).

## Summary

- A **main menu is a level** with its own game mode (World Settings > GameMode Override). The
  menu's game mode spawns no pawn (`DefaultPawnClass = nullptr`, `PlayerCanRestart` false), and
  its player controller only creates the menu widget. **Game Default Map** makes the game boot
  into it.
- **Input modes** decide who gets input: `FInputModeGameOnly` (the game, focus on the viewport),
  `FInputModeUIOnly` (only widgets) and `FInputModeGameAndUI` (widgets first, unhandled keys to
  the game). Set the cursor with `bShowMouseCursor` alongside, and set `DoNotLock` for menus.
- **Order matters:** the input mode is applied later than `SetFocus`. Set the input mode first,
  then add the widget and focus inside it, or let `SetWidgetToFocus` do the focusing. The input
  mode outlives a level change, so each gameplay level resets it to game-only.
- A **Widget Switcher** shows one child at a time; switch with `SetActiveWidget`. Panels derived
  from a common base announce "back" through a **native delegate**, bound with `AddUObject`.
- Bind your own child widgets' events in **`NativeOnInitialized`** (once). Set focus in
  **`NativeConstruct`** (every time the widget appears) or whenever the content changes.
- Keyboard and gamepad navigation is built in, as long as a focusable widget has **focus**. Every
  screen must focus a sensible widget whenever it appears.
- **`SetPause(true)`** asks the game mode and returns whether it worked. A pause stops game time,
  ticks and world timers. Widgets keep working, and input actions fire only if **Trigger when
  Paused** is on. In PIE, Escape is the editor's Stop key.
- A timer can pass arguments with **`FTimerDelegate::CreateUObject(this, &Func, Payload)`**.
- **`UGameUserSettings`** holds per-machine video settings: set, then `ApplySettings(false)`, which
  writes `GameUserSettings.ini`. Window mode changes need Standalone Game to test.
- **`UKismetSystemLibrary::QuitGame`** quits; in PIE it ends the session.

## Exercises

1. **A Controls panel.** Add a fourth page to the main menu, reached by a **Controls** button
   between Options and Quit. It lists the keys (arrows or WASD to steer, P or Escape to pause)
   and has a Back button that returns to the main buttons with focus on Controls.

   <details>
   <summary>Solution</summary>

   The panel needs a Back button and must take focus when shown, so it derives from
   `UGSMenuPanelWidget`. The base's `Refresh` is empty, which would leave focus on the hidden
   main panel, so a tiny C++ subclass overrides it. `BackButton` is protected in the base, so the
   subclass can use it.

   `Source/GridSnake/UI/GSControlsWidget.h` declares `UCLASS(Abstract) class UGSControlsWidget :
   public UGSMenuPanelWidget` with one public member, `virtual void Refresh() override;`. The
   `.cpp` (including `Components/Button.h`):

   ```cpp
   void UGSControlsWidget::Refresh()
   {
   	// Nothing to fill in; just take the focus so the keyboard keeps working.
   	BackButton->SetFocus();
   }
   ```

   In `GSMainMenuWidget.h`, add two `BindWidget` properties, `TObjectPtr<UButton> ControlsButton`
   and `TObjectPtr<UGSMenuPanelWidget> ControlsPanel` (the base type is enough: the menu only
   shows it and listens for Back), plus a private `UFUNCTION() void HandleControlsClicked();` and
   `void ReturnFromControls();`.

   In `GSMainMenuWidget.cpp` (add `#include "UI/GSMenuPanelWidget.h"`), in
   `NativeOnInitialized`:

   ```cpp
   	ControlsButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleControlsClicked);
   	ControlsPanel->OnBackRequested.AddUObject(this, &UGSMainMenuWidget::ReturnFromControls);
   ```

   And the two functions:

   ```cpp
   void UGSMainMenuWidget::HandleControlsClicked()
   {
   	ShowPanel(ControlsPanel);
   }

   void UGSMainMenuWidget::ReturnFromControls()
   {
   	ShowMainPanel();
   	// Back where we came from, not on Play.
   	ControlsButton->SetFocus();
   }
   ```

   The later `SetFocus` simply replaces the one inside `ShowMainPanel`. In the editor, create
   `W_Controls` with parent `GSControlsWidget`: a Vertical Box with a title Text, a few lines of
   Text and a Button named `BackButton` ("Back"). In `W_MainMenu`, add a Button
   `ControlsButton` ("Controls") to `MainPanel` before `QuitButton`, and drag a `W_Controls`
   from **User Created** into `PanelSwitcher`, named `ControlsPanel`. Compile `W_MainMenu`; a
   missing name shows up as a compile error.
   </details>

2. **Pause when the window loses focus.** If the player alt-tabs away during a level, the game
   should pause itself.

   <details>
   <summary>Solution</summary>

   Slate knows when the application is activated or deactivated and offers a native multicast
   delegate for it, `FSlateApplication::OnApplicationActivationStateChanged()`, which passes a
   `bool` (true when active). Check the declaration in `Framework/Application/SlateApplication.h`
   for your version, as this is lower-level API than the rest of the chapter. GridSnake already
   depends on the `Slate` module (chapter 14's loading screen).

   In `GSPlayerController.h`, declare an `EndPlay(const EEndPlayReason::Type)` override and a
   private `void HandleAppActivationChanged(bool bIsActive);`. In `GSPlayerController.cpp` (add `#include "Framework/Application/SlateApplication.h"`), at
   the end of `BeginPlay`:

   ```cpp
   	if (IsLocalController() && FSlateApplication::IsInitialized())
   	{
   		FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &AGSPlayerController::HandleAppActivationChanged);
   	}
   ```

   And:

   ```cpp
   void AGSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
   {
   	// The Slate application outlives every level: unbind, or it keeps a dead entry.
   	if (FSlateApplication::IsInitialized())
   	{
   		FSlateApplication::Get().OnApplicationActivationStateChanged().RemoveAll(this);
   	}
   	Super::EndPlay(EndPlayReason);
   }

   void AGSPlayerController::HandleAppActivationChanged(bool bIsActive)
   {
   	if (!bIsActive)
   	{
   		PauseGame();   // already does nothing outside Countdown/Playing or when paused
   	}
   }
   ```

   Don't resume automatically when the window comes back: the player should choose. `PauseGame`'s
   guards make the handler safe to call at any time. Test in Standalone Game. In PIE the
   "application" is the whole editor, so the game pauses only when you switch to another program,
   not when you click elsewhere in the editor.
   </details>

3. **Confirm before quitting.** Clicking Quit should ask "Quit GridSnake?" using this chapter's
   `ExConfirmDialog`. Yes quits; No returns to the menu with focus on Quit.

   <details>
   <summary>Solution</summary>

   Copy `ExConfirmDialog.h/.cpp` into `Source/GridSnake/UI/` (the GridSnake module already has
   every module it needs). Create `W_QuitConfirm` with parent `ExConfirmDialog` and the layout from
   the example: a full-screen Image to darken the menu and block clicks to it, a Text `Message`,
   and Buttons `YesButton` and `NoButton`. On the dialog's root Overlay, set **Navigation** Up,
   Down, Left and Right to **Stop**, so the arrow keys can't move focus out of the dialog to the
   menu buttons behind it.

   In `GSMainMenuWidget.h`, forward-declare `class UExConfirmDialog;` and add
   `UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UExConfirmDialog> QuitConfirmClass;`
   (if unset, Quit quits at once), plus private `void HandleQuitConfirmed(bool bConfirmed);` and
   `void QuitNow();`.

   In `GSMainMenuWidget.cpp` (add `#include "UI/ExConfirmDialog.h"`, and
   `#define LOCTEXT_NAMESPACE "GridSnakeMenu"` after the includes with `#undef` at the end):

   ```cpp
   void UGSMainMenuWidget::HandleQuitClicked()
   {
   	if (!QuitConfirmClass)
   	{
   		QuitNow();
   		return;
   	}

   	UExConfirmDialog* Dialog = CreateWidget<UExConfirmDialog>(GetOwningPlayer(), QuitConfirmClass);
   	Dialog->SetMessage(LOCTEXT("QuitQuestion", "Quit GridSnake?"));
   	Dialog->OnResult.BindUObject(this, &UGSMainMenuWidget::HandleQuitConfirmed);
   	// Above the menu. Its NativeConstruct focuses No.
   	Dialog->AddToViewport(20);
   }

   void UGSMainMenuWidget::HandleQuitConfirmed(bool bConfirmed)
   {
   	if (bConfirmed)
   	{
   		QuitNow();
   	}
   	else
   	{
   		QuitButton->SetFocus();
   	}
   }

   void UGSMainMenuWidget::QuitNow()
   {
   	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
   }
   ```

   Set **UI > Quit Confirm Class** = `W_QuitConfirm` in `W_MainMenu`'s Class Defaults.

   - The input mode is already UI only, so there is nothing to switch. `SetMessage` works before
     `AddToViewport` because `CreateWidget` has already bound `Message`.
   - The dialog removes itself before calling `OnResult`, so the menu's focus call lands on a
     visible button.
   - Nobody stores the dialog. While it's on the viewport, the viewport keeps it alive; once
     removed, the garbage collector may take it, and `BindUObject`'s weak reference keeps the
     other direction safe too.
   </details>

4. **The countdown and the pause.** Check what happens to the "3, 2, 1" countdown when you pause
   during it. Then explain what you would change if a countdown had to keep running during a
   pause (a real-time match clock, say).

   <details>
   <summary>Solution</summary>

   Start a level, press **P** while the banner shows `2`, wait five seconds, and resume. The
   countdown continues at `2` (or `1`, depending on how far into the second you paused), and the
   snake starts one or two seconds later. The countdown runs on a **world timer**
   (`GetWorldTimerManager().SetTimer(CountdownTimer, ...)` in `AGSGameMode::StartPlay`), and world
   timers only advance with game time, which stands still during a pause. Nothing needs to change.

   World timers have no "run while paused" switch. A clock that must keep going is better driven
   by `Tick` with the actor marked tickable when paused, measuring real time:

   ```cpp
   // In the constructor:
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bTickEvenWhenPaused = true;   // or SetTickableWhenPaused(true) at runtime

   void AMyClock::Tick(float DeltaSeconds)
   {
   	Super::Tick(DeltaSeconds);
   	// GetRealTimeSeconds keeps running during a pause; GetTimeSeconds does not.
   	const double Elapsed = GetWorld()->GetRealTimeSeconds() - StartRealTime;
   	// ... update the display from Elapsed ...
   }
   ```

   `SetTickableWhenPaused` affects only `Tick`. If `AGSGameMode` called it, its timers would still
   freeze, so the GridSnake countdown would behave exactly as before.
   </details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-16--menus-pause-and-input-modes)
