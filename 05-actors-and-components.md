# Chapter 5 — Actors and Components

Everything you can see or touch in an Unreal level is an **actor**, and almost everything an actor
can do comes from its **components**. This chapter explains what an actor is and how components
give it a transform, visuals and behaviour. It walks through the actor lifecycle: what runs in the
constructor, in `OnConstruction`, in `BeginPlay` and in `Tick`, and why each step exists. It then
shows the split this course uses everywhere: a C++ class that defines components and behaviour, and
a Blueprint subclass that picks the art. GridSnake gains its first two gameplay actors, the gem and
the exit, placed in `L_Level01`.

Previous: [Chapter 4](04-pixel-art-with-paper2d.md). Next:
[Chapter 6](06-tile-maps-and-the-orthographic-camera.md) (tile maps and the orthographic camera).

As in every chapter, nothing here was compiled or run by the authors (see the README). Code was
checked against the UE 5.8 API reference. Where a book like this would show program output, you get
an *Expected Output Log* block: the lines the code's `UE_LOG` calls print, with the parts that vary
marked as such.

## What an actor is

Chapter 3 introduced `UObject`: a C++ object that the engine can reflect, save, garbage-collect and
show in the editor. An **actor** (`AActor`) is a `UObject` that can exist *in a world*. That one
property brings a lot with it:

- An actor belongs to a level in a `UWorld`. It has a lifecycle tied to that world: it begins play
  when the world starts, it may tick every frame, and it ends play when it is destroyed or the
  world goes away.
- An actor can be **placed** in a level in the editor (it is then saved in the `.umap` file) or
  **spawned** at runtime with `UWorld::SpawnActor`. You never create an actor with `new` or with
  `NewObject`.
- An actor lives until something destroys it: a call to `Destroy()`, a life span running out, the
  level unloading, or the game ending. It does not disappear just because no C++ pointer refers to
  it, as an ordinary `UObject` would. The level keeps it alive.
- An actor can own **components**. An actor with no components is an empty shell: it has a name
  and a class, but no position, nothing to draw and nothing to collide with.

The last point surprises people coming from other engines. An `AActor` has no transform of its
own. `GetActorLocation()` returns the location of the actor's **root component**. With no root
component, there is no location, and `GetActorLocation()` returns zero.

The prefixes follow the convention from chapter 3: `A` for actor classes (`AActor`, `AGSGem`), `U`
for other `UObject` classes, including components (`UPaperFlipbookComponent`), and `F` for plain
structs (`FVector`, `FTransform`).

Not everything is an actor. A settings object (`UGSSettings`, chapter 3), a data asset (chapter 13)
or a save game (chapter 18) are plain `UObject`s: data with no place in the world. A good rule of
thumb: if it has a position in the level, or must react to the world's time, it is an actor (or a
component of one).

## Components

A **component** is a `UObject` that an actor owns and that adds one capability to it. Unreal has
three layers of component classes:

| Class | Has a transform? | Draws or collides? | Examples |
|---|---|---|---|
| `UActorComponent` | no | no | movement logic, inventories, health |
| `USceneComponent` | yes, and can have children | no | an invisible pivot, a camera, a spring arm |
| `UPrimitiveComponent` | yes | yes | sprites, flipbooks, tile maps, collision boxes |

`USceneComponent` derives from `UActorComponent`, and `UPrimitiveComponent` derives from
`USceneComponent`. GridSnake uses only a handful of them over the whole course:

| Component | Module | Used for | Chapter |
|---|---|---|---|
| `USceneComponent` | Engine | an invisible root to hang other components on | 5 |
| `UPaperFlipbookComponent` | Paper2D | animated sprites: gems, exit, snake head | 5 |
| `UPaperTileMapComponent` | Paper2D | the level's tiles | 6 |
| `UCameraComponent` | Engine | the orthographic board camera | 6 |
| `UBoxComponent` | Engine | trigger volumes for gems and the exit | 10 |
| `UPaperGroupedSpriteComponent` | Paper2D | the snake's body, many sprites in one draw call | 9 |

Scene components form a tree. Exactly one of them is the **root component**; its transform *is*
the actor's transform. Every other scene component is attached to a parent and stores a
**relative** transform: a location, rotation and scale relative to that parent. Move the root and
all the children move with it.

### Creating components in C++

Components that every instance of a class should have are created in the class's constructor:

```cpp
AGSGem::AGSGem()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Root);
}
```

- `CreateDefaultSubobject<T>(Name)` creates a component owned by this actor. The name must be
  unique within the actor. It is how the engine matches the component to saved data, so do not
  rename it once you have Blueprints or levels that use the class. A rename makes their overrides
  of that component's properties disappear.
- `SetRootComponent` makes a scene component the root. You may also assign to `RootComponent`
  directly; both work.
- `SetupAttachment(Parent)` records the parent for when the component is registered with the
  world. Use it **only in constructors**. At runtime, to attach a component that already exists,
  use `AttachToComponent` instead.
- The component pointers are `UPROPERTY` members of type `TObjectPtr<...>`. The `UPROPERTY` makes
  the garbage collector see them (chapter 3) and makes them appear in the editor.

`CreateDefaultSubobject` works only inside a constructor. Called anywhere else, the engine stops
with a fatal error saying that `CreateDefaultSubobject` can only be used inside UObject
constructors. To add a component at runtime, use `NewObject<UMyComponent>(this)` and then call
`RegisterComponent()` on it. GridSnake never needs to.

### Why use a separate scene root?

The gem could make its flipbook component the root. It uses an invisible `USceneComponent` as the
root and attaches the flipbook below it instead. This lets the flipbook move *relative to the
actor*: the gem bobs by changing the flipbook's relative Z, while the actor's location, which
later chapters use to work out the gem's grid cell, stays on the cell. It is a common pattern:
the root is the "logical" position, and the visible parts are offset from it.

### The component tree in the editor

Open any Blueprint based on a C++ actor class (you will make one later in this chapter) and look at
the **Components** panel at the top left. Components created in C++ are listed with their C++ names
(`Root`, `Flipbook`) and are marked as inherited. You can select them and change their properties
in the Details panel, but you cannot delete or re-parent them, because the C++ constructor owns
them. Components you add in the Blueprint editor appear in the same tree and are fully editable.
A screenshot would help here; in words: the tree shows `Root` with `Flipbook` indented below it.

## The actor lifecycle

