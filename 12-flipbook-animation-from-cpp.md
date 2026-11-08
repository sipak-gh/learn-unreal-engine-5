# Chapter 12 — Flipbook Animation from C++

At the end of chapter 11 GridSnake was a real game: gems score and grow the snake, three of them
open the exit, and the level restarts when you win or die. It still looks stiff. The head plays
the same idle loop whether the snake waits, moves, eats or crashes. A gem simply vanishes, and the
exit door jumps from closed to open. This chapter makes the game move. The head gets four
animations: idle, move, eat and dead. Eaten gems pop, the exit plays an opening animation, the
body fades from head to tail, and a dead snake blinks. Paper2D has no animation graph for sprites,
so you will pick animations in C++. That takes a small state machine, completion events,
throw-away effect actors, and a little colour maths.

Previous: [Chapter 11](11-interfaces-delegates-and-game-rules.md). Next:
[Chapter 13](13-data-driven-levels.md).

> **Reminder.** Nothing in this course was compiled or run by the authors (see the README,
> "How this course was checked"). Every *Expected Output Log* block shows the lines the code's
> `UE_LOG` calls will print. Parts in `<angle brackets>` vary on your machine.

## Animation in Paper2D

You made the flipbooks in chapter 4: `FB_SnakeHead_Idle`, `FB_Gem_Pop`, `FB_Exit_Opening` and the
others in `/Game/GridSnake/Art/Flipbooks`. A **flipbook** (`UPaperFlipbook`) is a list of
*keyframes*. Each keyframe is a sprite plus a **Frame Run**, the number of frames it stays on
screen. The flipbook's **Frames Per Second** turns frames into time:

```text
length in seconds = total frames / Frames Per Second
FB_SnakeHead_Eat:   4 frames / 16 fps = 0.25 s
```

A **flipbook component** (`UPaperFlipbookComponent`) shows one flipbook at a time on its own
clock: every tick it adds `DeltaTime × PlayRate` to its playback position. Past the end, a
*looping* component wraps around; a non-looping one stops on the last frame and fires an event.
**Looping** and **Playing** default to true, which is why every flipbook so far animated without
a line of code.

### No animation graph

For 3D characters Unreal has the **Animation Blueprint**, a state machine asset. Paper2D has
nothing like it for flipbooks. The documentation page "Setting up an animation state machine"
builds one in a Blueprint's Event Graph: every tick it reads the character's velocity, picks a
flipbook with a **Select** node and calls **Set Flipbook**.

GridSnake does the same in C++, with less work. The snake never has to *guess* its state from its
velocity: it knows when it starts moving, eats or dies, and switches animations at exactly those
moments. Third-party plugins add real 2D animation graphs; chapter 20 lists one.

### The component's controls

The `UPaperFlipbookComponent` members this chapter uses (header `PaperFlipbookComponent.h`,
module `Paper2D`; the functions are all `BlueprintCallable`):

| Function | What it does |
|---|---|
| `SetFlipbook(UPaperFlipbook*)` | Shows another flipbook and rewinds to its start. Returns `false` and changes nothing if the flipbook is already the current one, or if the component cannot change (Mobility **Static**) |
| `GetFlipbook()` | The current flipbook, or `nullptr` |
| `SetLooping(bool)`, `IsLooping()` | Wrap around at the end, or stop there |
| `Play()` | Continue forward from the current position |
| `PlayFromStart()` | Rewind to 0, then `Play()` |
| `Stop()`, `IsPlaying()` | Freeze on the current frame; ask whether the clock runs |
| `Reverse()`, `ReverseFromEnd()` | Play backwards |
| `SetPlayRate(float)`, `GetPlayRate()` | Speed multiplier: 2 is twice as fast, 0.5 half as fast |
| `SetPlaybackPosition(float, bool)` | Jump to a time in seconds (the `bool` asks for events on the way; the current source ignores it) |
| `GetFlipbookLength()`, `GetFlipbookLengthInFrames()`, `GetFlipbookFramerate()` | Facts about the current flipbook |
| `SetSpriteColor(FLinearColor)` | Tint the sprite (section "Colour and visibility") |
| `OnFinishedPlaying` | Dynamic multicast delegate: a non-looping flipbook reached its end |

Two traps hide in this table:

- **`SetFlipbook` with the current flipbook does not rewind**; it returns `false` and the
  animation continues. Code that *means* "start this animation" calls `PlayFromStart()` after it.
- **`PlayFromStart` always rewinds.** Call it every step and the animation never gets past its
  first frames. Change animation only on a *change* of state.

### Example: a state machine in a map

The smallest useful state machine: one flipbook per state in a `TMap`, and a timer that pushes
the actor along Idle → Walk → Attack. Attack plays once, and its end returns the actor to Idle.

`examples/ch12/ExFlipbookStates.h`:

```cpp
// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExFlipbookStates.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;

UENUM(BlueprintType)
enum class EExAnimState : uint8
{
	Idle,
	Walk,
	Attack
};

/**
 * A state machine in a few lines: one flipbook per state, a 1 s timer that
 * walks Idle -> Walk -> Attack, and Attack (played once) returning to Idle
 * when its flipbook finishes.
 */
UCLASS()
class AExFlipbookStates : public AActor
{
	GENERATED_BODY()

public:
	AExFlipbookStates();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPaperFlipbookComponent> Sprite;

	/** One flipbook per state. A UENUM is a valid TMap key for a UPROPERTY. */
	UPROPERTY(EditAnywhere, Category = "Example")
	TMap<EExAnimState, TObjectPtr<UPaperFlipbook>> Flipbooks;

private:
	void Advance();
	void EnterState(EExAnimState NewState);
	void PlayFlipbookFor(EExAnimState ForState);

	/** Bound to OnFinishedPlaying, a dynamic delegate: must be a UFUNCTION. */
	UFUNCTION()
	void HandleFinished();

	EExAnimState State = EExAnimState::Idle;
	FTimerHandle Timer;
};
```

`examples/ch12/ExFlipbookStates.cpp`:

```cpp
// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExFlipbookStates.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExAnim, Log, All);

AExFlipbookStates::AExFlipbookStates()
{
	PrimaryActorTick.bCanEverTick = false;

	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	SetRootComponent(Sprite);
}

void AExFlipbookStates::BeginPlay()
{
	Super::BeginPlay();

	Sprite->OnFinishedPlaying.AddDynamic(this, &AExFlipbookStates::HandleFinished);
	UE_LOG(LogExAnim, Log, TEXT("%s starts in %s"), *GetName(), *UEnum::GetValueAsString(State));
	PlayFlipbookFor(State);

	GetWorldTimerManager().SetTimer(Timer, this, &AExFlipbookStates::Advance, 1.0f, /*bLoop=*/true);
}

void AExFlipbookStates::Advance()
{
	// The timer drives Idle -> Walk -> Attack. Attack is left by its own end, not by the timer.
	if (State == EExAnimState::Idle)
	{
		EnterState(EExAnimState::Walk);
	}
	else if (State == EExAnimState::Walk)
	{
		EnterState(EExAnimState::Attack);
	}
}

void AExFlipbookStates::EnterState(EExAnimState NewState)
{
	if (NewState == State)
	{
		return; // never restart the running animation by accident
	}

	UE_LOG(LogExAnim, Log, TEXT("%s: %s -> %s"), *GetName(),
		*UEnum::GetValueAsString(State), *UEnum::GetValueAsString(NewState));
	State = NewState;
	PlayFlipbookFor(State);
}

void AExFlipbookStates::PlayFlipbookFor(EExAnimState ForState)
{
	const TObjectPtr<UPaperFlipbook>* Found = Flipbooks.Find(ForState);
	if (!Found || !*Found)
	{
		UE_LOG(LogExAnim, Warning, TEXT("%s has no flipbook for %s"), *GetName(), *UEnum::GetValueAsString(ForState));
		return;
	}

	Sprite->SetFlipbook(*Found);                         // resets play time if it is a new flipbook
	Sprite->SetLooping(ForState != EExAnimState::Attack); // only non-looping flipbooks finish
	Sprite->PlayFromStart();
}

void AExFlipbookStates::HandleFinished()
{
	EnterState(EExAnimState::Idle);
}
```

