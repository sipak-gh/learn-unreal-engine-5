# Chapter 9 — Time: Ticks and Timers

A game is a loop. Many times a second the engine reads input, updates every object that asked to
be updated, and draws a frame. How long one pass takes changes from frame to frame and from
machine to machine, and gameplay code has to cope with that. This chapter covers the **tick** (a
per-frame update with a variable delta time), a **fixed-step** simulation built on top of it, the
**timer manager**, and **input buffering**. GridSnake comes alive. The snake steps one cell every
`StepInterval` seconds, takes turns from a short queue, grows, dies on walls and on itself, and
draws its whole body with a single grouped sprite component. The game mode runs a 3-2-1 countdown
before the snake starts.

Previous: [Chapter 8](08-enhanced-input.md). Next: [Chapter 10](10-collision-in-2d.md).

> **Reminder.** Nothing in this course was compiled or run by the authors (see the README,
> "How this course was checked"). Every *Expected Output Log* block shows the lines the code's
> `UE_LOG` calls will print. Parts in `<angle brackets>` vary on your machine, and blocks marked
> *illustrative* show the shape of the output, not exact numbers.

## The game loop and Tick

Every frame, the engine runs roughly these phases in order:

1. Process input and send it to player controllers and their input components (chapter 8).
2. Tick the world: every registered **tick function** runs, group by group.
3. Run the physics simulation, which overlaps some of the tick groups.
4. Update timers, cameras and other engine systems, then render the frame.

The second phase is where your code runs. An actor takes part by having its **primary tick
function**, `PrimaryActorTick`, registered. The engine then calls the actor's virtual `Tick`
once per frame:

```cpp
virtual void Tick(float DeltaSeconds) override;
```

`DeltaSeconds` is the game time that passed since the previous frame: about 0.0167 at 60 frames
per second, 0.0333 at 30, and something irregular in between when the machine is busy. It is
already scaled by time dilation, so `slomo` (see below) slows ticking actors down for free.

### Switching ticking on

Ticking costs time every frame, so the engine only ticks actors that ask for it. The flags live in
`PrimaryActorTick` and are set in the constructor:

| Setting | Meaning |
|---|---|
| `PrimaryActorTick.bCanEverTick` | May this actor ever tick? Only read when the tick function is registered, so set it in the constructor. `false` saves the registration entirely. |
| `PrimaryActorTick.bStartWithTickEnabled` | If it can tick, does it start ticking at `BeginPlay`? Defaults to true. |
| `PrimaryActorTick.TickGroup` | Which phase of the frame it ticks in. |
| `PrimaryActorTick.TickInterval` | Seconds between ticks; 0 means every frame. |
| `PrimaryActorTick.bTickEvenWhenPaused` | Keep ticking while the game is paused (menus use this). |

At run time, `SetActorTickEnabled(bool)` switches ticking on and off and `SetActorTickInterval`
changes the interval. Components have the same machinery in `PrimaryComponentTick`.

The tick groups divide the frame around physics. Epic's actor ticking page lists them in order:

| Tick group | When it runs |
|---|---|
| `TG_PrePhysics` | At the start of the frame, before physics. The default. |
| `TG_DuringPhysics` | While physics simulates; physics data may be from this frame or the last. |
| `TG_PostPhysics` | After physics has finished; use it for work that needs this frame's physics results. |
| `TG_PostUpdateWork` | Last, after cameras have been updated. |

GridSnake simulates no physics, so the default group suits it. When one actor must see
another's result from the *same* frame, put the reader in a later group or add a dependency
with `AddTickPrerequisiteActor`.

### Frame-rate independence

The gems have ticked since chapter 5. Their bobbing uses `DeltaSeconds` correctly:

```cpp
RunningTime += DeltaSeconds;
const float Offset = FMath::Sin(RunningTime * BobSpeed + BobPhase) * BobHeight;
```

The bob depends on elapsed *time*, not on how many frames have passed, so it looks the same on
every machine. Compare a naive mover:

```cpp
// Wrong: "2 units per frame".
Location.X += 2.0f;
```

Its speed depends on the frame rate:

| Frame rate | Frames per second | Speed of `X += 2` per frame | Speed of `X += 120 * DeltaSeconds` |
|---|---|---|---|
| 30 fps | 30 | 60 units/s | 120 units/s |
| 60 fps | 60 | 120 units/s | 120 units/s |
| 144 fps | 144 | 288 units/s | 120 units/s |

Multiplying by `DeltaSeconds` fixes *smooth* motion. A grid snake has none: it jumps exactly one
cell, waits, and jumps again. It needs a clock that says "one step is due now".

## Fixed steps with an accumulator

The pattern is called a **fixed time step**. Treat `DeltaSeconds` as money paid into an account.
Each simulation step costs exactly `StepInterval`. Each frame, pay in the frame's time and buy as
many whole steps as the balance allows:

```cpp
Accumulator += DeltaSeconds;
while (Accumulator >= StepInterval)
{
	Accumulator -= StepInterval;
	Step();
}
```

Three properties make this the right tool:

- **Steps are exact.** Over any long stretch of time, the number of steps equals elapsed time
  divided by `StepInterval`, whatever the frame rate. Ten seconds at 0.15 s per step is 66 steps
  at 30 fps and 66 steps at 144 fps.
- **The remainder is kept.** If a frame leaves 0.04 s in the account, that time counts towards
  the next step instead of being lost. Without it, steps would drift later and later.
- **Long frames catch up.** A 0.35 s hitch owes two steps at 0.15 s (0.30 s), and the `while`
  loop runs both in the same frame.

### Example: a stepper that shows its work

`examples/ch09/ExFixedStepper.h`

```cpp
// Chapter 9 example: a fixed-step simulation driven by the variable-length Tick.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExFixedStepper.generated.h"

/** Drop into a level and press Play: logs every frame's delta time and the steps it ran. */
UCLASS()
class AExFixedStepper : public AActor
{
	GENERATED_BODY()

public:
	AExFixedStepper();

	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Seconds of simulated time per step. Independent of the frame rate. */
	UPROPERTY(EditAnywhere, Category = "Stepper", meta = (ClampMin = "0.01"))
	float StepInterval = 0.1f;

	/** Log this many frames, then stay quiet. */
	UPROPERTY(EditAnywhere, Category = "Stepper", meta = (ClampMin = "0"))
	int32 FramesToLog = 20;

private:
	void Step();

	float Accumulator = 0.0f;
	float SimulatedTime = 0.0f;
	int32 FrameCount = 0;
	int32 StepCount = 0;
};
```

`examples/ch09/ExFixedStepper.cpp`