An actor goes through a fixed sequence of calls. Knowing which one runs when is the difference
between code that works in the editor, in Play In Editor (PIE) and in a packaged game, and code
that works in only one of them. Epic's "Actor Lifecycle" page has the full flow chart. These are
the calls you override most:

| Function | Runs | Put here |
|---|---|---|
| Constructor | for the class default object (CDO) at start-up, and for every new instance | `CreateDefaultSubobject`, default values, tick settings |
| `OnConstruction(Transform)` | in the editor when the actor is placed, moved or edited; at runtime when it is spawned | anything the editor should show: derived visuals, layout |
| `PostInitializeComponents()` | once the components are initialized, before `BeginPlay` (game worlds only) | binding to your own components' events |
| `BeginPlay()` | when the world starts play, or right away for actors spawned during play | gameplay set-up: find other actors, start timers, log |
| `Tick(DeltaSeconds)` | every frame, if ticking is enabled | per-frame updates |
| `EndPlay(Reason)` | when the actor leaves play for any reason | clean-up: clear timers, unbind delegates |
| `Destroyed()` | when `Destroy()` is called on it | rarely needed; Epic recommends `EndPlay` |

The constructor deserves a warning. It runs for the **class default object**, the template
instance that every UClass has (chapter 3). The CDO is built when the module loads, before any
world exists. So the constructor must not use `GetWorld()`, look for other actors, or depend on
anything that only exists during play. Keep it to creating components and setting defaults.

`OnConstruction` is the C++ side of a Blueprint's **Construction Script**. It runs very often in the
editor (every time you drag the actor or change a property), so it must be cheap, and it must give
the same result every time it runs. GridSnake uses it to show the right sprite for a property as
soon as you change the property, without pressing Play.

### Watching the lifecycle

`examples/ch05/ExLifecycleLogger.h`

```cpp
// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExLifecycleLogger.generated.h"

/**
 * Logs every lifecycle function it overrides, with the object's name and whether
 * it is the class default object (CDO). Drop it in a level and watch the Output Log
 * while you place it, move it, press Play and press Stop.
 */
UCLASS()
class AExLifecycleLogger : public AActor
{
	GENERATED_BODY()

public:
	AExLifecycleLogger();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

	/** If above zero, the actor destroys itself this many seconds after BeginPlay. */
	UPROPERTY(EditAnywhere, Category = "Lifecycle", meta = (ClampMin = "0"))
	float DestroyAfterSeconds = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void LogStage(const TCHAR* Stage) const;
};
```

`examples/ch05/ExLifecycleLogger.cpp`

```cpp
// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExLifecycleLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogExLife, Log, All);

AExLifecycleLogger::AExLifecycleLogger()
{
	// The constructor also runs for the CDO, at engine start-up, before any level exists.
	UE_LOG(LogExLife, Log, TEXT("Constructor %s (CDO: %s)"), *GetName(),
		HasAnyFlags(RF_ClassDefaultObject) ? TEXT("true") : TEXT("false"));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AExLifecycleLogger::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LogStage(TEXT("OnConstruction"));
}

void AExLifecycleLogger::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	LogStage(TEXT("PostInitializeComponents"));
}

void AExLifecycleLogger::BeginPlay()
{
	Super::BeginPlay();
	LogStage(TEXT("BeginPlay"));

	if (DestroyAfterSeconds > 0.0f)
	{
		// Destroy() is called for us when the life span runs out.
		SetLifeSpan(DestroyAfterSeconds);
	}
}

void AExLifecycleLogger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogExLife, Log, TEXT("EndPlay %s reason %s"), *GetName(), *UEnum::GetValueAsString(EndPlayReason));
	Super::EndPlay(EndPlayReason);
}

void AExLifecycleLogger::Destroyed()
{
	Super::Destroyed();
	LogStage(TEXT("Destroyed"));
}

void AExLifecycleLogger::LogStage(const TCHAR* Stage) const
{
	UE_LOG(LogExLife, Log, TEXT("%s %s"), Stage, *GetName());
}
```

To try it:

1. Copy both files into `Source/<YourModule>/` of any UE 5.8 C++ project (GridSnake works:
   `Source/GridSnake/`). The module needs `Core`, `CoreUObject` and `Engine`, which every C++
   project has.
2. Close the editor and build the editor target from your IDE, as in chapter 2. Live Coding can
   compile new classes, but a full build and restart is the reliable way to add a `UCLASS`.
3. Open the Output Log (**Window > Output Log**) and type `LogExLife` in its search box.
4. In the Content Browser, open **C++ Classes > <YourModule>** and drag `ExLifecycleLogger` into
   the level.
5. Move it a little with the move gizmo, then press **Play**, wait a second, and press **Stop**.

Expected Output Log (filter: `LogExLife`). The lines are illustrative: the exact count of
constructor and `OnConstruction` lines depends on what the editor does while you drag, and actor
names may carry a different number.

```
LogExLife: Constructor Default__ExLifecycleLogger (CDO: true)        <- at editor start-up
LogExLife: Constructor ExLifecycleLogger_0 (CDO: false)             <- step 4: dropped in the level
LogExLife: OnConstruction ExLifecycleLogger_0
LogExLife: OnConstruction ExLifecycleLogger_0                       <- step 5: moved (one or more)
LogExLife: Constructor ExLifecycleLogger_0 (CDO: false)             <- Play: the PIE copy
LogExLife: PostInitializeComponents ExLifecycleLogger_0
LogExLife: BeginPlay ExLifecycleLogger_0
LogExLife: EndPlay ExLifecycleLogger_0 reason EEndPlayReason::EndPlayInEditor   <- Stop
```

- The first line comes from the CDO. Its name is `Default__` plus the class name without the `A`.
  It is logged when the module loads, long before you open a level. Scroll to the top of the log,
  or look in `Saved/Logs/<Project>.log`, to find it. The class constructor runs with
  `RF_ClassDefaultObject` set exactly once per class.
- Dropping the actor into the level constructs an instance in the **editor world** and runs
  `OnConstruction`. There is no `PostInitializeComponents` or `BeginPlay`: the editor world never
  begins play.
- Moving the actor runs `OnConstruction` again. That is the hook GridSnake's exit uses to update
  its sprite in the editor.
- Pressing Play does *not* reuse the editor's actor. PIE **duplicates** the editor world into a new
  play world, so the constructor runs again for the copy. A duplicated (or loaded) actor does not
  run `OnConstruction`: its components were already built and saved. It goes straight to
  `PostInitializeComponents` and `BeginPlay`. The copy keeps the same object name, in a world whose
  map name has the `UEDPIE_0_` prefix you saw in chapter 2.
