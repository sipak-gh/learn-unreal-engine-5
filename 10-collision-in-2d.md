# Chapter 10 — Collision in 2D

Unreal's collision is 3D, even in a 2D game. A Paper2D sprite is a flat picture in the X-Z plane,
but anything that collides is a box, capsule or extruded shape with depth along Y. In this chapter
you learn the parts of the collision system that every game uses: **object types** and **trace
channels**, the three **responses** (Block, Overlap, Ignore), **profiles**, **overlap events**
and **scene queries**. Then GridSnake uses them. Walls become solid through a custom `Grid`
channel, the closed exit becomes a wall, and gems and the exit turn into triggers that notice the
snake's head.

Previous: [Chapter 9](09-time-ticks-and-timers.md). Next: [Chapter 11](11-interfaces-delegates-and-game-rules.md).

> **Reminder.** Nothing in this course was compiled or run by the authors (see the README,
> "How this course was checked"). Every *Expected Output Log* block shows the lines the code's
> `UE_LOG` calls will print. Parts in `<angle brackets>` vary on your machine, and blocks marked
> *illustrative* show the shape of the output, not exact numbers.

## Collision in a 2D game

GridSnake's camera looks along −Y at the X-Z plane (chapter 6). Every sprite sits at some small Y
offset: the tile map at 0, gems and the exit at +5, the snake's body at +8 and its head at +10.
Those offsets decide what is drawn in front. They also mean that two flat shapes at different Y
values never touch, even when they cover the same spot on screen.

So every collision shape in a 2D Unreal game gets **depth along Y**. The snake's head box has a Y
half-extent of 16, the gem and exit triggers 16 as well, and the board's wall query reaches 50
units in each direction. A box 16 units deep at Y = 0 reaches from −16 to +16 and meets every
other box near the board plane, whatever the sprite offsets are. When a collision test that
"obviously" should succeed fails in a 2D game, check the Y extents first.

### Sprite and tile collision in Paper2D

Paper2D can generate collision for sprites and tile maps. Epic's sprite collision page describes
the **Sprite Collision Domain** setting, which you find on sprite assets and on tile maps:

| Sprite Collision Domain | Meaning |
|---|---|
| **None** | No collision at all. |
| **Use 2D Physics** | Collision in a separate 2D physics scene (Box2D). Epic marks it experimental. Ordinary 3D queries and overlaps do not see it. |
| **Use 3D Physics** | The 2D shape is extruded along Y into a 3D shape, **Collision Thickness** units deep, and takes part in normal 3D collision. |

GridSnake uses **Use 3D Physics** everywhere. It is the setting that works with the rest of the
engine: the line traces, overlap tests and trigger boxes in this chapter are all 3D. Chapter 6 kept it, the
default, on `TM_Level01` (**Details > Collision**).

**Tile collision** comes from the tile set. In chapter 6 you drew collision boxes on the wall
tiles of `TS_Dungeon` (tiles 4–7). A tile map builds its collision from the tiles on every layer
that has **Layer Collides** ticked, extruded by the map's **Collision Thickness**. The floor tiles
have no collision boxes, so floor cells produce no collision geometry.

Sprites can also simulate physics; Epic's Paper 2D physics page shows how to constrain them to
the 2D plane so they do not fall or turn out of it. GridSnake simulates nothing. The snake, gems
and exit do not use sprite collision either. Their sprite and flipbook components all
use the `NoCollision` profile, and each actor owns a plain `UBoxComponent` for its logic. A box
with an exact size is easier to reason about than a shape traced around a picture, and it keeps
"what you see" separate from "what the game tests".

## Channels, responses and profiles

Every primitive component (boxes, sprites, meshes, tile maps) carries a set of collision
settings. Open any of them in the Details panel, expand **Collision > Collision Presets**, and you
see the same grid of tick boxes. Here is what it means.

### Object types and trace channels

A **channel** is a category in the collision system. There are two kinds:

- **Object types** say what a component *is*. Each component has exactly one: `WorldStatic`
  (walls, floors), `WorldDynamic` (things that move but are not characters), `Pawn`,
  `PhysicsBody`, and a few more. In C++ they are `ECC_WorldStatic`, `ECC_WorldDynamic`,
  `ECC_Pawn` and so on.
