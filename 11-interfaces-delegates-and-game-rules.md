# Chapter 11 — Interfaces, Delegates and Game Rules

At the end of chapter 10 the snake could feel what it touched: driving over a gem logged
`Snake entered BP_GSGem_C_0 ...`, and walls killed it. Nothing *happened*, though. The gem stayed,
the score stayed at zero and the exit never opened. This chapter turns GridSnake into a game with
rules. Eating a gem grows the snake and scores 100 points. Three gems open the exit, and entering
it finishes the level. Dying, or finishing, restarts the level after a short pause. Each new rule is
easy to code; the work is in deciding *where* it goes. You will meet three Unreal tools that keep
classes from knowing too much about each other. **Interfaces** let the snake use anything it
enters without knowing its type. **Delegates** let an object announce an event without knowing who
listens. The **GameMode/GameState split** from chapter 7 gives the rules and the score one home
each.

Previous: [Chapter 10](10-collision-in-2d.md). Next:
[Chapter 12](12-flipbook-animation-from-cpp.md).

> **Reminder.** Nothing in this course was compiled or run by the authors (see the README,
> "How this course was checked"). Every *Expected Output Log* block shows the lines the code's
> `UE_LOG` calls will print. Parts in `<angle brackets>` vary on your machine.

## Who should know about whom

Start with the most direct way to add the rules. The snake already gets
`NotifyActorBeginOverlap` when its head enters a gem or the exit. It could do everything there:

```cpp
// Do NOT write this: every branch ties the snake to another class.
void AGSSnake::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (AGSGem* Gem = Cast<AGSGem>(OtherActor))
	{
		Grow(2);                                                // the growth rule,
		GetWorld()->GetGameState<AGSGameState>()->AddGem(100);  // the points,
		// ... then find the exit and open it after the third gem,
		Gem->Destroy();
	}
	else if (AGSExit* Exit = Cast<AGSExit>(OtherActor))
	{
		// ... and decide what finishing a level means.
	}
}
```

It would work, and every branch couples the snake to something it should not care about:

- **Every new pickup edits the snake.** A key, a portal or a speed pad each add a `Cast` branch.
- **Rules live in the pawn.** "A gem is worth 100 points" is a rule of the game, not a fact about
  snakes. A time-attack mode where gems add seconds would have to change the snake.
- **The exit cannot react on its own.** Someone has to find it and open it, and that someone must
  also know every other thing (a sound, a HUD message) that reacts to the last gem.

The fix is a rule of thumb you will apply for the rest of the course:

> **Actors report events. The game mode decides. The game state records. Anyone interested
> listens.**

Three tools put that rule into code:

| Problem | Tool | Who stops knowing about whom |
|---|---|---|
| The snake needs to do "the right thing" with whatever it enters | **Interface** `IGSSnakeInteractable` | The snake stops knowing gem and exit types |
| The gem count must reach the exit, the HUD, the sound | **Delegate** `OnAllGemsCollected` | The game state stops knowing who reacts |
| Points, growth, win and loss must live somewhere | **GameMode** functions, **GameState** data | Gems and exits stop knowing the rules |

The rest of this chapter takes them one at a time and then wires them together.

## Unreal interfaces

An interface is a list of functions that unrelated classes can promise to implement. A gem and an
exit share no useful base class besides `AActor`, but both can promise "the snake may enter me".
The snake then talks to the promise, not to the class.

Plain C++ already has this: an abstract class with pure virtual functions, inherited alongside the
real base class. Unreal wraps that idea so that Blueprints can implement an interface too, and so
that the engine can ask "does this object implement it?" at run time through reflection
(chapter 3).

### Two classes for one interface

An Unreal interface is declared as two classes in one header. Here is the one this chapter's
first example uses, `examples/ch11/ExUsable.h`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ExUsable.generated.h"

/** The reflection half of the interface: UHT needs a UObject-derived type to describe. */
UINTERFACE(MinimalAPI, Blueprintable)
class UExUsable : public UInterface
{
	GENERATED_BODY()
};

/**
 * The C++ half: the functions an implementer provides. Anything a player can
 * "use" (lamps, levers, signs) implements it, in C++ or in a Blueprint.
 */
class IExUsable
{
	GENERATED_BODY()

public:
	/** C++ classes override Use_Implementation; Blueprints implement the "Use" event. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Example")
	void Use(AActor* User);
};
```

- **`UExUsable`** derives from `UInterface`, which is a `UObject`. It exists only so that the
  reflection system has a `UClass` to describe the interface. You never inherit from it and
  never create one. You use it as a *type tag*: `Implements<UExUsable>()`,
  `UExUsable::StaticClass()`.
- **`IExUsable`** is a plain C++ class (no `UObject` base) that holds the functions. Actor
  classes inherit it next to their real base: `class AExUsableLamp : public AActor, public
  IExUsable`. Inheriting a second *`UObject`* base is not allowed; inheriting an interface is.
- **`UINTERFACE` specifiers.** `MinimalAPI` exports only what other modules need of the `U`
  class. `Blueprintable` lets Blueprints implement the interface (Class Settings > Interfaces);
  `meta = (CannotImplementInterfaceInBlueprint)` allows only C++ implementers.
- **Both halves** need `GENERATED_BODY()`; the file needs its `.generated.h` include.

### Three kinds of interface function

What you write in front of an interface function decides who can implement it and how callers
call it:

| Declared as | Implemented in | Called with |
|---|---|---|
| `UFUNCTION(BlueprintNativeEvent)` | C++ (`Use_Implementation`) **or** Blueprint (event `Use`) | `IExUsable::Execute_Use(Object, ...)` |
| `UFUNCTION(BlueprintImplementableEvent)` | Blueprint only | `IExUsable::Execute_Use(Object, ...)` |
| plain `virtual void Use(...) = 0;` (no `UFUNCTION`) | C++ only | `Cast<IExUsable>(Object)->Use(...)` |

GridSnake uses the first kind everywhere, because it keeps both doors open. Two rules come with
it:

- **Override `Use_Implementation`, never `Use`.** UHT generates `Use` itself. Your class declares
  `virtual void Use_Implementation(AActor* User) override;`, with no `UFUNCTION` (the interface
  already has one).
- **Call `Execute_Use`, never `Use`.** `IExUsable::Execute_Use(Object, User)` is a static function
  that UHT generates. It looks the function up through reflection and calls the C++
  `_Implementation` *or* the Blueprint event, whichever the object's class has. Calling
  `Interface->Use(User)` directly on an interface pointer is a mistake: the generated body stops
  the game with an assertion whose message tells you to call `Execute_Use` instead.

### Asking whether an object implements an interface

There are three ways to check, and they are not interchangeable:

| Test | True for C++ implementers | True for Blueprint-only implementers |
|---|---|---|
| `Object->Implements<UExUsable>()` | yes | yes |
| `Object->GetClass()->ImplementsInterface(UExUsable::StaticClass())` | yes | yes |
| `Cast<IExUsable>(Object) != nullptr` | yes | **no** |

A Blueprint that adds an interface in its Class Settings does not change the C++ class layout, so
there is no `IExUsable` part for `Cast` to find. The reflection data knows about the interface,
though, so `Implements<>` sees it and `Execute_` reaches it. The safe pattern is therefore
`if (Actor->Implements<UExUsable>()) { IExUsable::Execute_Use(Actor, this); }`. Note the two
prefixes: the `U` class in the test, the `I` class in the call, whose first argument is the target.

If you want to *store* a reference to "some object that implements the interface", for example in
a `UPROPERTY`, use `TScriptInterface<IExUsable>`. It keeps the `UObject*` (which the garbage
collector can see) and the interface pointer side by side. GridSnake never stores one, so this
course does not need it.

### Example: things you can use

`examples/ch11/` has three classes: the interface `ExUsable.h` (shown above), one C++
implementer and a user that uses everything in the level. You will add a Blueprint-only
implementer in the editor.

`examples/ch11/ExUsableLamp.h`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExUsable.h"
#include "ExUsableLamp.generated.h"

class UPointLightComponent;

/** A light that switches on and off when used. Implements IExUsable in C++. */
UCLASS()
class AExUsableLamp : public AActor, public IExUsable
{
	GENERATED_BODY()

public:
	AExUsableLamp();

