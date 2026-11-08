# Chapter 15 — A HUD with UMG

GridSnake has levels, gems, a score and a countdown, but the player only sees them in the Output
Log. This chapter puts them on screen with **UMG**, Unreal's UI framework. In UMG you draw a
layout in a visual designer and save it as a **Widget Blueprint**, and a C++ class behind it holds
the logic. You will learn how UMG relates to Slate, the lower-level framework it is built on. You
will write a C++ base class whose properties are filled in from the designer by name. You will
create the widget from the player controller, and keep it up to date by listening to game events
instead of checking every frame. GridSnake gains `W_HUD`, which shows:

- the level name, the gems as text (`Gems 1/3`) and as a row of icons, and the score;
- a big centred banner for the countdown and for the "Level complete!" and "Ouch!" messages.

Previous: [Chapter 14](14-level-transitions-and-persistence.md). Next:
[Chapter 16](16-menus-pause-and-input-modes.md) (menus, pause and input modes).

## UMG and Slate

Unreal has two UI layers, one built on the other.

- **Slate** is the C++ UI framework the editor itself is drawn with. Its widgets (`STextBlock`,
  `SButton`, ...) are plain C++ objects held by shared pointers, not `UObject`s: no reflection,
  no Blueprint access, no designer. Chapter 14's loading screen was built from Slate widgets.
- **UMG** (Unreal Motion Graphics) wraps Slate widgets in `UObject`s. Each UMG widget is a
  `UWidget` subclass, such as `UTextBlock`, `UButton` or `UImage`. It stores its settings as
  `UPROPERTY`s, so they can be saved in an asset, edited in the designer, and read and written
  from Blueprints and C++. When the widget is shown, it creates the matching Slate widget behind
  the scenes and passes its settings on.

```
UMG (UObjects, assets, designer)          Slate (plain C++, what is actually drawn)
────────────────────────────────          ────────────────────────────────────────
UTextBlock "GemsText"           ──creates──►  STextBlock
UHorizontalBox                  ──creates──►  SHorizontalBox
UUserWidget W_HUD               ──creates──►  the Slate tree for the whole layout
```

For game UI you work almost entirely in UMG. You only touch Slate types for small value types
such as `FMargin` (padding), `FSlateBrush` (how to draw an image) and the alignment enums, and
for tools like the Widget Reflector at the end of this chapter.

### Widget Blueprints

A **Widget Blueprint** is a Blueprint asset whose class derives from `UUserWidget`. Its editor has
two modes, switched with the **Designer** and **Graph** buttons at the top right:

- The **Designer** is a visual layout tool: the **Palette** lists the widgets you can add, the
  **Hierarchy** shows the tree you have built, **Details** edits the selected widget, and the
  canvas in the middle previews the result.
- The **Graph** is a normal Blueprint event graph. GridSnake leaves it empty: logic is in C++.

UMG widgets come in two kinds:

- **Panels** hold children and decide where they go: Canvas Panel (free placement with anchors),
  Horizontal Box and Vertical Box (a row or column), Overlay (stacked on top of each other), Grid
  Panel, Size Box, Widget Switcher (next chapter) and others.
- **Leaf widgets** draw something or react to input: Text, Image, Button, Check Box, Slider,
  Progress Bar, Spacer.

A child's layout settings (its padding, alignment, anchors) are not stored on the child itself.
They live on a **slot** object that the parent creates for each child: a `UCanvasPanelSlot` in a
Canvas Panel, a `UHorizontalBoxSlot` in a Horizontal Box, and so on. In the Details panel you see
them in a section called **Slot (Canvas Panel Slot)** or similar. The same split shows up in C++:
adding a child to a panel returns its slot, and you set padding on the slot.

### The UMG module

The UMG classes live in the `UMG` engine module. GridSnake's `GridSnake.Build.cs` adds it to the
**public** dependencies, because `UI/GSHUDWidget.h` derives from `UUserWidget`. A header that
names a module's types needs that module in the public list (chapter 2):

```diff
 			"DeveloperSettings",
-			"Paper2D"
+			"Paper2D",
+			"UMG"
 		});

-		// Used only inside our .cpp files: the loading screen (MoviePlayer) and the
-		// Slate widget types it is built from.
+		// Used only inside our .cpp files: the loading screen (MoviePlayer) and
+		// Slate, the UI framework that UMG and the loading screen are built on.
 		PrivateDependencyModuleNames.AddRange(new string[] {
 			"MoviePlayer",
 			"Slate",
```

Only `"UMG"` and the comment are new. `Slate` and `SlateCore` were already private dependencies
because of chapter 14's loading screen. The HUD uses Slate value types (`FMargin`, `FSlateBrush`,
alignment enums) only in its `.cpp`, so private is enough for them.

Forget `"UMG"` and the build fails at `#include "Blueprint/UserWidget.h"`: a module's headers are
only on the include path of modules that depend on it.

## A C++ base class for every widget

A Widget Blueprint is a class, and like any Blueprint class it can have a C++ parent. The pattern
used throughout this course (and in most C++ Unreal projects) is:

- The **C++ class** derives from `UUserWidget`. It declares the widgets it needs as properties,
  and contains all the logic.
- The **Widget Blueprint** is created with that class as its parent. It provides the layout, the
  fonts, colours and sizes. A designer can change all of these without touching code.

The connection between the two is the `BindWidget` meta tag. Here is the smallest useful example.

`examples/ch15/ExCounterWidget.h`

```cpp
// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExCounterWidget.generated.h"

class UTextBlock;

/**
 * The C++ parent of the Widget Blueprint W_ExCounter. The blueprint owns the
 * layout; this class owns the logic. W_ExCounter must contain a Text widget
 * named exactly CountText, or it will not compile.
 */
UCLASS(Abstract)
class UExCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the value and shows it. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	void SetCount(int32 NewCount);

	UFUNCTION(BlueprintPure, Category = "Example")
	int32 GetCount() const { return Count; }

protected:
	virtual void NativeConstruct() override;

	/** Filled in by UMG with the Text widget of the same name in W_ExCounter. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

private:
	int32 Count = 0;
};
```

`examples/ch15/ExCounterWidget.cpp`

```cpp
// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG.

#include "ExCounterWidget.h"
#include "Components/TextBlock.h"

void UExCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Replace the designer's preview text with the real value.
	SetCount(Count);
}

void UExCounterWidget::SetCount(int32 NewCount)
{
	Count = NewCount;

	// AsNumber formats for the current culture: 12345 shows as "12,345" in English.
	CountText->SetText(FText::AsNumber(Count));
}
```

There are four things to notice.

1. **`UCLASS(Abstract)`.** This class has no layout of its own, so creating it directly would give
   an empty widget whose `CountText` is null. `Abstract` stops that: `CreateWidget` refuses an
   abstract class, and the class is not offered in places that create instances. You can still
   pick it as the *parent* of a Widget Blueprint, and that is the only way it is meant to be used.