- Stopping PIE ends play for every actor with reason `EndPlayInEditor`. `Destroyed` is not called:
  the world is torn down, nobody called `Destroy()`. `UEnum::GetValueAsString` turns the enum value
  into its name, which works for any `UENUM`, here `EEndPlayReason::Type`.

Now set **Destroy After Seconds** to `2` on the placed actor (Details panel, **Lifecycle**
category) and play again:

```
LogExLife: Constructor ExLifecycleLogger_0 (CDO: false)
LogExLife: PostInitializeComponents ExLifecycleLogger_0
LogExLife: BeginPlay ExLifecycleLogger_0
LogExLife: EndPlay ExLifecycleLogger_0 reason EEndPlayReason::Destroyed      <- 2 s later
LogExLife: Destroyed ExLifecycleLogger_0
```

`SetLifeSpan(2)` starts a timer that calls `Destroy()`. A destroyed actor gets both `EndPlay`
(reason `Destroyed`) and `Destroyed`. The order shown is the one you get when, as here, the log
call comes *after* `Super::Destroyed()`: the engine's `AActor::Destroyed` is what routes `EndPlay`
in current versions. Do not rely on that order; put clean-up code in `EndPlay`, which runs for
every way an actor can leave play. When you press Stop afterwards nothing more is logged for this
actor: it is already gone.

### Spawned actors take a different path

Actors that are spawned with `SpawnActor` run the full construction sequence *inside the play
world*. You will see it later in this chapter with `ExSpawner`. Spawn an `ExLifecycleLogger` while
playing and you get:

```
LogExLife: Constructor ExLifecycleLogger_<n> (CDO: false)
LogExLife: OnConstruction ExLifecycleLogger_<n>
LogExLife: PostInitializeComponents ExLifecycleLogger_<n>
LogExLife: BeginPlay ExLifecycleLogger_<n>
```

For spawned actors, `OnConstruction` runs as part of `FinishSpawning`: after `PostActorCreated`
and before `PostInitializeComponents`. That is how a Blueprint's Construction Script can build
components for an actor spawned at runtime. And because the world is already playing,
`BeginPlay` runs right away, before `SpawnActor` returns.

The exact interleaving depends on how the actor comes into being: loaded with a level, duplicated
for PIE, spawned, or spawned deferred (`SpawnActorDeferred`, which lets you set properties between
`PostActorCreated` and `OnConstruction`). The practical rules:

- Defaults and components: constructor.
- What the editor should show: `OnConstruction`.
- Game logic that needs other actors or the world: `BeginPlay`.
- Clean-up: `EndPlay`.

## Ticking

An actor **ticks** when the engine calls its `Tick(float DeltaSeconds)` once per frame.
`DeltaSeconds` is the game time since the previous tick, in seconds: about 0.0167 at 60 frames per
second, 0.0069 at 144. Anything that changes over time must scale by it, or it runs faster on
faster machines.

Ticking is opt-in. The constructor sets it up through `PrimaryActorTick`, the actor's tick function:

| Setting | Meaning |
|---|---|
| `PrimaryActorTick.bCanEverTick = true` | register a tick function at all; without it `Tick` is never called |
| `PrimaryActorTick.bStartWithTickEnabled` | whether ticking starts enabled (default true) |
| `PrimaryActorTick.TickInterval` | seconds between ticks; 0 means every frame |
| `SetActorTickEnabled(bool)` | turn ticking on or off at runtime (needs `bCanEverTick`) |

Ticks happen only in **game worlds**: PIE, Simulate, standalone and packaged games. The editor's
own world does not tick actors. When the game is paused, ticking stops, and `DeltaSeconds` already
includes time dilation (slow motion), so code that scales by it gets both behaviours for free.

Components tick separately from their actor. A flipbook component, for example, ticks to advance
its animation even if its actor never ticks. Turning off the actor's tick does not stop its
components.

`examples/ch05/ExSpinner.h`

```cpp
// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSpinner.generated.h"

class UPaperSpriteComponent;

/**
 * A sprite that spins in the screen plane. Paper2D sprites lie in the X-Z plane,
 * so "spinning on screen" means rotating about the Y axis: that is Pitch.
 */
UCLASS()
class AExSpinner : public AActor
{
	GENERATED_BODY()

public:
	AExSpinner();

	virtual void Tick(float DeltaSeconds) override;

	/** Positive values spin one way, negative values the other. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spinner")
	float DegreesPerSecond = 90.0f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Pick any sprite for it in the Details panel (Source Sprite). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> Sprite;
};
```

`examples/ch05/ExSpinner.cpp`

```cpp
// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExSpinner.h"
#include "PaperSpriteComponent.h"

AExSpinner::AExSpinner()
{
	// Without this, Tick is never called, however you override it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Root);
}

void AExSpinner::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("%s spins at %.1f degrees per second"), *GetName(), DegreesPerSecond);
}

void AExSpinner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// FRotator is (Pitch, Yaw, Roll). Pitch turns about Y, the axis the 2D camera looks along.
	// Multiplying by DeltaSeconds makes the speed independent of the frame rate.
	AddActorLocalRotation(FRotator(DegreesPerSecond * DeltaSeconds, 0.0f, 0.0f));
}
```

To try it: the module needs `Paper2D` in its `Build.cs` (GridSnake has it from this chapter on).
Drag `ExSpinner` into a level, select its **Sprite** component in the Details panel and set
**Source Sprite** to any sprite, for example `S_Gem_Idle_0`. Put the viewport in the orthographic
view that shows sprites face-on (chapter 4: **Left** or **Right**, whichever has +X to the right),
then start **Simulate** (next to the Play button: the **⋮** menu > **Simulate**, or Alt+S).
Simulate runs the game world, including `BeginPlay` and `Tick`, but keeps the editor camera, so
you can watch from the view you chose.

Expected Output Log (filter: `LogTemp`; the actor name is illustrative):

```
LogTemp: ExSpinner_0 spins at 90.0 degrees per second
```

The sprite turns a quarter turn per second in the screen plane.

- `FRotator`'s constructor takes **(Pitch, Yaw, Roll)**, in degrees. Pitch is rotation about the Y
  axis, yaw about Z, roll about X. Paper2D sprites lie in the X-Z plane and the 2D camera looks
  along Y, so pitch is the one that spins a sprite like a clock hand. Yaw or roll would turn it
  edge-on.