	virtual void OnConstruction(const FTransform& Transform) override;

	// IExUsable. Override the _Implementation, never Use() itself.
	virtual void Use_Implementation(AActor* User) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(EditAnywhere, Category = "Example")
	bool bOn = true;
};
```

`examples/ch11/ExUsableLamp.cpp`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExUsableLamp.h"
#include "Components/PointLightComponent.h"

AExUsableLamp::AExUsableLamp()
{
	PrimaryActorTick.bCanEverTick = false;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	SetRootComponent(Light);
	// A Static light is baked into the level; a Movable one can be switched freely.
	Light->SetMobility(EComponentMobility::Movable);
}

void AExUsableLamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Light->SetVisibility(bOn);
}

void AExUsableLamp::Use_Implementation(AActor* User)
{
	bOn = !bOn;
	Light->SetVisibility(bOn);
	UE_LOG(LogTemp, Log, TEXT("%s switched %s by %s"), *GetName(), bOn ? TEXT("on") : TEXT("off"), *GetNameSafe(User));
}
```

`examples/ch11/ExUser.h`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExUser.generated.h"

/**
 * Uses every actor in the level that implements IExUsable, half a second after
 * play starts. It never names a concrete class: lamps, signs and anything
 * added later are all reached the same way.
 */
UCLASS()
class AExUser : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Example")
	void UseEverything();

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle Timer;
};
```

`examples/ch11/ExUser.cpp`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExUser.h"
#include "EngineUtils.h"
#include "ExUsable.h"
#include "TimerManager.h"

void AExUser::BeginPlay()
{
	Super::BeginPlay();

	// Wait a moment, so every actor in the level has begun play.
	GetWorldTimerManager().SetTimer(Timer, this, &AExUser::UseEverything, 0.5f, /*bLoop=*/false);
}

void AExUser::UseEverything()
{
	int32 Count = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;

		// Implements<> asks the class's reflection data: true for C++ and Blueprint implementers.
		if (!Actor->Implements<UExUsable>())
		{
			continue;
		}

		// Cast<IExUsable> works only if a C++ class inherits IExUsable.
		const bool bNative = Cast<IExUsable>(Actor) != nullptr;
		UE_LOG(LogTemp, Log, TEXT("Using %s (%s implementation)"), *Actor->GetName(),
			bNative ? TEXT("C++") : TEXT("Blueprint-only"));

		// Execute_ dispatches through reflection, so it reaches both kinds.
		IExUsable::Execute_Use(Actor, this);
		++Count;
	}

	UE_LOG(LogTemp, Log, TEXT("%s used %d actor(s)"), *GetName(), Count);
}
```

The modules are `Core`, `CoreUObject` and `Engine`, which every C++ project template already lists
in its `Build.cs`. `TActorIterator` lives in `EngineUtils.h` and visits every actor of a class in a
world. It is fine for a one-off example; in a game, prefer keeping references to the actors you
care about.

To see it:

1. Copy the five files into the `Source/<YourModule>/` folder of a scratch UE 5.8 C++ project
   (not GridSnake). Build.
2. Open any 3D level, for example a new **Basic** level. Drag an `ExUsableLamp` and an `ExUser`
   from **C++ Classes** into it.
3. Make a Blueprint-only implementer:
   - In the Content Browser, **Add > Blueprint Class > Actor**. Name it `BP_ExSign`.
   - Open it and click **Class Settings** in the toolbar. In the Details panel, under
     **Interfaces > Implemented Interfaces**, click **Add** and choose **ExUsable**. Compile.
   - In the **My Blueprint** panel, the **Interfaces** section now lists **Use**. Double-click it.
     An **Event Use** node appears in the Event Graph, with a **User** pin.
   - Drag from its execution pin and add a **Print String** node. Type `The sign reads: keep
     out` into **In String**. Compile and save.
   - Drag `BP_ExSign` into the level.
4. In the Output Log's search box, type `Using` to start with, then clear it after the run to see
   the other lines. Press **Play**.

Expected Output Log (filter: `LogTemp` and `LogBlueprintUserMessages`), after half a second:

```text
LogTemp: Using <ExUsableLamp_0> (C++ implementation)
LogTemp: <ExUsableLamp_0> switched off by <ExUser_0>
LogTemp: Using <BP_ExSign_C_0> (Blueprint-only implementation)
LogBlueprintUserMessages: [<BP_ExSign_C_0>] The sign reads: keep out
LogTemp: <ExUser_0> used 2 actor(s)
```

- **The order of the pairs** follows `TActorIterator`'s order, so the sign may come first. Each
  "Using" line is directly followed by that actor's line: `Execute_Use` runs immediately.
- **Blueprint-only** is reported for the sign, because `Cast<IExUsable>` returns null for it. The
  call reached it anyway: `Implements<>` and `Execute_` use reflection.
- **The lamp goes dark.** It started with `bOn = true`, so the first use switches it off.
  `OnConstruction` applied `bOn` when you placed it, so the editor and the game agree.
- **Print String** also writes to the top-left of the viewport. In the log, its category is
  `LogBlueprintUserMessages` and the object's name is in square brackets.
- **Names vary**: `_0`, `_C_0` and similar suffixes depend on what else is in the level, and
  levels that use *One File Per Actor* get longer names with `UAID` in them.

## Delegates