Modules: `Core`, `CoreUObject`, `Engine`, `Paper2D`. GridSnake's `GridSnake.Build.cs` already lists
`Paper2D` (chapter 4); a scratch project needs it added to `PublicDependencyModuleNames`.

To see it:

1. Copy both files into `Source/GridSnake/` (or a scratch project with Paper2D) and build. Delete
   them again when you are done with the chapter's examples.
2. Open `L_Level01`. In the Content Browser, open **C++ Classes > GridSnake** and drag an
   `ExFlipbookStates` into the level. Set its **Location** to (32, 20, -32): the centre of cell
   (2, 2), and at Y = 20 in front of everything else.
3. In its Details panel, under **Example > Flipbooks**, click **+** three times. Set the keys and
   values to **Idle** → `FB_SnakeHead_Idle`, **Walk** → `FB_SnakeHead_Move`, **Attack** →
   `FB_SnakeHead_Eat`. (A new map entry starts with the key **Idle**; change the keys before
   adding the next entry, because a map cannot hold the same key twice.)
4. Type `LogExAnim` into the Output Log's search box and press **Play**.

Expected Output Log (filter: `LogExAnim`), for the first five seconds:

```text
LogExAnim: <ExFlipbookStates_0> starts in EExAnimState::Idle
LogExAnim: <ExFlipbookStates_0>: EExAnimState::Idle -> EExAnimState::Walk
LogExAnim: <ExFlipbookStates_0>: EExAnimState::Walk -> EExAnimState::Attack
LogExAnim: <ExFlipbookStates_0>: EExAnimState::Attack -> EExAnimState::Idle
LogExAnim: <ExFlipbookStates_0>: EExAnimState::Idle -> EExAnimState::Walk
LogExAnim: <ExFlipbookStates_0>: EExAnimState::Walk -> EExAnimState::Attack
LogExAnim: <ExFlipbookStates_0>: EExAnimState::Attack -> EExAnimState::Idle
```

- **The timing.** The timer fires at 1 s (Walk) and 2 s (Attack). The 0.25 s Attack flipbook
  returns to Idle at about 2.25 s, and the cycle repeats every 2 seconds. The log file,
  `Saved/Logs/GridSnake.log`, timestamps every line if you want to check.
- **Two different drivers.** The timer leaves Idle and Walk; the flipbook's end leaves Attack, so
  a timer tick cannot cut the attack short. `EnterState` refuses a non-change, so nothing
  restarts an animation from frame 0 by accident.
- **Remove the Attack entry** and play: `has no flipbook for EExAnimState::Attack` is logged as a
  warning, and the actor is stuck in Attack, because nothing will ever finish. A missing asset in
  a one-shot state blocks the machine; remember that for the snake.
- **A `TMap` with a `UENUM` key** is a valid `UPROPERTY` with a proper editor UI, handy for many
  states. GridSnake has four, so it uses one property per state.

## A tiny state machine: EGSHeadAnim

The snake's head has four looks. Each is a value of a new enum in `Snake/GSSnake.h`:

```cpp
/** The head's animation states. Each one maps to a flipbook asset. */
UENUM(BlueprintType)
enum class EGSHeadAnim : uint8
{
	Idle,
	Move,
	Eat,
	Dead
};
```

`UENUM(BlueprintType)` makes the enum usable in Blueprints, for a future HUD or sound that wants
to know the head's state. A public getter goes with it:

```cpp
	UFUNCTION(BlueprintPure, Category = "Snake")
	EGSHeadAnim GetHeadAnim() const { return HeadAnim; }
```

### One property per state

The flipbooks are assets, so they are `UPROPERTY`s set in `BP_GSSnake`, never loaded by path in
C++. They get their own category, **Animation**, in the protected section of `AGSSnake`:

```cpp
	/** Looping: before the countdown ends. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> IdleFlipbook;

	/** Looping: while moving. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> MoveFlipbook;

	/** Played once when the snake grows, then back to Move. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> EatFlipbook;

	/** Played once on death; the last frame stays. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> DeadFlipbook;
```

`EditDefaultsOnly` because the look of the snake belongs to its class, not to one snake in one
level. The header forward-declares `class UPaperFlipbook;`; `GSSnake.cpp` includes
`PaperFlipbook.h`.

### Entering a state

All the state machine logic is one function. Private, in `GSSnake.h`:

```cpp
	void PlayHeadAnim(EGSHeadAnim NewAnim);

	/** Bound to the head flipbook's OnFinishedPlaying: one-shot animations end here. */
	UFUNCTION()
	void HandleHeadAnimFinished();

	// ... next to the other private members:
	EGSHeadAnim HeadAnim = EGSHeadAnim::Idle;
```

In `GSSnake.cpp`:

```cpp
void AGSSnake::PlayHeadAnim(EGSHeadAnim NewAnim)
{
	UPaperFlipbook* Flipbook = nullptr;
	bool bLoop = true;
	switch (NewAnim)
	{
	case EGSHeadAnim::Idle: Flipbook = IdleFlipbook; break;
	case EGSHeadAnim::Move: Flipbook = MoveFlipbook; break;
	case EGSHeadAnim::Eat:  Flipbook = EatFlipbook;  bLoop = false; break;
	case EGSHeadAnim::Dead: Flipbook = DeadFlipbook; bLoop = false; break;
	}

	HeadAnim = NewAnim;
	if (!Flipbook)
	{
		return;
	}

	// The state machine is just this switch: pick a flipbook, decide whether
	// it loops, restart it. One-shot states end in HandleHeadAnimFinished().
	Head->SetFlipbook(Flipbook);
	Head->SetLooping(bLoop);
	Head->PlayFromStart();
}
```

- **The `switch` is the whole mapping.** A state picks a flipbook and decides whether it loops.
  Eat and Dead play once; Idle and Move loop. The `switch` has no `default`, so if you add a fifth
  state, compilers that check enum switches (Clang does by default) warn that it is not handled.
- **`HeadAnim` changes even without a flipbook.** The state is game logic; the flipbook is only
  its picture. With an empty **Eat Flipbook** the head keeps its current animation, but
  `GetHeadAnim()` still reports Eat. The game plays fine with no animations at all.