- `AddActorLocalRotation` rotates the root component, and the sprite follows because it is
  attached. Internally it multiplies quaternions, so the spin continues smoothly past 90 degrees.
  The Rotation fields in the Details panel may jump between equivalent values (pitch 80, yaw 180,
  roll 180, say) while it spins. That is the same orientation written differently, not a bug.
- `DegreesPerSecond * DeltaSeconds` is the per-frame step. At 60 fps it is 1.5 degrees; at 120
  fps, 0.75. Either way the sprite turns 90 degrees per second.
- `BlueprintReadWrite` lets a Blueprint change the speed while playing.

Two console commands help with ticking. Open the console with the backquote key (`` ` ``) while
playing:

- `dumpticks` prints every registered tick function to the Output Log, with whether it is enabled.
  You will find `ExSpinner_0` and its components there.
- `stat game` shows how long the game thread spends on ticks each frame.

Ticking is convenient but not free: a few thousand ticking actors add up. GridSnake ticks only
what must change every frame (the gems' bob, here). Chapter 9 moves the snake with a **timer**
instead of `Tick`, because it should move a fixed number of times per second.

## Building AGSGem and AGSExit

GridSnake now gets two actors:

- **`AGSGem`**: a collectible. It shows an animated flipbook and bobs up and down. Each level has
  three. Chapters 10 and 11 make the snake collect them.
- **`AGSExit`**: the level exit. It is closed (a single frame) until the gems are collected, then
  open (an animation). Chapter 11 opens it; this chapter gives it the two states and lets you
  preview them in the editor.

The changed and new files compared to the chapter 4 snapshot:

- `Source/GridSnake/GridSnake.Build.cs`: adds the `Paper2D` module.
- New `Source/GridSnake/Gameplay/GSGem.h` and `GSGem.cpp`.
- New `Source/GridSnake/Gameplay/GSExit.h` and `GSExit.cpp`.
- `ASSETS.md`: the Blueprints and placements below.

### The Paper2D module

`Source/GridSnake/GridSnake.Build.cs`

```diff
 		PublicDependencyModuleNames.AddRange(new string[] {
 			"Core",
 			"CoreUObject",
 			"Engine",
 			"InputCore",
 			"EnhancedInput",
-			"DeveloperSettings"
+			"DeveloperSettings",
+			"Paper2D"
 		});
```

Chapter 1 enabled the Paper2D *plugin*, which makes its editors and asset types available. The
GridSnake *module* also has to declare that its C++ code uses Paper2D's C++ classes. Only then
does UnrealBuildTool add Paper2D's header folders to the include path and link against it.

If you forget this line, the build fails in one of two ways, depending on your compiler and the
order of things. Either the compiler cannot find the header (MSVC reports error C1083, "Cannot
open include file: 'PaperFlipbookComponent.h'"), or the linker cannot find symbols such as
`UPaperFlipbookComponent::StaticClass` ("unresolved external symbol"). Both mean the same thing:
add the module to `Build.cs` and build again. UnrealBuildTool picks up the change on the next
build; regenerating project files is only needed so your IDE's code completion finds the headers.

### The gem

`Source/GridSnake/Gameplay/GSGem.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSGem.generated.h"

class UPaperFlipbookComponent;

/**
 * A collectible gem. Place three in every level. BP_GSGem picks the flipbook;
 * the C++ class builds the components and makes the gem bob.
 */
UCLASS()
class AGSGem : public AActor
{
	GENERATED_BODY()

public:
	AGSGem();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

	/** How far the sprite bobs up and down, in world units (1 unit = 1 pixel here). */
	UPROPERTY(EditAnywhere, Category = "Gem", meta = (ClampMin = "0"))
	float BobHeight = 1.5f;

	/** Bobs per second, in radians (2*pi = one full bob). */
	UPROPERTY(EditAnywhere, Category = "Gem", meta = (ClampMin = "0"))
	float BobSpeed = 4.0f;

private:
	/** Random start offset so that several gems do not bob in lockstep. */
	float BobPhase = 0.0f;

	float RunningTime = 0.0f;
};
```

- `class UPaperFlipbookComponent;` is a **forward declaration**. The header only stores a pointer,
  so it does not need the full class definition. Only the `.cpp`, which calls its functions,
  includes `PaperFlipbookComponent.h`. Doing this in every header keeps compile times down; it is
  the convention in Epic's own code.
- The component pointers are `VisibleAnywhere`: the Details panel shows them, and you can edit
  the *component's* properties, but you cannot replace the component itself. `EditAnywhere` on a
  component pointer would let a designer swap or clear it, which you never want.
- `BlueprintReadOnly` on components lets Blueprint graphs get them (to change the flipbook's
  colour, say) without being able to reassign the pointer.
- `BobHeight` and `BobSpeed` are `EditAnywhere`, so they can be set on the Blueprint's Class
  Defaults *and* on each placed gem. `meta = (ClampMin = "0")` stops the editor from accepting
  negative values. It does not affect C++ code that assigns the property.
- `BobPhase` and `RunningTime` are not `UPROPERTY`s at all: they are runtime state that nobody
  needs to edit or save. Plain members are fine for that.

`Source/GridSnake/Gameplay/GSGem.cpp`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSGem.h"
#include "GridSnake.h"
#include "Engine/CollisionProfile.h"
#include "PaperFlipbookComponent.h"

AGSGem::AGSGem()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Root);
	// Sprites draw in the X-Z plane. A small +Y offset puts the gem in front of
	// the tile map (the camera looks down -Y from +Y; see chapter 6).
	Flipbook->SetRelativeLocation(FVector(0.0f, 5.0f, 0.0f));
	Flipbook->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AGSGem::BeginPlay()
{
	Super::BeginPlay();

	BobPhase = FMath::FRandRange(0.0f, UE_TWO_PI);
	UE_LOG(LogGridSnake, Verbose, TEXT("%s spawned at %s"), *GetName(), *GetActorLocation().ToString());
}

void AGSGem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RunningTime += DeltaSeconds;
	const float Offset = FMath::Sin(RunningTime * BobSpeed + BobPhase) * BobHeight;

	FVector Location = Flipbook->GetRelativeLocation();
	Location.Z = Offset;
	Flipbook->SetRelativeLocation(Location);
}
```

Walking through it:

- **Includes.** The class's own header comes first, then `GridSnake.h` for the `LogGridSnake`
  category (chapter 2), then what the `.cpp` uses. Includes are relative to the module root
  (`"Gameplay/GSGem.h"`), thanks to `PublicIncludePaths.Add(ModuleDirectory)` in `Build.cs`.
  Paper2D headers have no folder prefix: `"PaperFlipbookComponent.h"` is the include path shown
  on its API page.
- **Draw order.** The flipbook sits 5 units in front of the actor along +Y. Every sprite in
  GridSnake lies in the X-Z plane, and the camera (chapter 6) looks down −Y from +Y. A larger Y
  is closer to the camera, so it draws on top. The course uses fixed offsets for that: the tile
  map at 0, gems and exit at +5, the snake's body at +8, effects at +9 and its head at +10. This
  offset is set in C++ so every gem gets it, no matter where you place it.
- **No collision yet.** `SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName)` turns
  collision off for the flipbook. Sprites otherwise generate collision from their sprite's
  collision geometry, and the gem should not block anything. Chapter 10 gives the gem a proper
  trigger box.
- **Random phase.** `BeginPlay` picks a random start point on the sine wave with
  `FMath::FRandRange(0, UE_TWO_PI)`. Without it, all gems would bob in lockstep, which looks
  mechanical. It is set in `BeginPlay`, not the constructor: the constructor also runs for the
  CDO, and a random value there would be one value shared by every gem.
- **Verbose logging.** The log line uses verbosity `Verbose`. `LogGridSnake` was declared with
  default verbosity `Log`, so `Verbose` lines are compiled in but filtered out. To see them, run
  the console command `log LogGridSnake Verbose` before you play (in the editor, the console box
  is at the bottom of the Output Log). This is how you keep detailed logging in the code without
  flooding the log.
- **The bob.** `Tick` adds `DeltaSeconds` to the gem's own clock and sets the flipbook's relative Z
  to `sin(time × BobSpeed + phase) × BobHeight`. With the defaults, the gem moves ±1.5 units
  (pixels) and a full bob takes 2π / 4 ≈ 1.57 seconds. Only the *component* moves; the actor's
  location stays on its cell.

Why keep a `RunningTime` instead of reading `GetWorld()->GetTimeSeconds()`? Both work here. The
accumulated `DeltaSeconds` follows the actor's own time: if someone gives this gem a
`CustomTimeDilation` of 0.5, it bobs at half speed, and if its tick is disabled for a while, it
resumes where it stopped instead of jumping.

### The exit

`Source/GridSnake/Gameplay/GSExit.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSExit.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;

/**
 * The level exit. Closed until every gem is collected, then open; entering an
 * open exit finishes the level (chapter 11).
 */
UCLASS()
class AGSExit : public AActor
{
	GENERATED_BODY()

public:
	AGSExit();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Exit")
	void SetOpen(bool bNewOpen);

	UFUNCTION(BlueprintPure, Category = "Exit")
	bool IsOpen() const { return bIsOpen; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Door;

	UPROPERTY(EditDefaultsOnly, Category = "Exit")
	TObjectPtr<UPaperFlipbook> ClosedFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "Exit")
	TObjectPtr<UPaperFlipbook> OpenFlipbook;

	/** Tick this on a placed exit to preview (or test) the open state. */
	UPROPERTY(EditAnywhere, Category = "Exit")
	bool bIsOpen = false;

private:
	void UpdateVisuals();
};
```

The exit holds **asset references**: `ClosedFlipbook` and `OpenFlipbook` point at flipbook assets
(`UPaperFlipbook`), while `Door` is the component that shows one of them. The specifiers decide
where each can be edited:

| Specifier | Class Defaults (Blueprint editor) | Placed instance (level Details) | Used for |
|---|---|---|---|
| `VisibleAnywhere` | shown, read-only | shown, read-only | component pointers |
| `EditAnywhere` | editable | editable | `bIsOpen`, the gem's `BobHeight` |
| `EditDefaultsOnly` | editable | hidden | `ClosedFlipbook`, `OpenFlipbook` |
| `EditInstanceOnly` | hidden | editable | per-placement data (not used yet) |

The flipbooks are `EditDefaultsOnly` because they belong to the *kind* of exit (every exit of
`BP_GSExit` looks the same), not to one placed exit. `bIsOpen` is `EditAnywhere` so you can tick it
on one placed exit to preview the open state.

`SetOpen` is a `UFUNCTION(BlueprintCallable)`, so a Blueprint can call it. `IsOpen` is
`BlueprintPure`: a getter with no side effects, which shows up in Blueprint graphs as a node with
no execution pins.

`Source/GridSnake/Gameplay/GSExit.cpp`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSExit.h"
#include "GridSnake.h"
#include "Engine/CollisionProfile.h"
#include "PaperFlipbookComponent.h"

AGSExit::AGSExit()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Door = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Door"));
	Door->SetupAttachment(Root);
	Door->SetRelativeLocation(FVector(0.0f, 5.0f, 0.0f));
	Door->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AGSExit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs in the editor whenever the actor is placed, moved or edited, so the
	// level shows the right door state without pressing Play.
	UpdateVisuals();
}