2. **`UPROPERTY(meta = (BindWidget))`.** When the widget is created, UMG looks in the Widget
   Blueprint's tree for a widget with **exactly the same name** as the property (`CountText`) and
   a compatible type (a `UTextBlock` or a subclass), and stores a pointer to it in the property.
   The name match is case-sensitive and includes no prefix or suffix. `Count Text`, `countText`
   and `CountText1` do not match.
3. **`NativeConstruct`.** This is the C++ version of the Blueprint **Event Construct**. It runs
   when the widget is added to the screen, after the bindings are filled in. That makes it the
   place to draw initial values.
4. **The `.cpp` includes `Components/TextBlock.h`.** The header only forward-declares
   `UTextBlock`, which is enough for a pointer. Calling `SetText` needs the full class. UMG's
   built-in widgets live under `Components/` (`Components/Image.h`, `Components/Button.h`,
   `Components/HorizontalBox.h`, and so on).

### What happens when a name is missing

`BindWidget` is checked when the **Widget Blueprint compiles**, not when the game runs. If
`W_ExCounter` has no widget called `CountText`, or has one of the wrong type, the blueprint fails
to compile. The **Compiler Results** panel at the bottom of the Widget Blueprint editor shows an
error that names the property and its type. It reads roughly like this (the exact wording varies
between engine versions):

```
A required widget binding "CountText" of type Text was not found.
```

That is a feature. A misspelled name is caught the moment you press **Compile**, instead of
crashing on a null pointer at runtime. It also means that if you rename a widget in the designer,
or rename the property in C++, the blueprint tells you on its next compile.

A Widget Blueprint that does not compile cannot be used, and Play In Editor will show the
"blueprint has errors" warning. Fix the name and press **Compile** again.

### Optional bindings

Sometimes a widget is nice to have but not essential, so that a designer can leave it out. For
that use `BindWidgetOptional`:

```cpp
UPROPERTY(meta = (BindWidgetOptional))
TObjectPtr<UHorizontalBox> GemIconBox;
```