- **`SetFlipbook`, `SetLooping`, `PlayFromStart`**, in this order, so a one-shot never wraps
  around. `PlayFromStart` makes "eat again while eating" restart the eat animation.

`PlayHeadAnim` is called from the places where the snake's situation changes, never from `Tick`
or `Step`:

| Moment | Function | Head state |
|---|---|---|
| The level starts | `BeginPlay` | Idle |
| The countdown ends | `StartMoving` | Move |
| The level is won | `StopMoving` (if still alive) | Idle |
| A gem is eaten | `Grow` (if alive) | Eat, then back to Move or Idle |
| A wall or the body is hit | `Die` | Dead, and it stays |

The snake steps about seven times a second. If `Step` called `PlayHeadAnim(EGSHeadAnim::Move)`,
`PlayFromStart` would rewind the move loop each time, and the head would show its first frame
forever. Calling it on *events* starts each animation exactly once per change.

The calls are short. `BeginPlay`, after `PlaceOnBoard()`, binds the handler and starts idling:

```cpp
	Head->OnFinishedPlaying.AddDynamic(this, &AGSSnake::HandleHeadAnimFinished);
	PlayHeadAnim(EGSHeadAnim::Idle);
```

`StartMoving` adds `PlayHeadAnim(EGSHeadAnim::Move);` after `StepAccumulator = 0.0f;`.
`StopMoving` adds `if (bAlive) { PlayHeadAnim(EGSHeadAnim::Idle); }` at the end, so that no
future caller turns a dead head back to idle. `Grow` changes from one line to:

```cpp
void AGSSnake::Grow(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	PendingGrowth += Amount;
	if (bAlive)
	{
		PlayHeadAnim(EGSHeadAnim::Eat);
	}
}
```

`Grow(0)` must not play the eat animation, so the function returns early. The `GSGrow` console
command goes through `Grow`, so `GSGrow 2` plays the eat animation without driving to a gem.

## One-shot animations and OnFinishedPlaying

A looping flipbook never ends. A non-looping one plays to its last frame, stops there, and the
component broadcasts **`OnFinishedPlaying`**. It is declared in `PaperFlipbookComponent.h` as a
dynamic multicast delegate with no parameters, and it is `BlueprintAssignable`. In a Blueprint
it is the **On Finished Playing** event of the component.

Three facts about it come straight from the component's tick code:

- **It fires only for non-looping flipbooks.** A looping one wraps around silently.
- **It fires from the component's tick**, in the first tick that passes the end: at 60 frames
  per second, up to about 17 ms late. The actor's own tick can be off; the component ticks on
  its own.
- **`Stop()` does not fire it**, and neither does `SetFlipbook`. Only reaching the end does (or
  the start, when a non-looping flipbook plays in reverse).

`OnFinishedPlaying` is dynamic, so the function you bind must be a `UFUNCTION`, exactly as in
chapter 11. Without one, `AddDynamic` compiles but reports an ensure at run time, and the event
never arrives.

### Eat, then move on

The snake's handler is the second half of its state machine:

```cpp
void AGSSnake::HandleHeadAnimFinished()
{
	// Only non-looping flipbooks finish. Dead stays on its last frame.
	if (HeadAnim == EGSHeadAnim::Eat && bAlive)
	{
		PlayHeadAnim(bMoving ? EGSHeadAnim::Move : EGSHeadAnim::Idle);
	}
}
```

- **The handler checks the state, not the flipbook.** Eat and Dead both finish; only Eat leads
  somewhere. Dead stays on its last frame, because a stopped component keeps showing it.
- **`bMoving` decides where to go.** If the third gem lies next to the exit, the 0.25 s eat
  animation can finish after `StopMoving`.
- **`bAlive` is cheap insurance.** `Die` replaces Eat with Dead without calling the handler
  (`SetFlipbook` and `PlayFromStart` never broadcast), but a future order of calls might.

### The exit: Opening, then Open

The exit door works the same way with two flipbooks. One new property in `Gameplay/GSExit.h`,
between the existing two:

```cpp
	/** Played once when the exit opens during play, then OpenFlipbook loops. */
	UPROPERTY(EditDefaultsOnly, Category = "Exit")
	TObjectPtr<UPaperFlipbook> OpeningFlipbook;
```

and a private handler next to `HandleAllGemsCollected`:

```cpp
	UFUNCTION()
	void HandleDoorAnimFinished();
```

In `GSExit.cpp`, `BeginPlay` binds it right after the game state binding from chapter 11:

```cpp
	Door->OnFinishedPlaying.AddDynamic(this, &AGSExit::HandleDoorAnimFinished);
```

`SetOpen` now decides between an animated and an instant change:

```cpp
	bIsOpen = bNewOpen;
	UpdateCollision();

	// In play, opening is animated: the one-shot Opening flipbook first, then
	// HandleDoorAnimFinished() switches to the looping Open flipbook.
	if (bIsOpen && OpeningFlipbook && HasActorBegunPlay())
	{
		Door->SetFlipbook(OpeningFlipbook);
		Door->SetLooping(false);
		Door->PlayFromStart();
	}
	else
	{
		UpdateVisuals();
	}

	UE_LOG(LogGridSnake, Log, TEXT("%s is now %s"), *GetName(), bIsOpen ? TEXT("open") : TEXT("closed"));
}

void AGSExit::HandleDoorAnimFinished()
{
	if (bIsOpen)
	{
		UpdateVisuals();
	}
}

void AGSExit::UpdateVisuals()
{
	UPaperFlipbook* Wanted = bIsOpen ? OpenFlipbook : ClosedFlipbook;
	if (Wanted)
	{
		Door->SetFlipbook(Wanted);
		Door->SetLooping(true);
		Door->PlayFromStart();
	}
}
```

- **The collision opens at once; only the picture is animated.** A snake that reaches the exit
  during the 0.4 s opening still gets through. Game logic never waits for an animation, so a
  missing animation can never block the game.
- **`HasActorBegunPlay()`** keeps the animation for play. If something calls `SetOpen` before
  `BeginPlay` (a construction script, say), the door switches instantly, as `OnConstruction`
  (chapter 5) does for an exit placed with **Is Open** ticked.
- **`SetLooping(true)` in `UpdateVisuals`** undoes the opening's `SetLooping(false)`; without it,
  the Open loop would play once and freeze. With no **Opening Flipbook**, `SetOpen` falls back
  to `UpdateVisuals`, chapter 11's behaviour.

The whole exit sequence, for a level played to the third gem:

```text
level starts       Door: FB_Exit_Closed (looping, 1 frame)
third gem          SetOpen(true): collision opens, Door: FB_Exit_Opening (once, 0.4 s)
0.4 s later        OnFinishedPlaying -> HandleDoorAnimFinished -> UpdateVisuals
                   Door: FB_Exit_Open (looping)
```

## Effect actors

A gem pop is an animation without a home. The gem is destroyed the moment it is eaten, so its
own flipbook component cannot play the pop: it disappears with the gem. The usual answer is a
tiny **effect actor**. You spawn it where the effect should be, it plays its flipbook once, and
it destroys itself when `OnFinishedPlaying` fires. Each kind of effect is a Blueprint child with
its own flipbook, and the code that spawns it only needs a class.

### Example: a one-shot effect

`examples/ch12/ExOneShotEffect.h`:

```cpp
// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExOneShotEffect.generated.h"

class UPaperFlipbookComponent;

/**
 * Plays its flipbook once and destroys itself. Make a Blueprint child per
 * effect, set its Source Flipbook, and spawn it with SpawnActor.
 */
UCLASS()
class AExOneShotEffect : public AActor
{
	GENERATED_BODY()

public:
	AExOneShotEffect();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

private:
	UFUNCTION()
	void HandleFinished();

	double StartTime = 0.0;
};
```

`examples/ch12/ExOneShotEffect.cpp`:

```cpp
// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExOneShotEffect.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "PaperFlipbookComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogExFx, Log, All);

AExOneShotEffect::AExOneShotEffect()
{
	PrimaryActorTick.bCanEverTick = false; // the component ticks on its own

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	SetRootComponent(Flipbook);
	Flipbook->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Flipbook->SetLooping(false);

	// Safety net: even if the flipbook never finishes (say, other code switches
	// Looping back on), the actor is destroyed after this many seconds.
	InitialLifeSpan = 5.0f;
}

void AExOneShotEffect::BeginPlay()
{
	Super::BeginPlay();

	if (!Flipbook->GetFlipbook())
	{
		UE_LOG(LogExFx, Warning, TEXT("%s has no flipbook, destroying"), *GetName());
		Destroy();
		return;
	}

	Flipbook->OnFinishedPlaying.AddDynamic(this, &AExOneShotEffect::HandleFinished);
	Flipbook->SetLooping(false);
	Flipbook->PlayFromStart();
	StartTime = GetWorld()->GetTimeSeconds();
}

void AExOneShotEffect::HandleFinished()
{
	UE_LOG(LogExFx, Log, TEXT("%s finished after %.2f s (flipbook length %.2f s), destroying"), *GetName(),
		GetWorld()->GetTimeSeconds() - StartTime, Flipbook->GetFlipbookLength());
	Destroy();
}
```

Modules: `Core`, `CoreUObject`, `Engine`, `Paper2D`.

To see it:

1. Copy both files into `Source/GridSnake/` and build.
2. In the Content Browser, right-click `ExOneShotEffect` under **C++ Classes > GridSnake** and
   choose **Create Blueprint class based on ExOneShotEffect**. Name it `BP_ExPop`.
3. Open `BP_ExPop`, select the **Flipbook** component, set **Source Flipbook** = `FB_Gem_Pop`.
   Compile and save.
4. Drag two `BP_ExPop` into `L_Level01` at (32, 20, -32) and (64, 20, -32). On the second one,
   select **Flipbook**, open the **Source Flipbook** drop-down and choose **Clear**.
5. Type `LogExFx` into the Output Log's search box and press **Play**.

Expected Output Log (filter: `LogExFx`):

```text
LogExFx: Warning: <BP_ExPop_C_1> has no flipbook, destroying
LogExFx: <BP_ExPop_C_0> finished after <0.32> s (flipbook length 0.31 s), destroying
```

- **The pop plays once, about a third of a second, and both actors vanish** from the Outliner.
  `FB_Gem_Pop` has 5 frames at 16 fps: 0.3125 s, printed as `0.31`.
- **"Finished after" is a little longer** than the length, because `OnFinishedPlaying` fires in
  the first component tick past the end. The exact number depends on your frame rate.
- **The empty one warns and destroys itself at once.** Without a flipbook the component would
  never finish, and the actor would live until its life span ran out.
- **`InitialLifeSpan`** is an `AActor` property: after that many seconds the actor destroys itself.
  It is a second line of defence. To see it work, set **Initial Life Span** = 0.1 in `BP_ExPop`'s
  **Class Defaults** (category **Actor**) and play: the pop vanishes mid-animation, and no
  `finished` line is logged.
- **The actor does not tick.** The component does, which is all a flipbook needs.

Delete the two actors and `BP_ExPop` when you are done.

### AGSFlipbookEffect

GridSnake's version is the same class without the timing log and the life span, plus a depth
offset. `Source/GridSnake/Gameplay/GSFlipbookEffect.h` (new) declares `AGSFlipbookEffect` exactly
like `AExOneShotEffect`, minus `StartTime`. Its component is `UPROPERTY(VisibleAnywhere,
BlueprintReadOnly, Category = "Components") TObjectPtr<UPaperFlipbookComponent> Flipbook`, and
its private handler is `UFUNCTION() void HandleFinishedPlaying();`.

`Source/GridSnake/Gameplay/GSFlipbookEffect.cpp` (new):

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSFlipbookEffect.h"
#include "GridSnake.h"
#include "Engine/CollisionProfile.h"
#include "PaperFlipbookComponent.h"

AGSFlipbookEffect::AGSFlipbookEffect()
{
	PrimaryActorTick.bCanEverTick = false;

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	SetRootComponent(Flipbook);
	Flipbook->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Flipbook->SetLooping(false);
}

void AGSFlipbookEffect::BeginPlay()
{
	Super::BeginPlay();

	if (!Flipbook->GetFlipbook())
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s has no flipbook"), *GetName());
		Destroy();
		return;
	}

	// Depth (Y): in front of gems and doors (+5) and the body (+8), behind the head (+10).
	AddActorWorldOffset(FVector(0.0f, 9.0f, 0.0f));
	Flipbook->OnFinishedPlaying.AddDynamic(this, &AGSFlipbookEffect::HandleFinishedPlaying);
	Flipbook->SetLooping(false);
	Flipbook->PlayFromStart();
}

void AGSFlipbookEffect::HandleFinishedPlaying()
{
	Destroy();
}
```

- **Depth decides what covers what.** The board's camera looks along −Y from far out on +Y
  (chapter 6), so a larger Y is closer to the camera and drawn on top. GridSnake's layers are:
  tile map 0, gems and the exit door +5, body +8, effects +9, head +10. The effect is spawned
  at the gem's actor location (Y = 0) and moves itself to +9. At the moment of the pop the head
  is on the same cell and covers the middle of it; one step later a body segment is there, and
  the pop is drawn over it.
- **`SetLooping(false)` twice.** The constructor sets the default the Blueprint shows; the call
  in `BeginPlay` overrides a **Looping** ticked in a Blueprint child. **`NoCollision`**: effects
  are pictures and must never trigger the snake's overlap code.

### Spawning the pop

The gem gets a class property, in `Gameplay/GSGem.h` (with a forward declaration
`class AGSFlipbookEffect;` at the top):

```cpp
	/** Spawned where the gem was when it is collected (BP_GSGemPop). */
	UPROPERTY(EditDefaultsOnly, Category = "Gem")
	TSubclassOf<AGSFlipbookEffect> PickupEffectClass;