An interface helps when the *caller* knows that something must happen and the *target* decides
what. The opposite case is just as common. An object knows that something *has happened* ("all
gems are collected", "the snake died") but should not know who cares. That is what a delegate is
for.

A delegate is a type-safe, object-aware function pointer. The object that announces the event owns
a delegate variable. Anyone interested **binds** a function to it. When the event happens, the
owner **executes** the delegate (one listener) or **broadcasts** it (any number), and every bound
function runs. The owner never learns who was listening.

You have used one already. Chapter 10's overlap events are delegates:
`Box->OnComponentBeginOverlap.AddDynamic(...)` binds a function to a delegate that the engine
broadcasts. Now you will declare your own.

### Four kinds

Unreal has four common kinds of delegate. They differ in two ways: one target or many, and
whether the reflection system knows about them (*dynamic*) or not.

| Kind | Declared with | Targets | Return value | Blueprints | Bind with |
|---|---|---|---|---|---|
| Single | `DECLARE_DELEGATE_...` | one | allowed (`_RetVal_`) | no | `BindUObject`, `BindLambda`, `BindRaw`, `BindSP`, `BindStatic`, `BindUFunction` |
| Multicast | `DECLARE_MULTICAST_DELEGATE_...` | many | no | no | `AddUObject`, `AddLambda`, `AddWeakLambda`, `AddRaw`, `AddSP`, `AddStatic` |
| Dynamic single | `DECLARE_DYNAMIC_DELEGATE_...` | one | allowed (`_RetVal_`) | as a function parameter | `BindDynamic` |
| Dynamic multicast | `DECLARE_DYNAMIC_MULTICAST_DELEGATE_...` | many | no | `BlueprintAssignable` property | `AddDynamic`, `AddUniqueDynamic`, `RemoveDynamic` |

- **The macro name spells out the signature.** `_OneParam`, `_TwoParams` and so on up to
  `_NineParams`, or no suffix for none. `DECLARE_DELEGATE_RetVal_OneParam(int32, FExSquare,
  int32)` is "a single delegate named `FExSquare` that takes an `int32` and returns an `int32`".
- **Dynamic delegates name their parameters.** A native multicast takes types only:
  `DECLARE_MULTICAST_DELEGATE_OneParam(FExNotify, const FString&)`. A dynamic one takes type, then
  name: `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExScored, int32, Points)`. The names become
  the pin names in Blueprints.
- **Dynamic delegates find their target by name.** `AddDynamic(this, &AMyActor::HandleScored)`
  is a macro. It turns the function name into a string, and at run time the engine looks that
  name up among the object's reflected functions. So the target **must be a `UFUNCTION`**. If
  it is not, the code still compiles, but binding fails at run time with an ensure (an error
  with a call stack in the log) that reads `Unable to bind delegate to 'HandleScored' (function
  might not be marked as a UFUNCTION or object may be pending kill)`, and the function is never
  called.
- **Dynamic delegates cost more**, because the call goes through reflection. That matters only
  for something broadcast thousands of times per frame.
- **Blueprint event dispatchers are dynamic multicast delegates.** A
  `UPROPERTY(BlueprintAssignable)` of that type appears in Blueprints as **Bind Event to On
  Scored** and **Assign On Scored** nodes. Add `BlueprintCallable` to the property if Blueprints
  should also be able to broadcast it.

Pick by this rule: if Blueprints need to bind, use dynamic multicast. If only C++ binds, a native
multicast delegate is lighter and accepts lambdas. If exactly one object should answer, possibly
with a return value, use a single delegate.

For each delegate, GridSnake declares a type named `F...Signature` and a property named `On...`.
That naming is the engine's own convention: the overlap delegate type is
`FComponentBeginOverlapSignature`, the property `OnComponentBeginOverlap`.

### Calling a delegate

| Call | Use it on | When nothing is bound |
|---|---|---|
| `Execute(Args)` | single | asserts: check `IsBound()` first |
| `ExecuteIfBound(Args)` | single, no return value | does nothing, returns `false` |
| `Broadcast(Args)` | multicast | does nothing |

Broadcasting with no listeners is normal: this chapter's game state broadcasts `OnScoreChanged`
although nothing listens until chapter 15 adds the HUD.

**Do not rely on the order of listeners.** A multicast delegate calls all its bindings, but the
order is an implementation detail. If B must run after A, have A trigger B, or give the owner two
delegates ("before" and "after").

### Lifetimes and unbinding

A delegate keeps pointers to its listeners, so it matters what happens when a listener dies first.

- **`UObject` bindings are safe.** `BindUObject`, `AddUObject` and all dynamic bindings hold a
  *weak* reference. When the listener is destroyed and garbage collected, the binding stops
  firing instead of calling into freed memory.
- **Lambdas and `Raw` bindings are not.** A lambda that captures `this` and is called after
  `this` was destroyed is a crash waiting to happen. Use `AddWeakLambda(this, [...]{ ... })`,
  which ties the lambda to a `UObject` and skips it once that object is gone.
- **Unbind when the announcer outlives the listener.** A listener that lives shorter than the
  object it listens to should remove itself, usually in `EndPlay`.
  - Native multicast: `Delegate.Remove(Handle)` with the `FDelegateHandle` that `Add...` returned,
    or `Delegate.RemoveAll(this)` to remove every binding of an object.
  - Dynamic: `Delegate.RemoveDynamic(this, &AMyActor::HandleScored)`.
- **Bind once.** Binding the same function twice is a bug: depending on the kind of delegate it
  runs twice or reports an ensure. `AddUniqueDynamic` skips duplicates when the binding code can
  run more than once.

In GridSnake the announcers and listeners live and die together, because the whole level is
reloaded when it restarts. So the project never unbinds.

### Example: a delegate zoo

`examples/ch11/ExDelegateZoo.h`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExDelegateZoo.generated.h"

// Single, native: one target, may return a value. Parameters: types only.
DECLARE_DELEGATE_RetVal_OneParam(int32, FExSquare, int32);

// Multicast, native: any number of targets, no return value.
DECLARE_MULTICAST_DELEGATE_OneParam(FExNotify, const FString&);

// Multicast, dynamic: reflected, so Blueprints can bind. Parameters: type, then name.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExScored, int32, Points);

/** Declares one delegate of each common kind and exercises it in BeginPlay. */
UCLASS()
class AExDelegateZoo : public AActor
{
	GENERATED_BODY()

public:
	/** Shows up as "Assign On Scored" / "Bind Event to On Scored" in Blueprints. */
	UPROPERTY(BlueprintAssignable, Category = "Example")
	FExScored OnScored;

protected:
	virtual void BeginPlay() override;

private:
	/** A plain member function: fine for AddUObject, no UFUNCTION needed. */
	void NativeListener(const FString& Message);

	/** Dynamic delegates find their target by name, so this must be a UFUNCTION. */
	UFUNCTION()
	void HandleScored(int32 Points);

	FExSquare Square;
	FExNotify OnNotify;
};
```

`examples/ch11/ExDelegateZoo.cpp`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExDelegateZoo.h"

DEFINE_LOG_CATEGORY_STATIC(LogExDelegates, Log, All);

void AExDelegateZoo::BeginPlay()
{
	Super::BeginPlay();

	// 1. Single delegate. Execute() on an unbound delegate asserts, so check first.
	Square.BindLambda([](int32 Value) { return Value * Value; });
	if (Square.IsBound())
	{
		UE_LOG(LogExDelegates, Log, TEXT("Square(7) = %d"), Square.Execute(7));
	}

	// 2. Multicast delegate. Every Add returns a handle you can remove it with.
	const FDelegateHandle NativeHandle = OnNotify.AddUObject(this, &AExDelegateZoo::NativeListener);
	OnNotify.AddLambda([](const FString& Message)
	{
		UE_LOG(LogExDelegates, Log, TEXT("Lambda listener got '%s'"), *Message);
	});
	OnNotify.Broadcast(TEXT("hello"));

	OnNotify.Remove(NativeHandle);
	UE_LOG(LogExDelegates, Log, TEXT("After Remove:"));
	OnNotify.Broadcast(TEXT("again"));

	// 3. Dynamic multicast delegate. Broadcasting with no listeners is fine.
	OnScored.AddDynamic(this, &AExDelegateZoo::HandleScored);
	OnScored.Broadcast(10);
}

void AExDelegateZoo::NativeListener(const FString& Message)
{
	UE_LOG(LogExDelegates, Log, TEXT("Native listener got '%s'"), *Message);
}

void AExDelegateZoo::HandleScored(int32 Points)
{
	UE_LOG(LogExDelegates, Log, TEXT("Dynamic listener got %d points"), Points);
}
```

Modules: `Core`, `CoreUObject`, `Engine`. Delegates themselves live in `Core`; nothing extra to
include beyond `CoreMinimal.h`.

To see it:

1. Copy both files into your scratch project's `Source/<YourModule>/` folder and build.
2. Drag an `ExDelegateZoo` into any level.
3. Type `LogExDelegates` into the Output Log's search box and press **Play**.

Expected Output Log (filter: `LogExDelegates`):

```text
LogExDelegates: Square(7) = 49
LogExDelegates: Native listener got 'hello'
LogExDelegates: Lambda listener got 'hello'
LogExDelegates: After Remove:
LogExDelegates: Lambda listener got 'again'
LogExDelegates: Dynamic listener got 10 points
```

- **The two `hello` lines may swap**: the order of listeners is not part of the contract.
- **`Remove` needs the handle.** Only the native listener was removed, so `again` reaches the
  lambda alone. A lambda has no name, so keep its handle if you ever want it gone. It captures
  nothing, so it is safe; had it captured `this`, `AddWeakLambda(this, ...)` would be the choice.
- **Try the failure.** Delete the `UFUNCTION()` line above `HandleScored` and build. It compiles.
  Play, and instead of the last line the log shows the `Unable to bind delegate to
  'HandleScored'` ensure. Put the line back.
- **Blueprint side.** Make a Blueprint subclass of `ExDelegateZoo` and place it instead. In its
  Event Graph, right-click and search for `On Scored`: **Bind Event to On Scored** and **Assign On
  Scored** are your C++ property. Use **Assign On Scored** after **Event BeginPlay** and print the
  **Points** pin. It is heard: `AActor::BeginPlay` is what runs the Blueprint's **Event
  BeginPlay**, so the binding happens inside `Super::BeginPlay()`, before the C++ code broadcasts.
  A listener that binds *after* a broadcast misses it. Bind early, broadcast later.

## The game mode decides

Chapter 7 gave the game mode one job, "the rules", and so far it only runs chapter 9's countdown.
Now it gets the real rules. Everything that answers "what happens when ...?" goes into
`AGSGameMode`. The header gains two public entry points, `OnGemCollected(AGSGem*, AGSSnake*)` and
`OnExitReached(AGSSnake*)`; two protected helpers, `GetGSGameState()` and `GetSnake()`; a
`RestartLevel()` function; a private `FTimerHandle LevelEndTimer`; and these:

```cpp
// GSGameMode.h (excerpt)
protected:
	/** Segments the snake grows per gem. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0"))
	int32 GrowthPerGem = 2;

	/** Pause between dying (or finishing) and the next level loading. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0", Units = "s"))
	float LevelEndDelay = 1.5f;

private:
	UFUNCTION()
	void HandleSnakeDied(AGSSnake* Snake);
```

```cpp
// GSGameMode.cpp (excerpt)
void AGSGameMode::OnGemCollected(AGSGem* Gem, AGSSnake* Snake)
{
	AGSGameState* State = GetGSGameState();
	if (State->GetPhase() != EGSLevelPhase::Playing)
	{
		return;
	}

	State->AddGem(UGSSettings::Get()->PointsPerGem);
	Snake->Grow(GrowthPerGem);
}

void AGSGameMode::OnExitReached(AGSSnake* Snake)
{
	AGSGameState* State = GetGSGameState();
	if (State->GetPhase() != EGSLevelPhase::Playing)
	{
		return;
	}

	Snake->StopMoving();
	State->SetPhase(EGSLevelPhase::Completed);
	UE_LOG(LogGridSnake, Log, TEXT("Level complete with %d points"), State->GetScore());

	// Chapter 13 loads the next level from data; for now, play this one again.
	GetWorldTimerManager().SetTimer(LevelEndTimer, this, &AGSGameMode::RestartLevel, LevelEndDelay, /*bLoop=*/false);
}

void AGSGameMode::HandleSnakeDied(AGSSnake* Snake)
{
	GetGSGameState()->SetPhase(EGSLevelPhase::Failed);
	GetWorldTimerManager().SetTimer(LevelEndTimer, this, &AGSGameMode::RestartLevel, LevelEndDelay, /*bLoop=*/false);
}

void AGSGameMode::RestartLevel()
{
	// bRemovePrefixString strips the "UEDPIE_0_" prefix that Play In Editor adds.
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	UE_LOG(LogGridSnake, Log, TEXT("Restarting %s"), *LevelName);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}
```

The helpers are one-liners: `GetGSGameState()` returns `GetGameState<AGSGameState>()`, and
`GetSnake()` casts `UGameplayStatics::GetPlayerPawn(this, 0)` to `AGSSnake`.

- **Two ways in.** Gems and exits *call* the game mode: they know a game mode exists and what to
  report. The snake's death arrives by *delegate*: the snake knows nothing about game modes, and
  the game mode subscribes in `StartPlay` (below). Both are fine. Use a direct call when the
  reporter has a natural single recipient, a delegate when it should not care.
- **Every rule checks the phase first.** A gem entered after the level is complete, or after the
  snake died, must not count. Checking `Playing` here is simpler than making every caller check.
- **Rules are data.** `GrowthPerGem` and `LevelEndDelay` are `EditDefaultsOnly`, so a designer can
  tune them in `BP_GSGameMode`; `Units = "s"` shows the delay as seconds. Points per gem come from
  the project settings (chapter 3). `Grow` is a snake ability, but *how much* to grow is a rule.
- **`OpenLevel` restarts everything.** Loading the map again destroys the world, with its game
  mode, game state, snake, gems and exit, and builds fresh ones: no state to reset by hand. The
  load does not happen inside the call; the engine travels on a later frame.
- **`GetCurrentLevelName(this, true)`.** In Play In Editor the map is called
  `UEDPIE_0_L_Level01`. `OpenLevel` wants the asset name, `L_Level01`, so the prefix is removed.
- **`HandleSnakeDied` is a `UFUNCTION`**, because it is the target of a dynamic delegate. It is
  private: only the delegate calls it, and reflection does not care about C++ access.

`StartPlay` binds the death delegate, right after its log line:

```cpp
	// The player's pawn was spawned when the player joined, before StartPlay.
	if (AGSSnake* Snake = GetSnake())
	{
		Snake->OnDied.AddDynamic(this, &AGSGameMode::HandleSnakeDied);
	}
```

The countdown now writes to the game state instead of logging "Go!". On each tick
`TickCountdown` calls `State->SetCountdown(CountdownRemaining)` next to its `Countdown: %d` log.
On the last tick it calls `SetCountdown(0)` and `SetPhase(EGSLevelPhase::Playing)` before
`Snake->StartMoving()`. Both functions also use the new `GetSnake()` helper instead of casting
the player pawn themselves.

Chapter 7 showed why binding in `StartPlay` is safe: the player logs in and the pawn is spawned and
possessed *before* `StartPlay` runs.

The gem and the exit find the game mode with `GetWorld()->GetAuthGameMode<AGSGameMode>()`. "Auth"
stands for *authority*: only the machine that runs the game's rules has a game mode. In a
networked game a client gets `nullptr` there, which is why the calls sit inside an `if`. In single
player there is always one, but the habit is free.

## The game state records

The game mode decides; the game state remembers, and tells everyone what changed. Chapter 7's
`AGSGameState` held gems and score. Now it also holds the **phase** of the level and the
**countdown**, and it announces every change with a delegate:

```cpp
// GSGameState.h (excerpt)
/** Where the current level is in its life. */
UENUM(BlueprintType)
enum class EGSLevelPhase : uint8
{
	Countdown,
	Playing,
	Completed,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSGemsChangedSignature, int32, Collected, int32, Required);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSScoreChangedSignature, int32, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSAllGemsCollectedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSPhaseChangedSignature, EGSLevelPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSCountdownChangedSignature, int32, SecondsLeft);

UCLASS()
class AGSGameState : public AGameStateBase
{
	// ...
	// New BlueprintPure getters: AreAllGemsCollected(), GetPhase(), GetCountdown()

	/** Counts a gem and its points. Fires OnAllGemsCollected when the last one is taken. */
	void AddGem(int32 Points);

	void AddScore(int32 Points);
	void SetPhase(EGSLevelPhase NewPhase);
	void SetCountdown(int32 SecondsLeft);

	UPROPERTY(BlueprintAssignable, Category = "GridSnake")
	FGSGemsChangedSignature OnGemsChanged;

	// ... and the same for OnScoreChanged, OnAllGemsCollected, OnPhaseChanged, OnCountdownChanged

protected:
	// ... GemsCollected, GemsRequired, Score as before, plus two VisibleInstanceOnly members:
	EGSLevelPhase Phase = EGSLevelPhase::Countdown;
	int32 Countdown = 0;
};
```

```cpp
// GSGameState.cpp (excerpt)
void AGSGameState::AddGem(int32 Points)
{
	if (AreAllGemsCollected())
	{
		return;
	}

	++GemsCollected;
	UE_LOG(LogGridSnake, Log, TEXT("Gem %d/%d"), GemsCollected, GemsRequired);
	OnGemsChanged.Broadcast(GemsCollected, GemsRequired);
	AddScore(Points);

	if (AreAllGemsCollected())
	{
		UE_LOG(LogGridSnake, Log, TEXT("All gems collected - the exit opens"));
		OnAllGemsCollected.Broadcast();
	}
}

void AGSGameState::SetPhase(EGSLevelPhase NewPhase)
{
	if (Phase == NewPhase)
	{
		return;
	}

	Phase = NewPhase;
	UE_LOG(LogGridSnake, Log, TEXT("Phase: %s"), *UEnum::GetValueAsString(Phase));
	OnPhaseChanged.Broadcast(Phase);
}
```

`AddScore` and `SetCountdown` follow the same pattern: change the member, then broadcast
`OnScoreChanged` or `OnCountdownChanged` with the new value.

- **Change and announce in one place.** Every setter both changes the value and broadcasts.
  Nobody can change the score without the HUD hearing about it, because there is no other way to
  change it. That is why the members stay `protected` and the setters are the only door.
- **Events instead of polling.** Without delegates, the exit would check
  `AreAllGemsCollected()` in its `Tick`, 60 times a second, to catch one change per level. With
  `OnAllGemsCollected` it does nothing until the moment it matters.
- **Broadcast what listeners need.** `OnGemsChanged` sends both numbers, so a HUD can show "2/3"
  without asking again. `AddGem` refuses a fourth gem, so `OnAllGemsCollected` fires once.
- **The setters are not `BlueprintCallable`.** Blueprints may read and listen, but only the game
  mode writes.
- **`Phase: EGSLevelPhase::Countdown` is never logged.** The phase *starts* as `Countdown`, and
  `SetPhase` returns early when nothing changes. The first phase line you will see is `Playing`.

### State machines and `ensure`

`EGSLevelPhase` makes the level a small *state machine*: a set of states and the moves between
them. GridSnake's moves are Countdown → Playing → Completed or Failed, and then a fresh level. The
game mode only ever makes those moves, so `SetPhase` trusts it. In a bigger game it pays to let the
state machine itself refuse illegal moves, and to make a refusal loud without crashing. That is
what `ensure` is for.

| Macro | If the condition is false | In Shipping builds |
|---|---|---|
| `check(Cond)` / `checkf(Cond, Fmt, ...)` | stops the program (a crash with a report) | compiled out: `Cond` is **not** evaluated |
| `verify(Cond)` | like `check` | `Cond` is still evaluated |
| `ensure(Cond)` / `ensureMsgf(Cond, Fmt, ...)` | logs an error with a call stack, **once** per call site, breaks into an attached debugger, and continues | no report, but `Cond` is still evaluated |
| `ensureAlways(Cond)` | like `ensure`, but reports every time | as `ensure` |

All `ensure` macros *return* the condition, so they fit into an `if`: report the bug, then handle
it gracefully. Use `check` for "the program cannot continue" and `ensure` for "this is a bug, but
the game can survive it". Chapter 19 returns to asserts in depth.

`examples/ch11/ExPhaseMachine.h`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExPhaseMachine.generated.h"

UENUM(BlueprintType)
enum class EExPhase : uint8
{
	Ready,
	Playing,
	Won,
	Lost
};

/** A four-state phase machine that refuses illegal transitions. */
UCLASS()
class AExPhaseMachine : public AActor
{
	GENERATED_BODY()

public:
	/** Returns false, and reports an ensure, if the transition is not allowed. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	bool SetPhase(EExPhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Example")
	EExPhase GetPhase() const { return Phase; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, Category = "Example")
	EExPhase Phase = EExPhase::Ready;

private:
	static bool IsLegal(EExPhase From, EExPhase To);
};
```

`examples/ch11/ExPhaseMachine.cpp`:

```cpp
// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExPhaseMachine.h"

DEFINE_LOG_CATEGORY_STATIC(LogExPhase, Log, All);

void AExPhaseMachine::BeginPlay()
{
	Super::BeginPlay();

	SetPhase(EExPhase::Playing);
	SetPhase(EExPhase::Won);
	SetPhase(EExPhase::Playing); // illegal: a won game cannot resume
	SetPhase(EExPhase::Ready);   // legal: restart
}

bool AExPhaseMachine::IsLegal(EExPhase From, EExPhase To)
{
	switch (From)
	{
	case EExPhase::Ready:   return To == EExPhase::Playing;
	case EExPhase::Playing: return To == EExPhase::Won || To == EExPhase::Lost;
	case EExPhase::Won:
	case EExPhase::Lost:    return To == EExPhase::Ready;
	}
	return false;
}

bool AExPhaseMachine::SetPhase(EExPhase NewPhase)
{
	if (NewPhase == Phase)
	{
		return true;
	}

	// ensureMsgf logs the message and a call stack the first time it fails, then
	// returns the condition, so the game keeps running and we can bail out.
	if (!ensureMsgf(IsLegal(Phase, NewPhase), TEXT("Illegal phase change %s -> %s"),
		*UEnum::GetValueAsString(Phase), *UEnum::GetValueAsString(NewPhase)))
	{
		return false;
	}

	UE_LOG(LogExPhase, Log, TEXT("%s -> %s"), *UEnum::GetValueAsString(Phase), *UEnum::GetValueAsString(NewPhase));
	Phase = NewPhase;
	return true;
}
```

Modules: `Core`, `CoreUObject`, `Engine`. To see it, copy both files into your scratch project,
build, drag an `ExPhaseMachine` into a level, clear the Output Log's search box and press **Play**.

Expected Output Log (filter: `LogExPhase` and `Ensure`; the ensure block is approximate):

```text
LogExPhase: EExPhase::Ready -> EExPhase::Playing
LogExPhase: EExPhase::Playing -> EExPhase::Won
LogOutputDevice: Error: Ensure condition failed: IsLegal(Phase, NewPhase) [File:<path>/ExPhaseMachine.cpp] [Line: <n>]
Illegal phase change EExPhase::Won -> EExPhase::Playing
LogOutputDevice: Error: Stack:
LogOutputDevice: Error: <call stack, several lines>
LogExPhase: EExPhase::Won -> EExPhase::Ready
```

- **The game kept going.** `SetPhase` returned `false`, the phase stayed `Won`, and the legal
  `Ready` move worked.
- **The ensure block's exact shape varies** with engine version and platform; the condition,
  file, line and message are always in it. Expect a short hitch while the call stack is
  collected. An attached debugger stops on the line.
- **Play a second time** without restarting the editor: the move is still refused, but the ensure
  block does *not* repeat, because `ensure` reports once per call site per session.

## Wiring it together

With the three tools in place, a gem pickup becomes a chain of short, single-purpose calls. Here
is the third gem, from the snake's step to the open exit. Indentation means "called from":

```text
AGSSnake::Step -> UpdateVisuals -> SetActorLocation      the head moves one cell
  AGSSnake::NotifyActorBeginOverlap(BP_GSGem)          the engine reports the overlap
    BP_GSGem->Implements<UGSSnakeInteractable>()       true
    IGSSnakeInteractable::Execute_OnSnakeEntered(BP_GSGem, Snake)
      AGSGem::OnSnakeEntered_Implementation(Snake)     the gem reports...
        AGSGameMode::OnGemCollected(Gem, Snake)        ...the game mode decides...
          AGSGameState::AddGem(100)                    ...the game state records
            log "Gem 3/3"
            OnGemsChanged.Broadcast(3, 3)              (nobody listens yet: chapter 15)
            AddScore(100) -> OnScoreChanged.Broadcast(300)
            log "All gems collected - the exit opens"
            OnAllGemsCollected.Broadcast()
              AGSExit::HandleAllGemsCollected          the exit was listening
                AGSExit::SetOpen(true) -> log "BP_GSExit_C_0 is now open"
          AGSSnake::Grow(2)
        AGSGem::Destroy()
```

Who knows about whom now:

| Class | Knows about | Does not know about |
|---|---|---|
| `AGSSnake` | `IGSSnakeInteractable`, its own `OnDied` | gems, exits, game mode, game state |
| `AGSGem`, `AGSExit` | the interface, `AGSGameMode` (to report), `AGSGameState` (exit only, to listen) | each other, scoring, growth |
| `AGSGameMode` | the game state, the snake, the settings | which actors are interactable |
| `AGSGameState` | its own data | who listens |

A new pickup, say a slow-motion potion, now touches *no* existing class except perhaps the game
mode, if it brings a new rule. Exercise 1 builds one.

## Building the milestone

The game mode and game state changes are above in full. The remaining changes connect the
actors: one new header, and small edits to the gem, the exit and the snake.

**The interface**, `Source/GridSnake/Gameplay/GSSnakeInteractable.h` (new):

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GSSnakeInteractable.generated.h"

class AGSSnake;

UINTERFACE(MinimalAPI, Blueprintable)
class UGSSnakeInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the snake's head can enter: gems, exits, and whatever you invent
 * next (keys, portals, speed pads). The snake does not need to know the type.
 */
class IGSSnakeInteractable
{
	GENERATED_BODY()

public:
	/** Called when the snake's head enters this actor's cell. Implement in C++ (_Implementation) or Blueprint. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GridSnake")
	void OnSnakeEntered(AGSSnake* Snake);
};
```

**The snake** calls it, and announces its death. `GSSnake.h` declares the delegate type above
the class and two new public members next to `StartMoving`:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSSnakeDiedSignature, AGSSnake*, Snake);

	// ... inside the class:
	/** Freezes the snake in place (used when the level is complete). */
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void StopMoving();

	/** Fires once, when the snake hits a wall or itself. */
	UPROPERTY(BlueprintAssignable, Category = "Snake")
	FGSSnakeDiedSignature OnDied;
```

In `GSSnake.cpp`, include `Gameplay/GSSnakeInteractable.h`, then:

```cpp
void AGSSnake::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bAlive || !OtherActor)
	{
		return;
	}

	// The snake does not care what it entered, only that it can be entered.
	// Implements<>() and Execute_ work for C++ and Blueprint implementations alike.
	if (OtherActor->Implements<UGSSnakeInteractable>())
	{
		UE_LOG(LogGridSnake, Verbose, TEXT("Snake entered %s at cell %s"), *OtherActor->GetName(), *GetHeadCell().ToString());
		IGSSnakeInteractable::Execute_OnSnakeEntered(OtherActor, this);
	}
}
```

`StopMoving` sets `bMoving` to false and empties the direction queue, and `Die` gains one line at
the end, `OnDied.Broadcast(this);`.

- **The snake includes no gameplay class.** It knows the interface and nothing else, so the
  Blueprint-only interactable of exercise 2 works without a single C++ change.
- **The "Snake entered" log is now `Verbose`**, hidden by default. To see it, run the console
  command `log LogGridSnake Verbose` while playing (the backtick key **`** opens the console).
