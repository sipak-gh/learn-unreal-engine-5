# Chapter 8 — Enhanced Input

Chapter 7 gave GridSnake a pawn, but the snake ignores every key you press. This chapter connects
the player's hands to it with **Enhanced Input**, the input system of Unreal Engine 5. Enhanced
Input separates two questions. An **Input Action** says *what* the player wants, for example
"steer". An **Input Mapping Context** says *which keys* produce it. Between them, **modifiers**
reshape the raw values and **triggers** decide when the action fires. GridSnake gains steering on
WASD, the arrow keys, the gamepad's left stick and its D-pad. All of them feed one 2D action, and
the snake turns that vector into one of four grid directions. In this chapter the snake only turns
its head; it starts to move in chapter 9.

Previous: [Chapter 7](07-the-gameplay-framework.md). Next:
[Chapter 9](09-time-ticks-and-timers.md).

## Why Enhanced Input

Unreal Engine 4 configured input as named **action mappings** and **axis mappings** in Project
Settings > Input: "Jump" is Space Bar, "MoveForward" is W with scale 1 and S with scale −1. Code
bound functions to those names. That system still exists in UE5 for old projects, but it is
deprecated. New projects use Enhanced Input, which ships as a plugin that is enabled by default.

Enhanced Input splits the old mappings into small, reusable assets:

```text
 key or stick                                                        your code
 (W, Gamepad_Left2D)                                               (AGSSnake::OnSteer)
      │                                                                   ▲
      ▼                                                                   │
 Input Mapping Context ──► Modifiers ──► Triggers ──► Input Action ──► ETriggerEvent
 (IMC_Snake: which keys)   (reshape the   (decide      (IA_Steer:       (Started,
                            raw value)     when)        what it means)    Triggered, ...)
```

- An **Input Action** (`UInputAction`) is an asset that names one thing the player can do and the
  *type* of its value: a Boolean button, a 1D axis, a 2D axis or a 3D axis.
- An **Input Mapping Context** (`UInputMappingContext`) is an asset that maps keys to actions.
  The same action can appear in several contexts with different keys.
- **Modifiers** (`UInputModifier` subclasses) change the value a key produces: flip its sign, move
  it to another axis, apply a dead zone.
- **Triggers** (`UInputTrigger` subclasses) decide when the modified value counts: on press, on
  release, after holding for a while, as a chord with another action.
- Your code binds a function to an action and a **trigger event** (`ETriggerEvent`). It never
  names a key.

Contexts are added to and removed from a player at run time. A pause menu can add a context that
maps Escape to "resume", and a vehicle can add one for driving and remove it when the player gets
out. When two contexts map the same key, the one with the higher **priority** wins.

### What the project already has

GridSnake has been ready for Enhanced Input since chapter 1. `Config/DefaultInput.ini` makes the
engine create Enhanced Input objects for every player:

```ini
DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput
DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent
```

These are the **Default Classes** in Project Settings > Engine > Input. Because of the second
line, the input component that a pawn receives is a `UEnhancedInputComponent`, which is what the
snake's code casts to below. `GridSnake.Build.cs` has listed the `InputCore` (keys) and
`EnhancedInput` (actions, contexts, the subsystem) modules since chapter 2, so no build changes are
needed. If your own project lacks them, add `"EnhancedInput"` to `PublicDependencyModuleNames`,
or the linker will not find the classes this chapter uses.

## Input Actions

An Input Action is a small asset. GridSnake needs exactly one: **`IA_Steer`**, "the player
points in a direction".

To create it, right-click in the Content Browser (in `/Game/GridSnake/Input`) and choose
**Input > Input Action**. Double-click the new asset. The settings that matter now:

| Setting | GridSnake value | Meaning |
|---|---|---|
| **Value Type** | **Axis2D (Vector2D)** | The action's value is an `FVector2D`: X is right, Y is up |
| **Triggers** | (empty) | No action-level triggers; see below |
| **Modifiers** | (empty) | Action-level modifiers apply to every key mapped to it |
| **Consume Input** | on (default) | A key that drives this action is not passed to lower-priority contexts |

### Value types

The **Value Type** decides what `FInputActionValue` (header `InputActionValue.h`) carries:

| Value Type | C++ read | Typical use |
|---|---|---|
| Digital (bool) | `Value.Get<bool>()` | Buttons: jump, pause, confirm |
| Axis1D (float) | `Value.Get<float>()` | Triggers, a throttle, a scroll wheel |
| Axis2D (Vector2D) | `Value.Get<FVector2D>()` | Sticks, WASD, the D-pad as one action |
| Axis3D (Vector) | `Value.Get<FVector>()` | 3D motion controllers |

A key always reports its value on X: 1 while held, 0 when released. A 2D stick reports X and Y.
The action converts whatever arrives to its own type. That is why `IA_Steer` must be **Axis2D**.
If it were Digital, the snake would only learn "some key is down", not which way.

A new Input Action is **Digital (bool)** by default. Forgetting to change the Value Type is the
most common Enhanced Input mistake. The symptom is a handler that always reads (1, 0) or (0, 0).

### Trigger events

When the value of an action changes, Enhanced Input runs the action's triggers and reports an
**`ETriggerEvent`**. The Enhanced Input overview describes the events this way:

| Event | When |
|---|---|
| `Started` | Trigger evaluation began: the first frame the input is non-zero |
| `Ongoing` | Triggers are still evaluating, for example a Hold trigger before its time is up |
| `Triggered` | All trigger requirements are met. With no triggers, every frame the input is non-zero |
| `Completed` | Evaluation finished: the input went back to zero after triggering |
| `Canceled` | Evaluation stopped before triggering, for example a Hold released too early |

With an **empty trigger list**, a mapping behaves like a "Down" trigger. In the words of the
Enhanced Input overview, it "always fires, unless the value is 0". For a key, that means `Started` once on
the press, `Triggered` on every frame while it is held, and `Completed` once on release. `Ongoing`
and `Canceled` only appear with triggers that take time, such as **Hold**.

Epic's advice is to bind to `Triggered` in most cases, so that the triggers in the asset decide
what counts. GridSnake follows it. Triggered fires every frame while a key is held, so the
snake's handler must not do anything that should happen once per press. You will see how the code
handles that.