```cpp
// Chapter 9 example: a fixed-step simulation driven by the variable-length Tick.

#include "ExFixedStepper.h"

DEFINE_LOG_CATEGORY_STATIC(LogExStep, Log, All);

AExFixedStepper::AExFixedStepper()
{
	PrimaryActorTick.bCanEverTick = true;
	// TG_PrePhysics is the default group; set here only to show where it lives.
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void AExFixedStepper::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	++FrameCount;

	// Bank the frame's time, then spend it in whole steps. Whatever is left
	// over (less than one step) waits for the next frame.
	Accumulator += DeltaSeconds;
	int32 StepsThisFrame = 0;
	while (Accumulator >= StepInterval)
	{
		Accumulator -= StepInterval;
		Step();
		++StepsThisFrame;
	}

	if (FrameCount <= FramesToLog)
	{
		UE_LOG(LogExStep, Log, TEXT("Frame %d: dt=%.3f steps=%d (left over %.3f)"),
			FrameCount, DeltaSeconds, StepsThisFrame, Accumulator);
	}
}

void AExFixedStepper::Step()
{
	++StepCount;
	SimulatedTime += StepInterval;

	if (FrameCount <= FramesToLog)
	{
		UE_LOG(LogExStep, Log, TEXT("  step %d (simulated time %.2fs)"), StepCount, SimulatedTime);
	}
}
```

To see it:

1. Copy both files into the `Source/<YourModule>/` folder of a scratch UE 5.8 C++ project (not
   GridSnake). The module needs `Core`, `CoreUObject` and `Engine` in its `Build.cs`, which every
   new C++ project already has. Build.
2. Drag `ExFixedStepper` from the Content Browser (**C++ Classes > \<YourModule\>**) into any
   level.
3. Open **Window > Output Log** and type `LogExStep` in its search box.
4. Press **Play**. Stop after a second or two.

Expected Output Log (filter: `LogExStep`), at about 60 fps (illustrative; your frame times differ):

```text
LogExStep: Frame 1: dt=<0.016> steps=0 (left over 0.016)
LogExStep: Frame 2: dt=0.016 steps=0 (left over 0.032)
...
LogExStep: Frame 6: dt=0.016 steps=0 (left over 0.096)
LogExStep:   step 1 (simulated time 0.10s)
LogExStep: Frame 7: dt=0.016 steps=1 (left over 0.012)
LogExStep: Frame 8: dt=0.017 steps=0 (left over 0.029)
...
```

Now make the frames long. Raise the placed actor's **Frames To Log** to 1000, press **Play**,
open the console with the backtick key (`` ` ``, above Tab on a US keyboard) and type
`t.MaxFPS 5`. Each frame now lasts about 0.2 s, which pays for two 0.1 s steps.

Expected Output Log (filter: `LogExStep`), after `t.MaxFPS 5` (illustrative):

```text
LogExStep:   step 41 (simulated time 4.10s)
LogExStep:   step 42 (simulated time 4.20s)
LogExStep: Frame 87: dt=0.200 steps=2 (left over 0.001)
LogExStep:   step 43 (simulated time 4.30s)
LogExStep:   step 44 (simulated time 4.40s)
LogExStep: Frame 88: dt=0.200 steps=2 (left over 0.001)
```

Type `t.MaxFPS 0` to remove the cap again.

- **The step lines come before their frame line.** `Step()` logs inside the `while` loop; the
  frame summary is logged after it.
- **Simulated time keeps pace with real time** in both runs, whatever the frame rate.
- **`left over` is always less than `StepInterval`.** That is the loop's exit condition.
- **The first frames of a PIE session** often have odd `dt` values. Keep the viewport focused:
  an unfocused editor may throttle its frame rate.

### The spiral of death

If a step takes longer to *compute* than the simulated time it stands for, each frame owes more
steps than the last and the game grinds to a halt: the **spiral of death**. A GridSnake step
costs microseconds; heavier simulations (physics, pathfinding for hundreds of units) can hit it.

There are two guards:

- **The engine clamps long frames.** **World Settings > Tick > Max Undilated Frame Time** (an
  advanced property; expand the section's arrow to see it) caps `DeltaSeconds` for every actor.
  Epic documents it as "Largest possible frametime, not considering dilation. Equiv to
  1/SlowestFPS"; check its default in your project. A debugger breakpoint therefore does not
  make the snake run twenty steps when you resume, and below that frame rate game time runs
  slower than wall-clock time.
- **Your loop can cap catch-up.** Allow at most N steps per frame and throw away the rest of the
  debt. The game slows down instead of freezing. Exercise 1 adds this to the snake.

### Leftover time and smooth motion

`Accumulator / StepInterval` (between 0 and 1) says how far the world is between two steps.
Smooth fixed-step games interpolate their drawing by it. GridSnake *teleports* from cell to cell
on purpose: the classic look, and every position on screen is a real game state.

### The snake's clock

`AGSSnake` uses exactly this loop. Its constructor (`Source/GridSnake/Snake/GSSnake.cpp`) now sets
`PrimaryActorTick.bCanEverTick = true;` (chapter 8 had it off). The tick does nothing until the
snake has been told to move:

```cpp
void AGSSnake::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bMoving)
	{
		return;
	}

	// Fixed-step movement: frames come at any rate, steps at exactly one per
	// StepInterval. A long frame may owe more than one step.
	StepAccumulator += DeltaSeconds;
	while (bMoving && StepAccumulator >= StepInterval)
	{
		StepAccumulator -= StepInterval;
		Step();
	}
}

void AGSSnake::StartMoving()
{
	if (!bAlive || !Board)
	{
		return;
	}

	bMoving = true;
	StepAccumulator = 0.0f;
	UE_LOG(LogGridSnake, Log, TEXT("Snake starts moving %s, one step every %.2fs"),
		*UEnum::GetValueAsString(CurrentDirection), StepInterval);
}
```

- **`bMoving &&` in the loop condition.** `Step()` may kill the snake, and `Die` clears
  `bMoving`. Without that check, a long frame would keep stepping a dead snake.
- **`StartMoving` resets the accumulator.** Time that passed during the countdown must not turn
  into a burst of steps at "Go!". The snake's `Tick` returns early while `bMoving` is false, so
  the accumulator stays at zero anyway. Resetting it makes `StartMoving` correct on its own.
- **`StepInterval` comes from the project settings** (chapter 3) in `BeginPlay`:
  `StepInterval = UGSSettings::Get()->DefaultStepInterval;`. It is `VisibleInstanceOnly`, so you
  can watch it on the spawned snake while playing but cannot set it in `BP_GSSnake`. The setting
  is the single source of truth. Chapter 11 speeds the snake up from there.

## Input buffering

In chapter 8, a steer input changed `CurrentDirection` immediately and logged `Steer: ...`. That
was fine while the snake stood still. Once it steps every 0.15 s, two problems appear.

**Problem 1: fast turns get lost.** The snake moves right and the player presses Up then Left
inside one step (a U-turn). With an immediate change, Left *replaces* Up and the snake goes left,
into its own neck. Comparing with the actual direction (Right) instead rejects Left as a
reversal. Either way, the player's intent is lost.

**Problem 2: holding a key repeats.** Chapter 8's `IA_Steer` fires `Triggered` every frame while
a key is held. At 60 fps that is about nine events per step. All of them but the first must be
ignored.

The fix is a small queue of turns. Each step consumes at most one:

```cpp
void AGSSnake::OnSteer(const FInputActionValue& Value)
{
	// Compare against the direction the snake will have after the queued turns.
	const EGSDirection Reference = DirectionQueue.Num() > 0 ? DirectionQueue.Last() : CurrentDirection;

	EGSDirection NewDirection;
	if (DirectionFromInput(Value.Get<FVector2D>(), Reference, NewDirection))
	{
		QueueDirection(NewDirection);
	}
}