```

`TSubclassOf<AGSFlipbookEffect>` is a `UClass*` that the editor restricts to `AGSFlipbookEffect`
and its children: the **Pickup Effect Class** drop-down lists `BP_GSGemPop` and nothing that would
not fit. The C++ side does not know which flipbook the pop uses; the Blueprint child does.

`GSGem.cpp` includes `Engine/World.h` and `Gameplay/GSFlipbookEffect.h`. `OnSnakeEntered`
spawns the effect before the gem destroys itself:

```cpp
void AGSGem::OnSnakeEntered_Implementation(AGSSnake* Snake)
{
	// The gem only reports; the game mode decides what a gem is worth.
	if (AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GameMode->OnGemCollected(this, Snake);
	}

	if (PickupEffectClass)
	{
		GetWorld()->SpawnActor<AGSFlipbookEffect>(PickupEffectClass, GetActorTransform());
	}

	Destroy();
}
```

- **`SpawnActor<T>(UClass*, const FTransform&)`** constructs the actor and, because the game is
  running, calls its `BeginPlay` before returning: the pop is already playing. It is not
  attached to the gem, so destroying the gem does not touch it.
- **No class, no pop.** An empty **Pickup Effect Class** skips the spawn, as in chapter 11.
- **Why not pool?** Spawning costs a little each time; for three pops per level that does not
  matter. Games with hundreds of effects per second reuse hidden actors (exercise 2).

## Colour and visibility

Not every animation needs new frames. Two cheap tricks change how existing sprites look: tinting
them, and hiding them.

### Tinting with vertex colour

Every Paper2D sprite carries a colour in its vertices. The default sprite materials, including
`MaskedUnlitSpriteMaterial` (chapter 4's default for new sprites), multiply the texture by this
**vertex colour**. White leaves the texture unchanged, (1, 0.2, 0.2) keeps the red channel and
darkens the others, and black makes the sprite black. Three components set it:

| Component | Call |
|---|---|
| `UPaperSpriteComponent` | `SetSpriteColor(FLinearColor)` |
| `UPaperFlipbookComponent` | `SetSpriteColor(FLinearColor)`, the same tint for every frame |
| `UPaperGroupedSpriteComponent` | per instance: the last parameter of `AddInstance`, or `UpdateInstanceColor(Index, Color)` |

Three things to know:

- **Tinting multiplies, so it only darkens.** A tint cannot make a sprite brighter than its
  texture. Art meant to be tinted is often drawn in light greys.
- **Alpha is not a fade with a masked material.** `MaskedUnlitSpriteMaterial` either draws a
  pixel or drops it. Lowering the colour's alpha makes pixels vanish once they fall below the
  material's opacity mask clip value; it does not make them see-through. A smooth fade needs a
  translucent material such as `TranslucentUnlitSpriteMaterial`, set as the component's material.
- **Your own sprite material must use vertex colour too.** If you write one (the documentation
  page "Paper 2D sprite material" shows how), multiply the texture sample by a **Vertex Color**
  node. Without that node, `SetSpriteColor` changes nothing. To see how the defaults do it, turn
  on **Show Plugin Content** in the Content Browser's **Settings** and open
  `MaskedUnlitSpriteMaterial` in the Paper2D plugin's content folder.

### Example: a pulsing tint

`examples/ch12/ExSpritePulse.h`:

```cpp
// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSpritePulse.generated.h"

class UPaperFlipbookComponent;
class UPaperSpriteComponent;

/**
 * Tints a sprite and a flipbook with a colour that pulses between ColorA and
 * ColorB. Tinting needs no extra material: the default sprite materials
 * multiply the texture by the vertex colour that SetSpriteColor sets.
 */
UCLASS()
class AExSpritePulse : public AActor
{
	GENERATED_BODY()

public:
	AExSpritePulse();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

	UPROPERTY(EditAnywhere, Category = "Example")
	FLinearColor ColorA = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Example")
	FLinearColor ColorB = FLinearColor(1.0f, 0.2f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Example", meta = (ClampMin = "0"))
	float PulsesPerSecond = 1.0f;

private:
	float RunningTime = 0.0f;
};
```

`examples/ch12/ExSpritePulse.cpp`:

```cpp
// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExSpritePulse.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"

AExSpritePulse::AExSpritePulse()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	SetRootComponent(Sprite);

	// Sprites lie in the X-Z plane: +X is to the right of the sprite.
	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Sprite);
	Flipbook->SetRelativeLocation(FVector(24.0f, 0.0f, 0.0f));
}

void AExSpritePulse::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("%s pulses %s <-> %s, %.1f per second"), *GetName(),
		*ColorA.ToString(), *ColorB.ToString(), PulsesPerSecond);
}

void AExSpritePulse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RunningTime += DeltaSeconds;

	// Sin runs from -1 to 1; remap it to 0..1 for the blend.
	const float Alpha = 0.5f + 0.5f * FMath::Sin(RunningTime * UE_TWO_PI * PulsesPerSecond);
	const FLinearColor Color = FLinearColor::LerpUsingHSV(ColorA, ColorB, Alpha);

	Sprite->SetSpriteColor(Color);
	Flipbook->SetSpriteColor(Color);
}
```

Modules: `Core`, `CoreUObject`, `Engine`, `Paper2D`.

To see it:

1. Copy both files into `Source/GridSnake/` and build.
2. Drag an `ExSpritePulse` into `L_Level01` at (32, 20, -32). Select its **Sprite** component and
   set **Source Sprite** to the body sprite, `S_SnakeBody`. Select **Flipbook** and set **Source
   Flipbook** = `FB_Gem_Idle`.
3. Type `LogTemp` into the Output Log's search box and press **Play**.

Expected Output Log (filter: `LogTemp`):

```text
LogTemp: <ExSpritePulse_0> pulses (R=1.000000,G=1.000000,B=1.000000,A=1.000000) <-> (R=1.000000,G=0.200000,B=0.200000,A=1.000000), 1.0 per second
```

- **Both sprites pulse from their own colours to red and back, once a second.** The gem keeps
  animating: the tint and the frames are independent.
- **`FMath::Sin` of time × `2π × frequency`** oscillates at that frequency; the bobbing gem
  (chapters 5 and 9) uses the same idea.
- **Setting the colour every tick** sends new data to the renderer every frame. Fine for a few
  actors; for thousands of sprites, animate a material parameter instead.

Delete the actor and the example files when you are done.

### A body that fades

GridSnake tints each body segment, from **Body Head Color** next to the head to **Body Tail
Color** at the tail tip. Two properties in the snake's **Animation** category:

```cpp
	/** Tint of the segment behind the head. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FLinearColor BodyHeadColor = FLinearColor::White;

	/** Tint of the tail tip; segments in between blend. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FLinearColor BodyTailColor = FLinearColor(0.35f, 0.55f, 0.35f);
```

A private `FLinearColor GetSegmentColor(int32 SegmentIndex) const;` computes the tint of one
segment:

```cpp
FLinearColor AGSSnake::GetSegmentColor(int32 SegmentIndex) const
{
	// 0 for the first body segment, 1 for the tail tip.
	const int32 NumBodySegments = Segments.Num() - 1;
	const float Alpha = NumBodySegments > 1 ? float(SegmentIndex - 1) / float(NumBodySegments - 1) : 0.0f;
	return FLinearColor::LerpUsingHSV(BodyHeadColor, BodyTailColor, Alpha);
}
```

`UpdateVisuals` passes the colour to `AddInstance` where chapter 9 passed `FLinearColor::White`:

```cpp
			Body->AddInstance(Transform, BodySprite, /*bWorldSpace=*/true, GetSegmentColor(Index));