void AGSExit::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogGridSnake, Log, TEXT("%s starts %s"), *GetName(), bIsOpen ? TEXT("open") : TEXT("closed"));
}

void AGSExit::SetOpen(bool bNewOpen)
{
	if (bIsOpen == bNewOpen)
	{
		return;
	}

	bIsOpen = bNewOpen;
	UpdateVisuals();
	UE_LOG(LogGridSnake, Log, TEXT("%s is now %s"), *GetName(), bIsOpen ? TEXT("open") : TEXT("closed"));
}

void AGSExit::UpdateVisuals()
{
	UPaperFlipbook* Wanted = bIsOpen ? OpenFlipbook : ClosedFlipbook;
	if (Wanted)
	{
		Door->SetFlipbook(Wanted);
		Door->PlayFromStart();
	}
}
```

- `PrimaryActorTick.bCanEverTick = false`: the exit changes only when told to, so it never ticks.
  Its `Door` component still ticks to animate the open flipbook; that is the component's own tick.
- `OnConstruction` calls `UpdateVisuals`, so the door shows the right flipbook in the editor as
  soon as you place the exit or toggle **Is Open**. The result is stored in the `Door` component's
  properties and saved with the level, so a packaged game loads the exit already showing the right
  frame.
- `SetOpen` is the runtime path: it changes the state, updates the visuals and logs. The early
  `return` makes it safe to call twice; chapter 11's game rules rely on that.
- `UpdateVisuals` does nothing if the wanted flipbook is not set. That is the case for the C++
  class itself, which has no art. Only the Blueprint subclass sets the flipbooks. Checking for
  null instead of assuming the asset is there means a missing asset gives an empty door, not a
  crash.
- `SetFlipbook` swaps the flipbook asset; `PlayFromStart` restarts its animation at frame 0, so the
  opening animation does not start somewhere in the middle.

### Two ways to pick the art

The gem and the exit get their flipbooks in two different ways, on purpose:

- **The gem** uses the component's own property. `UPaperFlipbookComponent` has a **Source Flipbook**
  property. `BP_GSGem` sets it on the inherited `Flipbook` component. No C++ property is needed,
  because a gem only ever shows one flipbook in this chapter.
- **The exit** has two states, so it declares its own asset properties (`ClosedFlipbook`,
  `OpenFlipbook`) and copies the right one into the component in code.

Use the first when a component shows one fixed asset; use the second when code must choose
between assets at runtime.

## Blueprint subclasses as data

The C++ classes have no art. That is deliberate. The pattern used for every GridSnake actor:

- **C++** declares the components, the properties and the behaviour.
- A **Blueprint subclass** (`BP_GSGem`, `BP_GSExit`) fills in the asset references: which
  flipbook, which sound, which material. It usually has no graph logic at all.

A Blueprint subclass of a C++ class is a real subclass: `BP_GSGem` *is an* `AGSGem`, so every C++
function works on it, and C++ code can spawn it through a `TSubclassOf<AGSGem>` (later in this
chapter). The Blueprint's **Class Defaults** are simply its CDO's property values, saved as an
asset.

Why not set the art in C++? You can. `ConstructorHelpers` loads an asset by path in a constructor:

```cpp
// Possible, but not what GridSnake does.
#include "UObject/ConstructorHelpers.h"