void AGSSnake::QueueDirection(EGSDirection NewDirection)
{
	const EGSDirection Reference = DirectionQueue.Num() > 0 ? DirectionQueue.Last() : CurrentDirection;

	// Holding a key fires Triggered every frame: ignore repeats. A snake also
	// cannot reverse into its own neck.
	if (NewDirection == Reference || GSGrid::AreOpposite(NewDirection, Reference))
	{
		return;
	}

	if (DirectionQueue.Num() >= MaxQueuedTurns)
	{
		return;
	}

	DirectionQueue.Add(NewDirection);
}
```

The key idea is the **reference direction**: the direction the snake *will* have once every
queued turn has been applied, which is the last entry of the queue, or the current direction if
the queue is empty. Replay the U-turn with it:

| Event | Reference | Check | Queue afterwards |
|---|---|---|---|
| Moving Right, queue empty | Right | | `[]` |
| Press Up | Right | not same, not opposite: accept | `[Up]` |
| Up still held next frame | Up | same: ignore | `[Up]` |
| Press Left | Up | not same, not opposite: accept | `[Up, Left]` |
| Step | | pops Up, snake moves up | `[Left]` |
| Step | | pops Left, snake moves left | `[]` |

Both turns arrive, one per step, and the snake never reverses. `DirectionFromInput` (chapter 8)
also receives the reference. It needs a direction to break ties when two keys are held at
once, and the direction after the queued turns is the right one.

`MaxQueuedTurns` (2, `EditDefaultsOnly`) bounds the queue, so key mashing cannot build a list of
stale turns that play out long after. The queue is a plain `TArray`: `Add` appends, `[0]` peeks,
`RemoveAt(0)` pops. (`TQueue` is a lock-free queue for passing data between threads, the wrong
tool here.)

## Segments and growth

Chapter 7's snake was a single cell, `HeadCell`. The body needs a list of cells:

```cpp
/** Cells occupied by the snake. [0] is the head, the last entry is the tail tip. */
UPROPERTY(VisibleInstanceOnly, Category = "Snake")
TArray<FIntPoint> Segments;
```

`GetHeadCell()` now reads `Segments[0]`, and the new `GetLength()` returns `Segments.Num()`. A
snake of length 4 heading right, with the head in cell (12, 7):

```text
row 7:   (9,7) (10,7) (11,7) (12,7)
         tail                 head      Segments = [(12,7), (11,7), (10,7), (9,7)]
```

### One step

Here is the whole `Step` function. It is the heart of the game:

```cpp
void AGSSnake::Step()
{
	if (DirectionQueue.Num() > 0)
	{
		CurrentDirection = DirectionQueue[0];
		DirectionQueue.RemoveAt(0);
	}

	const FIntPoint NextCell = Segments[0] + GSGrid::ToOffset(CurrentDirection);

	if (!Board->IsInside(NextCell) || Board->IsWallTile(NextCell))
	{
		Die(TEXT("hit a wall"));
		return;
	}

	// The tail tip moves away this step unless the snake is growing, so the
	// head may safely enter the cell the tail is leaving.
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

	Segments.Insert(NextCell, 0);
	if (bTailMoves)
	{
		Segments.RemoveAt(Segments.Num() - 1);
	}
	else
	{
		--PendingGrowth;
	}

	UpdateVisuals();
}
```

Read it top to bottom:

1. **Turn.** Pop at most one queued direction.
2. **Look ahead.** `NextCell` is the head's cell plus one step (`GSGrid::ToOffset` from chapter
   7; rows grow downwards, so Up is Y − 1).
3. **Walls.** Leaving the board or entering a wall tile kills the snake. `IsWallTile` is chapter
   6's data check: any tile on a layer with *Layer Collides* set. No physics is involved;
   chapter 10 replaces this with a collision query.
4. **The body.** If any segment is in `NextCell`, the snake bites itself, except the tail tip:
   when not growing, the tail leaves its cell this very step, so the loop checks one cell fewer.
   Otherwise a snake chasing its own tail would die unfairly.
5. **Move.** Insert the new head at the front. Then either drop the tail tip (a normal move) or
   keep it and use up one unit of `PendingGrowth` (the snake grows by one cell).
6. **Draw.** `UpdateVisuals` moves the actor and the body sprites (next section).

`Die` stops everything and says why:

```cpp
void AGSSnake::Die(const TCHAR* Reason)
{
	bMoving = false;
	bAlive = false;
	DirectionQueue.Reset();
	UE_LOG(LogGridSnake, Warning, TEXT("Snake %s at cell %s (length %d)"), Reason, *GetHeadCell().ToString(), Segments.Num());
}
```

The cell in the message is the head's cell, the last cell the snake reached, not the wall it
tried to enter. Chapter 11 turns death into a game-mode event with a restart.

### Growth

Growing is just a counter:

```cpp
void AGSSnake::Grow(int32 Amount)
{
	PendingGrowth += FMath::Max(Amount, 0);
}
```

A gem (chapter 11) will call `Grow(1)`. The tail then stays put for one step, which is the new
segment; `Grow(5)` keeps it still for five steps. No special drawing code is needed.

### Placing the starting body

`PlaceOnBoard` replaces chapter 7's `SnapToBoard`. It takes the cell the pawn was spawned in (the
Player Start's cell) as the head and lays `InitialLength - 1` more segments *behind* it, opposite
to the start direction:

```cpp
	// The body trails behind the head, opposite to the starting direction.
	Segments.Reset();
	const FIntPoint Behind = GSGrid::ToOffset(CurrentDirection) * -1;
	for (int32 Index = 0; Index < InitialLength; ++Index)
	{
		Segments.Add(HeadCell + Behind * Index);
	}

	// The Player Start's rotation would otherwise tilt the sprites out of the X-Z plane.
	SetActorRotation(FRotator::ZeroRotator);
	UpdateVisuals();
	UE_LOG(LogGridSnake, Log, TEXT("Snake placed at cell %s facing %s, length %d"),
		*HeadCell.ToString(), *UEnum::GetValueAsString(CurrentDirection), Segments.Num());