```

- **`Segments[0]` is the head**, so `Alpha` maps body index 1 to 0 and the last index to 1. With
  a single body segment that would divide by zero, hence the `NumBodySegments > 1` test.
- **Colours are set only when the instances are rebuilt**, that is, when the length changes
  (chapter 9). The colour depends only on the index and the length, so moving instances keeps
  it right, and a growing snake gets a new gradient on every growth step.
- **`LerpUsingHSV` blends in hue, saturation and value**, taking the short way round the colour
  wheel. White has no hue; the engine stores it as 0, which is red. So the blend from white to
  green turns slightly warm on the way. Worked out with the formulas in the engine's `Color.cpp`,
  a 5-cell snake (4 body segments) gets:

| Segment | Alpha | `LerpUsingHSV` (R, G, B) | `FMath::Lerp` (R, G, B) |
|---|---|---|---|
| 1 | 0.00 | 1.00, 1.00, 1.00 | 1.00, 1.00, 1.00 |
| 2 | 0.33 | 0.85, 0.82, 0.75 | 0.78, 0.85, 0.78 |
| 3 | 0.67 | 0.64, 0.70, 0.53 | 0.57, 0.70, 0.57 |
| 4 | 1.00 | 0.35, 0.55, 0.35 | 0.35, 0.55, 0.35 |

  The HSV column passes through beige and olive; `FMath::Lerp(BodyHeadColor, BodyTailColor,
  Alpha)` stays grey-green. Keep the one you like. HSV shines between two saturated colours.

### The death blink

When the snake dies, its body blinks four times before the level restarts. It is a looping timer
that toggles the body's visibility and counts down. New members in `GSSnake.h`: a property in the
**Animation** category,

```cpp
	/** How often the body blinks after death. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (ClampMin = "0"))
	int32 DeathBlinks = 4;
```

and private `void ToggleBodyVisibility();`, `FTimerHandle DeathBlinkTimer;` and
`int32 DeathBlinksLeft = 0;`. `GSSnake.cpp` now includes `TimerManager.h`. At the end of `Die`,
before the broadcast:

```cpp
	PlayHeadAnim(EGSHeadAnim::Dead);
	DeathBlinksLeft = DeathBlinks * 2;
	if (DeathBlinksLeft > 0)
	{
		GetWorldTimerManager().SetTimer(DeathBlinkTimer, this, &AGSSnake::ToggleBodyVisibility, 0.1f, /*bLoop=*/true);
	}

	OnDied.Broadcast(this);
}

void AGSSnake::ToggleBodyVisibility()
{
	Body->SetVisibility(!Body->IsVisible());
	if (--DeathBlinksLeft <= 0)
	{
		Body->SetVisibility(true);
		GetWorldTimerManager().ClearTimer(DeathBlinkTimer);
	}
}
```

- **One blink is two toggles**, off and on, hence `DeathBlinks * 2`. Four blinks at 0.1 s per
  toggle take 0.8 s, which fits inside the game mode's 1.5 s **Level End Delay**.
- **The head is not blinking.** `Body` is the grouped sprite component; the head is a separate
  component that shows its Dead flipbook throughout.
- **`SetVisibility(true)` at the end** is a safety net: whatever the count, the body ends up
  visible. With an odd number of toggles it would otherwise stay hidden.
- **`SetVisibility` versus `SetHiddenInGame`.** The first hides a component in game and editor,
  the second only in play. Both work for a blink; `SetVisibility` pairs neatly with `IsVisible`.

## Frame rate and timing

Two clocks run in GridSnake now. The snake steps every **Step Interval** (0.15 s by default,
from **Project Settings > Game > GridSnake**). Each flipbook plays at its own **Frames Per
Second** (set in chapter 4). They know nothing of each other.

`examples/ch12/flipbook_timing.py` reads the placeholder strips that `Tools/make_placeholder_art.py`
writes, counts the 16-pixel frames in each, and prints every flipbook's length. It uses only the
Python standard library, so unlike the C++ examples you can run it right now. From the folder
that holds `GridSnake.uproject`:

```console
$ python3 Tools/make_placeholder_art.py
wrote Textures/T_Tiles.png (64x32)
wrote Textures/T_SnakeHead_Idle.png (32x16)
wrote Textures/T_SnakeHead_Move.png (32x16)
...
wrote Audio/SW_Music.wav (8.00s)
$ python3 ../../examples/ch12/flipbook_timing.py RawAssets/Textures
flipbook             frames  fps   length  mode  PlayRate for 1 cycle/step
FB_SnakeHead_Idle         2    4   0.500s  loop  3.33
FB_SnakeHead_Move         2    8   0.250s  loop  1.67
FB_SnakeHead_Eat          4   16   0.250s  once  -
FB_SnakeHead_Dead         3    8   0.375s  once  -
FB_Gem_Idle               6    8   0.750s  loop  5.00
FB_Gem_Pop                5   16   0.312s  once  -
FB_Exit_Opening           4   10   0.400s  once  -
FB_Exit_Open              4    8   0.500s  loop  3.33
```

This output is real: it was produced by running the script with Python 3.10 on the course's
placeholder art. (The path `../../examples` assumes you are in `project/ch12` of the course
repository; adjust it if your project lives elsewhere.) An optional second argument sets the step
interval; with `0.1`, the loop column reads 5.00, 2.50, 7.50 and 5.00.

The core of the script (the rest is argument handling and printing) reads each strip's width
from the PNG header and divides:

```python
def png_width(path):
    # A PNG starts with an 8-byte signature and the IHDR chunk header (8 bytes);
    # width and height follow as big-endian 32-bit integers.
    with open(path, "rb") as f:
        header = f.read(24)
    width, _height = struct.unpack(">II", header[16:24])
    return width

# for each flipbook name and its Frames Per Second from chapter 4:
        frames = png_width(os.path.join(folder, "T_%s.png" % name)) // TILE
        length = frames / fps
        rate = "-" if mode == "once" else "%.2f" % (length / step)