If the layout has a `GemIconBox`, it is bound. If not, the blueprint still compiles and the
property stays null, so **every use must check it** (`if (GemIconBox) { ... }`). Keep the list
of names in a comment or a document (GridSnake's `ASSETS.md` does this), because a misspelled
optional widget is silently ignored rather than reported. (The **Is Variable** check box next to a
widget's name only matters for Blueprint graphs; bound widgets don't need it.)

### The widget's lifetime functions

`UUserWidget` has several virtual functions that run at different points in its life. They are
the C++ versions of the Blueprint events with similar names.

| Function | When it runs | Typical use |
|---|---|---|
| `NativeOnInitialized` | Once, after the widget and its tree are created (bindings are valid). | Bind button clicks (chapter 16). |
| `NativePreConstruct` | Before construct, **also in the designer preview**. | Apply style properties so the designer shows them. |
| `NativeConstruct` | Each time the widget's Slate widgets are built, which is normally when it is added to the screen. | Draw initial values, subscribe to game events. |
| `NativeTick` | Every frame while the widget is on screen. | Animations or smooth values (exercise 3). |
| `NativeDestruct` | When the widget is removed from the screen, or the level ends. | Unsubscribe from game events. |

`NativeConstruct` and `NativeDestruct` come in pairs and can run more than once for the same
widget object, if you remove it and add it again. Anything you set up in one should be torn down
in the other. Always call `Super::` in each override.

### Trying the counter widget

To see the widget you need a Widget Blueprint with this class as parent:

1. Add `"UMG"` to your module's `PublicDependencyModuleNames` in its `Build.cs`.
2. Copy `ExCounterWidget.h` and `ExCounterWidget.cpp` into your module's source folder, close the
   editor and build from your IDE (a new class needs a full build, as chapter 2 explained).
3. Open the editor. In the Content Browser, right-click in a folder and choose **User Interface >
   Widget Blueprint**.
4. The **Pick Parent Class for New Widget Blueprint** dialog opens. Expand **All Classes**, type
   `ExCounterWidget` in the search box, select it and press **Select**. Name the asset
   `W_ExCounter`.
5. Double-click `W_ExCounter` to open it. A new Widget Blueprint in UE 5 starts empty, with no
   root panel. From the **Palette**, drag a **Canvas Panel** into the **Hierarchy**.
6. Drag a **Text** widget from the Palette onto the Canvas Panel in the Hierarchy.
7. With the Text selected, at the top of the **Details** panel, rename it from `TextBlock` to
   `CountText`. In **Appearance > Font**, set **Size** to 48.
8. Press **Compile**. It compiles cleanly. Rename the Text to something else and compile again to
   see the binding error, then rename it back.
9. Press **Save**.

The next example puts it on screen.

## Creating the HUD

Creating a widget at runtime takes two calls:

```cpp
// Blueprint/UserWidget.h
template <typename WidgetT = UUserWidget, typename OwnerType = UObject>
WidgetT* CreateWidget(OwnerType OwningObject, TSubclassOf<UUserWidget> UserWidgetClass = WidgetT::StaticClass(),
    FName WidgetName = NAME_None);

// UUserWidget
void AddToViewport(int32 ZOrder = 0);
void RemoveFromParent();   // declared on UWidget
```

- **`CreateWidget`** creates an instance of a widget class. The class is a `TSubclassOf` because
  you want the Widget Blueprint (`W_HUD`), not the abstract C++ parent. The owner can be a
  player controller, another widget, a world or a game instance. Use the **player controller**
  for anything that belongs to a player: the widget then knows its *owning player*
  (`GetOwningPlayer()`), goes on that player's part of the screen in split-screen, and gets that
  player's input.
- **`AddToViewport`** puts the widget on the game's viewport, on top of the world. `ZOrder`
  orders widgets that are both on the viewport: higher numbers draw on top. GridSnake's HUD uses
  the default 0, and the menus of chapter 16 use 10 so they cover the HUD.
- **`RemoveFromParent`** takes it off the screen again. The widget object still exists, and you
  can add it again later.

When the level changes, the engine removes the old world's widgets from the viewport for you.
Widgets don't survive a level change unless you add them again.

### Example: showing a widget from an actor

`examples/ch15/ExWidgetSpawner.h`

```cpp
// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExWidgetSpawner.generated.h"

class UExCounterWidget;

/**
 * Shows a counter widget for the first player when play starts and adds one
 * to it every second. Place it in any level and set Widget Class to W_ExCounter.
 */
UCLASS()
class AExWidgetSpawner : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** A Widget Blueprint whose parent class is ExCounterWidget. */
	UPROPERTY(EditAnywhere, Category = "Example")
	TSubclassOf<UExCounterWidget> WidgetClass;

private:
	void AddOne();

	/** UPROPERTY keeps the widget alive while we point at it. */
	UPROPERTY(Transient)
	TObjectPtr<UExCounterWidget> Widget;

	FTimerHandle CountTimer;
};
```

`examples/ch15/ExWidgetSpawner.cpp`

```cpp
// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG.

#include "ExWidgetSpawner.h"
#include "ExCounterWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExSpawner, Log, All);

void AExWidgetSpawner::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!WidgetClass || !PlayerController)
	{
		UE_LOG(LogExSpawner, Warning, TEXT("%s: set Widget Class (and play with a player)"), *GetName());
		return;
	}

	// The owning player decides whose screen the widget is on and who gives it input.
	Widget = CreateWidget<UExCounterWidget>(PlayerController, WidgetClass);
	Widget->AddToViewport();
	UE_LOG(LogExSpawner, Log, TEXT("Widget shown: %s"), *Widget->GetName());

	GetWorldTimerManager().SetTimer(CountTimer, this, &AExWidgetSpawner::AddOne, 1.0f, /*bLoop=*/true);
}

void AExWidgetSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(CountTimer);
	if (Widget)
	{
		Widget->RemoveFromParent();
	}

	Super::EndPlay(EndPlayReason);
}

void AExWidgetSpawner::AddOne()
{
	Widget->SetCount(Widget->GetCount() + 1);
	UE_LOG(LogExSpawner, Log, TEXT("Count: %d"), Widget->GetCount());
}
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`, `UMG`.

To see it, after creating `W_ExCounter` as described in the previous section:

1. Copy both files next to the counter widget's files and build.
2. Find `ExWidgetSpawner` in the **Place Actors** panel (search for it), or in the Content
   Browser's **C++ Classes** folder, and drag it into any level.
3. In its **Details** panel, set **Example > Widget Class** to `W_ExCounter`. The drop-down only
   lists classes derived from `ExCounterWidget`, because the property is a
   `TSubclassOf<UExCounterWidget>`.
4. Press **Play**. A big `0` appears in the top-left corner of the viewport and counts up once
   per second.

Expected Output Log (filter: `LogExSpawner`):

```
LogExSpawner: Widget shown: W_ExCounter_C_0
LogExSpawner: Count: 1
LogExSpawner: Count: 2
LogExSpawner: Count: 3
```

- The name `W_ExCounter_C_0` tells you the object is an instance of the class `W_ExCounter_C`,
  the class generated from the `W_ExCounter` Blueprint (every Blueprint class has the `_C`
  suffix). The number at the end may differ. If you see the warning instead, set **Widget Class**.
- `Widget` is a `UPROPERTY`, so the garbage collector knows the actor uses it (chapter 3). A
  widget you remove and plan to add again needs a reference like this.
- Notice what is **not** in the code: no "update every frame". The text changes only when
  `SetCount` is called. That idea is the heart of the HUD's design.

### GridSnake: the player controller creates the HUD

GridSnake creates its HUD in `AGSPlayerController::BeginPlay`, after the code from chapters 8 and
7 that adds the input mapping context and sets the view target. Here is the change to the
header:

`Source/GridSnake/Core/GSPlayerController.h` (added)

```cpp
class UGSHUDWidget;

	// ... inside the class, after DefaultMappingContext:

	/** The HUD layout to show while playing (W_HUD). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGSHUDWidget> HUDWidget;
```

And the end of `BeginPlay`:

`Source/GridSnake/Core/GSPlayerController.cpp` (added)

```cpp
#include "UI/GSHUDWidget.h"

	// ... at the end of AGSPlayerController::BeginPlay:

	// Widgets are owned by a player: CreateWidget with the controller as owner.
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UGSHUDWidget>(this, HUDWidgetClass);
		HUDWidget->AddToViewport();
	}
```

- `HUDWidgetClass` is `EditDefaultsOnly`. You set it once, in `BP_GSPlayerController`'s Class
  Defaults, not per placed instance (there are no placed player controllers).
- `HUDWidget` is `Transient`: it is a runtime pointer, never saved.
- The `if` makes the HUD optional. A test level whose controller has no HUD class set just has no
  HUD. The downside is that forgetting to set it is silent, which is the first thing to check when
  the HUD does not appear.

Why the player controller? It exists for the whole level, belongs to one player, and survives
the snake dying. Unreal also has an `AHUD` class (the GameMode's **HUD Class**), an older
canvas-drawing system that some projects create widgets from. Either works; GridSnake keeps all
per-player UI in its controller, next to chapter 16's pause menu.

## Designing W_HUD

`W_HUD` is the Widget Blueprint whose parent class is `UGSHUDWidget` (the full class is in the
next section). It has this hierarchy. Names in **bold** must match the C++ properties exactly.

```
[Canvas Panel]
├─ [Horizontal Box]  anchored top-left, offset (24, 16), Size To Content
│  ├─ [Text] LevelText        "Level"       font size 24
│  ├─ [Spacer] 32 x 1
│  ├─ [Text] GemsText         "Gems 0/3"    font size 24
│  ├─ [Horizontal Box] GemIconBox  (optional; filled with icons by C++)
│  ├─ [Spacer] 32 x 1
│  └─ [Text] ScoreText        "Score 0"     font size 24
└─ [Text] BannerText          anchored centre, Alignment (0.5, 0.5), Size To Content,
                              font size 48, Justification Center, text empty
```

The bold names are **LevelText**, **GemsText**, **ScoreText**, **BannerText** (required) and
**GemIconBox** (optional). The text in quotes is design-time preview text that C++ overwrites.

### Anchors, offsets and alignment

A Canvas Panel places each child with three settings in its **Slot (Canvas Panel Slot)**
section:

- **Anchors** pick a point (or a stretch area) of the parent that the child is attached to.
  Top-left is (0, 0), the centre is (0.5, 0.5), bottom-right is (1, 1). The **Anchors** drop-down
  shows a grid of presets. When the screen size changes, the child stays at the same distance
  from its anchor. A HUD anchored top-left stays top-left on every screen.
- **Position X / Position Y** are the offset from the anchor, in UMG units (pixels at the
  reference resolution, see DPI scaling below).
- **Alignment** says which point *of the child* sits at that position. (0, 0) is the child's
  top-left corner, (0.5, 0.5) its centre. A centre anchor with a (0, 0) alignment would put the
  banner's top-left corner at the screen centre, which looks off-centre. Alignment (0.5, 0.5)
  centres it properly.
- **Size To Content** makes the slot as big as the child wants to be, instead of a fixed size.
  Text that changes length ("Gems 0/3", "Level complete!") needs this, or it gets clipped or
  wrapped.

### Building the layout

1. Build the project first. `GridSnake.Build.cs` now depends on `UMG`, and `UGSHUDWidget` must
   exist before you can pick it as a parent class.
2. In the Content Browser, create the folder `Content/GridSnake/UI`. Right-click inside it and
   choose **User Interface > Widget Blueprint**.
3. In **Pick Parent Class for New Widget Blueprint**, expand **All Classes**, search for
   `GSHUDWidget`, select it, press **Select**, and name the asset `W_HUD`. Open it.
4. The Compiler Results already complain: the four required bindings are missing. That is
   expected while the layout is empty.
5. From the **Palette**, drag a **Canvas Panel** into the **Hierarchy**. It becomes the root.
6. Drag a **Horizontal Box** onto the Canvas Panel. In its Details:
   - **Slot (Canvas Panel Slot) > Anchors**: the top-left preset.
   - **Position X** 24, **Position Y** 16.
   - Tick **Size To Content**.
7. Drag these into the Horizontal Box, in order. A small horizontal line in the Hierarchy shows
   where a dropped widget will go, so watch it to get the order right:
   - **Text**: rename it `LevelText`. Set **Content > Text** to `Level` and **Appearance > Font >
     Size** to 24.
   - **Spacer**: set **Size** to X 32, Y 1.
   - **Text**: rename it `GemsText`, text `Gems 0/3`, size 24.
   - **Horizontal Box**: rename it `GemIconBox`. Leave it empty: C++ fills it.
   - **Spacer**: size 32 × 1.
   - **Text**: rename it `ScoreText`, text `Score 0`, size 24.
8. Drag one more **Text** onto the **Canvas Panel** itself, not into the box. Rename it
   `BannerText`. In its Details:
   - **Anchors**: the centre preset.
   - **Position X** 0, **Position Y** 0.
   - **Alignment** X 0.5, Y 0.5.
   - Tick **Size To Content**.
   - **Appearance > Font > Size** 48, and **Appearance > Justification**: the "align text
     center" button.
   - Clear **Content > Text**, so the banner is empty until C++ fills it.
9. Press **Compile**. The binding errors are gone. If one remains, compare the widget names with
   the list above character by character.
10. Switch to **Graph** and click **Class Defaults** in the toolbar. The Details panel shows the
    C++ properties in a **HUD** category:
    - **Gem Icon Sprite** = `S_Gem_Idle_0` (the gem sprite from chapter 4).
    - **Gem Icon Size** = (32, 32).
    - **Missing Color**: a dark grey with alpha 0.6, for example (0.1, 0.1, 0.1, 0.6), which is
      also the C++ default.
11. **Compile** and **Save**.

A screenshot of the finished Hierarchy panel next to the designer canvas would help here. Compare
your Hierarchy with the tree above: the indentation shows which widgets are inside which.

The text uses UMG's default font, Roboto. For a pixel font, drag a `.ttf` file into the Content
Browser. Unreal creates a **Font Face** and a **Font** asset from it, and you pick the Font in
**Appearance > Font > Font Family** (the label may differ slightly in your version).

### Screen sizes and DPI scaling

UMG layouts are designed at a reference size and scaled to the real screen. The scale comes from
the **DPI Scale Curve** in **Project Settings > Engine > User Interface**. By default it scales by
the shortest side of the viewport, with a scale of 1.0 at 1080 pixels. On a 4K screen (2160
pixels tall) the HUD is drawn twice as big in pixels, so it looks the same.

In the designer, the **Screen Size** drop-down at the top right of the canvas previews common
resolutions, and the DPI scale for each is shown in the corner of the canvas. Check that the HUD
still fits on a small screen, and that nothing anchored to the centre drifts.

Epic's UMG best-practice guide recommends avoiding nested Canvas Panels: a canvas is flexible
but costlier to lay out than a box. One canvas at the root, with boxes inside, is the usual shape.

## Event-driven UI

The HUD needs to show values that live in `AGSGameState` (chapter 11): gems collected and
required, the score, the phase and the countdown. There are three ways to keep a widget in sync
with game data.

| Approach | How | Cost | Verdict |
|---|---|---|---|
| Property binding | In the designer, the **Bind** drop-down next to a property (such as a Text's **Text**) calls a function to get the value. | Called **every frame**, for every bound property, whether the value changed or not. | Convenient for prototypes, wasteful and hard to find later. |
| Polling in `NativeTick` | Read the game state every frame and set the text. | Same as above, but at least in one visible place. | Only for values that really change every frame. |
| **Events** | The game state broadcasts a delegate when a value changes. The widget subscribes and updates only then. | Zero while nothing happens. | GridSnake's choice. |

The game state already broadcasts the events. Chapter 11 declared them as dynamic multicast
delegates so both Blueprints and C++ can subscribe:

```cpp
// Core/GSGameState.h (since chapter 11)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSGemsChangedSignature, int32, Collected, int32, Required);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSScoreChangedSignature, int32, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSPhaseChangedSignature, EGSLevelPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSCountdownChangedSignature, int32, SecondsLeft);
```

Events have a second advantage: the game code does not know the HUD exists. The game state
announces "the score changed" and anyone may listen: the HUD now, a sound in chapter 17, an
achievement system later. Nothing in the game code changes when the UI does.

### The HUD class

`Source/GridSnake/UI/GSHUDWidget.h` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/GSGameState.h"
#include "GSHUDWidget.generated.h"

class UHorizontalBox;
class UImage;
class UPaperSprite;
class UTextBlock;

/**
 * The in-level HUD. Layout lives in the Widget Blueprint W_HUD (whose parent
 * class is this); logic lives here. Widgets marked BindWidget must exist in
 * W_HUD with exactly the same name, or the blueprint will not compile.
 */
UCLASS(Abstract)
class UGSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** "Gems 1/3". */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GemsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;

	/** Big centred text: the countdown, then level messages. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BannerText;

	/** Optional row of gem icons, one per required gem, filled in from C++. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> GemIconBox;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TObjectPtr<UPaperSprite> GemIconSprite;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FVector2D GemIconSize = FVector2D(32.0, 32.0);

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FLinearColor CollectedColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FLinearColor MissingColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.6f);

private:
	// Delegate handlers must be UFUNCTIONs to bind to dynamic delegates.
	UFUNCTION()
	void HandleGemsChanged(int32 Collected, int32 Required);

	UFUNCTION()
	void HandleScoreChanged(int32 Score);

	UFUNCTION()
	void HandlePhaseChanged(EGSLevelPhase Phase);

	UFUNCTION()
	void HandleCountdownChanged(int32 SecondsLeft);

	void BuildGemIcons(int32 Required);
	void SetBanner(const FText& Text);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> GemIcons;

	TWeakObjectPtr<AGSGameState> GameState;
};
```

- The header includes `Core/GSGameState.h` rather than forward-declaring, because the
  `HandlePhaseChanged` UFUNCTION takes an `EGSLevelPhase` by value. UnrealHeaderTool needs the
  full enum for that.
- The four handlers are `UFUNCTION()`s. A *dynamic* delegate calls its functions by name through
  reflection, so `AddDynamic` only accepts reflected functions. Forget the `UFUNCTION()` and
  the binding fails: depending on the case you get a compile error or a runtime "function not
  found" ensure. They can still be private.
- `GameState` is a `TWeakObjectPtr`, not a `TObjectPtr`. The HUD wants to *talk* to the game
  state later (to unsubscribe), but has no business keeping it alive. A weak pointer becomes null
  if the object is destroyed, and `Get()` then returns `nullptr` (chapter 3).

`Source/GridSnake/UI/GSHUDWidget.cpp` (new), first part:

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSHUDWidget.h"
#include "GridSnake.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSprite.h"

#define LOCTEXT_NAMESPACE "GridSnakeHUD"

void UGSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AGSGameState* State = GetWorld()->GetGameState<AGSGameState>();
	if (!State)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("HUD: no AGSGameState in this world"));
		return;
	}
	GameState = State;

	State->OnGemsChanged.AddDynamic(this, &UGSHUDWidget::HandleGemsChanged);
	State->OnScoreChanged.AddDynamic(this, &UGSHUDWidget::HandleScoreChanged);
	State->OnPhaseChanged.AddDynamic(this, &UGSHUDWidget::HandlePhaseChanged);
	State->OnCountdownChanged.AddDynamic(this, &UGSHUDWidget::HandleCountdownChanged);

	// Show the level name from the level list, or the map name for unlisted maps.
	const UGSLevelFlowSubsystem* LevelFlow = GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
	const FGSLevelInfo* Level = LevelFlow ? LevelFlow->GetCurrentLevelInfo() : nullptr;
	LevelText->SetText(Level ? Level->DisplayName : FText::FromString(UGameplayStatics::GetCurrentLevelName(this, true)));

	// Events only report changes, so draw the current values once now.
	BuildGemIcons(State->GetGemsRequired());
	HandleGemsChanged(State->GetGemsCollected(), State->GetGemsRequired());
	HandleScoreChanged(State->GetScore());
	HandleCountdownChanged(State->GetCountdown());
}

void UGSHUDWidget::NativeDestruct()
{
	// The game state may outlive this widget; don't leave it calling a dead object.
	if (AGSGameState* State = GameState.Get())
	{
		State->OnGemsChanged.RemoveAll(this);
		State->OnScoreChanged.RemoveAll(this);
		State->OnPhaseChanged.RemoveAll(this);
		State->OnCountdownChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}
```

Walk through `NativeConstruct`:

1. **Find the game state.** A widget reaches its world with `GetWorld()`. The game mode spawns
   the game state before any player controller, so it exists when the controller's `BeginPlay`
   creates the HUD. The `if` guards against a level whose game mode uses another game state.
2. **Subscribe.** `AddDynamic` adds the HUD to the delegate's list. From now on, every
   `OnGemsChanged.Broadcast(...)` calls `HandleGemsChanged` with the same arguments, so the
   parameter lists must match exactly.
3. **Show the level name.** `GetCurrentLevelInfo()` (chapter 14) returns null for a map that is
   not in the level list, such as a test map. The HUD then shows the map name;
   `GetCurrentLevelName(this, true)` strips the PIE prefix (`UEDPIE_0_`).
4. **Draw the current values.** Easy to forget: events only report *changes*, and the game state
   may have set the gem count before the HUD subscribed. So the HUD calls its own handlers once
   with the current values. Subscribing first, then reading, means no change can slip through.

`NativeDestruct` undoes step 2. `RemoveAll(this)` removes every binding on that delegate whose
target object is the HUD. Exercise 4 looks at what would happen without it.

### The handlers

`Source/GridSnake/UI/GSHUDWidget.cpp`, the handlers:

```cpp
void UGSHUDWidget::HandleGemsChanged(int32 Collected, int32 Required)
{
	GemsText->SetText(FText::Format(LOCTEXT("Gems", "Gems {0}/{1}"), Collected, Required));

	for (int32 Index = 0; Index < GemIcons.Num(); ++Index)
	{
		GemIcons[Index]->SetColorAndOpacity(Index < Collected ? CollectedColor : MissingColor);
	}
}

void UGSHUDWidget::HandleScoreChanged(int32 Score)
{
	// The run score banks earlier levels; Score is this level's points.
	const UGSLevelFlowSubsystem* LevelFlow = GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
	const int32 Total = Score + (LevelFlow ? LevelFlow->GetRunScore() : 0);
	ScoreText->SetText(FText::Format(LOCTEXT("Score", "Score {0}"), Total));
}

void UGSHUDWidget::HandlePhaseChanged(EGSLevelPhase Phase)
{
	switch (Phase)
	{
	case EGSLevelPhase::Playing:
		SetBanner(FText::GetEmpty());
		break;
	case EGSLevelPhase::Completed:
		SetBanner(LOCTEXT("Complete", "Level complete!"));
		break;
	case EGSLevelPhase::Failed:
		SetBanner(LOCTEXT("Failed", "Ouch!"));
		break;
	default:
		break;
	}
}

void UGSHUDWidget::HandleCountdownChanged(int32 SecondsLeft)
{
	SetBanner(SecondsLeft > 0 ? FText::AsNumber(SecondsLeft) : FText::GetEmpty());
}
```

- The score shown is the **run** total: the points banked by earlier levels (kept in the level
  flow subsystem since chapter 14, because the game state is destroyed with each level) plus this
  level's points.
- The banner shows the countdown numbers 3, 2, 1, clears when the phase becomes `Playing`, and
  shows a message when the level ends.
- Each handler sets text only. None of them asks "did the value change?". They are only called
  when it did.

And the banner helper:

```cpp
void UGSHUDWidget::SetBanner(const FText& Text)
{
	BannerText->SetText(Text);
	BannerText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
```

`ESlateVisibility` has five values, and the difference matters for anything drawn over a game:

| Value | Drawn | Takes space in layout | Receives mouse clicks |
|---|---|---|---|
| `Visible` | yes | yes | yes |
| `Collapsed` | no | no | no |
| `Hidden` | no | yes | no |
| `HitTestInvisible` | yes | yes | no, and neither do its children |
| `SelfHitTestInvisible` | yes | yes | not itself, but its children can |

A HUD is display-only, so its visible parts should be `HitTestInvisible`: a click on the banner
should never be eaten by text. An empty banner is `Collapsed` so it costs nothing to lay out.
GridSnake does not use the mouse while playing, but menus in the next chapter do, and an invisible
full-screen widget that blocks clicks is a classic UI bug.

## Building widgets in code

The gem icons cannot be laid out in the designer, because the number of required gems comes from
the level data (chapter 13). So C++ creates one `UImage` per gem when the HUD is constructed.

```cpp
void UGSHUDWidget::BuildGemIcons(int32 Required)
{
	if (!GemIconBox || !GemIconSprite)
	{
		return;
	}

	GemIconBox->ClearChildren();
	GemIcons.Reset();

	for (int32 Index = 0; Index < Required; ++Index)
	{
		// Widgets created at runtime belong to the widget tree, like the ones
		// made in the designer.
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		// A Paper2D sprite is a region of a texture (it implements
		// ISlateTextureAtlasInterface), so a UMG brush can draw it directly.
		Icon->SetBrushResourceObject(GemIconSprite);
		Icon->SetDesiredSizeOverride(GemIconSize);
		Icon->SetColorAndOpacity(MissingColor);

		if (UHorizontalBoxSlot* IconSlot = GemIconBox->AddChildToHorizontalBox(Icon))
		{
			IconSlot->SetPadding(FMargin(2.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
		GemIcons.Add(Icon);
	}
}
```

Line by line:

- **The guard.** Without `GemIconBox` or `GemIconSprite` the HUD shows no icons; the text works.
- **`ClearChildren` and `Reset`.** `NativeConstruct` can run more than once for the same widget,
  so the row is rebuilt from scratch instead of gaining a second set of icons.
- **`WidgetTree->ConstructWidget<UImage>(...)`.** Every `UUserWidget` has a `WidgetTree`, the
  object that owns its layout's widgets. Create code-made widgets through it so they are owned
  like designer widgets: not with `NewObject`, and not with `CreateWidget`, which is for whole
  `UUserWidget`s.
- **`SetBrushResourceObject(GemIconSprite)`.** An image is drawn with a *brush* (`FSlateBrush`),
  which points at a resource object: usually a texture or a material. A Paper2D sprite works too.
  `UPaperSprite` implements `ISlateTextureAtlasInterface`, which tells Slate "draw this
  rectangle of this texture". So the HUD reuses the gem sprite from chapter 4 with no extra
  texture. If the icon looks blurry, check the source texture's **Filter**: pixel art needs
  **Nearest**, which chapter 4's Paper2D import settings already applied.
- **`SetDesiredSizeOverride(GemIconSize)`.** The sprite is 16 × 16 pixels. The override makes
  the image ask the layout for 32 × 32 instead.
- **`SetColorAndOpacity(MissingColor)`.** The colour multiplies the image, so dark grey with alpha
  0.6 makes a dim "not yet" gem, and white shows the sprite's real colours.
- **`AddChildToHorizontalBox(Icon)`** adds the child and returns its slot. The padding (2 units
  left and right) and vertical centring are set on the **slot**, as described at the start of the
  chapter.
- **`GemIcons.Add(Icon)`.** The array lets `HandleGemsChanged` recolour the first `Collected`
  icons. It is a `UPROPERTY(Transient)` because it holds `UObject` pointers.

The variable is named `IconSlot`, not `Slot`. `UWidget` already has a member called `Slot` (the
widget's own slot in *its* parent), and a local variable with the same name would hide it. The
compiler warns about that ("declaration hides class member"), and recent Unreal versions treat
that warning as an error by default.

### Example: a list built at runtime

The same technique works for any list whose length is only known at runtime: high scores, level
names, inventory items.

`examples/ch15/ExDynamicList.h`

```cpp
// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG, Slate, SlateCore.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExDynamicList.generated.h"

class UVerticalBox;

/**
 * The parent of W_ExDynamicList, which only needs a Vertical Box named List.
 * The rows are created in C++ at runtime.
 */
UCLASS(Abstract)
class UExDynamicList : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Throws away the old rows and builds Count new ones. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	void Rebuild(int32 Count);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> List;

	UPROPERTY(EditAnywhere, Category = "Example", meta = (ClampMin = "0", ClampMax = "50"))
	int32 InitialCount = 5;
};
```

`examples/ch15/ExDynamicList.cpp`

```cpp
// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG, Slate, SlateCore.

#include "ExDynamicList.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

DEFINE_LOG_CATEGORY_STATIC(LogExList, Log, All);

#define LOCTEXT_NAMESPACE "ExDynamicList"

void UExDynamicList::NativeConstruct()
{
	Super::NativeConstruct();

	Rebuild(InitialCount);
}

void UExDynamicList::Rebuild(int32 Count)
{
	List->ClearChildren();

	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Widgets made at runtime belong to the widget tree, like designer widgets.
		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Row->SetText(FText::Format(LOCTEXT("Row", "Row {0} of {1}"), Index + 1, Count));

		// Adding a child creates its slot: the layout settings live there.
		// (Don't name the variable "Slot": UWidget already has a member called Slot.)
		if (UVerticalBoxSlot* RowSlot = List->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 2.0f));
			RowSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	UE_LOG(LogExList, Log, TEXT("%s now has %d rows"), *GetName(), List->GetChildrenCount());
}

#undef LOCTEXT_NAMESPACE
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`, `UMG`, plus `Slate` and `SlateCore` for the
Slate value types (`FMargin`, `HAlign_Center`). A new C++ project's `Build.cs` has a commented-out
line that adds exactly those two; uncomment it.

To see it:

1. Copy both files into your module and build.
2. Create a Widget Blueprint with parent class `ExDynamicList` and name it `W_ExDynamicList`.
3. In its Designer, add a **Canvas Panel**, then drag a **Vertical Box** onto it and rename it
   `List`. Set its slot's **Anchors** to the centre preset, **Alignment** (0.5, 0.5) and tick
   **Size To Content**. Compile and save.
4. Open the level Blueprint (toolbar **Blueprints** button > **Open Level Blueprint**). From
   **Event BeginPlay**, add a **Create Widget** node with **Class** `W_ExDynamicList`, and connect
   its **Return Value** to an **Add to Viewport** node. Compile.
5. Press **Play**. Five centred lines read `Row 1 of 5` to `Row 5 of 5`.

Expected Output Log (filter: `LogExList`):

```
LogExList: W_ExDynamicList_C_0 now has 5 rows
```

- Change **Initial Count** in the Widget Blueprint's **Class Defaults** to 12 and play again: 12
  rows, no layout work.
- For hundreds of rows, a widget per item gets slow. UMG's **List View** only creates widgets for
  the visible rows and reuses them as you scroll, at the cost of more setup.

## Text and localization

Every piece of text in the HUD is an `FText`, never an `FString`. Of Unreal's three string types
(chapter 3), `FString` is for code (paths, logs), `FName` for identifiers, and `FText` for **text
shown to the player**. UMG's text widgets take `FText`, which pushes you towards doing it right:

- **Localization.** An `FText` created with `LOCTEXT` carries a namespace and a key. The engine's
  localization tools gather every such text into a table that translators fill in, and at runtime
  the text is looked up in the player's language. An `FString` can't be translated.
- **Culture-aware formatting.** `FText::AsNumber(12345)` gives `12,345` in English and `12.345` in
  German. `FText::AsPercent`, `FText::AsDate` and friends exist too.
- **Word order.** With `FText::Format` placeholders a translator can move `{0}` to wherever their
  grammar needs it. String concatenation does not allow that.

### LOCTEXT and NSLOCTEXT

```cpp
#define LOCTEXT_NAMESPACE "GridSnakeHUD"

const FText Message = LOCTEXT("Complete", "Level complete!");   // namespace from the #define

#undef LOCTEXT_NAMESPACE

const FText Other = NSLOCTEXT("GridSnakeHUD", "Complete", "Level complete!");  // namespace inline
```

- The first argument is the **key**: unique within the namespace, and stable even when the
  English text changes. The second is the source text, shown until there is a translation.
- `LOCTEXT` reads the namespace from the `LOCTEXT_NAMESPACE` macro, so a `.cpp` defines it after
  its includes and **undefines it at the end**. Unity builds paste several `.cpp` files into one
  compilation unit, so a namespace left defined would leak into, or clash with, the next file.
- `NSLOCTEXT` takes the namespace as an argument; handy in headers and one-off places.
- `FText::FromString(...)` makes an `FText` **without** a key, so it is never translated. Use it
  for text that should not be translated: a map name fallback (as `NativeConstruct` does), or a
  player's name.
- `FText::GetEmpty()` is the empty text. `Text.IsEmpty()` tests for it, as `SetBanner` does.

### FText::Format

```cpp
// Ordered arguments: {0}, {1}, ...
FText::Format(LOCTEXT("Gems", "Gems {0}/{1}"), Collected, Required);

// Named arguments, clearer when there are several.
FFormatNamedArguments Args;
Args.Add(TEXT("Collected"), Collected);
Args.Add(TEXT("Required"), Required);
FText::Format(LOCTEXT("GemsNamed", "Gems {Collected}/{Required}"), Args);
```

Arguments can be numbers, other `FText`s and a few other types. Numbers are formatted for the
current culture, so `Score {0}` with 12345 shows `Score 12,345`.

The localization pipeline itself (gathering text, translating, compiling the tables) is managed
in the **Localization Dashboard** (**Tools > Localization Dashboard**; the label may differ
slightly in your version). GridSnake does not ship translations, but because every visible string
is already a `LOCTEXT`, adding a language later is a data task, not a code change.

### Trying the HUD

That is the whole ch15 change: `UMG` in `Build.cs`, the new `UGSHUDWidget`, and the HUD creation
in the player controller. To finish the editor side:

1. Build, then create and lay out `W_HUD` as in [Designing W_HUD](#designing-w_hud), including the
   Class Defaults (Gem Icon Sprite `S_Gem_Idle_0`, Gem Icon Size (32, 32), Missing Color).
2. Open `BP_GSPlayerController` (from chapter 8). In **Class Defaults**, set **UI > HUD Widget
   Class** = `W_HUD`. Compile and save.
3. Open `L_Level01` and press **Play**.

You should see:

- Top left: `First Steps` (the display name of level 0 in `DA_LevelList`), `Gems 0/3`, three dim
  gem icons, and `Score 0`.
- In the centre: `3`, `2`, `1`, then the banner disappears and the snake starts moving.
- Each gem you eat lights up one icon, changes the text to `Gems 1/3` and so on, and raises the
  score.
- Crashing shows `Ouch!`. Reaching the open exit shows `Level complete!`. In level 2 the score
  starts from the total you banked in level 1.

This chapter adds no new log lines. The Output Log shows the same lines as in chapter 14, for
example (filter: `LogGridSnake`):

```
LogGridSnake: Countdown: 3
LogGridSnake: Countdown: 2
LogGridSnake: Countdown: 1
LogGridSnake: Phase: EGSLevelPhase::Playing
LogGridSnake: Gem 1/3
```

Only the screen is new. If something is wrong:

- **No HUD at all.** `BP_GSPlayerController` has no **HUD Widget Class**, or the game mode is not
  using `BP_GSPlayerController` (check **World Settings > GameMode Override** and the game mode's
  **Player Controller Class**).
- **`W_HUD` does not compile.** A required name is misspelled. The Compiler Results name the
  property.
- **Text but no icons.** **Gem Icon Sprite** is not set in `W_HUD`'s Class Defaults, or the box is
  not named exactly `GemIconBox`. The optional binding then fails silently.
- **`HUD: no AGSGameState in this world` warning.** The level is using a game mode whose game
  state is not `AGSGameState`.
- **Text cut off or wrapping.** The Horizontal Box or the banner does not have **Size To
  Content** ticked.

### The Widget Reflector

When a layout misbehaves at runtime, the **Widget Reflector** shows the live Slate tree. Open it
from **Tools > Debug > Widget Reflector** (the label may differ slightly in your version). Start
PIE, click **Pick Hit-Testable Widgets** or **Pick Painted Widgets** in the reflector, and move
the mouse over the game viewport. The reflector highlights the widget under the cursor and shows
its place in the hierarchy, its size and its visibility. Because the reflector shows Slate
widgets, you see `STextBlock` and `SHorizontalBox` rather than UMG names: the two layers from the
start of the chapter in action.

## Summary

- **Slate** is Unreal's C++ UI framework; **UMG** wraps it in `UObject` widgets that can be
  designed visually, saved as assets and used from Blueprints. Add the `UMG` module to
  `Build.cs` (public if headers use its types).
- A **Widget Blueprint** gives the layout; a `UCLASS(Abstract)` C++ parent derived from
  `UUserWidget` gives the logic. `UPROPERTY(meta = (BindWidget))` connects them by exact name,
  checked when the blueprint compiles. `BindWidgetOptional` allows a missing widget (check for
  null).
- Panel children's layout lives in **slots**. A Canvas Panel slot has anchors, position,
  alignment and Size To Content; boxes have padding and alignment.
- `CreateWidget<T>(PlayerController, Class)` then `AddToViewport()` shows a widget for a player.
  GridSnake does it in `AGSPlayerController::BeginPlay`, with the class set in
  `BP_GSPlayerController`.
- Update UI from **events**, not every frame. Subscribe in `NativeConstruct` with `AddDynamic`
  (handlers must be `UFUNCTION`s), draw the current values once, and unsubscribe in
  `NativeDestruct` with `RemoveAll(this)`.
- Widgets built at runtime come from `WidgetTree->ConstructWidget<T>()` and are added with
  `AddChildTo...`, which returns the slot to configure. A Paper2D sprite can be an image brush.
- Player-visible text is `FText`: `LOCTEXT`/`NSLOCTEXT` for localizable strings, `FText::Format`
  and `FText::AsNumber` for culture-aware formatting, `FText::FromString` for text that should not
  be translated.

## Exercises

1. **Show the snake's length.** Add a `LengthText` to the HUD that shows `Length 3` and updates
   whenever the snake grows. Keep the HUD event-driven: no ticking.

   <details>
   <summary>Solution</summary>

   The snake owns its length, so it should announce changes. In `Snake/GSSnake.h`, next to the
   existing `FGSSnakeDiedSignature`:

   ```cpp
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSLengthChangedSignature, int32, Length);

   // ... in the class, public, next to OnDied:
   UPROPERTY(BlueprintAssignable, Category = "Snake")
   FGSLengthChangedSignature OnLengthChanged;
   ```

   In `Snake/GSSnake.cpp`, broadcast where the length changes. In `Step()`, the `else` branch is
   the only place the snake gets longer:

   ```cpp
   	else
   	{
   		--PendingGrowth;
   		OnLengthChanged.Broadcast(Segments.Num());
   	}
   ```

   Also broadcast at the end of `PlaceOnBoard()` (after `Segments` is filled), so a listener
   learns the starting length: `OnLengthChanged.Broadcast(Segments.Num());`.

   In `UI/GSHUDWidget.h`, add an optional binding (so old layouts still compile) and a handler:

   ```cpp
   UPROPERTY(meta = (BindWidgetOptional))
   TObjectPtr<UTextBlock> LengthText;

   // private:
   UFUNCTION()
   void HandleLengthChanged(int32 Length);

   TWeakObjectPtr<AGSSnake> Snake;   // forward-declare: class AGSSnake;
   ```

   In `UI/GSHUDWidget.cpp` (add `#include "Snake/GSSnake.h"`), at the end of `NativeConstruct`:

   ```cpp
   	if (AGSSnake* OwnSnake = Cast<AGSSnake>(GetOwningPlayerPawn()))
   	{
   		Snake = OwnSnake;
   		OwnSnake->OnLengthChanged.AddDynamic(this, &UGSHUDWidget::HandleLengthChanged);
   		HandleLengthChanged(OwnSnake->GetLength());
   	}
   ```

   In `NativeDestruct`: `if (AGSSnake* OwnSnake = Snake.Get()) { OwnSnake->OnLengthChanged.RemoveAll(this); }`.

   ```cpp
   void UGSHUDWidget::HandleLengthChanged(int32 Length)
   {
   	if (LengthText)
   	{
   		LengthText->SetText(FText::Format(LOCTEXT("Length", "Length {0}"), Length));
   	}
   }
   ```

   The game mode normally possesses the snake when the player joins, before any `BeginPlay`, so
   `GetOwningPlayerPawn()` finds it. But `BeginPlay` order between actors is not guaranteed: if
   the snake is not placed yet, `GetLength()` returns 0 and the broadcast at the end of
   `PlaceOnBoard()` corrects it. Finally, add a **Text** named `LengthText` (with a Spacer before
   it) to `W_HUD`'s Horizontal Box.
   </details>