```

Before this part, `HeadCell` is computed with `Board->WorldToCell(GetActorLocation())`. If that
cell is off the board, the function logs a warning and uses cell (1, 1), as chapter 7's
`SnapToBoard` did.

`FIntPoint` supports `+`, and `* int32`, so `HeadCell + Behind * Index` reads like the maths it
is. This is why chapter 7 asked you to keep the cells left of the Player Start free: a snake
heading right trails to the left.

### What it costs

`TArray` is a contiguous array, like `std::vector` (see Epic's containers page). For a snake of
length *n*, `Insert(NextCell, 0)` is O(n) (one `memmove` shifts every element), `RemoveAt` of the
last element is O(1), and the self-bite loop and `UpdateVisuals` are O(n).

So a step is O(n). The largest snake on a 24 × 14 board has 336 cells, under 3 KB of
`FIntPoint`s: far cheaper than drawing one frame. A ring buffer plus a `TSet<FIntPoint>` would
make steps O(1), at the cost of more code that can get out of sync. Wait for the profiler.

`Segments` is a `UPROPERTY` with `VisibleInstanceOnly`. Select the spawned `BP_GSSnake_C_0` in
the Outliner while playing and you can watch the array change in the Details panel.

## Drawing the body with UPaperGroupedSpriteComponent

Each segment needs a sprite on screen. The obvious design is one `UPaperSpriteComponent` per
segment. It works, but every component is a separate object. Each has its own transform to
update, its own render-thread **scene proxy**, and usually its own draw call. A long snake then
means hundreds of components that are created and destroyed as it grows.

Paper2D has a better fit: `UPaperGroupedSpriteComponent`. One component holds a list of
**instances**, each with its own transform, sprite and colour, and the component renders them
together. The snake has one `Body` component, attached to the root next to the head:

```cpp
	Body = CreateDefaultSubobject<UPaperGroupedSpriteComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
```

The sprite comes from the new `BodySprite` property, which you set to `S_SnakeBody` (chapter 4)
in `BP_GSSnake`. The parts of the API that GridSnake uses, as listed on the 5.8 API page:

| Function | What it does |
|---|---|
| `int32 AddInstance(const FTransform& Transform, UPaperSprite* Sprite, bool bWorldSpace, FLinearColor Color)` | Adds an instance; returns its index. |
| `bool UpdateInstanceTransform(int32 InstanceIndex, const FTransform& NewInstanceTransform, bool bWorldSpace, bool bMarkRenderStateDirty, bool bTeleport)` | Moves one instance. |
| `bool UpdateInstanceColor(int32 InstanceIndex, FLinearColor NewInstanceColor, bool bMarkRenderStateDirty)` | Recolours one instance. |
| `bool RemoveInstance(int32 InstanceIndex)` | Removes one instance. |
| `void ClearInstances()` | Removes all instances. |
| `int32 GetInstanceCount()` | How many instances there are. |

### UpdateVisuals

```cpp
void AGSSnake::UpdateVisuals()
{
	// Teleport: the snake jumps from cell to cell, it does not slide.
	SetActorLocation(Board->CellToWorld(Segments[0]));
	Head->SetRelativeRotation(GSGrid::ToSpriteRotation(CurrentDirection));

	const int32 NumBodySegments = Segments.Num() - 1;
	if (Body->GetInstanceCount() != NumBodySegments)
	{
		// The length changed: rebuild every instance. Cheap at snake sizes.
		Body->ClearInstances();
		for (int32 Index = 1; Index < Segments.Num(); ++Index)
		{
			const FTransform Transform(Board->CellToWorld(Segments[Index]) + FVector(0.0f, BodyDepth, 0.0f));
			Body->AddInstance(Transform, BodySprite, /*bWorldSpace=*/true, FLinearColor::White);
		}
		return;
	}

	for (int32 Index = 1; Index < Segments.Num(); ++Index)
	{
		const FTransform Transform(Board->CellToWorld(Segments[Index]) + FVector(0.0f, BodyDepth, 0.0f));
		const bool bLast = Index == Segments.Num() - 1;
		Body->UpdateInstanceTransform(Index - 1, Transform, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/bLast, /*bTeleport=*/true);
	}
}
```

- **The actor is the head**, on `Segments[0]`. Instance *i − 1* draws segment *i*, so the body
  has one instance fewer than `Segments`.
- **World space.** The `Body` component moves with the actor, so instance transforms in local
  space would move every time the head moves. Passing `bWorldSpace = true` lets the code think
  in board positions; the component converts them to its local space.
- **Two paths.** When the length changed, rebuild all instances (cheap at a few hundred);
  otherwise move the existing ones.
- **Only the last update marks the render state dirty.** Marking it dirty tells the engine to
  send the component's data to the render thread again. Doing that once per step instead of once
  per segment is the whole point of batching.
- **`bTeleport = true`** says the instance jumped rather than moved (it matters for physics and
  motion blur).
- **Depth.** `BodyDepth` is `constexpr float BodyDepth = 8.0f;` in an anonymous namespace at the
  top of `GSSnake.cpp` (internal linkage, like a `static` global in C): in front of gems (+5),
  behind the head (+10).

### Example: a grid of instances

This example draws a 10 × 10 grid with one component and recolours a random cell four times a
second.

`examples/ch09/ExGroupedSprites.h`

```cpp
// Chapter 9 example: one grouped sprite component drawing a 10x10 grid.
// Build.cs modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExGroupedSprites.generated.h"

class UPaperGroupedSpriteComponent;
class UPaperSprite;

/** Place in a level, pick a Sprite, press Play: random cells change colour. */
UCLASS()
class AExGroupedSprites : public AActor
{
	GENERATED_BODY()

public:
	AExGroupedSprites();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPaperGroupedSpriteComponent> Grid;

	/** Drawn once per cell. Any sprite works, e.g. S_SnakeBody. */
	UPROPERTY(EditAnywhere, Category = "Grid")
	TObjectPtr<UPaperSprite> Sprite;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", ClampMax = "100"))
	int32 Columns = 10;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", ClampMax = "100"))
	int32 Rows = 10;

	/** Distance between cell centres, in Unreal units. */
	UPROPERTY(EditAnywhere, Category = "Grid")
	float Spacing = 16.0f;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "0.05"))
	float RecolourInterval = 0.25f;

private:
	void RecolourRandomInstance();

	FTimerHandle RecolourTimer;
};
```

`examples/ch09/ExGroupedSprites.cpp`

```cpp
// Chapter 9 example: one grouped sprite component drawing a 10x10 grid.

#include "ExGroupedSprites.h"
#include "PaperGroupedSpriteComponent.h"
#include "PaperSprite.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExGrouped, Log, All);

AExGroupedSprites::AExGroupedSprites()
{
	PrimaryActorTick.bCanEverTick = false;

	Grid = CreateDefaultSubobject<UPaperGroupedSpriteComponent>(TEXT("Grid"));
	SetRootComponent(Grid);
}