```

The script assumes one keyframe per sprite with a **Frame Run** of 1, which is what **Create
Flipbook** produced in chapter 4. If you lengthen a keyframe in the flipbook editor, the real
length grows; the flipbook editor's timeline, or `GetFlipbookLength()` in C++, is then the
authority.

What the table tells you:

- **The move loop and the step are out of phase.** The move cycle takes 0.25 s, a step 0.15 s.
  Nobody notices with 2 frames, but a walk cycle whose feet should land on each step would look
  wrong. The last column is the fix: a **PlayRate** of 1.67 plays the cycle in 0.15 s
  (exercise 1).
- **One-shots must be short enough.** Eat lasts 0.25 s, under two steps. A longer one would
  still be playing at the next gem; harmless here, since `PlayFromStart` restarts it, but decide
  it on purpose.
- **Game logic never waits for an animation.** The exit is passable before its opening ends, and
  the level restarts on the game mode's timer, not when the Dead flipbook finishes.

Frame rate independence comes for free: a 16 fps flipbook shows 16 frames per second whether
the game runs at 30 or 144 frames per second. On a hitch, frames are skipped, not slowed.

## Building the milestone

The new class, the snake's state machine, the death blink and the exit's handler are above in
full. This section lists every change, then wires up the assets.

**Files changed since chapter 11** (in `Source/GridSnake/`):

| File | Change |
|---|---|
| `Gameplay/GSFlipbookEffect.h/.cpp` | New: plays its flipbook once, then destroys itself |
| `Gameplay/GSGem.h/.cpp` | `PickupEffectClass`; spawn it in `OnSnakeEntered_Implementation` |
| `Gameplay/GSExit.h/.cpp` | `OpeningFlipbook`, `HandleDoorAnimFinished`; animated `SetOpen`; `UpdateVisuals` loops |
| `Snake/GSSnake.h` | `EGSHeadAnim`, `GetHeadAnim`, the **Animation** properties, `PlayHeadAnim`, `HandleHeadAnimFinished`, `GetSegmentColor`, `ToggleBodyVisibility` and their members |
| `Snake/GSSnake.cpp` | Includes `PaperFlipbook.h` and `TimerManager.h`; head states in `BeginPlay`, `StartMoving`, `StopMoving`, `Grow` and `Die`; segment colours in `UpdateVisuals` |

The game mode and game state do not change. No new log lines are printed when everything is
set up: animation is something you watch, not read. The only new message is a warning from
`AGSFlipbookEffect` when a Blueprint child has no flipbook.

### Editor steps

These are `project/ch12/ASSETS.md` in narrative form.

1. **Build.** New `UPROPERTY`s and a new class: close the editor, build `GridSnakeEditor` from your
   IDE and reopen the project.
2. **Create the pop effect.**
   - In the Content Browser, go to **C++ Classes > GridSnake > Gameplay**. Right-click
     `GSFlipbookEffect` and choose **Create Blueprint class based on GSFlipbookEffect**.
   - Name it `BP_GSGemPop` and save it in `/Game/GridSnake/Blueprints`.
   - Open it. In the **Components** panel select **Flipbook**. In the Details panel, under
     **Sprite**, set **Source Flipbook** = `FB_Gem_Pop`. Leave **Looping** as it is: the C++
     switches it off anyway.
   - Compile and save. A data-only Blueprint opens in a reduced editor; click **Open Full
     Blueprint Editor** if you do not see the Components panel.
3. **Give the snake its animations.** Open `BP_GSSnake` and click **Class Defaults**. Under
   **Animation** set:
   - **Idle Flipbook** = `FB_SnakeHead_Idle`
   - **Move Flipbook** = `FB_SnakeHead_Move`
   - **Eat Flipbook** = `FB_SnakeHead_Eat`
   - **Dead Flipbook** = `FB_SnakeHead_Dead`
   - Leave **Body Head Color** (white), **Body Tail Color** (0.35, 0.55, 0.35) and **Death
     Blinks** (4) at their defaults, or tune them. Compile and save.
4. **Let gems pop.** Open `BP_GSGem` > **Class Defaults** > **Gem**, set **Pickup Effect Class** =
   `BP_GSGemPop`. Compile and save.
5. **Animate the door.** Open `BP_GSExit` > **Class Defaults** > **Exit**, set **Opening
   Flipbook** = `FB_Exit_Opening`. Closed and Open Flipbook keep their chapter 5 values. Compile
   and save.
6. **Check the one-shot lengths.** Double-click `FB_SnakeHead_Eat` and `FB_SnakeHead_Dead`. The
   flipbook editor shows the frames along a timeline, and the Details panel shows **Frames Per
   Second**. The code plays them once, so this length is exactly what you see: Eat is 4 frames at
   16 fps, 0.25 s.
7. **Play.** A screenshot cannot show any of this, so watch for:
   - The head idles during the countdown, switches to the faster move loop, and still turns with
     every steer. The body shades from white behind the head to a dull green at the tail tip.
   - A gem pops in a short burst over the body while the head munches, then moves on.
   - After the third gem the door plays its opening once, then loops the open one. At the exit
     the head goes back to idle.
   - On a crash the head shows its dead animation and keeps the last frame, and the body blinks
     four times. `GSGrow 2` in the console (backtick key) tests the eat animation.

Expected Output Log (filter: `LogGridSnake`): the same lines as chapter 11, gem by gem, ending in
`Level complete with 300 points` and `Restarting L_Level01`. If `BP_GSGemPop` has no flipbook,
each eaten gem adds:

```text
LogGridSnake: Warning: <BP_GSGemPop_C_0> has no flipbook
```

While playing, select `BP_GSSnake_C_0` in the Outliner: its **Head** component's **Source
Flipbook** changes with the state.

### When it goes wrong

| Symptom | Cause | Fix |
|---|---|---|
| The head always shows the same animation | The **Animation** flipbooks are empty in `BP_GSSnake`, or the old **Head > Source Flipbook** is all you see | Assign the four flipbooks under **Class Defaults > Animation** |
| The head stays on the eat animation's last frame | `HandleHeadAnimFinished` is not called: not bound, or not a `UFUNCTION` | Check `BeginPlay`'s `AddDynamic` and the `UFUNCTION()` line; look for `Unable to bind delegate` in the log |
| The head's move animation never gets past its first frame | `PlayHeadAnim` is called every step | Call it only from the state-change functions, never from `Step` or `Tick` |
| Gems vanish without a pop | **Pickup Effect Class** empty, or `BP_GSGemPop` has no flipbook (see the warning) | Steps 2 and 4 |
| The pop is hidden behind the body | The effect's depth offset is below the body's (+8) | `AddActorWorldOffset(FVector(0, 9, 0))` in `AGSFlipbookEffect::BeginPlay` |
| The door opens without animating | **Opening Flipbook** empty in `BP_GSExit` | Step 5 |
| The door plays the opening, then freezes | `UpdateVisuals` does not switch looping back on | `Door->SetLooping(true)` before `PlayFromStart` in `UpdateVisuals` |
| The body is white everywhere | **Body Head Color** and **Body Tail Color** are both white, or the sprite uses a material without a Vertex Color node | Check the colours; use a default sprite material |

## Summary

- A flipbook is keyframes plus a frame rate; its length is frames ÷ fps. A flipbook component
  plays one at a time on its own clock (`DeltaTime × PlayRate`), looping and playing by default.
- Paper2D has no animation graph for sprites. A `UENUM`, one flipbook property per state and a
  `switch` make a state machine: `SetFlipbook`, `SetLooping`, `PlayFromStart`.
- Change animations on *events* (start, eat, die), never every step. `SetFlipbook` with the
  current flipbook does nothing; `PlayFromStart` always rewinds.
- A non-looping flipbook stops on its last frame and fires `OnFinishedPlaying`, a dynamic
  delegate (bind a `UFUNCTION`). Use it to leave one-shot states: Eat → Move, Opening → Open.
- An effect actor spawns, plays once and destroys itself. Spawn it from a
  `TSubclassOf<>` property so that a Blueprint child chooses the flipbook.
- The Y axis is depth: give effects, body and head distinct Y offsets so the right one is on top.
- `SetSpriteColor` and the grouped component's instance colour tint through vertex colour. The
  default sprite materials support it; masked materials cut out instead of fading.
- A looping timer that toggles `SetVisibility` is a blink. Game logic never waits for an
  animation.

## Exercises

1. **Move in step.** Make the head's move loop play exactly one cycle per snake step, whatever
   the **Step Interval**.

<details><summary>Solution</summary>

In `AGSSnake::PlayHeadAnim`, set the play rate between `SetLooping` and `PlayFromStart`:

```cpp
	Head->SetFlipbook(Flipbook);
	Head->SetLooping(bLoop);
	// One move cycle per step; every other state at its authored speed.
	const float Rate = (NewAnim == EGSHeadAnim::Move && StepInterval > 0.0f)
		? Flipbook->GetTotalDuration() / StepInterval
		: 1.0f;
	Head->SetPlayRate(Rate);
	Head->PlayFromStart();