- **Overlaps fire on a teleport.** `SetActorLocation` without a sweep still updates overlaps at
  the new position, as chapter 10 relied on. If the snake skips a gem, check that chapter first.
- **`OnDied` is broadcast last**, after `bAlive` is false and the log line is written, so a
  listener sees a fully dead snake.

**The gem** inherits the interface and reports. `GSGem.h` includes
`Gameplay/GSSnakeInteractable.h`, changes the class line to `class AGSGem : public AActor, public
IGSSnakeInteractable` and declares `virtual void OnSnakeEntered_Implementation(AGSSnake* Snake)
override;` in its public section. `GSGem.cpp`, which now includes `Core/GSGameMode.h`:

```cpp
void AGSGem::OnSnakeEntered_Implementation(AGSSnake* Snake)
{
	// The gem only reports; the game mode decides what a gem is worth.
	if (AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GameMode->OnGemCollected(this, Snake);
	}

	Destroy();
}
```

`Destroy()` runs while the engine is still inside the overlap notification that started this
chain. That is allowed and common: the actor is marked for destruction and removed safely. Just do
not use the gem after the call.

**The exit** inherits the interface too, and listens for the last gem. `GSExit.h` adds the same
base class and override, plus a private `UFUNCTION() void HandleAllGemsCollected();`.
`GSExit.cpp`, which now includes `Core/GSGameMode.h` and `Core/GSGameState.h`:

```cpp
// In BeginPlay, before the log line:
	// Event-driven: the exit is told when to open instead of polling the gem count.
	if (AGSGameState* GameState = GetWorld()->GetGameState<AGSGameState>())
	{
		GameState->OnAllGemsCollected.AddDynamic(this, &AGSExit::HandleAllGemsCollected);
	}

void AGSExit::OnSnakeEntered_Implementation(AGSSnake* Snake)
{
	// A closed exit blocks the Grid channel, so the snake dies before entering;
	// this check is a safety net for exits opened and closed by other code.
	if (!bIsOpen)
	{
		return;
	}

	if (AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GameMode->OnExitReached(Snake);
	}
}

void AGSExit::HandleAllGemsCollected()
{
	SetOpen(true);
}
```

The game state exists before any `BeginPlay` runs (chapter 7), so binding there is safe. The exit
and the game state die together when the level reloads, so the exit never unbinds.

### Editor steps

These are `project/ch11/ASSETS.md` in narrative form. There are no new assets.

1. **Build.** A new header with a `UINTERFACE` and new base classes for the gem and the exit
   change class layouts, which Live Coding handles poorly. Close the editor, build
   `GridSnakeEditor` from your IDE and reopen the project.
2. **Check the rules.** Open `BP_GSGameMode` > **Class Defaults**. Under **Rules** you find
   **Growth Per Gem** = 2 and **Level End Delay** = 1.5 s, inherited from C++. Leave them.