void AExGroupedSprites::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs in the editor on every edit, so the grid is visible without playing.
	Grid->ClearInstances();
	if (!Sprite)
	{
		return;
	}

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			// Local space: X goes right, rows go down the screen (-Z).
			const FTransform Local(FVector(Column * Spacing, 0.0f, -Row * Spacing));
			Grid->AddInstance(Local, Sprite, /*bWorldSpace=*/false, FLinearColor::White);
		}
	}
}

void AExGroupedSprites::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogExGrouped, Log, TEXT("%s draws %d instances with one component"),
		*GetName(), Grid->GetInstanceCount());

	GetWorldTimerManager().SetTimer(RecolourTimer, this, &AExGroupedSprites::RecolourRandomInstance,
		RecolourInterval, /*bLoop=*/true);
}

void AExGroupedSprites::RecolourRandomInstance()
{
	const int32 Count = Grid->GetInstanceCount();
	if (Count == 0)
	{
		return;
	}

	const int32 Index = FMath::RandRange(0, Count - 1);
	const FLinearColor Color = FLinearColor::MakeRandomColor();

	// true: mark the render state dirty so the change shows this frame.
	Grid->UpdateInstanceColor(Index, Color, /*bMarkRenderStateDirty=*/true);
	UE_LOG(LogExGrouped, Log, TEXT("instance %d is now #%s"), Index, *Color.ToFColor(true).ToHex());
}
```

To see it:

1. Copy both files into the scratch project's module, add `"Paper2D"` to its
   `PublicDependencyModuleNames` in `Build.cs` (chapter 2), enable the Paper2D plugin (**Edit >
   Plugins**), and build.
2. Make any small sprite (chapter 4 shows how).
3. Drag `ExGroupedSprites` into a level and set **Grid > Sprite**. The grid appears at once
   (`OnConstruction` runs in the editor); change **Columns** to watch it rebuild.
4. Look along the Y axis, filter the Output Log for `LogExGrouped`, and press **Play**.

Expected Output Log (filter: `LogExGrouped`; indices and colours are random):

```text
LogExGrouped: ExGroupedSprites_<1> draws 100 instances with one component
LogExGrouped: instance <37> is now #<5FD0A2FF>
LogExGrouped: instance <82> is now #<E0337BFF>
...
```

- **`OnConstruction` builds, `BeginPlay` animates.** `OnConstruction` reruns on every edit, so
  it starts with `ClearInstances` to avoid stacking grids.
- **Local space here, world space in the snake.** This grid moves with its actor, so local
  coordinates are the natural choice.
- **`ToFColor(true).ToHex()`** prints the random colour as 8-bit sRGB `RRGGBBAA`.
- **Measure it.** Type `stat scenerendering` in the console while playing. It shows rendering
  statistics such as draw call counts. Exercise 4 compares the snake drawn both ways.

## Timers

Ticking runs code every frame. Often you want something else: "call this once in two seconds" or
"call this every second until I say stop". You could count time in `Tick`, but the engine has a
service for it, the **timer manager** (`FTimerManager`). Each world has one; reach it from an
actor with `GetWorldTimerManager()`. The game instance has another one that survives level
changes.

The core calls, all on `FTimerManager` (include `"TimerManager.h"`):

| Call | Effect |
|---|---|
| `SetTimer(Handle, this, &AMyActor::Func, Rate, bLoop, FirstDelay = -1)` | Calls `Func` after `Rate` seconds (or `FirstDelay`, if given), and every `Rate` seconds after that if `bLoop`. Fills in `Handle`. |
| `SetTimer(Handle, Delegate, Rate, bLoop, FirstDelay = -1)` | The same with any `FTimerDelegate`, for example a lambda. |
| `SetTimerForNextTick(Delegate)` | Calls the delegate once, on the next frame. |
| `ClearTimer(Handle)` | Cancels the timer and invalidates the handle. |
| `PauseTimer(Handle)` / `UnPauseTimer(Handle)` | Freezes and resumes it; the remaining time is kept. |
| `IsTimerActive(Handle)`, `GetTimerRemaining(Handle)`, `GetTimerElapsed(Handle)` | Queries. |
| `ClearAllTimersForObject(Object)` | Cancels every timer bound to that object. |

Things worth knowing, from Epic's gameplay timers page and the API reference:

- **`FTimerHandle` is just an ID.** Keep it to clear, pause or query the timer. `SetTimer` with
  the same handle replaces the old timer.
- **Timers are cancelled when their object is destroyed.** Clearing them in `EndPlay` is still
  good hygiene, and required for plain lambdas, which have no object to check.
- **Rate ≤ 0 clears.** `ClearTimer` is documented as identical to `SetTimer` with such a rate.
- **Next tick.** In the 5.8 API `SetTimerForNextTick` *returns* an `FTimerHandle` (older guide
  text says it fills none); keep or ignore it.
- **Long frames.** A looping timer whose rate is shorter than a frame may fire several times in
  that frame to catch up, much like the accumulator. The 5.8 API also has an overload taking
  `FTimerManagerTimerParameters`, whose `bMaxOncePerFrame` flag turns that off.
- **Game time.** World timers advance with the world's game time. They stop while the game is
  paused and slow down under `slomo`. Chapter 16 relies on this for the pause menu.

### Example: three kinds of timer

`examples/ch09/ExTimerDemo.h`

```cpp
// Chapter 9 example: looping, one-shot and next-tick timers.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExTimerDemo.generated.h"

/** Drop into a level and press Play: three kinds of timer log as they fire. */
UCLASS()
class AExTimerDemo : public AActor
{
	GENERATED_BODY()

public:
	AExTimerDemo();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Seconds between calls of the looping timer. */
	UPROPERTY(EditAnywhere, Category = "Timers", meta = (ClampMin = "0.01"))
	float LoopRate = 0.5f;

	/** Seconds until the one-shot timer fires and stops the loop. */
	UPROPERTY(EditAnywhere, Category = "Timers", meta = (ClampMin = "0.01"))
	float OneShotDelay = 1.75f;

private:
	void OnLoop();

	FTimerHandle LoopTimer;
	FTimerHandle OneShotTimer;
	int32 LoopCount = 0;
};
```

`examples/ch09/ExTimerDemo.cpp`

```cpp
// Chapter 9 example: looping, one-shot and next-tick timers.

#include "ExTimerDemo.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExTimer, Log, All);

AExTimerDemo::AExTimerDemo()
{
	// Timers are driven by the world's timer manager; this actor needs no Tick.
	PrimaryActorTick.bCanEverTick = false;
}