AGSGem::AGSGem()
{
	// ...
	static ConstructorHelpers::FObjectFinder<UPaperFlipbook> IdleFlipbook(
		TEXT("/Game/GridSnake/Art/Flipbooks/FB_Gem_Idle.FB_Gem_Idle"));
	if (IdleFlipbook.Succeeded())
	{
		Flipbook->SetFlipbook(IdleFlipbook.Object);
	}
}
```

It works, but it has costs:

- The path is a string. Rename or move the asset in the editor and the code breaks silently
  (`Succeeded()` returns false, and the gem is invisible).
- The asset is loaded when the CDO is constructed, at module start-up, whether or not a level uses
  it.
- Changing the art needs a C++ build. With a Blueprint subclass, an artist changes a dropdown.

So GridSnake keeps content choices in content. C++ refers to classes and assets through
properties, Blueprints fill them in, and the editor keeps the references valid when assets move.

### Creating the Blueprints

These steps follow the snapshot's `project/ch05/ASSETS.md`.

1. Build the project with the new files: close the editor, build the **GridSnakeEditor** target
   from your IDE (Development Editor), and start the editor again. The C++ classes `GSGem` and
   `GSExit` appear in the Content Browser under **C++ Classes > GridSnake > Gameplay**. If you do
   not see a **C++ Classes** folder, open the Content Browser's **Settings** and enable **Show C++
   Classes** (the label may differ slightly in your version).
2. Right-click `GSGem` > **Create Blueprint class based on GSGem**.
   - Name it `BP_GSGem` and save it in `Content/GridSnake/Blueprints` (the folder from chapter 2).
   - The Blueprint editor opens. In the **Components** panel, select **Flipbook**.
   - In the **Details** panel, find the **Sprite** category and set **Source Flipbook** to
     `FB_Gem_Idle`. The viewport shows the gem.
   - Click **Compile**, then **Save**.
3. Create `BP_GSExit` from `GSExit` the same way.
   - Click **Class Defaults** in the toolbar. The Details panel shows the actor's own properties.
   - In the **Exit** category, set **Closed Flipbook** = `FB_Exit_Closed` and **Open Flipbook** =
     `FB_Exit_Open`.
   - Compile and save. The viewport may still show an empty door: `OnConstruction` runs for
     instances in a level, and the preview updates once you compile or move something.

The Details panel turns C++ names into display names: `ClosedFlipbook` becomes "Closed Flipbook",
and `bIsOpen` becomes "Is Open" (the `b` prefix of booleans is dropped).

While you are here, chapter 3's console command can confirm what reflection sees. Play, open the
console and run `GSDumpClass GSExit`: it lists the properties and functions that `AGSExit` itself
declares, including `SetOpen` marked `[BlueprintCallable]`.

## Placing actors

A Blueprint class is a template. To put gems in the level, you place **instances** of it.

1. Open `L_Level01`.
2. Drag `BP_GSGem` from the Content Browser into the viewport three times, and `BP_GSExit` once.
3. Select each one and type its location into the **Details** panel, under **Transform >
   Location**. Use Y = 0, and X and Z multiples of 16, for example:
   - gems at (16, 0, −16), (48, 0, −32) and (80, 0, −48);
   - the exit at (112, 0, −16).

   X grows to the right and Z grows upwards. Negative Z is *down*: chapter 6 explains why the
   board's rows grow towards −Z. These positions are temporary; chapter 6 moves every actor onto
   a cell of the tile map.
4. Notice the sprites sit slightly in front of Y = 0. That is the flipbook's +5 relative offset
   from the C++ constructor. Every gem gets it without you doing anything.
5. Save the level (**Ctrl+S**).

Snapping helps with step 3. In the viewport toolbar, the grid snap button sets the move step;
chapter 4 suggested 16. With snapping on, dragging the gizmo moves in whole tiles.

### Seeing it move

The default view when you press **Play** is still a first-person pawn standing at the origin and
looking along +X. It sees the sprites edge-on, so you see nothing useful. Chapter 7 fixes that
with a proper camera. For now, use **Simulate**:

1. Switch the viewport to the orthographic view that shows sprites face-on (**Left** or **Right**,
   whichever has +X pointing right; see chapter 4).
2. Start **Simulate** (the **⋮** menu next to Play > **Simulate**, or Alt+S).
3. The gems bob, each at its own phase, and sparkle (`FB_Gem_Idle` loops at 8 frames per second).
   The exit shows its single closed frame.
4. While simulating, select a gem and change **Bob Height** to `4`: the bob grows immediately.
   Changes made during Simulate are thrown away when you stop, unless you right-click the actor
   and choose **Keep Simulation Changes** (the label may differ slightly in your version).
5. Stop simulating (**Esc** or the **Stop** button).

Expected Output Log (filter: `LogGridSnake`; actor names are illustrative):

```
LogGridSnake: GSGameInstance initialised: 3 gems per level, step 0.15s
LogGridSnake: BP_GSExit_C_0 starts closed
LogGridSnake: GridSnake game mode started on map UEDPIE_0_L_Level01 (collect 3 gems)
```

- The exit's line comes *before* the game mode's. `AGameModeBase::StartPlay` is what tells the
  world to begin play, and `AGSGameMode::StartPlay` (chapter 2) calls `Super::StartPlay()` before
  logging. So every placed actor's `BeginPlay` has already run when the game mode logs.
- The object name of a placed Blueprint instance is the generated class name (`BP_GSExit_C`) plus
  a number. The **Outliner** shows a different string, the actor's **label** (`BP_GSExit`), which
  you can rename freely. Labels exist only in the editor. `GetName()` returns the object name;
  `GetActorNameOrLabel()` returns the label in editor builds and the name otherwise, which is
  handy for log lines meant for humans.
- The gems log nothing at the default verbosity. Run `log LogGridSnake Verbose` in the console,
  simulate again, and three more lines appear, such as
  `LogGridSnake: Verbose: BP_GSGem_C_0 spawned at X=16.000 Y=0.000 Z=-16.000`.

Now try the exit's construction hook. Stop simulating, select the exit and tick **Is Open** in the
Details panel (category **Exit**). The door switches to the looping open animation in the editor
viewport, without Play. Untick it to go back. Each toggle runs `OnConstruction`, which calls
`UpdateVisuals`. Leave it unticked: the level should start with a closed exit.

## Spawning at runtime

Placed actors are one way to get actors into a world. The other is to **spawn** them while the game
runs. GridSnake spawns pop effects in chapter 12 and the player's snake in chapter 7 (the game mode
does that for you). Here is the general mechanism.

`examples/ch05/ExSpawner.h`

```cpp
// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSpawner.generated.h"