3. **Check the level.** `L_Level01` must contain exactly three `BP_GSGem` and one `BP_GSExit`
   whose **Is Open** is off. The number three comes from **Project Settings > Game > GridSnake >
   Gems Per Level**: if the level has fewer gems, the exit never opens.
4. **Play.**
   - Each gem disappears when the head enters it, and the snake grows by two segments over the
     next two steps. After the third gem the exit switches to its open flipbook.
   - Entering the open exit stops the snake; 1.5 seconds later the level reloads. Crashing into
     a wall reloads it after the same pause.
   - While playing, select `GSGameState` in the Outliner: **Phase**, **Score** and **Gems
     Collected** update live in its Details panel.

Expected Output Log (filter: `LogGridSnake`), for a level played to the end:

```text
LogGridSnake: GameState ready: 0/3 gems
LogGridSnake: Board <GSBoard_0>: 24x14 cells of 16 units
LogGridSnake: Snake placed at cell X=12 Y=7 facing EGSDirection::Right, length 3
LogGridSnake: <BP_GSExit_C_0> starts closed
LogGridSnake: View target: <GSBoard_0>
LogGridSnake: GridSnake game mode started on map <UEDPIE_0_L_Level01> (collect 3 gems)
LogGridSnake: Countdown: 3
LogGridSnake: Countdown: 2
LogGridSnake: Countdown: 1
LogGridSnake: Phase: EGSLevelPhase::Playing
LogGridSnake: Snake starts moving EGSDirection::Right, one step every 0.15s
LogGridSnake: Gem 1/3
LogGridSnake: Gem 2/3
LogGridSnake: Gem 3/3
LogGridSnake: All gems collected - the exit opens
LogGridSnake: <BP_GSExit_C_0> is now open
LogGridSnake: Phase: EGSLevelPhase::Completed
LogGridSnake: Level complete with 300 points
LogGridSnake: Restarting L_Level01
```