void AExTimerDemo::BeginPlay()
{
	Super::BeginPlay();

	FTimerManager& Timers = GetWorldTimerManager();

	// 1. Once, on the next frame. Handy for "after everyone's BeginPlay".
	Timers.SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UE_LOG(LogExTimer, Log, TEXT("next tick (world time %.3f)"), GetWorld()->GetTimeSeconds());
	}));

	// 2. A looping timer that calls a member function.
	Timers.SetTimer(LoopTimer, this, &AExTimerDemo::OnLoop, LoopRate, /*bLoop=*/true);

	// 3. A one-shot timer bound to a lambda. A weak lambda is skipped if this
	//    actor has been destroyed by the time the timer fires.
	Timers.SetTimer(OneShotTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UE_LOG(LogExTimer, Log, TEXT("one-shot fired after %d loops, clearing loop"), LoopCount);
		GetWorldTimerManager().ClearTimer(LoopTimer);
	}), OneShotDelay, /*bLoop=*/false);

	UE_LOG(LogExTimer, Log, TEXT("timers set: loop every %.2fs, one-shot in %.2fs"), LoopRate, OneShotDelay);
}

void AExTimerDemo::OnLoop()
{
	++LoopCount;
	UE_LOG(LogExTimer, Log, TEXT("loop %d (remaining on one-shot: %.2f)"),
		LoopCount, GetWorldTimerManager().GetTimerRemaining(OneShotTimer));
}

void AExTimerDemo::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Stop everything this actor started, even timers whose handles we lost.
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}
```

To see it: copy both files into the scratch project, build, drag `ExTimerDemo` into a level,
filter the Output Log for `LogExTimer` and press **Play**. Wait two seconds.

Expected Output Log (filter: `LogExTimer`; the world time and the remaining times are
illustrative, since timers fire on the first frame at or after their due time):

```text
LogExTimer: timers set: loop every 0.50s, one-shot in 1.75s
LogExTimer: next tick (world time <0.017>)
LogExTimer: loop 1 (remaining on one-shot: 1.25)
LogExTimer: loop 2 (remaining on one-shot: 0.75)
LogExTimer: loop 3 (remaining on one-shot: 0.25)
LogExTimer: one-shot fired after 3 loops, clearing loop
```

- **`timers set` comes first.** Setting a timer never calls it at once.
- **Member function vs lambda.** `SetTimer(Handle, this, &AExTimerDemo::OnLoop, ...)` binds a
  member function to an object. `FTimerDelegate::CreateWeakLambda(this, [this]() { ... })` binds
  a lambda and *weakly* ties it to `this`: if the actor is destroyed first, the call is skipped.
  A plain `CreateLambda` capturing `this` would call into freed memory. Chapter 11 covers
  delegates properly.
- **`GetTimerRemaining`** counts down 1.25, 0.75, 0.25 between loops.
- **Changing timers from inside a timer callback** is allowed. Epic's page states that it is safe
  to set timers in a function called by a timer, even reusing the handle of the timer being
  called, and its own example clears a looping timer from inside that timer's callback.

### The countdown

`AGSGameMode` uses a looping timer for the 3-2-1 countdown. `Source/GridSnake/Core/GSGameMode.h`
gains `int32 CountdownSeconds = 3` (`EditDefaultsOnly`, category **Rules**, `ClampMin = "0"`) and
the private `void TickCountdown();`, `FTimerHandle CountdownTimer;` and
`int32 CountdownRemaining = 0;`. `GSGameMode.cpp` includes `"Kismet/GameplayStatics.h"` and
`"TimerManager.h"`:

```cpp
void AGSGameMode::StartPlay()
{
	Super::StartPlay();

	UE_LOG(LogGridSnake, Log, TEXT("GridSnake game mode started on map %s (collect %d gems)"),
		*GetWorld()->GetMapName(), UGSSettings::Get()->GemsPerLevel);

	// A looping one-second timer counts down; the last tick releases the snake.
	CountdownRemaining = CountdownSeconds;
	TickCountdown();
	if (CountdownRemaining > 0)
	{
		GetWorldTimerManager().SetTimer(CountdownTimer, this, &AGSGameMode::TickCountdown, 1.0f, /*bLoop=*/true);
	}
}

void AGSGameMode::TickCountdown()
{
	if (CountdownRemaining > 0)
	{
		UE_LOG(LogGridSnake, Log, TEXT("Countdown: %d"), CountdownRemaining);
		--CountdownRemaining;
		return;
	}

	GetWorldTimerManager().ClearTimer(CountdownTimer);
	UE_LOG(LogGridSnake, Log, TEXT("Go!"));

	if (AGSSnake* Snake = Cast<AGSSnake>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Snake->StartMoving();
	}
}
```

Trace it with `CountdownSeconds = 3`:

| Time | Call | Logs | `CountdownRemaining` after |
|---|---|---|---|
| 0 s | `StartPlay` calls `TickCountdown()` directly | `Countdown: 3` | 2 |
| 1 s | timer | `Countdown: 2` | 1 |
| 2 s | timer | `Countdown: 1` | 0 |
| 3 s | timer | `Go!`, clears the timer, starts the snake | 0 |

- **The first call is direct**, because a timer's first call comes after one full period.
- **`CountdownSeconds = 0`** skips the timer and goes straight to "Go!". `ClearTimer` on a
  never-set handle does nothing.
- **The pawn exists here.** Chapter 7 showed that the player's pawn is spawned and possessed
  before `StartPlay`, and that `Super::StartPlay()` runs every actor's `BeginPlay`. By the time
  the timer fires, the snake has placed itself on the board.
  `UGameplayStatics::GetPlayerPawn(this, 0)` returns player 0's pawn, and the `Cast` returns
  null if it is not a snake, so a misconfigured game mode logs nothing instead of crashing.

### Timer or Tick?

| Use a timer when... | Use Tick when... |
|---|---|
| Something happens once after a delay (respawn in 2 s). | Something changes every frame (bobbing, camera smoothing). |
| Something repeats at a slow, fixed rate (countdown, spawn waves). | The rate changes often, or you need the leftover time (fixed-step simulation). |
| The actor otherwise has nothing to do each frame. | You need the work ordered with other ticks (tick groups, prerequisites). |

The game mode has no per-frame work, so its countdown is a timer. A looping 0.15 s timer would
also move the snake, but Tick shows the accumulator pattern you need for any simulation, and
changing the speed is just a new `StepInterval`, with no timer to restart.

## Console commands for time

The console (the backtick key, or **Tilde** on some layouts) has commands that bend time. They
are the fastest way to test frame-rate independence. In PIE, click into the game viewport first
so the console goes to the game.

| Command | Effect |
|---|---|
| `t.MaxFPS 10` | Caps the frame rate at 10; `t.MaxFPS 0` removes the cap. |
| `slomo 0.25` | Game time runs at a quarter of real time (`slomo 1` restores it). Scales `DeltaSeconds` and timers. |
| `pause` | Toggles the game's pause. Actors stop ticking and world timers stop. |
| `stat fps` / `stat unit` | Shows the frame rate / frame, game-thread, draw-thread and GPU times. Repeat to hide. |
| `dumpticks` | Writes every registered tick function to the log. |

`slomo` is a cheat-manager command, so shipping builds lack it. Commands change between
versions; typing the first letters in the console shows autocompletion with descriptions.

Try them on GridSnake once the milestone below works:

- **`t.MaxFPS 10`.** Frames of 0.1 s against 0.15 s steps: the snake steps on two frames out of
  three, jerkier but just as fast. Replace the `while` with `if` and cap at 4 fps: the snake now
  slows down, one step per 0.25 s frame. That is the bug the loop prevents.
- **`slomo 0.25`.** The countdown takes four times as long, and so does each step. Both run on
  game time.
- **`pause`.** The snake and the countdown freeze; `pause` again resumes. (The PIE toolbar's
  **Pause** button suspends the whole simulation instead.)
- **`dumpticks`.** Search the output for `GSSnake` and `GSGem`: both have a tick function. The
  board and the exit set `bCanEverTick = false`, so they do not appear.

### A temporary Exec function: GSGrow

Testing growth by eating gems needs chapter 11. Until then, the snake has a console command:

```cpp
	/** Console: GSGrow 5 */
	UFUNCTION(Exec)
	void GSGrow(int32 Amount);