- **Trace channels** say what a *query* is looking for. `Visibility` ("can I see through
  this?") and `Camera` ("may the camera pass?") are built in: `ECC_Visibility` and `ECC_Camera`.
  A trace channel is not something a component *is*; it is a question components answer.

Both kinds share one enum, `ECollisionChannel`, and one list of **responses**. Every component
stores a response to every channel, object types and trace channels alike.

### Block, Overlap, Ignore

There are three responses, ordered from strongest to weakest:

| Response | Between two components | For a query on a trace channel |
|---|---|---|
| **Block** | They stop each other; hit events are possible. | The component stops the trace and is reported as a hit. |
| **Overlap** | They pass through each other; overlap events are possible. | The component is reported as an overlap (multi queries) but does not stop the trace. |
| **Ignore** | Nothing happens. | The component is invisible to the query. |

When two components meet, each looks up its response to the *other's object type*, and the
weaker of the two wins. A trigger that overlaps `Pawn` and a pawn that blocks `WorldDynamic`
produce an overlap, not a block. A component that ignores the other side produces nothing,
whatever the other side says. This is the rule to remember when "my overlap does not fire":
**both sides must agree on at least Overlap**.

Overlap *events* need one more thing: **Generate Overlap Events** (`SetGenerateOverlapEvents`
in C++) must be on for **both** components. The response says that the two may overlap; the flag
says that someone wants to be told.

### Collision Enabled

Independently of responses, **Collision Enabled** decides which parts of the system see a
component at all. The C++ type is `ECollisionEnabled::Type`:

| Value | Meaning |
|---|---|
| `NoCollision` | Not in the collision system at all. The cheapest setting, used for every sprite in GridSnake. |
| `QueryOnly` | Traces, sweeps and overlaps see it, but the physics simulation does not. The right setting for triggers. |
| `PhysicsOnly` | The physics simulation sees it (rigid bodies, constraints), queries do not. |
| `QueryAndPhysics` | Both. |
| `ProbeOnly`, `QueryAndProbe` | Newer states for physics contacts that are reported without a collision response. GridSnake does not need them. |

GridSnake simulates no physics, so every collision component in it is `QueryOnly` or
`NoCollision`, apart from the board's tile map, which keeps its profile's default.

### Profiles

Setting dozens of responses by hand for every component would be tedious. A **profile** (shown
as **Collision Presets** in the editor) is a named bundle: Collision Enabled, an object type, and
a response to every channel. A few of the built-in ones, as Epic's collision response reference
page describes them:

| Profile | Description on Epic's page |
|---|---|
| `NoCollision` | No collision. |
| `BlockAll` | A `WorldStatic` object that blocks all actors by default. |
| `OverlapAll` | A `WorldStatic` object that overlaps all actors by default. |
| `BlockAllDynamic` | A `WorldDynamic` object that blocks all actors by default. |
| `OverlapAllDynamic` | A `WorldDynamic` object that overlaps all actors by default. |
| `OverlapOnlyPawn` | A `WorldDynamic` object that overlaps `Pawn`, `Camera` and `Vehicle`; other channels use their defaults. |
| `Pawn` | A `Pawn` object, for the capsule of a playable character or AI. |

For the "all" profiles the page adds: "All new custom channels will use its own default
response." Keep that sentence in mind for the next section. Open **Project Settings > Engine >
Collision > Preset** to see every profile's full settings.

In C++, `SetCollisionProfileName(Name)` applies a profile. `UCollisionProfile` has constants for
the common names, such as `UCollisionProfile::NoCollision_ProfileName` and
`UCollisionProfile::BlockAll_ProfileName` (include `"Engine/CollisionProfile.h"`). The
individual setters change one thing at a time:

| Call | Effect |
|---|---|
| `SetCollisionEnabled(ECollisionEnabled::QueryOnly)` | Collision Enabled. |
| `SetCollisionObjectType(ECC_WorldDynamic)` | The object type. |
| `SetCollisionResponseToAllChannels(ECR_Ignore)` | Every response at once. |
| `SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap)` | One response. |
| `SetGenerateOverlapEvents(true)` | The overlap events flag; not part of a profile. |

Order matters: applying a profile overwrites the responses, so call `SetCollisionProfileName`
first and adjust single channels afterwards. Once you change a single setting, the editor shows
the preset as **Custom**.

### The GridSnake matrix

Here is every collision component in GridSnake after this chapter. "Grid" is the custom channel
of the next section.

| Component | Collision Enabled | Object type | WorldStatic | WorldDynamic | Pawn | Visibility, Camera | Grid | Overlap events |
|---|---|---|---|---|---|---|---|---|
| Board `TileMap` | profile `BlockAll` | `WorldStatic` | Block | Block | Block | Block | **Block** | profile default |
| Gem `Trigger` | `QueryOnly` | `WorldDynamic` | Ignore | Ignore | **Overlap** | Ignore | Ignore | on |
| Exit `Trigger` | `QueryOnly` | `WorldDynamic` | Ignore | Ignore | **Overlap** | Ignore | Ignore | on |
| Exit `Blocker` | `QueryOnly` | `WorldStatic` | Ignore | Ignore | Ignore | Ignore | **Block** while closed | off |
| Snake `Collision` | `QueryOnly` | `Pawn` | Ignore | **Overlap** | Ignore | Ignore | Ignore | on |
| All sprites and flipbooks | `NoCollision` | | | | | | | |

Read it pair by pair:

- **Snake and gem.** The snake responds Overlap to `WorldDynamic` (the gem's type); the gem
  responds Overlap to `Pawn` (the snake's type). Both generate overlap events. Result: an
  overlap event when the head enters a gem's cell.
- **Snake and tile map.** The snake ignores `WorldStatic`. Result: nothing. The snake does not
  bump into walls through collision at all; the board is *asked* about walls instead.
- **Snake and blocker.** Both ignore each other. Nothing.
- **A query on Grid.** Only the tile map and a closed exit's blocker respond Block. Everything
  else ignores Grid. That is exactly the set of "cells the snake cannot enter".

## A custom trace channel: Grid

The built-in channels have meanings the engine already uses. `Visibility` is what the mouse
cursor and many gameplay traces test. `Camera` is what the spring arm tests. GridSnake needs its
own question: "may the snake's head enter this cell?" A **custom trace channel** called `Grid` is
that question.

Create it in the editor (the snapshot's `DefaultEngine.ini` already contains the result):

1. **Edit > Project Settings > Engine > Collision.**
2. Under **Trace Channels**, click **New Trace Channel...**.
3. **Name** `Grid`, **Default Response** **Ignore**. Click **Accept**.

The editor writes one line to `Config/DefaultEngine.ini`:

```ini
[/Script/Engine.CollisionProfile]
; GridSnake (ch10): Project Settings > Engine > Collision > New Trace Channel "Grid",
; Default Response Ignore. The first custom channel becomes ECC_GameTraceChannel1,
; which GridSnake.h names ECC_GSGrid. Ignore by default: only components that
; opt in (the tile map's walls, closed exits) block it.
+DefaultChannelResponses=(Channel=ECC_GameTraceChannel1,DefaultResponse=ECR_Ignore,bTraceType=True,bStaticObject=False,Name="Grid")
```

The editor writes only the header and the `+DefaultChannelResponses` line. Custom channels have no C++ names of their own. The engine
reserves a fixed set of enum values, `ECC_GameTraceChannel1` to `ECC_GameTraceChannel18`, and
hands them out in the order you create channels, for object types and trace channels alike. The
first one you create becomes `ECC_GameTraceChannel1`. `GridSnake.h` gives it a readable name:

```cpp
/**
 * The "Grid" trace channel from Project Settings > Engine > Collision. Anything
 * that blocks it is a cell the snake cannot enter. Custom channels are numbered
 * in the order they were created, so this must match DefaultEngine.ini.
 */
#define ECC_GSGrid ECC_GameTraceChannel1
```

If you had already created another custom channel in this project, `Grid` got a different
number. Read the `Channel=` value in the ini line and fix the `#define` to match.

**Why Default Response Ignore?** The default response is what every profile that does not
mention the channel uses, and no built-in profile mentions `Grid`. With **Block**, every
component in the level would block Grid unless told otherwise: a decorative mesh, a trigger
volume someone adds later, possibly even the Player Start's capsule. Each of them would turn its
cell into a wall without anyone noticing why. With **Ignore**, Grid is *opt-in*. A component
blocks it only when its code says so, and GridSnake says so in exactly two places: the tile map
and the exit's blocker. (Earlier drafts of this course used Block. Ignore is the safer default
and is what the snapshots use.)

As quoted above, `BlockAll` uses each custom channel's own default response, Ignore for Grid.
That is why the board sets the Grid response explicitly, right after the profile:

```cpp
	TileMapComponent = CreateDefaultSubobject<UPaperTileMapComponent>(TEXT("TileMap"));
	SetRootComponent(TileMapComponent);
	// Tiles with collision in the tile set become blocking geometry. Block the
	// Grid channel explicitly so wall cells are found by IsCellBlocked().
	TileMapComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	TileMapComponent->SetCollisionResponseToChannel(ECC_GSGrid, ECR_Block);
```

The tile map blocks Grid, but only where it has collision geometry, which means only on wall
tiles of colliding layers.

## Asking the world: scene queries

A **scene query** asks the collision world a question without moving anything. The `UWorld`
functions follow one naming scheme:

| Part | Choices |
|---|---|
| Shape | `LineTrace` (a ray), `Sweep` (a moving box, sphere or capsule), `Overlap` (a shape standing still) |
| Result | `Test` (a bool), `Single` (the first blocking hit), `Multi` (all overlaps up to and including the first block) |
| Filter | `ByChannel` (a trace channel's responses), `ByObjectType` (a list of object types), `ByProfile` |

So `LineTraceSingleByChannel` shoots a ray and returns the first component that blocks the given
channel. `OverlapBlockingTestByChannel` places a shape and returns `true` if any component
overlapping it *blocks* the channel. Epic's traces page walks through the single and multi
variants with pictures.

Every query takes an `FCollisionQueryParams`. The constructor GridSnake uses is
`FCollisionQueryParams(FName TraceTag, bool bTraceComplex, const AActor* IgnoreActor = nullptr)`:

- **`SCENE_QUERY_STAT(Name)`** for the tag. The macro declares a stat with that name, so the
  query appears under its own name in profiling tools instead of a generic bucket.
- **`bTraceComplex`** chooses per-triangle collision (`true`) or the simple collision shapes
  (`false`). Tile collision boxes are simple shapes, so `false`.
- **`IgnoreActor`** leaves one actor out, usually the caller. `AddIgnoredActor` adds more.

### IsCellBlocked

`AGSBoard` gets a query that answers the Grid question for one cell (`GSBoard.h`):

```cpp
	/**
	 * True if anything that blocks the Grid channel overlaps the cell: wall
	 * tiles (through their collision), closed exits, or any actor you set to
	 * block Grid.
	 * Asks the physics scene rather than the tile data.
	 */
	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsCellBlocked(FIntPoint Cell) const;
```

`GSBoard.cpp`, which now also includes `"Engine/CollisionProfile.h"` and `"Engine/World.h"`:

```cpp
bool AGSBoard::IsCellBlocked(FIntPoint Cell) const
{
	// A small box in the middle of the cell, deep enough (Y) to reach every layer.
	const float HalfSize = GetCellSize() * 0.25f;
	const FCollisionShape Probe = FCollisionShape::MakeBox(FVector(HalfSize, 50.0f, HalfSize));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSCellBlocked), /*bTraceComplex=*/false);
	return GetWorld()->OverlapBlockingTestByChannel(CellToWorld(Cell), FQuat::Identity, ECC_GSGrid, Probe, Params);
}
```

- **The probe is a quarter-cell box**, 4 units each way for 16-unit cells, centred on the cell.
  It is small enough never to touch the neighbouring cells, so a wall next door does not count.
  It is large enough to find a blocker that does not cover the exact centre.
- **Y half-extent 50.** Tile map layers may sit at slightly different Y values, and extruded
  collision has its own thickness. A probe 100 units deep reaches all of them.
- **`FCollisionShape::MakeBox`** takes *half* extents, like `UBoxComponent`.
- **`FQuat::Identity`**: the box is not rotated.
- **No ignored actor.** Nothing that should be ignored blocks Grid anyway.

The snake's `Step` now calls it instead of chapter 9's data check:

```cpp
	if (!Board->IsInside(NextCell) || Board->IsCellBlocked(NextCell))
	{
		Die(TEXT("hit a wall"));
		return;
	}
```

`IsInside` stays. Outside the board there is no tile map, so the query would find nothing there
and say "free".

### Example: a probe with a box and a ray

`ExGridProbe` runs both kinds of query from where it stands: an overlap test with a small box,
and a line trace to a point you drag in the viewport.

`examples/ch10/ExGridProbe.h`

```cpp
// Chapter 10 example: scene queries - a box overlap test and a line trace.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExGridProbe.generated.h"

/** Place in a level, drag the TraceEnd diamond onto something solid, press Play. */
UCLASS()
class AExGridProbe : public AActor
{
	GENERATED_BODY()

public:
	AExGridProbe();

	/** True if anything that blocks Channel overlaps a cube of HalfSize around Center. */
	UFUNCTION(BlueprintCallable, Category = "Probe")
	bool IsBlocked(FVector Center, float HalfSize, ECollisionChannel Channel) const;

protected:
	virtual void BeginPlay() override;

	/** The trace channel both queries use. Custom channels appear by their names. */
	UPROPERTY(EditAnywhere, Category = "Probe")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Half the side of the box tested at the actor's location. */
	UPROPERTY(EditAnywhere, Category = "Probe", meta = (ClampMin = "0.1"))
	float ProbeHalfSize = 4.0f;

	/** End of the line trace, relative to the actor. MakeEditWidget adds a viewport handle. */
	UPROPERTY(EditAnywhere, Category = "Probe", meta = (MakeEditWidget))
	FVector TraceEnd = FVector(100.0f, 0.0f, 0.0f);
};
```

`examples/ch10/ExGridProbe.cpp`

```cpp
// Chapter 10 example: scene queries - a box overlap test and a line trace.

#include "ExGridProbe.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogExProbe, Log, All);

AExGridProbe::AExGridProbe()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

bool AExGridProbe::IsBlocked(FVector Center, float HalfSize, ECollisionChannel Channel) const
{
	const FCollisionShape Probe = FCollisionShape::MakeBox(FVector(HalfSize));
	// The stat name shows up in profiling tools; the actor ignores itself.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ExGridProbe), /*bTraceComplex=*/false, this);
	return GetWorld()->OverlapBlockingTestByChannel(Center, FQuat::Identity, Channel, Probe, Params);
}

void AExGridProbe::BeginPlay()
{
	Super::BeginPlay();

	const FVector Start = GetActorLocation();
	const FVector End = GetActorTransform().TransformPosition(TraceEnd);
	const FString ChannelName = UEnum::GetValueAsString(TraceChannel.GetValue());

	UE_LOG(LogExProbe, Log, TEXT("Box at %s is %s on %s"), *Start.ToString(),
		IsBlocked(Start, ProbeHalfSize, TraceChannel) ? TEXT("blocked") : TEXT("free"), *ChannelName);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ExGridProbeTrace), /*bTraceComplex=*/false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params))
	{
		UE_LOG(LogExProbe, Log, TEXT("Hit %s (component %s) at distance %.1f"),
			*GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()), Hit.Distance);
		DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Red, false, 10.0f);
	}
	else
	{
		UE_LOG(LogExProbe, Log, TEXT("Nothing blocks %s between %s and %s"),
			*ChannelName, *Start.ToString(), *End.ToString());
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 10.0f);
	}
}
```

- **`meta = (MakeEditWidget)`** on an `FVector` property draws a small diamond handle in the
  level viewport when the actor is selected. Drag it, and the property updates. The value is
  relative to the actor, so `BeginPlay` converts it with `GetActorTransform().TransformPosition`.
- **`TEnumAsByte<ECollisionChannel>`** is how an old-style enum is stored in a `UPROPERTY`.
  Custom channels appear in the dropdown under the names you gave them.
- **`Hit.Distance`** is the distance from the start to the hit; `Hit.ImpactPoint` is where the
  ray touched. `Hit.GetActor()` and `Hit.GetComponent()` say what it hit.
- **`DrawDebugLine`** draws for 10 seconds: red to the hit, green when nothing was hit.
- **The member is called `TraceChannel`**, not `Channel`, so it does not hide the `Channel`
  parameter of `IsBlocked`.

To see it, in GridSnake (the example needs no other modules):

1. Copy both files into `Source/GridSnake/` and build. Open `L_Level01`.
2. Drag `ExGridProbe` from **C++ Classes > GridSnake** into the level. Set its **Location** to
   (32, 0, −32), the centre of cell (2, 2), next to the left border wall.
3. Select it. Set **Probe > Trace Channel** to **Grid**. Set **Trace End** to (−48, 0, 0), or drag
   its diamond to the left until it is inside the border wall.
4. Filter the Output Log for `LogExProbe` and press **Play**.

Expected Output Log (filter: `LogExProbe`; illustrative, assuming cell (2, 2) is floor and
column 0 is wall):

```text
LogExProbe: Box at X=32.000 Y=0.000 Z=-32.000 is free on ECC_GameTraceChannel1
LogExProbe: Hit <GSBoard_0> (component TileMap) at distance 24.0
```

The ray starts at X = 32 and goes left. The wall tile in column 0 covers X = −8 to 8 (tile
centres sit on multiples of 16), so the ray meets its collision at X = 8, 24 units from the start, if the tile's collision box covers the
whole tile. `UEnum::GetValueAsString` prints the enum's C++ name, so the custom channel shows as
`ECC_GameTraceChannel1`, not `Grid`.

Now try the differences between channels:

- Move the probe onto the closed exit (snap it to the exit's location). The box test says
  `blocked on ECC_GameTraceChannel1`. Switch **Trace Channel** to **Visibility** and play again:
  `free on ECC_Visibility`. The exit's blocker answers only the Grid question.
- With **Visibility**, the ray still hits the wall: `BlockAll` blocks every built-in channel.

### Traces and sweeps

`IsCellBlocked` could have been a line trace or a sweep:

- **A line trace** tests a single line; anything that does not cross it is missed (exercise 3).
- **A sweep** moves a shape from A to B: "what would I hit if I moved there?", the question of
  continuous movement.
- **An overlap test** asks "is anything there right now?", the natural fit for a cell.

### Why the snake does not use physics movement

Unreal can move actors with collision built in: `SetActorLocation(NewLocation, /*bSweep=*/true)`
stops the actor at the first blocking hit, and physics simulation moves bodies with forces.
Neither suits a grid game:

- A sweep that stops halfway leaves the snake *between* cells, and the code must undo the move.
- "May I enter this cell?" belongs *before* the move, so every game state stays on the grid.
- Physics simulation resolves contacts approximately; a grid game wants exact, repeatable rules.

So GridSnake asks first and teleports second. Collision is a *source of information*, not a way
of moving.

## Data checks vs collision queries

Chapter 9's `IsWallTile` and chapter 10's `IsCellBlocked` answer nearly the same question. Both
are still in `AGSBoard`:

| | `IsWallTile` (chapter 6) | `IsCellBlocked` (chapter 10) |
|---|---|---|
| Reads | The tile map's layers: a tile on any layer with Layer Collides | The collision scene: anything that blocks Grid |
| Sees actors | No | Yes: closed exits, and any actor you set to block Grid |
| Depends on | Layer flags only | Tile set collision boxes, Layer Collides, Collision Domain, the Grid channel |
| Cost | A few array reads | A physics query (still microseconds) |
| Fails when | A decorative layer is marked colliding | The tile set has no collision boxes, or the channel is wrong |

`IsWallTile` is exact and cheap, but it only knows about tiles. `IsCellBlocked` sees *anything*
that opted in to Grid. The closed exit needs no special code in the snake: it blocks Grid, so its
cell is a wall. A level designer can drop any blocking actor into a level and the snake respects
it.

One consequence for the `Floor` layer of chapter 9: its **Layer Collides** flag only matters to
`IsWallTile`. Floor tiles have no collision boxes in the tile set, so even a colliding floor layer
produces no geometry, and `IsCellBlocked` ignores it. Keep the flag unticked anyway; `IsWallTile`
is still there for exercises and tools.

### Self-collision stays data

The snake does **not** find its own body through collision. The body is drawn by instances of
one `NoCollision` component, and the check from chapter 9 stays exactly as it was:

```cpp
	const bool bTailMoves = PendingGrowth == 0;
	const int32 CellsToCheck = bTailMoves ? Segments.Num() - 1 : Segments.Num();
	for (int32 Index = 0; Index < CellsToCheck; ++Index)
	{
		if (Segments[Index] == NextCell)
		{
			Die(TEXT("bit itself"));
			return;
		}
	}
```

A collision version would need a box per segment, moved *before* the query, plus the tail
special case. `Segments` already has the exact answer. Use the game's rules where it has rules,
and collision where the world has shapes.

## Triggers: gems and the exit

A **trigger** is a component that overlaps things and blocks nothing, so that overlap events tell
you when something enters it. Gems and the exit get one each. Here is the gem's new constructor
(`Source/GridSnake/Gameplay/GSGem.cpp`):

```cpp
	// WorldDynamic trigger: ignores everything except the snake (a Pawn).
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->InitBoxExtent(FVector(6.0f, 16.0f, 6.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Trigger);
```

The `Trigger` replaces chapter 5's plain `USceneComponent` `Root` as the root; the header swaps
the property and forward-declares `class UBoxComponent;`:

```cpp
	/** Root. A trigger that overlaps the snake's head box. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Trigger;
```

- **`InitBoxExtent`** sets half extents in a constructor (`SetBoxExtent` at run time): a 12 × 12
  square, inside the 16-unit cell, 32 units deep.
- **Ignore all, then Overlap `Pawn`.** Starting from "ignore everything" makes the trigger
  invisible to everything you did not think of, including the Grid channel.
- **The bobbing stays visual.** The gem's `Tick` moves only the `Flipbook` component up and down.
  The trigger, the root, stays on the cell centre.

### Snapping to the grid

Gems are placed by hand, and a hand-placed gem is rarely exactly on a cell centre. The snake's
head always is. If a gem sits 7 units off, its trigger may never meet the head's box. So
`AGSBoard` gets a helper:

```cpp
	/** Moves an actor to the centre of the cell it is in and returns that cell. */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FIntPoint SnapActorToCell(AActor* Actor) const;
```

```cpp
FIntPoint AGSBoard::SnapActorToCell(AActor* Actor) const
{
	if (!Actor)
	{
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}

	const FIntPoint Cell = WorldToCell(Actor->GetActorLocation());
	Actor->SetActorLocation(CellToWorld(Cell));
	return Cell;
}
```

Gems and exits call it at the start of `BeginPlay`. Both `.cpp` files now include
`"Gameplay/GSBoard.h"` and `"Kismet/GameplayStatics.h"`:

```cpp
	// Hand-placed gems are rarely exactly on a cell centre; the snake's head is.
	if (const AGSBoard* Board = Cast<AGSBoard>(UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass())))
	{
		Board->SnapActorToCell(this);
	}
```

This is where snapping really starts. If chapter 6's asset notes made you expect gems to snap
already, they did not until now.

### The exit: a trigger and a blocker

The exit has the same `Trigger`, plus a second box that makes a closed exit a wall
(`GSExit.cpp`, constructor):

```cpp
	// Only ever answers Grid queries; physics and other traces ignore it.
	Blocker = CreateDefaultSubobject<UBoxComponent>(TEXT("Blocker"));
	Blocker->SetupAttachment(Trigger);
	Blocker->InitBoxExtent(FVector(6.0f, 16.0f, 6.0f));
	Blocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Blocker->SetCollisionObjectType(ECC_WorldStatic);
	Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
	Blocker->SetCollisionResponseToChannel(ECC_GSGrid, ECR_Block);
	Blocker->SetGenerateOverlapEvents(false);
```

The open state switches the blocker's Grid response:

```cpp
void AGSExit::UpdateCollision()
{
	Blocker->SetCollisionResponseToChannel(ECC_GSGrid, bIsOpen ? ECR_Ignore : ECR_Block);
}
```

`UpdateCollision` is called in the same two places as chapter 5's `UpdateVisuals`: at the end of
`OnConstruction`, so a placed exit has the right state in the editor, and in `SetOpen`, so
opening the exit at run time removes the wall. The header gains the `Trigger` and `Blocker`
properties and the private `void UpdateCollision();`.

Two boxes, because each has one job: the trigger *overlaps* the snake, the blocker *blocks*
Grid. One component could do both, but would be harder to read in the Details panel.

### The snake's head box

The snake's root box finally gets collision (`GSSnake.cpp`, constructor):

```cpp
	// The head's box: a Pawn that only overlaps WorldDynamic triggers (gems,
	// exits). It never blocks anything; walls are found by asking the board.
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->InitBoxExtent(FVector(4.0f, 16.0f, 4.0f));
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_Pawn);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	SetRootComponent(Collision);
```

In `BeginPlay`, once the board is known, the box is sized from the real cell size:

```cpp
	// A quarter-cell box sits well inside the head's cell, never touching neighbours.
	const float HalfSize = Board->GetCellSize() * 0.25f;
	Collision->SetBoxExtent(FVector(HalfSize, 16.0f, HalfSize));
```

With 16-unit cells the head box is ±4 and a gem trigger ±6 around their centres. In the same
cell they overlap. One cell apart, 16 units, they are 6 units short of touching. Only the head
has a box; the body has no collision, so a gem under the body's path never fires again.

### Reacting to overlaps

There are two ways to hear about overlaps:

- **On the actor:** override `NotifyActorBeginOverlap(AActor* OtherActor)` (and
  `NotifyActorEndOverlap`). It fires when *any* of the actor's components starts overlapping
  another actor. Simple, and enough when the actor has one collision component.
- **On a component:** bind to its `OnComponentBeginOverlap` and `OnComponentEndOverlap`
  delegates. The handler learns exactly which component of yours and which of theirs are
  involved. The example below uses these.

The snake uses the first (`GSSnake.h` declares
`virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;`):

```cpp
void AGSSnake::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	UE_LOG(LogGridSnake, Log, TEXT("Snake entered %s at cell %s"), *GetNameSafe(OtherActor), *GetHeadCell().ToString());
}
```

For now it only logs; chapter 11 adds collecting and finishing.

The call happens *during* the step. `Step` inserts the new head into `Segments`, then
`UpdateVisuals` calls `SetActorLocation`, which updates the head box's overlaps and runs
`NotifyActorBeginOverlap` before returning. That is why `GetHeadCell()` already reports the new
cell.

### Example: a trigger box that logs

`examples/ch10/ExTriggerBox.h`

```cpp
// Chapter 10 example: a trigger box that logs what enters and leaves it.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExTriggerBox.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
struct FHitResult;

/** Place across a moving actor's path and press Play: logs enter and leave. */
UCLASS()
class AExTriggerBox : public AActor
{
	GENERATED_BODY()

public:
	AExTriggerBox();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Box;

private:
	// Dynamic delegates find their handler by name through reflection, so
	// handlers must be UFUNCTIONs with exactly the delegate's parameters.
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
```

`examples/ch10/ExTriggerBox.cpp`

```cpp
// Chapter 10 example: a trigger box that logs what enters and leaves it.

#include "ExTriggerBox.h"
#include "Components/BoxComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogExTrigger, Log, All);

AExTriggerBox::AExTriggerBox()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->InitBoxExtent(FVector(32.0f, 32.0f, 32.0f));
	// A WorldDynamic object that overlaps every channel and blocks nothing.
	Box->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	// Not part of the profile: both components of a pair need it for events.
	Box->SetGenerateOverlapEvents(true);
	// Draw the wireframe while playing, not only in the editor.
	Box->SetHiddenInGame(false);
	SetRootComponent(Box);
}

void AExTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	Box->OnComponentBeginOverlap.AddDynamic(this, &AExTriggerBox::HandleBeginOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &AExTriggerBox::HandleEndOverlap);

	// Anything that was already inside before the bindings existed raised no event.
	TArray<AActor*> AlreadyInside;
	Box->GetOverlappingActors(AlreadyInside);
	UE_LOG(LogExTrigger, Log, TEXT("%s ready, %d actor(s) already inside"), *GetName(), AlreadyInside.Num());
}

void AExTriggerBox::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogExTrigger, Log, TEXT("%s entered (component %s, from sweep: %s)"),
		*GetNameSafe(OtherActor), *GetNameSafe(OtherComp), bFromSweep ? TEXT("yes") : TEXT("no"));
}

void AExTriggerBox::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogExTrigger, Log, TEXT("%s left (component %s)"), *GetNameSafe(OtherActor), *GetNameSafe(OtherComp));
}
```

- **The handler signature.** `OnComponentBeginOverlap` is a dynamic multicast delegate with six
  parameters. The handler must match them exactly:
  - `OverlappedComponent`: your component (`Box`).
  - `OtherActor`, `OtherComp`: the actor and the component that came in.
  - `OtherBodyIndex`: which body of `OtherComp`, for components with several (instanced meshes).
    Usually 0.
  - `bFromSweep`, `SweepResult`: whether the overlap was found by a sweeping move, and if so the
    hit details. A teleport has `bFromSweep == false` and an empty `SweepResult`.
  The end-overlap delegate has the first four only.
- **`UFUNCTION()` is required.** `AddDynamic` stores the function's *name* and finds it through
  reflection, so a handler that is not a `UFUNCTION` cannot be bound. Chapter 11 covers
  delegates in depth.
- **Bind in `BeginPlay`.** It runs once per actor in play, after the components exist. Binding
  in the constructor also works, but `BeginPlay` keeps the binding next to the code that uses it.
- **`OverlapAllDynamic` plus `SetGenerateOverlapEvents(true)`.** The profile sets responses; the
  flag is separate and explicitly turned on.
- **`SetHiddenInGame(false)`** shows the box's wireframe while playing. Collision components are
  hidden in game by default.

To see it, in GridSnake:

1. Copy both files into `Source/GridSnake/` and build. Open `L_Level01`.
2. Drag `ExTriggerBox` into the level at **Location** (280, 0, −112), a few cells right of the
   Player Start, on the snake's starting row. The box spans 64 units: four cells.
3. Filter the Output Log for `entered` (that catches both the example's and the snake's lines),
   press **Play**, and let the snake run right through the box.

Expected Output Log (filter: `entered`; illustrative):

```text
LogExTrigger: BP_GSSnake_C_0 entered (component Collision, from sweep: no)
LogGridSnake: Snake entered ExTriggerBox_<1> at cell X=15 Y=7
```

Clear the filter to see the other two lines:

```text
LogExTrigger: ExTriggerBox_<1> ready, <0> actor(s) already inside
LogExTrigger: BP_GSSnake_C_0 left (component Collision)
```

- **Both sides get told.** The trigger's delegate and the snake's `NotifyActorBeginOverlap` fire
  for the same overlap. Do not rely on their order.
- **The snake enters at cell 15** (box edge X = 248; the head box reaches 4 units) and leaves at
  cell 20. **`from sweep: no`**: `UpdateVisuals` moves without a sweep.
- **`already inside`** is usually 0. If a wall tile lies inside the box, the board counts too,
  because `OverlapAllDynamic` overlaps `WorldStatic`, and the tile map generates overlap events
  if its `Generate Overlap Events` flag is on.

### Example: report an actor's collision settings

When an overlap does not fire, the first question is "what are the settings *really*, at run
time?". This component prints them.

`examples/ch10/ExCollisionReport.h`

```cpp
// Chapter 10 example: a component that reports its owner's collision settings.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExCollisionReport.generated.h"

/** Add to any actor or Blueprint (Add > Ex Collision Report) and press Play. */
UCLASS(ClassGroup = (Debug), meta = (BlueprintSpawnableComponent))
class UExCollisionReport : public UActorComponent
{
	GENERATED_BODY()

public:
	UExCollisionReport();

protected:
	virtual void BeginPlay() override;

	/** The channel whose response is reported for every primitive component. */
	UPROPERTY(EditAnywhere, Category = "Report")
	TEnumAsByte<ECollisionChannel> ChannelToReport = ECC_Visibility;
};
```

`examples/ch10/ExCollisionReport.cpp`

```cpp
// Chapter 10 example: a component that reports its owner's collision settings.

#include "ExCollisionReport.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogExCollision, Log, All);

namespace
{
	// ECollisionEnabled is an old-style namespaced enum (ECollisionEnabled::Type).
	// A switch gives short names; newer "probe" values share the default branch.
	const TCHAR* CollisionEnabledName(ECollisionEnabled::Type Enabled)
	{
		switch (Enabled)
		{
		case ECollisionEnabled::NoCollision:     return TEXT("NoCollision");
		case ECollisionEnabled::QueryOnly:       return TEXT("QueryOnly");
		case ECollisionEnabled::PhysicsOnly:     return TEXT("PhysicsOnly");
		case ECollisionEnabled::QueryAndPhysics: return TEXT("QueryAndPhysics");
		default:                                 return TEXT("Other (probe)");
		}
	}
}

UExCollisionReport::UExCollisionReport()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UExCollisionReport::BeginPlay()
{
	Super::BeginPlay();

	TArray<UPrimitiveComponent*> Primitives;
	GetOwner()->GetComponents(Primitives);
	UE_LOG(LogExCollision, Log, TEXT("%s has %d primitive component(s)"), *GetOwner()->GetName(), Primitives.Num());

	const FString ChannelName = UEnum::GetValueAsString(ChannelToReport.GetValue());
	for (const UPrimitiveComponent* Primitive : Primitives)
	{
		UE_LOG(LogExCollision, Log, TEXT("  %s: profile %s, %s, object type %s, overlap events %s, %s -> %s"),
			*Primitive->GetName(),
			*Primitive->GetCollisionProfileName().ToString(),
			CollisionEnabledName(Primitive->GetCollisionEnabled()),
			*UEnum::GetValueAsString(Primitive->GetCollisionObjectType()),
			Primitive->GetGenerateOverlapEvents() ? TEXT("on") : TEXT("off"),
			*ChannelName,
			*UEnum::GetValueAsString(Primitive->GetCollisionResponseToChannel(ChannelToReport)));
	}
}
```

- **`UCLASS(ClassGroup = (Debug), meta = (BlueprintSpawnableComponent))`** makes the component
  appear in a Blueprint's **Add** menu, under its class group.
- **`GetComponents(Primitives)`** fills an array with every component of that type the owner
  has. `UPrimitiveComponent` is the base of everything that renders or collides.
- **`ECollisionEnabled::Type`** is an older, namespaced enum style. The small `switch` prints
  short names and sends the probe states to the `default` branch.
- **The getters** mirror the setters: `GetCollisionProfileName`, `GetCollisionEnabled`,
  `GetCollisionObjectType`, `GetGenerateOverlapEvents` and
  `GetCollisionResponseToChannel`.

To see it:

1. Copy both files into `Source/GridSnake/` and build.
2. Open `BP_GSSnake`. In the **Components** panel, click **Add**, search for **Ex Collision
   Report**, and add it. Compile and save.
3. Filter the Output Log for `LogExCollision` and press **Play**.

Expected Output Log (filter: `LogExCollision`; the component order and the values shown in
`<angle brackets>` may differ):

```text
LogExCollision: BP_GSSnake_C_0 has 3 primitive component(s)
LogExCollision:   Collision: profile <Custom>, QueryOnly, object type ECC_Pawn, overlap events on, ECC_Visibility -> ECR_Ignore
LogExCollision:   Head: profile NoCollision, NoCollision, object type <ECC_WorldStatic>, overlap events <on>, ECC_Visibility -> ECR_Ignore
LogExCollision:   Body: profile NoCollision, NoCollision, object type <ECC_WorldStatic>, overlap events <on>, ECC_Visibility -> ECR_Ignore
```

- **`Custom`** is the profile name once single settings have been changed.
- Set **Channel To Report** to **Grid** on the component in `BP_GSSnake` and play again: every
  line ends in `ECC_GameTraceChannel1 -> ECR_Ignore`. The snake never blocks its own wall query.
  Remove the component when you are done.

## When overlaps fire

Overlap events are not continuous checks. The engine updates a component's overlaps at specific
moments, and knowing them saves debugging time.

- **When a component moves.** `SetActorLocation` and the other movement functions update the
  overlaps of the moved components when they finish. A move *without* a sweep (a teleport) still
  updates overlaps at the destination; it just cannot notice anything it jumped over, and
  `bFromSweep` is false. GridSnake moves one cell per step, so it never jumps over a trigger.
  (We could not run this; if no events arrive, work through the table at the end of the
  chapter.)
- **When collision settings change.** Changing responses or the overlap flag can create or end
  overlaps. Several setters take a `bUpdateOverlaps` argument, such as
  `SetBoxExtent(Extent, bUpdateOverlaps)` and `SetCollisionProfileName(Name, bUpdateOverlaps)`.
- **When an actor starts inside a trigger.** This is the tricky case. A gem placed under the
  Player Start, or a trigger that begins play around an actor, may or may not raise a begin-overlap
  event, depending on how the actor came into the world. Placed actors follow their
  **Update Overlaps Method During Level Streaming** property (Details panel, advanced
  **Collision** section). Two robust habits:
  - Query current overlaps in `BeginPlay`, as `ExTriggerBox` does with
    `GetOverlappingActors`.
  - Call `UpdateOverlaps()` on the component or actor after setting things up, if you need the
    events.
- **Never on a component with Generate Overlap Events off,** and never between components that
  ignore each other.

### Seeing collision

Two viewport tools help:

- **`show collision`** in the console (while playing, or in the editor viewport through
  **Show > Collision**) draws every collision shape as a wireframe. The board's wall tiles, the
  gem and exit boxes and the snake's head box all appear. A closed exit's blocker looks the same
  as an open one: the drawing shows shapes, not responses. Type it again to hide them.
- **Physics debug drawing.** Older guides use `pxvis collision`. That was a PhysX command from
  Unreal Engine 4, and UE5 uses the Chaos physics engine instead. Chaos has its own debug drawing
  console variables under `p.Chaos.DebugDraw.`. Type that prefix in the console and let
  autocompletion list what your version offers.

## Building the milestone

The changes against `project/ch09`:

- `Config/DefaultEngine.ini`: the `Grid` trace channel.
- `GridSnake.h`: `#define ECC_GSGrid ECC_GameTraceChannel1`.
- `Gameplay/GSBoard.h/.cpp`: the tile map's collision profile and Grid response,
  `IsCellBlocked`, `SnapActorToCell`.
- `Gameplay/GSGem.h/.cpp`: the `Trigger` root, snapping in `BeginPlay`.
- `Gameplay/GSExit.h/.cpp`: the `Trigger` root, the `Blocker`, `UpdateCollision`, snapping.
- `Snake/GSSnake.h/.cpp`: the head box's collision, its size in `BeginPlay`,
  `NotifyActorBeginOverlap`, and `IsCellBlocked` in `Step`. The `Collision` property comment
  now reads "Root. A query-only box that overlaps gems and exits in the head's cell."

Every changed line is shown above. The editor steps follow `project/ch10/ASSETS.md`:

1. **Create the Grid channel.** **Edit > Project Settings > Engine > Collision > Trace Channels >
   New Trace Channel...**. Name `Grid`, **Default Response** **Ignore**, **Accept**. Then open
   `Config/DefaultEngine.ini` in a text editor and check the `+DefaultChannelResponses` line: it
   must say `Channel=ECC_GameTraceChannel1`. If it shows another number, you created a custom
   channel earlier; change the `#define` in `GridSnake.h` to match. (If you copied the snapshot's
   `Config` folder, the line is already there and the channel appears in the list.)
2. **Build.** Close the editor and build `GridSnakeEditor`. The root components of the gem and
   exit changed, which Live Coding cannot handle.
3. **Check the wall tiles.**
   - Open `TS_Dungeon`. Select each wall tile (4 to 7) and confirm it has a collision box
     (chapter 6, **Add Box**). In the tile map editor, **Show > Collision** draws them.
   - Open `TM_Level01`. In the **Layers** panel, the `Walls` layer has **Layer Collides**
     ticked and `Floor` does not.
   - In the tile map's **Details > Collision**, check **Sprite Collision Domain** is **Use 3D
     Physics**. **Collision Thickness** only needs to be more than 0, since the probe reaches
     50 units each way. Raise it to 50 or more if you also want deep shapes for traces along Y.
   - Save.
4. **Check the Blueprints.** Open `BP_GSGem` and `BP_GSExit`. The Components panel now shows
   **Trigger** as the root (and **Blocker** in the exit), with the sprite attached to it.
   Compile and save both.
5. **Play** in `L_Level01` with the Output Log filtered for `LogGridSnake`.
6. While playing, type `show collision` in the console.

Expected Output Log (filter: `LogGridSnake`), driving over a gem and then into a wall:

```text
LogGridSnake: GameState ready: 0/3 gems
LogGridSnake: Board <GSBoard_0>: 24x14 cells of 16 units
LogGridSnake: View target: <GSBoard_0>
LogGridSnake: <BP_GSExit_C_0> starts closed
LogGridSnake: Snake placed at cell X=12 Y=7 facing EGSDirection::Right, length 3
LogGridSnake: GridSnake game mode started on map <UEDPIE_0_L_Level01> (collect 3 gems)
LogGridSnake: Countdown: 3
LogGridSnake: Countdown: 2
LogGridSnake: Countdown: 1
LogGridSnake: Go!
LogGridSnake: Snake starts moving EGSDirection::Right, one step every 0.15s
LogGridSnake: Snake entered BP_GSGem_C_<1> at cell <X=5 Y=4>
LogGridSnake: Warning: Snake hit a wall at cell X=5 Y=1 (length 3)
```

- **The gem is not collected.** The snake passes through it; drive over it again and it logs
  again. Chapter 11 adds the rules.
- **The wall line is the same as in chapter 9**, but it now comes from a collision query. The
  cell is the head's last cell, next to the wall.
- **The closed exit is a wall.** Drive into it: `Snake hit a wall at cell ...`. Then stop, select
  the exit in the level, tick **Exit > Is Open**, and play again. Now the snake enters it and
  logs `Snake entered <BP_GSExit_C_0> at cell ...`.
- **`show collision`** draws the wall tiles, the triggers and the head box. Stop PIE with
  **Esc**.

### When it goes wrong

| Symptom | Cause | Fix |
|---|---|---|
| The snake drives through walls | The tiles have no collision, the `Walls` layer does not collide, or Collision Domain is not 3D | Step 3; check with `show collision` |
| The snake dies on its first step in open floor | Something blocks Grid where it should not, or the `#define` names the wrong channel | Check the ini line and the `#define` (step 1); use `ExGridProbe` on that cell |
| Every cell is free, even walls | `ECC_GSGrid` is a different channel from the one the tile map blocks | Same check; rebuild after fixing the `#define` |
| No `Snake entered` line on a gem | The gem is off-centre and did not snap, or a Blueprint overrides the trigger's collision | Check the gem's **Trigger** in the Blueprint; add `ExCollisionReport` to it |
| The closed exit lets the snake in | The exit's `bIsOpen` is ticked on the placed actor | Untick **Exit > Is Open** |
| `Snake entered BP_GSGem_C_...` appears twice in a row | The gem is visited twice; there is no collection yet | Expected until chapter 11 |

## Summary

- Collision in Unreal is 3D. Sprites lie in X-Z, so collision shapes need depth along Y. Paper2D
  collision should use the **Use 3D Physics** domain; tile collision comes from the tile set's
  boxes on colliding layers.
- **Object types** say what a component is; **trace channels** say what a query asks. Every
  component has a **response** to every channel. Between two components the weaker response
  wins, and overlap events need **Generate Overlap Events** on both.
- **Collision Enabled** decides whether queries, physics, both or neither see a component.
  **Profiles** bundle settings; apply the profile first, then adjust single channels.
- A **custom trace channel** is created in Project Settings, stored in `DefaultEngine.ini`, and
  named in C++ with a `#define` for its `ECC_GameTraceChannelN`. GridSnake's `Grid` defaults to
  Ignore, so blocking it is opt-in.
- **Scene queries** (`LineTrace`, `Sweep`, `Overlap` × `Test`/`Single`/`Multi` × `ByChannel`/…)
  ask the world without moving anything. `IsCellBlocked` is an `OverlapBlockingTestByChannel`
  with a quarter-cell box.
- Use **data** where the game has rules (self-bites) and **collision** where the world has
  shapes (walls, closed exits, anything a designer adds).
- **Triggers** are `QueryOnly` boxes that overlap one object type. Listen with
  `NotifyActorBeginOverlap` or the components' `OnComponentBeginOverlap` delegate.
- Overlaps update when things move or settings change. Actors that start inside a trigger need
  care: query `GetOverlappingActors` in `BeginPlay`. `show collision` shows the shapes.

## Exercises

1. **Spikes.** Add a `GSSpikes` actor: walking onto it kills the snake, but `IsCellBlocked` must
   not see it (the snake has to *enter* the cell to die).

<details><summary>Solution</summary>

The spikes are a trigger like the gem, with an actor tag that marks them as deadly. Create
`Source/GridSnake/Gameplay/GSSpikes.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSSpikes.generated.h"

class UBoxComponent;
class UPaperSpriteComponent;

/** A cell that kills the snake when its head enters it. */
UCLASS()
class AGSSpikes : public AActor
{
	GENERATED_BODY()

public:
	AGSSpikes();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> Sprite;
};
```

And `GSSpikes.cpp`:

```cpp
#include "Gameplay/GSSpikes.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSpriteComponent.h"

AGSSpikes::AGSSpikes()
{
	Tags.Add(TEXT("Deadly"));

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->InitBoxExtent(FVector(6.0f, 16.0f, 6.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Trigger);
	Sprite->SetRelativeLocation(FVector(0.0f, 5.0f, 0.0f));
	Sprite->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AGSSpikes::BeginPlay()
{
	Super::BeginPlay();

	if (const AGSBoard* Board = Cast<AGSBoard>(UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass())))
	{
		Board->SnapActorToCell(this);
	}
}
```

`Die` is private to the snake, so the snake decides. In `AGSSnake::NotifyActorBeginOverlap`,
after the log line:

```cpp
	if (OtherActor && OtherActor->ActorHasTag(TEXT("Deadly")))
	{
		Die(TEXT("hit spikes"));
	}
```

The trigger ignores Grid, so `IsCellBlocked` lets the head in. The move then raises the overlap,
and the snake dies *in* the spike cell:
`LogGridSnake: Warning: Snake hit spikes at cell X=.. Y=.. (length ..)`. `Die` clears
`bMoving`, and the `while (bMoving && ...)` loop in `Tick` stops at once. Make a Blueprint child,
set its sprite, and place it. The tag is the simplest marker; chapter 11's interfaces are the
cleaner way to give actors behaviour.

</details>

2. **Block only while closed.** Explain how the exit blocks the snake only while it is closed.
   Then add a `bBlockWhenClosed` property (default true) so a designer can make an exit that never
   blocks.

<details><summary>Solution</summary>

The `Blocker` box responds Block to Grid while `bIsOpen` is false. `IsCellBlocked` asks
"does anything block Grid in this cell?", so a closed exit's cell answers yes, and `Step` treats
it as a wall. `SetOpen(true)` calls `UpdateCollision`, which switches the response to Ignore,
and from the next step on the cell is free. The snake's code knows nothing about exits.

In `GSExit.h`, next to `bIsOpen`:

```cpp
	/** If false, a closed exit is only a picture: the snake can enter it. */
	UPROPERTY(EditAnywhere, Category = "Exit")
	bool bBlockWhenClosed = true;
```

In `GSExit.cpp`:

```cpp
void AGSExit::UpdateCollision()
{
	const bool bBlocks = !bIsOpen && bBlockWhenClosed;
	Blocker->SetCollisionResponseToChannel(ECC_GSGrid, bBlocks ? ECR_Block : ECR_Ignore);
}
```

`OnConstruction` already calls `UpdateCollision`, so the change shows in the editor at once.
With `show collision` on you cannot see the difference, because the box exists in both states.
Use `ExGridProbe` on the exit's cell with **Trace Channel** = **Grid** instead.

</details>

3. **A ray instead of a box.** Rewrite `IsCellBlocked` with a line trace and find a case where it
   gives a different answer.

<details><summary>Solution</summary>

Shoot the ray along Y, straight through the cell centre, from in front of the board to behind it:

```cpp
bool AGSBoard::IsCellBlocked(FIntPoint Cell) const
{
	const FVector Center = CellToWorld(Cell);
	const FVector Start = Center + FVector(0.0f, 100.0f, 0.0f);
	const FVector End = Center - FVector(0.0f, 100.0f, 0.0f);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSCellBlockedTrace), /*bTraceComplex=*/false);
	return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_GSGrid, Params);
}
```

For full-tile walls and the exit's blocker, both versions agree: the ray passes through the
centre of the box. They differ when collision does not cover the centre. In `TS_Dungeon`, give
one wall tile a box that fills only the left 6 of its 16 units. The box probe spans units 4 to 12
of the cell and overlaps it; the ray at unit 8 passes beside it and reports the cell free. A ray
tests one line, a box tests an area. The ray must also start and end outside the extruded
collision, which is why it spans ±100 in Y.

</details>

4. **Ignore the camera.** Some of Unreal's systems trace on `Visibility` and `Camera`. Make sure
   the gems ignore both. Write it with the `OverlapAllDynamic` profile as a starting point, and
   verify it with `ExCollisionReport`.

<details><summary>Solution</summary>

The snapshot's gems already ignore both, because the trigger starts with
`SetCollisionResponseToAllChannels(ECR_Ignore)`. Starting from a profile instead:

```cpp
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Trigger->SetGenerateOverlapEvents(true);
```

The profile comes first. Calling `SetCollisionProfileName` after the two `SetCollisionResponse…`
calls would reset them to Overlap. Note that `OverlapAllDynamic` overlaps *every* object type,
not just `Pawn`, so the gem would now also overlap other `WorldDynamic` things, such as the exit's
trigger if one were close.

Add **Ex Collision Report** to `BP_GSGem` and play with **Channel To Report** set to
**Visibility**, then **Camera**:

```text
LogExCollision:   Trigger: profile <Custom>, QueryOnly, object type ECC_WorldDynamic, overlap events on, ECC_Visibility -> ECR_Ignore
LogExCollision:   Trigger: profile <Custom>, QueryOnly, object type ECC_WorldDynamic, overlap events on, ECC_Camera -> ECR_Ignore
```

(One line per run, from each placed gem; illustrative.) Put the snapshot's version back
afterwards: ignoring everything except `Pawn` is the tighter rule.

</details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-10--collision-in-2d)