### Watching the events

This example pawn binds one handler to all five events and logs which one fired.

`examples/ch08/ExInputPawn.h`

```cpp
// Learn Unreal Engine 5 - chapter 8 example.
// Build.cs modules: Core, CoreUObject, Engine, InputCore, EnhancedInput.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExInputPawn.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionInstance;

/**
 * Logs every trigger event of one input action, so you can watch the
 * Started / Ongoing / Triggered / Completed / Canceled life cycle.
 * Needs two assets: a Boolean action (e.g. IA_ExJump) and a context mapping Space Bar to it.
 */
UCLASS()
class AExInputPawn : public APawn
{
	GENERATED_BODY()

public:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Runs whenever the controller changes: the right moment to add a mapping context. */
	virtual void NotifyControllerChanged() override;

protected:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> Context;

private:
	void OnJumpEvent(const FInputActionInstance& Instance);
};
```

`examples/ch08/ExInputPawn.cpp`

```cpp
// Learn Unreal Engine 5 - chapter 8 example.

#include "ExInputPawn.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogExInput, Log, All);

void AExInputPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !Context)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(Context, /*Priority=*/0);
		UE_LOG(LogExInput, Log, TEXT("Added %s for %s"), *Context->GetName(), *PC->GetName());
	}
}

void AExInputPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!JumpAction)
	{
		UE_LOG(LogExInput, Warning, TEXT("%s: set Jump Action in the Details panel"), *GetName());
		return;
	}

	// One handler for every event; the FInputActionInstance tells us which one fired.
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	for (const ETriggerEvent Event : { ETriggerEvent::Started, ETriggerEvent::Ongoing, ETriggerEvent::Triggered,
	                                   ETriggerEvent::Completed, ETriggerEvent::Canceled })
	{
		Input->BindAction(JumpAction, Event, this, &AExInputPawn::OnJumpEvent);
	}
}

void AExInputPawn::OnJumpEvent(const FInputActionInstance& Instance)
{
	UE_LOG(LogExInput, Log, TEXT("%s value=%s elapsed=%.2fs"),
		*UEnum::GetValueAsString(Instance.GetTriggerEvent()),
		*Instance.GetValue().ToString(),
		Instance.GetElapsedTime());
}
```

To see it:

1. Copy the pair into the `Source/<YourModule>/` folder of a scratch UE 5.8 C++ project (not
   GridSnake). Make sure the module's `Build.cs` lists `InputCore` and `EnhancedInput`. Build.
2. In the Content Browser, create an Input Action `IA_ExJump` (leave it Digital) and an Input
   Mapping Context `IMC_ExJump`. In the context, add a mapping for `IA_ExJump` and set its key to
   **Space Bar**.
3. Drag **ExInputPawn** into a level. In the Details panel, set **Input > Jump Action** =
   `IA_ExJump`, **Input > Context** = `IMC_ExJump`, and **Pawn > Auto Possess Player** =
   **Player 0** (chapter 7).
4. Press Play, then tap Space once, then hold it for about a second.

Expected Output Log (filter: `LogExInput`):

```text
LogExInput: Added IMC_ExJump for <PlayerController_0>
LogExInput: ETriggerEvent::Started value=true elapsed=<0.00>s
LogExInput: ETriggerEvent::Triggered value=true elapsed=<0.00>s
LogExInput: ETriggerEvent::Completed value=<value> elapsed=<t>s
LogExInput: ETriggerEvent::Started value=true elapsed=<0.00>s
LogExInput: ETriggerEvent::Triggered value=true elapsed=<0.00>s
LogExInput: ETriggerEvent::Triggered value=true elapsed=<0.02>s
... one Triggered line per frame while Space is held ...
LogExInput: ETriggerEvent::Completed value=<value> elapsed=<about 1.00>s
```

- **Counts depend on the frame rate.** A quick tap may produce one `Triggered` line or several;
  holding for a second at 60 frames per second produces about sixty. Only the shape is fixed:
  `Started`, then `Triggered` lines, then one `Completed`.
- **The `Completed` value** is shown as `<value>` because this course did not confirm whether
  it reports the last held value or the released one. Do not write code that depends on it. The
  runtime example below avoids it on purpose.
- **No `Ongoing` or `Canceled`.** They need a trigger that takes time. Open `IMC_ExJump`, click
  the **+** next to **Triggers** under the Space Bar mapping, and choose **Hold** with a hold time
  of 0.5 seconds. Play again: a short tap now logs `Started`, a few `Ongoing` lines and
  `Canceled`. A long press logs `Ongoing` until half a second has passed, then `Triggered`.
- **`FInputActionInstance`** (header `InputAction.h`) carries more than the value: the event, how
  long the action has been evaluated (`GetElapsedTime`) and how long it has been triggered
  (`GetTriggeredTime`). Bind to it when you need those. Most handlers only need the value.
- **`UEnum::GetValueAsString`** works because `ETriggerEvent` is a reflected `UENUM`, like
  `EGSDirection` in chapter 7.

## Mapping Contexts and modifiers

The Input Mapping Context says which keys drive which actions. Create **`IMC_Snake`** with
right-click > **Input > Input Mapping Context**, open it, and add one mapping for `IA_Steer`.
Under it, add one row per key. Each row has its own list of **Modifiers** and **Triggers**, added
with the **+** next to the list.

GridSnake reads the action's value as a direction on screen: +X is right, +Y is up. Every key and
stick must therefore produce the matching unit vector:

| Key | Modifiers (in order) | Raw value | Value after modifiers |
|---|---|---|---|
| D, Right | none | (1, 0) | (1, 0): right |
| A, Left | Negate | (1, 0) | (−1, 0): left |
| W, Up | Swizzle Input Axis Values (Order YXZ) | (1, 0) | (0, 1): up |
| S, Down | Swizzle Input Axis Values (YXZ), then Negate | (1, 0) | (0, 1), then (0, −1): down |
| Gamepad Left Thumbstick 2D-Axis | Dead Zone (Radial, Lower Threshold 0.25) | (x, y) | 0 near the centre, else rescaled |
| Gamepad D-pad Right / Left / Up / Down | same as D / A / W / S | (1, 0) | as for the keys |

Why each one works:

- **A key is one-dimensional.** Pressed, it reports 1 on X and 0 on Y. So D needs nothing: (1, 0)
  already means right.
- **Negate** multiplies the value by −1. By default it flips X, Y and Z, so A's (1, 0) becomes
  (−1, 0). (−0 is still zero.)
- **Swizzle Input Axis Values** reorders the axes. **YXZ** means "the new X is the old Y, the new
  Y is the old X". W's (1, 0) becomes (0, 1). YXZ is the modifier's default order, so adding the
  modifier is usually enough. The Enhanced Input overview recommends exactly this table for WASD
  and the arrow keys.
- **S needs both.** Swizzle moves its 1 to Y, and Negate flips it to −1. Exercise 2 asks whether
  the order of the two matters.
- **The stick is already 2D.** Pushing it up gives positive Y in Unreal, so it needs no swizzle.
  But a real stick never rests at exactly (0, 0). Without a dead zone it would report tiny values,
  and the action would fire all the time. **Dead Zone** maps everything inside the lower threshold
  to zero and rescales the rest to 0–1. **Radial** measures the length of the (x, y) vector
  rather than each axis alone, which suits a round stick.

When two keys of the same action are held, for example W and D, Enhanced Input combines their
values. By default it keeps the value with the larger magnitude per axis, so W + D gives (1, 1).
(The action's **Accumulation Behavior** setting can switch this to adding the values instead.)
The snake has to decide what a diagonal means; that comes later in this chapter.

### Contexts are UObjects too

Every asset in this section is an ordinary `UObject`: an action, a context, a modifier. The editor
is a convenient way to create them, but code can do the same. The next example builds `IA_Steer`
and the WASD half of `IMC_Snake` without a single asset. You would rarely do this in a real game,
but seeing it in code makes the asset settings concrete.

`examples/ch08/ExRuntimeInput.h`

```cpp
// Learn Unreal Engine 5 - chapter 8 example.
// Build.cs modules: Core, CoreUObject, Engine, InputCore, EnhancedInput.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExRuntimeInput.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * WASD steering without a single asset: the input action, the mapping context
 * and the modifiers are created with NewObject. Shows that input assets are
 * ordinary UObjects. Set it as the pawn to possess (Auto Possess Player = Player 0).
 */
UCLASS()
class AExRuntimeInput : public APawn
{
	GENERATED_BODY()

public:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

private:
	/** Creates the action and context the first time it is called. */
	void CreateInputObjects();

	void OnMove(const FInputActionValue& Value);
	void OnMoveReleased();

	// UPROPERTY keeps the garbage collector from deleting objects only C++ points to.
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Context;

	FVector2D LastValue = FVector2D::ZeroVector;
};
```

`examples/ch08/ExRuntimeInput.cpp`

```cpp
// Learn Unreal Engine 5 - chapter 8 example.

#include "ExRuntimeInput.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

DEFINE_LOG_CATEGORY_STATIC(LogExRuntime, Log, All);

void AExRuntimeInput::CreateInputObjects()
{
	if (MoveAction)
	{
		return;
	}

	// The same settings you would make in the Input Action editor.
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeMove"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Runtime"));

	// Each helper adds one modifier to a mapping, like the + in the context editor.
	auto AddSwizzle = [this](FEnhancedActionKeyMapping& Mapping)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
		Swizzle->Order = EInputAxisSwizzle::YXZ; // X goes to Y: a key now pushes "up"
		Mapping.Modifiers.Add(Swizzle);
	};
	auto AddNegate = [this](FEnhancedActionKeyMapping& Mapping)
	{
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	};

	// MapKey returns a reference into the context's array. Use it before the
	// next MapKey call, which may reallocate the array.
	Context->MapKey(MoveAction, EKeys::D);                // (1, 0)
	AddNegate(Context->MapKey(MoveAction, EKeys::A));     // (-1, 0)
	AddSwizzle(Context->MapKey(MoveAction, EKeys::W));    // (0, 1)
	FEnhancedActionKeyMapping& Down = Context->MapKey(MoveAction, EKeys::S);
	AddSwizzle(Down);                                     // (0, 1)
	AddNegate(Down);                                      // (0, -1)
}

void AExRuntimeInput::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	CreateInputObjects();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(Context, /*Priority=*/0);
	}
}

void AExRuntimeInput::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInputObjects();

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AExRuntimeInput::OnMove);
	// Completed fires once when the last key is released.
	Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AExRuntimeInput::OnMoveReleased);
}

void AExRuntimeInput::OnMove(const FInputActionValue& Value)
{
	// Triggered fires every frame while a key is held; log only changes.
	const FVector2D Move = Value.Get<FVector2D>();
	if (!Move.Equals(LastValue))
	{
		LastValue = Move;
		UE_LOG(LogExRuntime, Log, TEXT("Move %s"), *Move.ToString());
	}
}

void AExRuntimeInput::OnMoveReleased()
{
	LastValue = FVector2D::ZeroVector;
	UE_LOG(LogExRuntime, Log, TEXT("Move released"));
}
```

- **`NewObject<UInputAction>(this, Name)`** creates the action with the pawn as its **outer**,
  the object that owns it (chapter 3). `ValueType` is the same property the asset editor shows as
  **Value Type**; it is public, so code can set it before the action is used.
- **Modifiers are instanced objects.** `FEnhancedActionKeyMapping::Modifiers` is an array of
  `TObjectPtr<UInputModifier>`, one object per modifier. In an asset, each modifier is a
  sub-object of the context, so the example creates them with the context as their outer, too.
- **`MapKey` returns a reference.** `UInputMappingContext::MapKey(Action, Key)` adds a row and
  returns `FEnhancedActionKeyMapping&`, the row itself. The reference points into the context's
  array of mappings, and the next `MapKey` may grow that array and move it. So the code uses each
  reference immediately, and keeps `Down` only while no other mapping is added. A reference into
  a `TArray` is valid only until the array changes size; keeping one longer is a classic C++ bug.
- **`UPROPERTY(Transient)`.** The objects are created at run time, so only C++ pointers refer to
  them. Without `UPROPERTY`, the garbage collector would not know about those pointers and could
  delete the objects (chapter 3). `Transient` says "never save this", which is right for objects
  that are rebuilt every time.
- **Lazy creation.** `CreateInputObjects` runs from whichever of `NotifyControllerChanged` and
  `SetupPlayerInputComponent` comes first. Both need the objects, and the example does not have to
  know which one the engine calls first.
- **Two handler signatures.** `OnMove` takes the value; `OnMoveReleased` takes nothing.
  `BindAction` accepts a member function with no parameters, with `const FInputActionValue&`, or
  with `const FInputActionInstance&`. Choose the smallest one that gives you what you need.
- **`Completed` ends a press.** The release handler resets `LastValue` instead of reading the
  value, so it does not depend on what `Completed` reports.
- In real projects, input objects belong in assets: designers can change them without a build,
  and players can rebind keys that live in assets.

To see it, copy the pair into your scratch project and build. Drag **ExRuntimeInput** into a level,
set **Pawn > Auto Possess Player** = **Player 0**, press Play, and press W, then W and D together,
then release both.

Expected Output Log (filter: `LogExRuntime`):

```text
LogExRuntime: Move X=0.000 Y=1.000
LogExRuntime: Move X=1.000 Y=1.000
LogExRuntime: Move released
```

The middle line shows the default accumulation: W gives (0, 1), D gives (1, 0), and the
combination keeps the largest value on each axis. If you release W first, a fourth line,
`Move X=1.000 Y=0.000`, appears before `Move released`.

## Adding the context: the local player subsystem

A mapping context does nothing until it is added to a player. Contexts are managed by the
**Enhanced Input Local Player Subsystem**, `UEnhancedInputLocalPlayerSubsystem`. A **local
player** (`ULocalPlayer`) is a player at this machine: a screen, an input device, a viewport. There
is one per split-screen player, and it outlives levels, pawns and even controllers. A **local player
subsystem** is an object the engine creates automatically for every local player. You never spawn
it; you ask for it.

GridSnake adds its context in the player controller. The controller is the player, while the pawn
is only a body (chapter 7), and the steering keys belong to the player.

`Source/GridSnake/Core/GSPlayerController.h` (changes from chapter 7)

```cpp
class UInputMappingContext;

/**
 * The player's will. Owns the view (the board's fixed camera, not the pawn)
 * and the input mapping context; later also the screens.
 */
UCLASS()
class AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

protected:
	virtual void BeginPlay() override;

	/** Keyboard and gamepad bindings for playing a level (IMC_Snake). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
};
```

`Source/GridSnake/Core/GSPlayerController.cpp` (changes from chapter 7)

```cpp
#include "Core/GSPlayerController.h"
#include "GridSnake.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"

// ... the constructor is unchanged ...

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// Mapping contexts belong to the local player, so they survive pawn changes.
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			InputSubsystem->AddMappingContext(DefaultMappingContext, /*Priority=*/0);
		}
		else
		{
			UE_LOG(LogGridSnake, Warning, TEXT("%s has no DefaultMappingContext - set it in BP_GSPlayerController"), *GetName());
		}
	}

	if (AActor* Board = UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass()))
	{
		SetViewTarget(Board);
		UE_LOG(LogGridSnake, Log, TEXT("View target: %s"), *Board->GetName());
	}
}
```

- **The property.** `DefaultMappingContext` is a pointer to an asset, so it is set in a Blueprint
  subclass, `BP_GSPlayerController`, like every asset reference in this course. `EditDefaultsOnly`
  shows it in the Blueprint's Class Defaults but not on placed instances; controllers are spawned,
  never placed, so there are no instances to edit. The header only forward-declares
  `UInputMappingContext`, because a `TObjectPtr` member does not need the full class.
- **`ULocalPlayer::GetSubsystem<T>(LocalPlayer)`** is a static template that returns the
  subsystem of type `T` for that local player, or null if the local player is null.
  `GetLocalPlayer()` is null for a controller that does not belong to a player at this machine.
  The `IsLocalController()` check above already rules that out, and the `if` handles it anyway.
- **The includes.** `Engine/LocalPlayer.h` declares `ULocalPlayer`, and `EnhancedInputSubsystems.h`
  declares the subsystem.
- **`AddMappingContext(Context, Priority)`** comes from `IEnhancedInputSubsystemInterface`, an
  interface the local player subsystem implements. It also takes an optional
  `FModifyContextOptions`, which the code leaves at its defaults. Priority 0 is the base layer;
  contexts with higher numbers are checked first. Its partners are `RemoveMappingContext(Context)`
  and `ClearAllMappings()`, and `HasMappingContext(Context)` checks whether a context is applied.
- **Warn, do not crash.** A missing context is a set-up mistake in the Blueprint, so the warning
  names the property and the Blueprint to fix.
- **Order.** The context is added before the view target is set. The two are independent, but
  input set-up first, then the view, reads naturally.

### Where else could the context go?

There are three common places to add a mapping context. Each has a reason:

| Place | Used by | Good for | Watch out for |
|---|---|---|---|
| Player controller `BeginPlay` | GridSnake | Input that belongs to the player, whatever the pawn | Runs once; to change contexts later, add and remove them explicitly |
| Pawn `NotifyControllerChanged` | `ExInputPawn` above, recent Epic templates | Input that belongs to one kind of pawn (a vehicle, a turret) | Remove the context again when the pawn is released, or it stays active |
| Pawn `BeginPlay` | Older tutorials | Nothing it does better | A pawn possessed later, or re-possessed, never adds its context |

`NotifyControllerChanged` is a virtual function on `APawn`. According to its API page, it runs on
the server and on the owning client whenever the pawn's controller changes, which makes it the
right hook for pawn-specific contexts. GridSnake has one pawn type and one set of keys, so the
controller is the simplest home. Chapter 16 adds a pause action to the same context and binds it
in the controller; the action's **Trigger when Paused** option lets it fire during a pause.

One subtlety: by default, when a context is added, keys that are already held down are ignored
until they are released. This stops a key that opened a menu from also triggering an action in
the menu's context. `FModifyContextOptions` controls it.

## Binding actions in C++

The context turns keys into `IA_Steer` values. Now the snake needs a function that runs when
`IA_Steer` fires. Pawns bind input in `SetupPlayerInputComponent`.

`Source/GridSnake/Snake/GSSnake.h` (changes from chapter 7)

```cpp
class AGSBoard;
class UBoxComponent;
class UInputAction;
class UPaperFlipbookComponent;
struct FInputActionValue;

// ...

public:
	// ... GetDirection, GetHeadCell ...

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	// ... BeginPlay, Collision, Head ...

	/** IA_Steer: an Axis2D action. Keys and sticks are mapped to it in IMC_Snake. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SteerAction;

	// ... Board, HeadCell, CurrentDirection ...

private:
	void OnSteer(const FInputActionValue& Value);

	// ... SnapToBoard, UpdateHeadRotation ...
```

- **`struct FInputActionValue;`** is a forward declaration of a struct. The header only mentions
  the type in a function parameter passed by reference, which does not need the full definition.
  Forward-declare it as `struct`, matching its definition; MSVC warns when the keyword differs.
- **`SetupPlayerInputComponent` is public** in `APawn`, so the override is public too.
- **`OnSteer` is not a `UFUNCTION`.** `BindAction` with a member function pointer works with plain
  C++ functions. Only the overload that takes a function *name* needs a `UFUNCTION`.
- **`SteerAction` is set in `BP_GSSnake`**, like the head's flipbook, because it refers to an
  asset.

`Source/GridSnake/Snake/GSSnake.cpp` (changes from chapter 7)

```cpp
#include "Snake/GSSnake.h"
#include "GridSnake.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "EnhancedInputComponent.h"
#include "Gameplay/GSBoard.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbookComponent.h"

// ...

void AGSSnake::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// DefaultInput.ini makes every input component an Enhanced Input component.
	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (SteerAction)
	{
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, this, &AGSSnake::OnSteer);
	}
	else
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s has no SteerAction - set it in BP_GSSnake"), *GetName());
	}
}
```

- **When it runs.** The engine creates an input component for a pawn when a local player
  controller possesses it, then calls `SetupPlayerInputComponent` with it. When the pawn is
  released, the component goes away with its bindings. Binding here means a pawn has bindings
  exactly while a player controls it. For the snake, that happens during `RestartPlayer` (step 6
  of the start-up sequence in chapter 7), before any `BeginPlay`.
- **`CastChecked<T>`** is a cast that must succeed. If it fails, a development build stops with
  an assertion that names both classes. Here a failure means `DefaultInput.ini` no longer selects
  Enhanced Input, a configuration error worth stopping for. A plain `Cast` would return null and
  move the crash to the next line, with a less helpful message.
- **`BindAction(Action, Event, Object, Function)`** connects one action and one trigger event to
  one member function. The function's signature picks the overload. `OnSteer` takes
  `const FInputActionValue&`, so it receives the action's current value.
- **`ETriggerEvent::Triggered`**: with no triggers in `IMC_Snake`, `OnSteer` runs on every frame
  while any steering input is non-zero. The next section makes that harmless.
- **The `SteerAction` check.** `BindAction` with a null action would bind nothing useful. The
  warning tells you which Blueprint property to set.

## From a vector to a grid direction

`IA_Steer` delivers an `FVector2D`. The snake needs one of four `EGSDirection` values. The
conversion is a small, stateless function, so it lives in an anonymous namespace in
`GSSnake.cpp`, visible only in that file (chapter 3).

```cpp
namespace
{
	/**
	 * Turns a 2D input (keys, D-pad or stick) into one of four directions.
	 * Returns false for input too small to mean anything. On a perfect diagonal
	 * (two keys held) it prefers turning over going straight on.
	 */
	bool DirectionFromInput(const FVector2D& Input, EGSDirection Current, EGSDirection& OutDirection)
	{
		const double AbsX = FMath::Abs(Input.X);
		const double AbsY = FMath::Abs(Input.Y);
		if (FMath::Max(AbsX, AbsY) < 0.5)
		{
			return false;
		}

		bool bHorizontal = AbsX > AbsY;
		if (FMath::IsNearlyEqual(AbsX, AbsY))
		{
			const bool bMovingHorizontally = Current == EGSDirection::Left || Current == EGSDirection::Right;
			bHorizontal = !bMovingHorizontally;
		}

		if (bHorizontal)
		{
			OutDirection = Input.X > 0.0 ? EGSDirection::Right : EGSDirection::Left;
		}
		else
		{
			OutDirection = Input.Y > 0.0 ? EGSDirection::Up : EGSDirection::Down;
		}
		return true;
	}
}
```

Step by step:

1. **Too small? Ignore it.** If neither axis reaches 0.5, the function returns `false`. Keys and
   the D-pad always give 1, so this only matters for the stick. After the dead zone, a value of
   0.5 means the stick is pushed a good part of the way, so a resting thumb does not steer.
2. **The dominant axis wins.** `(0.9, 0.3)` is clearly "right"; `(−0.2, −0.8)` is clearly "down".
   Comparing the absolute values picks the axis the player meant.
3. **A tie is a turn.** Holding W and D gives (1, 1), exactly diagonal. A snake moving right
   already goes right, so the only *new* information is "up". The tie-break picks the axis
   perpendicular to the current direction. On a stick, ties are rare; on a keyboard they happen
   whenever two keys overlap during a quick turn. `IsNearlyEqual` compares with a small tolerance,
   because floating-point values that should be equal often differ in the last bits.
4. **The sign gives the direction.** +X is right and +Y is up, as the mapping context promised.
   Note how the input's +Y (up) becomes `EGSDirection::Up`, whose grid offset is (0, −1) because
   tile rows grow downwards (chapter 7). The input's axes and the grid's axes are different
   conventions, and `EGSDirection` sits between them.

The output parameter `OutDirection` is only written when the function returns `true`. The caller
must not read it otherwise. That is the usual Unreal pattern for "maybe a value": a `bool` return
and an `Out` reference.

```cpp
void AGSSnake::OnSteer(const FInputActionValue& Value)
{
	EGSDirection NewDirection;
	if (!DirectionFromInput(Value.Get<FVector2D>(), CurrentDirection, NewDirection))
	{
		return;
	}

	// A snake cannot reverse into its own neck.
	if (NewDirection == CurrentDirection || GSGrid::AreOpposite(NewDirection, CurrentDirection))
	{
		return;
	}

	CurrentDirection = NewDirection;
	UpdateHeadRotation();
	UE_LOG(LogGridSnake, Log, TEXT("Steer: %s"), *UEnum::GetValueAsString(CurrentDirection));
}
```

- **`Value.Get<FVector2D>()`** reads the value as the type the action declares. If `IA_Steer` were
  still Digital, it would return (1, 0) for any key, and the snake could only ever turn right.
- **The same direction is ignored.** `Triggered` fires every frame while a key is held. The first
  frame turns the snake; every later frame asks for the direction it already has and returns
  early. That is why one key press logs one line, not sixty.
- **The opposite direction is ignored.** A snake moving right that turns left would run into its
  own neck. `GSGrid::AreOpposite` from chapter 7 catches it. In classic Snake a reversal is simply
  not allowed, so the input is dropped silently.
- **Only the head turns.** `UpdateHeadRotation` sets the `Head` component's pitch (chapter 7). The
  snake does not move yet; chapter 9 adds a timer that steps it one cell in `CurrentDirection`, and
  later a queue of turns, so two quick key presses within one step both count.

## Debugging input

When a key does nothing, the problem is usually one of five things: the context is not added, the
action is not bound, the action has the wrong value type, a modifier is missing, or a higher
priority context consumes the key. Enhanced Input has tools for each.

### showdebug enhancedinput

Press the console key (`` ` `` by default, chapter 1) during Play and enter:

```text
showdebug enhancedinput
```

The viewport shows an overlay of the Enhanced Input state of the local player: the applied
mapping contexts with their priorities, the actions they map and their current values. Press W
and watch `IA_Steer`'s value change to (0, 1). If `IMC_Snake` is not in the list, the controller
did not add it. If it is listed but `IA_Steer` never changes, look at the mapping's keys and
modifiers. Enter the same command again to toggle the overlay off.

`showdebug devices` lists the input devices the engine sees, which helps when a gamepad seems
dead. The overlays are drawn by the player's HUD (chapter 7); the default `AHUD` is enough.

### Injecting input

Enhanced Input can also *simulate* input. The `Input.+key` console command presses a key until
`Input.-key` releases it. The documented example uses the gamepad stick:

```text
Input.+key Gamepad_Left2D X=0 Y=1
Input.-key Gamepad_Left2D
```

The first line holds the left stick straight up. A snake facing right turns up and logs
`Steer: EGSDirection::Up`. The key name is the `FKey` name, which you can see in the key picker
without spaces. This is useful to test gamepad mappings without a gamepad, and to reproduce a bug
exactly. In C++, `UEnhancedPlayerInput::InjectInputForAction` does the same for an action instead
of a key.

### Chords

Triggers are not only about timing. A **Chorded Action** trigger makes a mapping fire only while
*another* action is triggered: Shift + Space, or a shoulder button plus a face button. GridSnake
does not need chords, but they show how actions can depend on each other. This example creates
everything in code again, so you can read the setup in one place.

`examples/ch08/ExChordAction.h`

```cpp
// Learn Unreal Engine 5 - chapter 8 example.
// Build.cs modules: Core, CoreUObject, Engine, InputCore, EnhancedInput.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExChordAction.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Space alone logs "jump"; Shift + Space logs "dash". The dash mapping has a
 * Chorded Action trigger that requires the Shift action to be active.
 * Everything is created in code, like ExRuntimeInput.
 */
UCLASS()
class AExChordAction : public APawn
{
	GENERATED_BODY()

public:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

private:
	void CreateInputObjects();

	void OnJump();
	void OnDash();

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ShiftAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Context;
};
```

`examples/ch08/ExChordAction.cpp`

```cpp
// Learn Unreal Engine 5 - chapter 8 example.

#include "ExChordAction.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"

DEFINE_LOG_CATEGORY_STATIC(LogExChord, Log, All);

void AExChordAction::CreateInputObjects()
{
	if (Context)
	{
		return;
	}

	// New input actions are Boolean (Digital) by default: right for buttons.
	ShiftAction = NewObject<UInputAction>(this, TEXT("IA_Shift"));
	JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
	DashAction = NewObject<UInputAction>(this, TEXT("IA_Dash"));

	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Chord"));

	// The chord action first, so it is evaluated before the action that depends on it.
	Context->MapKey(ShiftAction, EKeys::LeftShift);
	Context->MapKey(JumpAction, EKeys::SpaceBar);

	// Space -> Dash, but only while ShiftAction is triggered.
	UInputTriggerChordAction* Chord = NewObject<UInputTriggerChordAction>(Context);
	Chord->ChordAction = ShiftAction;
	Context->MapKey(DashAction, EKeys::SpaceBar).Triggers.Add(Chord);
}

void AExChordAction::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	CreateInputObjects();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(Context, /*Priority=*/0);
	}
}