```

```cpp
void AGSSnake::GSGrow(int32 Amount)
{
	Grow(Amount);
	UE_LOG(LogGridSnake, Log, TEXT("GSGrow: %d segments pending"), PendingGrowth);
}
```

A `UFUNCTION(Exec)` becomes a console command on the objects the player controller asks in turn,
including itself, its HUD, its cheat manager and its possessed pawn. Type `GSGrow 5` while the
snake moves: it grows by five over the next five steps. Chapter 19 moves this into a cheat
manager, compiled out of shipping builds.

## Building the milestone

The changes against `project/ch08`:

- `Core/GSGameMode.h/.cpp`: the countdown (shown above).
- `Snake/GSSnake.h/.cpp`: ticking, the turn queue, `Segments`, growth, death, `PlaceOnBoard`,
  the grouped `Body` component and `GSGrow`. `SnapToBoard`, `UpdateHeadRotation` and the
  `Steer:` log line are gone.

`Source/GridSnake/Snake/GSSnake.h` declares everything shown above. In short: forward
declarations of `UPaperGroupedSpriteComponent` and `UPaperSprite`; the public `Tick`,
`StartMoving`, `Grow`, `GetLength`, `IsAlive` and the `Exec` `GSGrow`; the `Body` component and
the `BodySprite`, `InitialLength` (3), `MaxQueuedTurns` (2), `StepInterval` and `Segments`
properties (`Segments` replaces `HeadCell`); the private `QueueDirection`, `Step`, `Die`,
`PlaceOnBoard` and `UpdateVisuals`; and plain members `DirectionQueue`, `StepAccumulator`,
`PendingGrowth`, `bMoving` and `bAlive`. Open the snapshot for the exact specifiers. The plain
members hold no object pointers, so they need no `UPROPERTY`. `BeginPlay` reads `StepInterval`
from the settings and calls `PlaceOnBoard()`. `GSSnake.cpp` adds the includes
`"Core/GSSettings.h"` and `"PaperGroupedSpriteComponent.h"`. `Paper2D` has been in
`GridSnake.Build.cs` since chapter 5.

The editor steps follow `project/ch09/ASSETS.md`:

1. **Check the floor layer.** Open `TM_Level01`. In the tile map editor's **Layers** panel, select
   the `Floor` layer. In the Details panel for the layer, make sure **Layer Collides** is
   **unticked**, and that it is ticked for `Walls`. (The label may differ slightly in your
   version.) `IsWallTile` treats *every* tile on a colliding layer as a wall. A colliding
   `Floor` layer turns the whole board into walls, and the snake dies on its first step. Save.
2. **Build.** Close the editor and build `GridSnakeEditor` (new members can confuse Live Coding).
3. **Set the body sprite.** Open `BP_GSSnake`, click **Class Defaults**, and under **Snake** set:
   - **Body Sprite** = `S_SnakeBody`.
   - Leave **Initial Length** at 3 and **Max Queued Turns** at 2.

   Compile and save. `BP_GSGameMode` needs no change: its **Rules > Countdown Seconds** shows 3,
   inherited from C++.
4. **Play** in `L_Level01`, with the Output Log filtered for `LogGridSnake`.

Expected Output Log (filter: `LogGridSnake`):

```text
LogGridSnake: GameState ready: 0/3 gems
LogGridSnake: Board <GSBoard_0>: 24x14 cells of 16 units
LogGridSnake: View target: <GSBoard_0>
LogGridSnake: Snake placed at cell X=12 Y=7 facing EGSDirection::Right, length 3
LogGridSnake: GridSnake game mode started on map <UEDPIE_0_L_Level01> (collect 3 gems)
LogGridSnake: Countdown: 3
LogGridSnake: Countdown: 2
LogGridSnake: Countdown: 1
LogGridSnake: Go!
LogGridSnake: Snake starts moving EGSDirection::Right, one step every 0.15s
```

- The first lines are chapter 7's start-up (an exit's `starts closed` line may appear among
  them). The countdown lines come a second apart; then the snake steps every 0.15 s (**Project
  Settings > Game > GridSnake > Default Step Interval**).
- Steer with arrows, WASD or a gamepad. A quick U-turn makes both turns, one per step.
- Drive into a wall. The snake stops and logs a warning such as:

  ```text
  LogGridSnake: Warning: Snake hit a wall at cell X=5 Y=1 (length 3)
  ```

  The cell is illustrative; it is the head's last cell, next to the wall. Steering back into the
  body (make it longer with `GSGrow 5` first) logs `Snake bit itself at cell ...` instead.
- The snake drives *over* gems and the exit. They have no collision yet; that is chapter 10.
- Stop PIE with **Esc**.

### When it goes wrong

| Symptom | Cause | Fix |
|---|---|---|
| `Snake hit a wall` on the very first step, in open floor | The `Floor` layer has **Layer Collides** ticked | Untick it in `TM_Level01` (step 1) |
| The head moves but no body is drawn | **Body Sprite** not set | Set it in `BP_GSSnake` (step 3) |
| Body drawn behind the tile map | Body depth wrong, or the tile map moved to Y > 8 | Keep the board at Y = 0; body instances use Y +8 |
| Countdown logs, but the snake never moves | The pawn is not a `GSSnake`, so the cast fails | **Default Pawn Class** = `BP_GSSnake` in `BP_GSGameMode` (chapter 7) |
| No countdown at all | The level overrides the game mode | World Settings > **GameMode Override** = None or `BP_GSGameMode` |
| Turns are ignored | `SteerAction` or the mapping context is missing | The warnings from chapter 8 say which; check the input assets |
| `GSGrow` says "Command not recognized" | The game viewport does not have focus, or the pawn is not possessed | Click into the game view first; check the possession |

## Summary

- `Tick(DeltaSeconds)` runs on actors with `PrimaryActorTick.bCanEverTick`. Multiply rates by
  `DeltaSeconds`; for discrete steps, use an accumulator.
- A **fixed time step** banks frame time and spends it in whole steps with a `while` loop. Cap
  the catch-up (and rely on Max Undilated Frame Time) to avoid the spiral of death.
- An **input queue** keeps quick turns: compare with the *last queued* direction, ignore repeats
  and reversals, bound the queue.
- The snake is a `TArray<FIntPoint>`, head first: insert a head, drop the tail unless
  `PendingGrowth`. Self-bites skip the departing tail. Death is pure data in this chapter.
- `UPaperGroupedSpriteComponent` draws many sprite instances from one component. Mark the render
  state dirty once per batch.
- The **timer manager** runs one-shot, looping and next-tick timers on game time.
  `AGSGameMode` counts down 3-2-1 with a looping timer.
- `t.MaxFPS`, `slomo`, `pause`, `stat unit` and `dumpticks` test time-dependent code;
  `UFUNCTION(Exec)` on the pawn adds a quick console command.

## Exercises

1. **Cap the catch-up.** Change `AGSSnake::Tick` so that it runs at most 3 steps per frame. If
   more were owed, forget the extra debt. Test it with `t.MaxFPS 2` and a short Default Step
   Interval such as 0.05.

<details><summary>Solution</summary>

In `GSSnake.cpp`:

```cpp
	// In AGSSnake::Tick, replacing the while loop.
	constexpr int32 MaxStepsPerFrame = 3;

	StepAccumulator += DeltaSeconds;
	int32 StepsThisFrame = 0;
	while (bMoving && StepAccumulator >= StepInterval)
	{
		if (StepsThisFrame == MaxStepsPerFrame)
		{
			// Too far behind: drop the debt instead of spiralling.
			StepAccumulator = 0.0f;
			break;
		}
		StepAccumulator -= StepInterval;
		Step();
		++StepsThisFrame;
	}