/**
 * Spawns Count actors of ClassToSpawn in a row when play starts. Each spawned
 * actor removes itself after LifeSpan seconds.
 */
UCLASS()
class AExSpawner : public AActor
{
	GENERATED_BODY()

public:
	AExSpawner();

	/** Any actor class: a C++ class or a Blueprint such as BP_GSGem. */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	TSubclassOf<AActor> ClassToSpawn;

	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0", ClampMax = "100"))
	int32 Count = 3;

	/** Distance between two spawned actors, along +X. */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	float Spacing = 32.0f;

	/** Seconds until each spawned actor is destroyed. 0 keeps them forever. */
	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0"))
	float LifeSpan = 3.0f;

protected:
	virtual void BeginPlay() override;
};
```

`examples/ch05/ExSpawner.cpp`

```cpp
// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExSpawner.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogExSpawn, Log, All);

AExSpawner::AExSpawner()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AExSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!ClassToSpawn)
	{
		UE_LOG(LogExSpawn, Warning, TEXT("%s: set Class To Spawn in the Details panel"), *GetName());
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	// Spawn even if the new actor's collision overlaps something.
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Location = GetActorLocation() + FVector(Spacing * Index, 0.0f, 0.0f);
		AActor* Spawned = GetWorld()->SpawnActor<AActor>(ClassToSpawn, Location, FRotator::ZeroRotator, Params);
		if (!Spawned)
		{
			UE_LOG(LogExSpawn, Warning, TEXT("Spawning %s failed"), *ClassToSpawn->GetName());
			continue;
		}

		if (LifeSpan > 0.0f)
		{
			Spawned->SetLifeSpan(LifeSpan);
		}
		UE_LOG(LogExSpawn, Log, TEXT("Spawned %s at %s"), *Spawned->GetName(), *Location.ToString());
	}
}
```

To try it in GridSnake: drag `ExSpawner` into `L_Level01` at (0, 0, −96). In its Details panel,
set **Class To Spawn** to `BP_GSGem`. Simulate.

Expected Output Log (filter: `LogExSpawn`). The `<n>` numbers vary: the engine picks unique names,
and they may be small numbers or large ones.

```
LogExSpawn: Spawned BP_GSGem_C_<n> at X=0.000 Y=0.000 Z=-96.000
LogExSpawn: Spawned BP_GSGem_C_<n> at X=32.000 Y=0.000 Z=-96.000
LogExSpawn: Spawned BP_GSGem_C_<n> at X=64.000 Y=0.000 Z=-96.000
```

Three bobbing gems appear in a row and vanish three seconds later. Delete the spawner from the
level when you are done: it is not part of GridSnake.

- **`TSubclassOf<AActor>`** is a class reference restricted to `AActor` and its subclasses. In the
  Details panel it is a dropdown that lists only valid classes, Blueprints included. A plain
  `UClass*` property would accept any class; `TSubclassOf` gives you a type check in the editor
  and in C++. In GridSnake you will see `TSubclassOf<AGSGem>`-style properties that accept only the
  right family of classes.
- **`GetWorld()->SpawnActor<T>(Class, Location, Rotation, Params)`** creates the actor in this
  world and runs its construction sequence (constructor, `OnConstruction`,
  `PostInitializeComponents`, `BeginPlay`) before returning. The template argument is the C++ type
  of the returned pointer; the `Class` argument may be a subclass, such as a Blueprint. It returns
  `nullptr` if the spawn fails, so check it.
- **`FActorSpawnParameters`** holds the options. `Owner` records who spawned it.
  `SpawnCollisionHandlingOverride` decides what happens if the new actor would overlap blocking
  collision: `AlwaysSpawn` ignores it; other values try to move the actor or refuse to spawn.
- **`SetLifeSpan(Seconds)`** destroys the actor after a delay. `Destroy()` destroys it right away:
  it gets `EndPlay`, is removed from the level, and the garbage collector frees it later.
- After `Destroy()`, any pointer you still hold points at an object that is being torn down. Test
  with `IsValid(Actor)`, which is false for null *and* for actors pending destruction. Chapter 3's
  rule applies: store such pointers in `UPROPERTY` members or use `TWeakObjectPtr` so the engine
  can clear them.
- `SpawnActorDeferred<T>` plus `FinishSpawning` is the variant for setting properties *before*
  `OnConstruction` and `BeginPlay` run. GridSnake does not need it.

## Summary

- An actor is a `UObject` that lives in a world. It is placed in a level or spawned, and it stays
  until destroyed. Its transform is its root component's transform.
- Components add capabilities: `UActorComponent` for logic, `USceneComponent` for a transform and
  attachment, `UPrimitiveComponent` for anything that draws or collides.
- Create default components in the constructor with `CreateDefaultSubobject`, `SetRootComponent`
  and `SetupAttachment`. Keep their pointers in `VisibleAnywhere` `TObjectPtr` properties.
- The constructor runs for the CDO too: only defaults and components there. `OnConstruction` is
  for what the editor should show. `BeginPlay` is for game logic, `EndPlay` for clean-up.
- PIE duplicates the editor world, so placed actors are constructed again but do not rerun
  `OnConstruction`. Spawned actors run the whole sequence.
- Ticking is opt-in (`PrimaryActorTick.bCanEverTick`), happens only in game worlds, and must scale
  by `DeltaSeconds`. Components tick independently.
- C++ classes define structure and behaviour; Blueprint subclasses pick the assets. Avoid
  hard-coded asset paths.
- `TSubclassOf`, `SpawnActor`, `SetLifeSpan`, `Destroy` and `IsValid` cover runtime creation and
  removal.
- GridSnake now has `AGSGem` (bobbing, animated) and `AGSExit` (closed/open with an editor
  preview), their Blueprints, and three gems and an exit in `L_Level01`.

## Exercises

1. Gems already start at a random point of their bob. Add a `bRandomPhase` property (default true)
   so that a designer can switch it off for one gem, or for all gems in `BP_GSGem`, and make them
   bob in sync.

<details><summary>Solution</summary>

In `GSGem.h`, next to the other gem properties:

```cpp
	/** Off: every gem starts its bob at the same point, so they move in sync. */
	UPROPERTY(EditAnywhere, Category = "Gem")
	bool bRandomPhase = true;