2. **Animate the banner.** Make the banner "pop" (start big and shrink to normal size) each time
   it shows new text, using a UMG Widget Animation played from C++.

   <details>
   <summary>Solution</summary>

   In `W_HUD`'s Designer, open the **Animations** panel (at the bottom of the Designer; use
   **Window > Animations** if it is hidden). Click **+ Animation** and name the animation
   `BannerPop`. With `BannerPop` selected, select `BannerText`, click **+ Track** and add the
   **Transform** track. At time 0, set **Render Transform > Scale** to (1.5, 1.5) and add a key.
   At 0.2 s, set it to (1, 1) and add a key. (The labels may differ slightly in your version.)
   Compile.

   In `GSHUDWidget.h`:

   ```cpp
   class UWidgetAnimation;

   	UPROPERTY(Transient, meta = (BindWidgetAnim))
   	TObjectPtr<UWidgetAnimation> BannerPop;
   ```

   `BindWidgetAnim` binds by name like `BindWidget`, and the property must be `Transient`.
   (`BindWidgetAnimOptional` exists too.) In `GSHUDWidget.cpp`, add
   `#include "Animation/WidgetAnimation.h"` and change `SetBanner`:

   ```cpp
   void UGSHUDWidget::SetBanner(const FText& Text)
   {
   	BannerText->SetText(Text);
   	BannerText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
   	if (!Text.IsEmpty() && BannerPop)
   	{
   		PlayAnimation(BannerPop);
   	}
   }
   ```

   `PlayAnimation` returns a handle to the running animation. The return type has changed
   between engine versions, and here we don't need it, so ignore it. Each countdown number now
   pops, and so do the end messages. The scale is applied around the widget's **Render Transform
   Pivot**, which defaults to its centre (0.5, 0.5).
   </details>