```

`UPaperFlipbook::GetTotalDuration()` is the cycle length in seconds at rate 1 (0.25 s). At rate
*R* a cycle takes `0.25 / R` seconds, so `R = 0.25 / 0.15 = 1.67` makes it one step long: the
last column of `flipbook_timing.py`'s table. The component keeps its play rate across
`SetFlipbook`, so the other states must reset it to 1. After an eat, Move restarts mid-step; to
lock the phase too, add `if (HeadAnim == EGSHeadAnim::Move) { Head->SetPlaybackPosition(0.0f,
false); }` at the end of `Step()`.

</details>

2. **Pool the pops.** Reuse finished pop effects instead of spawning a new actor for every gem.

<details><summary>Solution</summary>

`AGSFlipbookEffect` learns to play again and to hide instead of dying. In `GSFlipbookEffect.h`,
public:

```cpp
	/** Plays the effect again at Location. Used by pools instead of spawning. */
	void Replay(const FVector& Location);
	bool IsPlaying() const;

	/** When true, the effect hides itself at the end instead of being destroyed. */
	bool bPooled = false;
```

In `GSFlipbookEffect.cpp`:

```cpp
void AGSFlipbookEffect::Replay(const FVector& Location)
{
	SetActorLocation(Location + FVector(0.0f, 9.0f, 0.0f));
	SetActorHiddenInGame(false);
	Flipbook->PlayFromStart();
}

bool AGSFlipbookEffect::IsPlaying() const
{
	return Flipbook->IsPlaying();
}

void AGSFlipbookEffect::HandleFinishedPlaying()
{
	if (bPooled)
	{
		SetActorHiddenInGame(true);
		return;
	}
	Destroy();
}
```

The pool lives in the game mode, which already outlives every gem. In `GSGameMode.h` add
`class AGSFlipbookEffect;` at the top, a public function and a private array:

```cpp
	/** Plays an effect at Location, reusing a finished one of the same class. */
	void PlayEffect(TSubclassOf<AGSFlipbookEffect> EffectClass, const FVector& Location);

	// private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSFlipbookEffect>> EffectPool;
```

In `GSGameMode.cpp`, which includes `Engine/World.h` and `Gameplay/GSFlipbookEffect.h`:

```cpp
void AGSGameMode::PlayEffect(TSubclassOf<AGSFlipbookEffect> EffectClass, const FVector& Location)
{
	if (!EffectClass)
	{
		return;
	}

	for (AGSFlipbookEffect* Effect : EffectPool)
	{
		if (IsValid(Effect) && Effect->GetClass() == EffectClass.Get() && !Effect->IsPlaying())
		{
			Effect->Replay(Location);
			return;
		}
	}

	AGSFlipbookEffect* Effect = GetWorld()->SpawnActor<AGSFlipbookEffect>(EffectClass, FTransform(Location));
	if (IsValid(Effect))
	{
		Effect->bPooled = true;
		EffectPool.Add(Effect);
	}
}
```

In `AGSGem::OnSnakeEntered_Implementation`, delete the `SpawnActor` block and call
`GameMode->PlayEffect(PickupEffectClass, GetActorLocation());` after `OnGemCollected`, inside the
existing `if`. Play: the Outliner shows a single `BP_GSGemPop_C_0` that stays after the first gem
and reappears for the next. `bPooled` can be set after `SpawnActor` returns, because the effect
reads it only when it finishes. `IsValid` skips an effect that destroyed itself for lack of a
flipbook. A level restart discards the world, pool included.

</details>

3. **A tail tip.** Draw the last body segment with its own sprite, pointing away from the body.

<details><summary>Solution</summary>

Assets: draw a 16 × 16 PNG of a tail tip pointing **right** (+X), the way the head art faces.
Import it into `Art/Textures`, apply the Paper2D texture settings as in chapter 4, right-click >
**Sprite Actions > Create Sprite**, name it `S_SnakeTail` and move it to `Art/Sprites`.

In `GSSnake.h`, a property next to `BodySprite` and a private helper:

```cpp
	/** Sprite of the last segment, drawn facing right; optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Snake")
	TObjectPtr<UPaperSprite> TailSprite;

	// private:
	FTransform GetSegmentTransform(int32 Index) const;
```

In `GSSnake.cpp`:

```cpp
FTransform AGSSnake::GetSegmentTransform(int32 Index) const
{
	const FVector Location = Board->CellToWorld(Segments[Index]) + FVector(0.0f, BodyDepth, 0.0f);
	if (!TailSprite || Index != Segments.Num() - 1)
	{
		return FTransform(Location);
	}

	// Point the tail away from the segment in front of it.
	const FIntPoint Away = Segments[Index] - Segments[Index - 1];
	for (EGSDirection Direction : {EGSDirection::Up, EGSDirection::Down, EGSDirection::Left, EGSDirection::Right})
	{
		if (GSGrid::ToOffset(Direction) == Away)
		{
			return FTransform(GSGrid::ToSpriteRotation(Direction), Location);
		}
	}
	return FTransform(Location);
}
```

In `UpdateVisuals`, replace both `const FTransform Transform(...)` lines with
`const FTransform Transform = GetSegmentTransform(Index);`, and in the rebuild loop choose the
sprite per instance:

```cpp
			UPaperSprite* Sprite = (TailSprite && Index == Segments.Num() - 1) ? TailSprite.Get() : BodySprite.Get();
			Body->AddInstance(Transform, Sprite, /*bWorldSpace=*/true, GetSegmentColor(Index));
```

Set **Tail Sprite** = `S_SnakeTail` in `BP_GSSnake`. The loops start at index 1, so
`Segments[Index - 1]` exists. The tail stays the last instance, turned by
`UpdateInstanceTransform` each step; a rebuild gives the new last segment the tail sprite.
Growth never puts two segments on one cell, so `Away` is always one of the four offsets.

</details>

4. **A flash for every gem.** Without a HUD, flash the screen white for a moment when a gem is
   collected.

<details><summary>Solution</summary>

The player camera manager can fade the whole view to a colour. In `AGSGameMode::OnGemCollected`,
at the end (include `Camera/PlayerCameraManager.h` and `GameFramework/PlayerController.h`):

```cpp
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			// From 60 % white to clear in a fifth of a second.
			PC->PlayerCameraManager->StartCameraFade(0.6f, 0.0f, 0.2f, FLinearColor::White);
		}
	}
```

`StartCameraFade(FromAlpha, ToAlpha, Duration, Color, bShouldFadeAudio, bHoldWhenFinished)`
blends the view towards `Color`, from `FromAlpha` to `ToAlpha`: 0.6 → 0 is a flash that fades
out. The fade belongs to the camera manager, not the view target, so it works with the board's
camera. Chapter 15 could do the same with a UMG animation.

</details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-12--flipbook-animation-from-c)