```

At 2 fps a frame owes about ten 0.05 s steps; the snake takes three, so the game slows down
instead of freezing (Max Undilated Frame Time may shorten the frame first). To keep the step
phase, use `FMath::Fmod(StepAccumulator, StepInterval)` instead of `0.0f`.

</details>

2. **Wrap around.** Make the snake leave the board on one side and come back on the other,
   instead of dying at the edge. Walls inside the board must still kill it.

<details><summary>Solution</summary>

In `AGSSnake::Step`, wrap `NextCell` before testing it. `AGSBoard::GetGridSize()` (chapter 6)
returns the board's size in cells:

```cpp
	FIntPoint NextCell = Segments[0] + GSGrid::ToOffset(CurrentDirection);

	// Wrap around the edges. Adding Size before the modulo keeps -1 positive.
	const FIntPoint Size = Board->GetGridSize();
	NextCell.X = (NextCell.X + Size.X) % Size.X;
	NextCell.Y = (NextCell.Y + Size.Y) % Size.Y;

	if (Board->IsWallTile(NextCell))
	{
		Die(TEXT("hit a wall"));
		return;
	}
```

`IsInside` is no longer needed. C++'s `%` keeps the left operand's sign (`-1 % 24` is `-1`), hence
adding the size first. `TM_Level01` has a solid border, so remove some border tiles to see it.
Everything else works in cells and needs no change.

</details>

3. **A ghost arrow.** Show the next queued turn: a small arrow sprite drawn in the cell in front
   of the head, pointing in the queued direction. Hide it when the queue is empty. Use an arrow
   sprite of your own, or any sprite.

<details><summary>Solution</summary>

`GSSnake.h` forward-declares `class UPaperSpriteComponent;` and adds a component and a private
helper:

```cpp
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> NextTurnArrow;

	void UpdateNextTurnArrow();
```

In the constructor
(`GSSnake.cpp`, which now also includes `"PaperSpriteComponent.h"`):

```cpp
	NextTurnArrow = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("NextTurnArrow"));
	NextTurnArrow->SetupAttachment(Collision);
	NextTurnArrow->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	NextTurnArrow->SetVisibility(false);
```

The helper places the arrow one cell from the head, in the queued direction. Grid Y grows
downwards while world Z grows upwards, hence the minus sign:

```cpp
void AGSSnake::UpdateNextTurnArrow()
{
	if (DirectionQueue.Num() == 0 || !Board)
	{
		NextTurnArrow->SetVisibility(false);
		return;
	}

	const EGSDirection Next = DirectionQueue[0];
	const FIntPoint Offset = GSGrid::ToOffset(Next);
	const float Cell = Board->GetCellSize();
	NextTurnArrow->SetRelativeLocationAndRotation(
		FVector(Offset.X * Cell, 12.0f, -Offset.Y * Cell),
		GSGrid::ToSpriteRotation(Next));
	NextTurnArrow->SetVisibility(true);
}
```

Call it at the end of `QueueDirection`, at the end of `UpdateVisuals`, and in `Die`. In
`BP_GSSnake`, set the **NextTurnArrow** component's **Source Sprite**. Y +12 is in front of the
head; being relative to the actor, the arrow follows the snake.

</details>

4. **Count the cost.** Replace the grouped `Body` with one `UPaperSpriteComponent` per segment,
   created at run time. Compare `stat scenerendering` for both versions with a long snake
   (`GSGrow 100`).

<details><summary>Solution</summary>

Keep the comparison on a copy of the project, or revert afterwards. In `GSSnake.h`, next to
`Body`:

```cpp
	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	TArray<TObjectPtr<UPaperSpriteComponent>> BodyParts;
```

The `UPROPERTY` matters: it keeps the garbage collector informed. In `UpdateVisuals`, replace
the `Body` code (keep the first two lines that move the actor and turn the head):

```cpp
	const int32 NumBodySegments = Segments.Num() - 1;

	// Grow: create and register components until there is one per segment.
	while (BodyParts.Num() < NumBodySegments)
	{
		UPaperSpriteComponent* Part = NewObject<UPaperSpriteComponent>(this);
		Part->SetSprite(BodySprite);
		Part->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Part->SetupAttachment(Collision);
		Part->RegisterComponent();
		BodyParts.Add(Part);
	}

	for (int32 Index = 1; Index < Segments.Num(); ++Index)
	{
		BodyParts[Index - 1]->SetWorldLocation(Board->CellToWorld(Segments[Index]) + FVector(0.0f, BodyDepth, 0.0f));
	}
```

`NewObject` creates the component with the snake as outer; `RegisterComponent` creates its
render state. The engine registers `CreateDefaultSubobject` components, but run-time ones are
your job. The snake never shrinks, so there is no `DestroyComponent` code.

With `stat scenerendering` on, grow each version by 100 and compare draw calls and primitives.
Expect the per-component version to grow with the snake and the grouped one to stay nearly
flat; exact numbers depend on the renderer's own instancing, so compare the *trend*.

</details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-9--time-ticks-and-timers)