3. **Count the score up smoothly.** When the score jumps from 100 to 200, show it counting up over
   about a quarter of a second instead of jumping.

   <details>
   <summary>Solution</summary>

   This is a value that really does change every frame while it animates, so `NativeTick` is the
   right tool. In `GSHUDWidget.h`:

   ```cpp
   	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

   private:
   	int32 TargetScore = 0;
   	float DisplayedScore = 0.0f;
   	void ShowScore(int32 Value);
   ```

   In `GSHUDWidget.cpp`, `HandleScoreChanged` now stores the target instead of setting text:

   ```cpp
   void UGSHUDWidget::HandleScoreChanged(int32 Score)
   {
   	const UGSLevelFlowSubsystem* LevelFlow = GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
   	TargetScore = Score + (LevelFlow ? LevelFlow->GetRunScore() : 0);
   }

   void UGSHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
   {
   	Super::NativeTick(MyGeometry, InDeltaTime);

   	if (FMath::RoundToInt(DisplayedScore) != TargetScore)
   	{
   		// Move at 400 points per second, never past the target.
   		DisplayedScore = FMath::FInterpConstantTo(DisplayedScore, float(TargetScore), InDeltaTime, 400.0f);
   		ShowScore(FMath::RoundToInt(DisplayedScore));
   	}
   }

   void UGSHUDWidget::ShowScore(int32 Value)
   {
   	ScoreText->SetText(FText::Format(LOCTEXT("Score", "Score {0}"), Value));
   }
   ```

   In `NativeConstruct`, after the initial `HandleScoreChanged(State->GetScore());`, add
   `DisplayedScore = float(TargetScore); ShowScore(TargetScore);` so a new level starts at the
   banked total instead of counting up from 0. `FInterpConstantTo` moves at a constant speed;
   `FMath::FInterpTo` would slow down near the target. C++ user widgets tick by default (a class
   can opt out with `UCLASS(meta = (DisableNativeTick))`).
   </details>