- **The startup lines** are chapter 10's; their order among the `BeginPlay`s may differ in your
  level (chapter 7 explained why). Cell, direction and length are the course's defaults.
- **`Phase: ...Playing` comes before the snake moves**, because `TickCountdown` sets the phase
  and then calls `StartMoving`. There is no `Countdown` phase line: that is the starting value.
- **`Phase: ...Completed` comes before `Level complete`**, because `OnExitReached` sets the phase
  first. 300 points is three gems at the default **Points Per Gem** of 100.
- **After `Restarting`**, the level loads again and the log starts over from `GameState ready`.

If the snake dies instead, the end of the log looks like this:

```text
LogGridSnake: Warning: Snake hit a wall at cell X=<x> Y=<y> (length <n>)
LogGridSnake: Phase: EGSLevelPhase::Failed
LogGridSnake: Restarting L_Level01
```

With `log LogGridSnake Verbose` entered, every gem and the exit add a line before their effects,
for example `LogGridSnake: Verbose: Snake entered <BP_GSGem_C_1> at cell X=<x> Y=<y>`.

### When it goes wrong

| Symptom | Cause | Fix |
|---|---|---|
| Gems are not collected, and the Verbose line is missing | The overlap does not happen | Chapter 10's collision settings: the gem's trigger must overlap `Pawn`, the snake's box must generate overlap events |
| The Verbose line appears, but nothing else | The gem's class does not implement the interface, or the game mode is not `AGSGameMode` | Check the base classes in `GSGem.h`; check World Settings > GameMode Override |
| `Gem 3/3` never comes, the exit never opens | Fewer gems in the level than **Gems Per Level** | Place three gems, or change the setting |
| The snake dies at the open exit | The exit is still solid on the `Grid` channel | `SetOpen` must call `UpdateCollision` (chapter 10) |
| The level never restarts after death | `OnDied` was not bound | Look for `Unable to bind delegate` in the log: `HandleSnakeDied` must be a `UFUNCTION` |
| `Restarting UEDPIE_0_L_Level01` and an error that the map is not found | The PIE prefix was not removed | Pass `true` as the second argument of `GetCurrentLevelName` |