void AExChordAction::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInputObjects();

	// Started fires once per press, unlike Triggered, which repeats while held.
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AExChordAction::OnJump);
	Input->BindAction(DashAction, ETriggerEvent::Started, this, &AExChordAction::OnDash);
}

void AExChordAction::OnJump()
{
	UE_LOG(LogExChord, Log, TEXT("jump"));
}

void AExChordAction::OnDash()
{
	UE_LOG(LogExChord, Log, TEXT("dash"));
}
```

- **`UInputTriggerChordAction`** (header `InputTriggers.h`, shown as **Chorded Action** in the
  editor) has one setting, `ChordAction`: the action that must be triggered at the same time.
  The trigger goes into the mapping's `Triggers` array, just as modifiers go into `Modifiers`.
- **Mapping order.** The Shift mapping is added first. The chord can only be judged once the
  Shift action has been evaluated in the same frame; listing it first makes that order explicit.
- **`Started` for one-shot actions.** Jump and dash should happen once per press, so the example
  binds `Started`, which fires on the first frame only.
- **Two mappings on Space.** Space appears twice: plain for jump, chorded for dash. Enhanced Input
  is designed to block the plain mapping while the chord is active. It adds a
  **Chord Blocker** trigger (`UInputTriggerChordBlocker`) to other mappings of the same key. This
  course did not verify that blocking in 5.8, so check the log.

Drag **ExChordAction** into a level, set **Auto Possess Player** = **Player 0**, press Play, tap
Space, then hold Left Shift and tap Space.

Expected Output Log (filter: `LogExChord`):

```text
LogExChord: jump
LogExChord: dash
```

If Shift + Space prints both `jump` and `dash` in your version, the plain mapping was not blocked.
The fix in that case is to add a **Chord Blocker** trigger with `ChordAction` = the Shift action
to the jump mapping yourself.

### Letting players rebind keys

Keys in a mapping context can be marked **player mappable**, and the **Enhanced Input User
Settings** system can then save each player's own bindings. A key-rebinding screen is built on
top of that. GridSnake's options screen (chapters 16 and 17) has volume sliders only. If you add
rebinding later, start from those two features in Epic's Enhanced Input documentation rather
than editing contexts by hand.

## Building the milestone

The C++ changes are done. These steps are `project/ch08/ASSETS.md` in narrative form.

1. **Build.** No new classes were added, only members, so Live Coding (**Ctrl+Alt+F11**, chapter
   2) is usually enough. Because both headers changed, closing the editor and building from the
   IDE is the safe choice.
2. **Create the folder.** In the Content Browser, create `/Game/GridSnake/Input`.
3. **Create `IA_Steer`.**
   - Right-click in the folder > **Input > Input Action**, name it `IA_Steer`, and open it.
   - Set **Value Type** = **Axis2D (Vector2D)**. Save.
4. **Create `IMC_Snake`.**
   - Right-click > **Input > Input Mapping Context**, name it `IMC_Snake`, and open it.
   - Click **+** next to **Mappings**, and choose `IA_Steer` in the new row.
   - Add one key per row under it with the **+** next to the action. To set a key, either pick it
     from the drop-down or click the keyboard button next to it and press the key.
   - Add the keys and modifiers from the table in "Mapping Contexts and modifiers": D, A, W, S;
     Right, Left, Up, Down; **Gamepad Left Thumbstick 2D-Axis**; and the four **Gamepad D-pad**
     directions. Use the **+** next to **Modifiers** under each key to add them, in the order
     given. For the stick's Dead Zone, expand the modifier and set **Lower Threshold** = 0.25
     and **Type** = **Radial**.
   - Save.
5. **Create `BP_GSPlayerController`.**
   - In **C++ Classes > GridSnake > Core**, right-click `GSPlayerController` > **Create Blueprint
     class based on GSPlayerController**. Save it as
     `/Game/GridSnake/Blueprints/BP_GSPlayerController`.
   - In **Class Defaults**, under **Input**, set **Default Mapping Context** = `IMC_Snake`.
     Compile and save.
6. **Set the snake's action.** Open `BP_GSSnake`. In **Class Defaults**, under **Input**, set
   **Steer Action** = `IA_Steer`. Compile and save.
7. **Point the game mode at the controller.** Open `BP_GSGameMode`. In **Class Defaults**, under
   **Classes**, set **Player Controller Class** = `BP_GSPlayerController`. Compile and save.
   Without this step the game still spawns the C++ `GSPlayerController`, which has no context.
8. **Play.** Press W, A, S and D one after another.

Expected Output Log (filter: `LogGridSnake`), after the start-up lines from chapter 7:

```text
LogGridSnake: Steer: EGSDirection::Up
LogGridSnake: Steer: EGSDirection::Left
LogGridSnake: Steer: EGSDirection::Down
LogGridSnake: Steer: EGSDirection::Right
```

- The snake starts facing right. W turns it up, A turns it left, S down, D right; each press
  logs one line, and the head sprite turns with it.
- **Pressing the current or opposite direction logs nothing.** Press D again, or A while facing
  right: no line. That is the reversal rule in `OnSteer`.
- **Arrow keys, D-pad and stick** log the same lines. Push the stick only slightly and nothing
  happens; that is the dead zone plus the 0.5 threshold.
- The start-up lines are the same as in chapter 7. No `has no DefaultMappingContext` or
  `has no SteerAction` warning may appear.

### When it goes wrong

| Symptom | Cause | Fix |
|---|---|---|
| `has no DefaultMappingContext - set it in BP_GSPlayerController` | The context is not set, or the game mode still spawns the C++ controller | Step 5, and step 7 |
| `has no SteerAction - set it in BP_GSSnake` | The action is not set on the snake | Step 6 |
| No warning, no `Steer` lines | The viewport has no keyboard focus, or `IMC_Snake` has no keys | Click into the viewport; use `showdebug enhancedinput` to see whether the context is applied |
| Every key turns the snake right, or only D and A work | `IA_Steer` is still Digital or Axis1D | Step 3: Value Type = Axis2D |
| W turns down and S turns up | Negate is on W instead of S | Compare with the modifier table |
| W turns right | W's Swizzle is missing | Add Swizzle Input Axis Values to W and Up |
| The stick steers on its own | No dead zone on the stick mapping | Add the Dead Zone modifier |
| Crash in `CastChecked` on Play | `DefaultInput.ini` no longer selects the Enhanced Input classes | Restore the two lines from chapter 1 |

## Summary

- Enhanced Input separates **what** (Input Actions, with a value type) from **which keys**
  (Input Mapping Contexts). **Modifiers** reshape values; **triggers** decide when an action
  fires; code binds to an action and an **`ETriggerEvent`**.
- A key reports 1 on X. For WASD as one 2D action: D needs nothing, A needs Negate, W needs
  Swizzle (YXZ), and S needs both. A stick needs a Dead Zone.
- Without triggers, an action fires `Started` once, `Triggered` every frame while held and
  `Completed` on release. Handlers bound to `Triggered` must be safe to call repeatedly.
- Contexts are added through the **Enhanced Input Local Player Subsystem**:
  `ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())->AddMappingContext(Context, Priority)`.
  GridSnake does it in the player controller's `BeginPlay`; pawn-specific contexts belong in
  `NotifyControllerChanged`.
- Pawns bind actions in `SetupPlayerInputComponent`, after
  `CastChecked<UEnhancedInputComponent>`. The handler's signature chooses what it receives:
  nothing, `FInputActionValue` or `FInputActionInstance`.
- `AGSSnake::OnSteer` turns the 2D value into a grid direction with a dominant axis, breaks ties
  in favour of turning, and ignores the same and the opposite direction.
- `showdebug enhancedinput`, `showdebug devices` and `Input.+key` let you see and simulate input.

## Exercises

1. **Boost.** Add a Boolean action `IA_Boost`, mapped to Space Bar and to **Gamepad Face Button
   Bottom**, and log `Boost on` when it is pressed and `Boost off` when it is released. The snake
   does nothing else with it yet.

<details><summary>Solution</summary>

Create `IA_Boost` (leave **Value Type** = Digital) and add it to `IMC_Snake` with the two keys
and no modifiers. In `GSSnake.h`, next to `SteerAction`:

```cpp
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> BoostAction;
```

and in the `private` section:

```cpp
	void OnBoostStarted();
	void OnBoostCompleted();