```

In `GSGem.cpp`, `BeginPlay`:

```cpp
	BobPhase = bRandomPhase ? FMath::FRandRange(0.0f, UE_TWO_PI) : 0.0f;
```

Build, untick **Random Phase** on the `BP_GSGem` Class Defaults, and simulate: the three gems now
move together. Because the property is `EditAnywhere`, you can also untick it on single placed
gems. A placed gem keeps its own value only if you changed it there; otherwise it follows the
Blueprint default. The Details panel shows a yellow "reset" arrow next to values that differ from
the default.

</details>

2. The exit never ticks. Use `dumpticks` to compare a gem and the exit while simulating. Then make
   gems tick only 30 times per second and see whether you can tell the difference.

<details><summary>Solution</summary>

Start Simulate, open the console (`` ` ``) and run `dumpticks`. Filter the Output Log for
`BP_GSGem` and then for `BP_GSExit`. Each gem has an actor tick function and a tick function for
its `Flipbook` component. The exit has only its `Door` component's tick: `bCanEverTick = false`
means the actor never registered a tick function, but the flipbook component still ticks to
animate.

To lower the gems' tick rate, add one line to the `AGSGem` constructor:

```cpp
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 30.0f;
```

With an interval, `Tick` receives the time since the gem's previous tick, so `RunningTime` still
advances at the right speed. On a 60 Hz display the bob now updates every other frame; with
movements of a pixel or two, it is very hard to see. Tick intervals are a cheap way to save time
on actors whose updates do not need every frame. Remove the line again afterwards; the course
keeps the default.

</details>

3. Add an editor-only arrow to the exit that shows which way it faces. It must not exist in a
   packaged game.

<details><summary>Solution</summary>

`UArrowComponent` draws an arrow in the editor and is hidden in game by default. Create it with
`CreateEditorOnlyDefaultSubobject`, which returns null in builds without editor data, and wrap the
member in `WITH_EDITORONLY_DATA` so it does not exist there at all.

`GSExit.h`, in the `protected` section:

```cpp
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class UArrowComponent> Arrow;
#endif
```

`GSExit.cpp`: add `#include "Components/ArrowComponent.h"` and, at the end of the constructor:

```cpp
#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Root);
		// The arrow points along its local +X. Rotate the component to aim it.
		Arrow->SetRelativeLocation(FVector(0.0f, 6.0f, 0.0f));
	}
#endif
```

Build, and every exit in the level shows an arrow pointing right (+X). Select the **Arrow**
component on one exit and change its rotation's **Pitch** to 90 or −90 to aim it up or down in the
screen plane (pitch, as with `ExSpinner`). The arrow is not drawn in PIE, and the component is not
cooked into packaged builds. `UArrowComponent` is in the Engine module, so `Build.cs` needs no
change.

</details>

4. Spawn one extra gem from the game mode when play starts, at (80, 0, −80), with the class chosen
   in `BP_GSGameMode`.

<details><summary>Solution</summary>

`Core/GSGameMode.h`: forward-declare the gem and add a class property.

```cpp
class AGSGem;

UCLASS()
class AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	virtual void StartPlay() override;

protected:
	/** If set, one gem of this class is spawned when play starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	TSubclassOf<AGSGem> TestGemClass;
};
```

`Core/GSGameMode.cpp`: include the gem's header and spawn after `Super::StartPlay()`.

```cpp
#include "Gameplay/GSGem.h"

void AGSGameMode::StartPlay()
{
	Super::StartPlay();

	if (TestGemClass)
	{
		AGSGem* Gem = GetWorld()->SpawnActor<AGSGem>(TestGemClass, FVector(80.0f, 0.0f, -80.0f), FRotator::ZeroRotator);
		UE_LOG(LogGridSnake, Log, TEXT("Spawned test gem %s"), *GetNameSafe(Gem));
	}

	UE_LOG(LogGridSnake, Log, TEXT("GridSnake game mode started on map %s (collect %d gems)"),
		*GetWorld()->GetMapName(), UGSSettings::Get()->GemsPerLevel);
}
```

Build, open `BP_GSGameMode`, click **Class Defaults**, and set **Debug > Test Gem Class** to
`BP_GSGem`. The dropdown lists only `AGSGem` and its subclasses. Simulate: a fourth gem appears,
and the log shows `LogGridSnake: Spawned test gem BP_GSGem_C_<n>`. `GetNameSafe` prints `None`
instead of crashing if the spawn failed. The world has already begun play when `Super::StartPlay()`
returns, so the new gem's `BeginPlay` runs inside `SpawnActor`. Undo the change afterwards; levels
place their own gems.

</details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-5--actors-and-components)