## Summary

- Keep classes from knowing too much: **actors report, the game mode decides, the game state
  records, anyone interested listens**.
- An Unreal interface is a `U` class for reflection (`UINTERFACE`) and an `I` class to inherit.
  With `BlueprintNativeEvent` functions, C++ overrides `Func_Implementation` and Blueprints
  implement an event.
- Test with `Implements<UMyInterface>()` and call with `IMyInterface::Execute_Func(Object, ...)`:
  both work for Blueprint-only implementers. `Cast<IMyInterface>` does not.
- Delegates come as single or multicast, native or dynamic. Dynamic multicast delegates are
  Blueprint event dispatchers; they bind by name, so their targets must be `UFUNCTION`s.
- `UObject` bindings are weak and safe; lambdas need `AddWeakLambda` or explicit removal. Never
  rely on the order of a multicast's listeners.
- `AGSGameMode` owns the rules (gem value, growth, win, loss, restart) and checks the phase before
  applying any of them. `AGSGameState` owns the data and broadcasts every change from its setters.
- `EGSLevelPhase` turns the level into a small state machine. `ensure` reports an impossible state
  without crashing; `check` stops the program.
- `OpenLevel` with the current level's name (PIE prefix removed) is a complete restart.

## Exercises

1. **A slow-down potion.** Add an actor that, when the snake enters it, halves the snake's speed
   for five seconds and then disappears. Do not change `AGSSnake`.

<details><summary>Solution</summary>

The potion is a new interactable, built like the gem. `Source/GridSnake/Gameplay/GSSlowPotion.h`
declares `class AGSSlowPotion : public AActor, public IGSSnakeInteractable` with a constructor, the
`OnSnakeEntered_Implementation` override, a `TObjectPtr<UBoxComponent> Trigger`, and two
`EditAnywhere` floats, `SpeedFactor = 0.5f` and `Duration = 5.0f`. The constructor builds
`Trigger` exactly like `AGSGem`'s (a 6 × 16 × 6 box that overlaps only `Pawn`) and makes it the
root. The interesting part is in `GSSlowPotion.cpp`, which includes `Snake/GSSnake.h` and
`TimerManager.h`:

```cpp
void AGSSlowPotion::OnSnakeEntered_Implementation(AGSSnake* Snake)
{
	// Time dilation scales the DeltaSeconds the snake's Tick receives, so its
	// step accumulator fills more slowly. No snake code changes.
	Snake->CustomTimeDilation = SpeedFactor;

	// The potion is about to be destroyed, so the timer is bound to the snake.
	FTimerHandle Unused;
	GetWorldTimerManager().SetTimer(Unused, FTimerDelegate::CreateWeakLambda(Snake, [Snake]()
	{
		Snake->CustomTimeDilation = 1.0f;
	}), Duration, /*bLoop=*/false);

	Destroy();
}
```

`CustomTimeDilation` is a public `AActor` property that scales time for one actor's ticks, so
the snake's step accumulator fills at half speed. Timers belong to the world, so the timer
survives the potion's `Destroy`, and `CreateWeakLambda` skips the lambda if the snake is gone. Give
a Blueprint child a sprite, place it on a free cell and play.

</details>

2. **A Blueprint-only interactable.** Make `BP_GSSign`, an actor that prints "Hello, snake!"
   when the snake enters it, without writing C++.

<details><summary>Solution</summary>

1. **Add > Blueprint Class > Actor**, name it `BP_GSSign`, open it.
2. **Add > Box Collision**. Drag it onto **DefaultSceneRoot** to make it the root. Set **Box
   Extent** = (6, 16, 6), like the gem's trigger. Its default **Collision Presets** value,
   `OverlapAllDynamic`, overlaps the snake's `Pawn` body, and **Generate Overlap Events** is on.
3. **Class Settings > Interfaces > Implemented Interfaces > Add > GSSnakeInteractable**. Compile.
4. In **My Blueprint > Interfaces**, double-click **On Snake Entered**. Connect a **Print
   String** with `Hello, snake!`. Compile and save.
5. Place it on a free floor cell, centred on the cell like the gems, and play.

Expected Output Log (filter: `LogBlueprintUserMessages`):

```text
LogBlueprintUserMessages: [<BP_GSSign_C_0>] Hello, snake!
```

The snake reached a class it has never heard of, through `Implements<>` and `Execute_`.

</details>

3. **Print the score with a lambda.** Without a HUD, log `Score: <n>` whenever the score changes,
   using `AddLambda`.

<details><summary>Solution</summary>

`OnScoreChanged` is dynamic, and dynamic delegates accept only `UFUNCTION`s (`AddDynamic`), not
lambdas. Add a native multicast delegate next to it. In `GSGameState.h`:

```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(FGSScoreChangedNative, int32);
// ... in the class, public:
	/** C++-only twin of OnScoreChanged; accepts lambdas. */
	FGSScoreChangedNative OnScoreChangedNative;
```

In `AGSGameState::AddScore`, broadcast both: add `OnScoreChangedNative.Broadcast(Score);`. Then in
`AGSGameMode::StartPlay`:

```cpp
	GetGSGameState()->OnScoreChangedNative.AddWeakLambda(this, [](int32 Score)
	{
		UE_LOG(LogGridSnake, Log, TEXT("Score: %d"), Score);
	});
```

Each gem now logs `Score: 100`, `200`, `300` right after its `Gem n/3` line. Without a new
delegate, bind a `UFUNCTION() void HandleScoreChanged(int32 Score)` with `AddDynamic` instead.

</details>

4. **The missing `UFUNCTION`.** Remove `UFUNCTION()` above `HandleSnakeDied` in `GSGameMode.h`,
   build and crash the snake into a wall. What happens, and why?

<details><summary>Solution</summary>

It compiles, because `AddDynamic` takes an ordinary member function pointer. At run time,
`StartPlay` logs an ensure: `Unable to bind delegate to 'HandleSnakeDied' (function might not be
marked as a UFUNCTION or object may be pending kill)`. `AddDynamic` is a macro that also passes the
function's *name*. Dynamic delegates store that name and call the function through reflection, and
a function without `UFUNCTION` has no reflection data, so there is nothing to bind. When the snake
dies, `OnDied` broadcasts to no one: the log shows the `hit a wall` warning but no `Phase:
EGSLevelPhase::Failed` and no `Restarting`, and the dead snake lies there forever. Put the
`UFUNCTION()` back.

</details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-11--interfaces-delegates-and-game-rules)