```

In `AGSSnake::SetupPlayerInputComponent`, after the steer binding:

```cpp
	if (BoostAction)
	{
		EnhancedInput->BindAction(BoostAction, ETriggerEvent::Started, this, &AGSSnake::OnBoostStarted);
		EnhancedInput->BindAction(BoostAction, ETriggerEvent::Completed, this, &AGSSnake::OnBoostCompleted);
	}
```

And the handlers:

```cpp
void AGSSnake::OnBoostStarted()
{
	UE_LOG(LogGridSnake, Log, TEXT("Boost on"));
}

void AGSSnake::OnBoostCompleted()
{
	UE_LOG(LogGridSnake, Log, TEXT("Boost off"));
}
```

Set **Input > Boost Action** = `IA_Boost` in `BP_GSSnake`. Expected, for one press and release:

```text
LogGridSnake: Boost on
LogGridSnake: Boost off
```

`Started` and `Completed` each fire once per press, so they suit a state that switches on and off.
Binding `Triggered` would log `Boost on` on every frame. The handlers take no parameters,
because the event alone carries all the information they need.

</details>

2. **Why the Swizzle?** Explain why W needs Swizzle Input Axis Values while the stick does not.
   Then answer: does the order of S's two modifiers matter? Would it matter if Negate were set to
   flip only X?

<details><summary>Solution</summary>

A key is a one-dimensional input: pressed, it reports 1 on **X**. Without a modifier, W would give
(1, 0), the same as D, so the snake would turn right. Swizzle YXZ swaps X and Y, so W gives
(0, 1). A stick is already two-dimensional and reports its vertical deflection on Y, so it needs
no swizzle.

For S, the order does not matter with default settings. Negate flips X, Y *and* Z, so:

- Swizzle, then Negate: (1, 0) → (0, 1) → (0, −1).
- Negate, then Swizzle: (1, 0) → (−1, 0) → (0, −1).

If Negate were set to flip only X (untick **Y** and **Z** in the modifier), the order would matter:

- Swizzle, then Negate X: (1, 0) → (0, 1) → (0, 1), which is *up*, a bug.
- Negate X, then Swizzle: (1, 0) → (−1, 0) → (0, −1), which is down, correct.

Modifiers run top to bottom, so the table in `ASSETS.md` gives an order and relies on Negate's
default of flipping all axes. Mapping modifiers also run before any modifiers on the action
itself.

</details>

3. **A debug context.** Create `IMC_Debug`, which maps the arrow keys to a Boolean action
   `IA_DebugPing`. Add it with a higher priority than `IMC_Snake`, and log `Ping` from the player
   controller whenever an arrow key is pressed. What happens to steering with the arrow keys, and
   with WASD?

<details><summary>Solution</summary>

Create `IA_DebugPing` (Digital) and `IMC_Debug` with the four arrow keys mapped to it. In
`GSPlayerController.h`, add:

```cpp
protected:
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DebugMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DebugPingAction;