4. **Why unbind?** `NativeDestruct` removes the HUD from four delegates. Explain what would
   happen without it, given that dynamic delegates hold *weak* references to their targets.

   <details>
   <summary>Solution</summary>

   `NativeDestruct` runs when the HUD leaves the screen, for example `RemoveFromParent()`, which
   chapter 16 will do to swap screens. The widget *object* may live on, and the game state
   certainly does: it lives until the level ends. Without the unbind:

   - **While the widget object is alive but off screen,** every broadcast still calls its
     handlers. It updates text nobody sees: wasted work. If the widget is later added again,
     `NativeConstruct` runs again and binds again. `AddDynamic` does not check for duplicates
     (that is what `AddUniqueDynamic` is for), so the same handler is now in the list twice and
     runs twice per broadcast. Development builds report this with an ensure ("Attempting to
     bind a delegate twice") in the Output Log.
   - **After the widget object is garbage-collected,** the binding remains in the game state's
     list, but a dynamic delegate stores its target as a weak object pointer. A broadcast checks
     each target and skips dead ones, so nothing crashes. The stale entries just sit in the list
     until the delegate compacts it (it does so, for example, when something new is added).

   So forgetting to unbind is usually not a crash but a leak of work, and a source of confusing
   behaviour ("why does my hidden widget still react?"). Bind in `NativeConstruct`, unbind in
   `NativeDestruct`. The `TWeakObjectPtr` covers the opposite order: at level end the game state
   may be destroyed first, and the HUD must not touch it then.
   </details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-15--a-hud-with-umg)