private:
	void OnDebugPing();
```

with a forward declaration `class UInputAction;`. In `GSPlayerController.cpp`, include
`EnhancedInputComponent.h`. In `BeginPlay`, after adding `DefaultMappingContext`:

```cpp
		if (DebugMappingContext)
		{
			InputSubsystem->AddMappingContext(DebugMappingContext, /*Priority=*/1);
		}
```

A player controller has its own input component, and binds in `SetupInputComponent`:

```cpp
void AGSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (DebugPingAction)
	{
		UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(InputComponent);
		EnhancedInput->BindAction(DebugPingAction, ETriggerEvent::Started, this, &AGSPlayerController::OnDebugPing);
	}
}

void AGSPlayerController::OnDebugPing()
{
	UE_LOG(LogGridSnake, Log, TEXT("Ping"));
}
```

Set both properties in `BP_GSPlayerController`. Pressing an arrow key now logs `Ping` and no
longer steers. `IMC_Debug` has the higher priority, so its mappings are evaluated first, and
`IA_DebugPing` has **Consume Input** on by default. The arrow keys are consumed, and
`IMC_Snake`'s arrow mappings never see them. WASD, the stick and the D-pad still steer, because
`IMC_Debug` does not map them. Remove the context with `RemoveMappingContext(DebugMappingContext)`
and the arrows steer again. This is how a menu context can take over some keys and leave the rest
alone.

</details>

4. **Started or Triggered?** Rebind `IA_Steer` to `ETriggerEvent::Started` instead of
   `Triggered`. Test with the keys and with the stick. What breaks, and why does GridSnake keep
   `Triggered`? When would a **Pulse** trigger be the right tool?

<details><summary>Solution</summary>

With `Started`, `OnSteer` runs only on the frame the action goes from zero to non-zero. Single
key presses still work. Two cases break:

- **Overlapping keys.** Hold W, then press D before releasing W. The action's value changes from
  (0, 1) to (1, 1), but it never went back to zero, so there is no new `Started` and the turn is
  lost.
- **The stick.** Roll the stick from right to up without passing through the centre. The value
  never drops below the dead zone, so again there is no new `Started`, and the snake keeps its old
  direction.

`Triggered` fires on every frame with the current value, so the snake always sees the latest
direction. The "ignore the same direction" rule in `OnSteer` turns that stream of values back
into one turn per change. That combination, a continuous event plus a state check, is the usual
way to read directions.

A **Pulse** trigger fires `Triggered` once on the press and then repeatedly at a fixed interval
while the input is held. It suits *repeat* actions, such as moving a menu selection or a cursor
one cell at a time while a direction is held. GridSnake's menus (chapter 16) use UMG's own
navigation instead, but a grid-based level editor would be a good use for Pulse.

</details>

---

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-8--enhanced-input)
