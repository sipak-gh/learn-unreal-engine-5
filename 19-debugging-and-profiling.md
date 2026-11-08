# Chapter 19 — Debugging and Profiling

GridSnake is feature complete: it has levels, menus, sound and saved progress. Before shipping it,
this chapter covers the tools that find bugs and slow code. You will attach the IDE debugger to
the editor, and use structured logging, asserts and console variables. You will also use debug
drawing, cheat commands and the Visual Logger, then measure code with stat commands and Unreal
Insights. GridSnake gains a `GS.Debug.Grid` overlay that colours free and blocked cells, and a
cheat manager with grow, collect-all, win and unlock-all commands. It also gains a stat group for
the snake's step, an Insights trace scope, and Visual Logger entries for each step and each death.

Previous: [Chapter 18](18-saving-progress.md). Next: [Chapter 20](20-ship-it-and-make-your-own.md).

## Debugging C++ with the IDE

Logs tell you what happened. A debugger shows what is happening *now*: you can stop the program on
a line, look at every variable, and step one line at a time. Unreal's editor is an ordinary
program (`UnrealEditor.exe`, or `UnrealEditor` on Linux and macOS), and your game module is a
library it loads. Debugging your game therefore means debugging the editor.

### Pick a build configuration

Chapter 2 introduced the configurations. Two of them matter for debugging:

| Configuration | Engine code | Your module | Use it for |
|---|---|---|---|
| **Development Editor** | optimized | optimized | everyday work, the default |
| **DebugGame Editor** | optimized | **not optimized** | stepping through your own code |

In an optimized build the compiler reorders and removes code. A breakpoint may land on the wrong
line, and the debugger may show local variables as "optimized away". DebugGame Editor builds only
*your* module without optimization, so your code steps cleanly while the engine stays fast.

In Visual Studio, pick **DebugGame Editor** and **Win64** in the toolbar's configuration and
platform boxes. In Rider, the same names are in the run configuration selector. On Linux, build it
with `Build.sh GridSnakeEditor Linux DebugGame -project=<path>/GridSnake.uproject`.

If one function is all you need, you can turn optimization off for just that code and stay on
Development Editor:

```cpp
UE_DISABLE_OPTIMIZATION
void AGSSnake::Step()
{
	// ...
}
UE_ENABLE_OPTIMIZATION
```

Never commit this. It is a temporary aid, and a forgotten pair slows the shipped game.

### Start or attach

There are two ways to get the debugger and the editor together:

1. **Start the editor from the IDE.** Close the editor first. In Visual Studio, right-click the
   `GridSnake` project in Solution Explorer, choose **Set as Startup Project**, then press **F5**.
   The IDE builds the module and starts the editor with the debugger attached. In Rider, choose
   the `GridSnake` run configuration and click **Debug**.
2. **Attach to a running editor.** In Visual Studio use **Debug > Attach to Process...**
   (Ctrl+Alt+P), type `UnrealEditor` in the filter, select the process and click **Attach**. In
   Rider use **Run > Attach to Process...**. Handy when a bug shows up unexpectedly.

With DebugGame Editor, prefer starting from the IDE, so the editor loads the matching build.

### Breakpoints and the watch window

Open `Snake/GSSnake.cpp`, find `AGSSnake::Step()` and click in the left margin of the line
`const FIntPoint NextCell = ...` (or press **F9**). A red dot appears. Press **Play** in the
editor. Execution stops on that line at the first step, and the editor freezes. This is expected:
the game thread is paused.

Now look around:

- **Locals** (Visual Studio: **Debug > Windows > Locals**) lists the variables in scope. Expand
  `this` to see the snake's members: `CurrentDirection`, `DirectionQueue`, `Segments`,
  `PendingGrowth`.
- **Watch** lets you type an expression, for example `Segments[0]` or
  `Board->GetGridSize()`.
- **Call Stack** shows how you got here: `AGSSnake::Tick` called `Step`, called by the engine's
  tick functions, and so on down to `main`.
- **F10** steps over a line, **F11** steps into a call, **Shift+F11** steps out, and **F5**
  continues until the next breakpoint.

Right-click a breakpoint to make it **conditional**, for example `Segments.Num() > 5`, so it only
stops when the snake is longer than five.

Engine types such as `FString`, `TArray` and `FName` are readable in the debugger thanks to a
*natvis* file that tells Visual Studio how to show them. The engine ships one
(`Engine/Extras/VisualStudioDebugging/Unreal.natvis`; check the path in your engine folder), and
the engine's setup normally installs it. If `FString` values show as raw pointers, copy that file
into your Visual Studio user folder's `Visualizers` directory. Rider has its own support built in.

### Live Coding while debugging

Live Coding (Ctrl+Alt+F11, chapter 2) patches the running editor with a changed function. It works
with a debugger attached, but it has some sharp edges:

- While the editor is open with Live Coding enabled, building from the IDE fails with a message
  that you cannot build while Live Coding is active. Use Ctrl+Alt+F11 in the editor, or close the
  editor and build from the IDE.
- A patched function is new code at a new address. Breakpoints set before the patch may not bind
  to it. Set them again after patching.
- Header changes, new `UPROPERTY`s and Build.cs changes still need a restart. To be safe, stop
  debugging, close the editor, build, and press F5 again.

When in doubt: close the editor, build from the IDE, start with F5.

## Logging, revisited

You have used `UE_LOG` since chapter 2. Three more tools make logs more useful: changing
verbosity at run time, structured log lines, and on-screen messages.

### Verbosity at run time

`DECLARE_LOG_CATEGORY_EXTERN(LogGridSnake, Log, All)` in `GridSnake.h` has two verbosities. The
first (`Log`) is the default at run time: `Verbose` and `VeryVerbose` lines are hidden. The second
(`All`) is the most detailed level compiled into the program. Lines above it do not even exist in
the binary. With `All`, everything is compiled in, and you can raise the run-time level whenever
you need it.

GridSnake already has `Verbose` lines, for example the gem's `spawned at` line and the snake's
`Snake entered ...` line. To see them:

| Where | What to type or write |
|---|---|
| Console, while playing | `Log LogGridSnake Verbose` (and `Log LogGridSnake Log` to go back) |
| Command line | `-LogCmds="LogGridSnake Verbose"` |
| `Config/DefaultEngine.ini` | a `[Core.Log]` section with the line `LogGridSnake=Verbose` |

The console form is the one you will use most. The ini form is for a verbosity you want every time
the project starts.

Two command-line switches help outside the editor. `-log` opens a separate log window for a
Standalone or packaged game. `-LogCmds` works there too. In the editor, the Output Log's
**Filters** menu hides categories and verbosities from view, but it only hides lines. It cannot
show lines that the category's verbosity suppressed.

### Structured logging with UE_LOGFMT

`UE_LOG` takes a `printf` format: `%s`, `%d`, `*SomeString`. Get a specifier wrong and you get
garbage or a crash. `UE_LOGFMT` (in `Logging/StructuredLog.h`, in the `Core` module, so no Build.cs
change) uses named placeholders instead:

```cpp
#include "Logging/StructuredLog.h"

UE_LOGFMT(LogGridSnake, Warning, "Snake {Reason} at cell ({X}, {Y}), length {Length}",
	FString(Reason), Cell.X, Cell.Y, Segments.Num());
```

The differences from `UE_LOG`:

- The format is a plain string literal, **not** wrapped in `TEXT()`.
- `{Reason}`, `{X}`, `{Y}` and `{Length}` are filled in order by the values that follow. Their
  types are known at compile time, so there is no `%d` versus `%s` to get wrong, and no `*` to
  forget.
- The names are kept with the values as *fields*. The text in the Output Log is the same as
  before, but tools that read structured logs can search for every warning whose `Length` was
  over 20, for example.
- You can name each value explicitly instead of relying on the order:
  `UE_LOGFMT(LogGridSnake, Log, "Gem {Index}", ("Index", Index));`.

The values must be types the structured logger knows how to write: numbers, `bool`, `FString`,
`FName`, `FText` and some others. GridSnake's `Reason` is a `const TCHAR*`, and the snapshot wraps
it in `FString(...)` to be on the safe side. If a type is not supported, the compiler tells you.
Convert the value to `FString` or to a number and pass that.

### On-screen messages

Sometimes you want a value on screen, not in a log you have to scroll through:

```cpp
#include "Engine/Engine.h"

if (GEngine)
{
	GEngine->AddOnScreenDebugMessage(/*Key=*/1, /*TimeToDisplay=*/0.0f, FColor::Cyan,
		FString::Printf(TEXT("Queue: %d"), DirectionQueue.Num()));
}
```

The key decides whether messages stack. With `-1`, every call adds a new line. With any other
number, the message replaces the previous one with the same key, which is what you want for a
value printed every frame. A time of `0` shows the message for one frame. On-screen messages are
for development only; they are not shown in Shipping builds. Use them for live values. Use the log
for events you may want to read later.

## Asserts

An assert states something that must be true. If it is false, the program has a bug, and the
sooner it stops, the closer you are to the cause. Unreal has three families, and the difference
between them is what happens in which build:

| Macro | If false | Expression in Shipping |
|---|---|---|
| `check(Expr)`, `checkf(Expr, Fmt, ...)` | logs, then stops the program (a crash with a callstack) | removed: **not evaluated** |
| `verify(Expr)`, `verifyf(...)` | like `check` | **still evaluated**, result ignored |
| `ensure(Expr)`, `ensureMsgf(Expr, Fmt, ...)` | logs an error with a callstack, **keeps running** | evaluated, no report |
| `ensureAlways(Expr)`, `ensureAlwaysMsgf(...)` | like `ensure`, but reports every time | evaluated, no report |

`check` is for *invariants*: things that can only be false if the code is broken. Because the
expression disappears in Shipping, never put work in it that must happen. `check(Save())` would
never save in Shipping. That is what `verify` is for: the call always happens, and only the test is
dropped.

`ensure` is for problems you want to hear about, but that the program can survive. It also returns
the result, so you can recover:

```cpp
if (!ensureMsgf(LevelList != nullptr, TEXT("No level list set in Project Settings")))
{
	return; // recover: carry on without levels
}
```

A failing `ensure` reports only the first time per call site in a session, so it does not flood
the log. When a debugger is attached, both `check` and `ensure` stop in it, right on the failing
line.

GridSnake's new assert is at the top of `AGSSnake::Step()`:

```cpp
// check() stops the program in Debug and Development builds if it fails:
// for things that must never happen. Tick only steps a live snake on a board.
check(Board && Segments.Num() > 0);
```

`Tick` only calls `Step` while `bMoving` is true, and the snake only starts moving after it found
a board and placed its segments. If this ever fails, something upstream is broken, and the next
line (`Segments[0]`) would read out of bounds anyway. Better to stop here with a clear message. If
it fires, the Output Log and the crash reporter show something like:

```text
Assertion failed: Board && Segments.Num() > 0 [File:<path>/Snake/GSSnake.cpp] [Line: <n>]
```

A failing `ensure` logs a line containing `Ensure condition failed:` plus the expression, followed
by a callstack. The category and exact wording of these lines vary between versions; search the
log for `Assertion failed` or `Ensure condition failed`.

## Console variables and commands

A *console variable* (CVar) is a named, typed setting, such as `t.MaxFPS` from chapter 1. You can
set it from the console, from config files and from the command line. Your own CVars are the
cheapest way to add a debug switch: no UI, no Blueprint, no rebuild to toggle.

### A CVar in C++

`TAutoConsoleVariable` (in `HAL/IConsoleManager.h`) declares one and registers it when the module
loads. GridSnake's lives at the top of `Gameplay/GSBoard.cpp`:

```cpp
#include "HAL/IConsoleManager.h"

// A console variable: type "GS.Debug.Grid 1" in the console (backtick key) to turn it on.
// ECVF_Cheat hides it in Shipping builds.
static TAutoConsoleVariable<bool> CVarGSDebugGrid(
	TEXT("GS.Debug.Grid"),
	false,
	TEXT("Draw the board's cells: red = blocked for the snake, green = free."),
	ECVF_Cheat);
```

- The name uses dots for namespaces: `GS.` for the project, `Debug.` for debug switches. Prefix
  your own names, because they share one global list with every engine CVar.
- `static` keeps the C++ variable private to this file. The console finds it by its name string.
- The help text is shown when you type `GS.Debug.Grid ?` in the console.
- `ECVF_Cheat` marks it as a cheat: it is hidden and cannot be changed in Shipping builds.
  `ECVF_Default` means no special flags.
- The value types you can use are `bool`, `int32`, `float` and `FString`.

Reading the value is one call:

```cpp
if (CVarGSDebugGrid.GetValueOnGameThread())
{
	DrawDebugGrid();
}
```

`GetValueOnGameThread` returns a copy kept for the game thread, so reading it every frame is cheap.
Rendering code would use `GetValueOnRenderThread` instead. That is why the two exist.

Ways to set it:

- In the console: `GS.Debug.Grid 1`. Type `GS.Debug.Grid` alone to print the current value.
- On the command line: `-ExecCmds="GS.Debug.Grid 1"` runs console commands at startup.
- In `Config/DefaultEngine.ini`, under `[SystemSettings]`, for a value you want every time (the
  same place `t.MaxFPS` went in chapter 1).

### Console commands

A *command* runs code instead of storing a value. `FAutoConsoleCommand` registers one together
with a delegate, the function to run. You do not need an actor or a cheat manager. The example
below has one of each kind, plus a CVar read by an actor.

`examples/ch19/ExConsoleTools.h`

```cpp
// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExConsoleTools.generated.h"

/**
 * Reads the console variable Ex.DebugLevel every frame and reacts to it.
 * The .cpp also registers the console commands Ex.Hello and Ex.Greet, which need no actor.
 *
 * Try it: drag AExConsoleTools into any level, press Play, open the console (`) and type
 *   Ex.DebugLevel 1
 *   Ex.DebugLevel 2
 *   Ex.Hello
 *   Ex.Greet Ada Lovelace
 */
UCLASS()
class AExConsoleTools : public AActor
{
	GENERATED_BODY()

public:
	AExConsoleTools();

	virtual void Tick(float DeltaSeconds) override;

private:
	/** The value seen last frame. -1 so the first frame always reports. */
	int32 LastDebugLevel = -1;
};
```

`examples/ch19/ExConsoleTools.cpp`

```cpp
// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExConsoleTools.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

// A console variable: a named, typed value that the console, config files and the
// command line can set. "static" keeps the object private to this file; it registers
// itself with the console manager when the module loads.
static TAutoConsoleVariable<int32> CVarExDebugLevel(
	TEXT("Ex.DebugLevel"),
	0,
	TEXT("Debug detail for AExConsoleTools: 2 or more shows its position on screen. Changes are logged."),
	ECVF_Cheat);

// A console command without arguments. The lambda runs when you type the name.
static FAutoConsoleCommand CmdExHello(
	TEXT("Ex.Hello"),
	TEXT("Prints a greeting to the Output Log."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UE_LOG(LogTemp, Display, TEXT("Ex.Hello: hello from a console command"));
	}),
	ECVF_Default);

// A console command with arguments: the console splits the rest of the line at spaces.
static FAutoConsoleCommand CmdExGreet(
	TEXT("Ex.Greet"),
	TEXT("Ex.Greet <name...>: greets someone by name."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		const FString Name = Args.Num() > 0 ? FString::Join(Args, TEXT(" ")) : TEXT("nobody");
		UE_LOG(LogTemp, Display, TEXT("Ex.Greet: hello, %s (%d argument(s))"), *Name, Args.Num());
	}),
	ECVF_Default);

AExConsoleTools::AExConsoleTools()
{
	PrimaryActorTick.bCanEverTick = true;

	// A root component gives the actor a location in the level.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AExConsoleTools::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Reading a CVar on the game thread is cheap: no lookup by name, just a stored value.
	const int32 DebugLevel = CVarExDebugLevel.GetValueOnGameThread();
	if (DebugLevel != LastDebugLevel)
	{
		UE_LOG(LogTemp, Display, TEXT("%s: Ex.DebugLevel changed from %d to %d"), *GetName(), LastDebugLevel, DebugLevel);
		LastDebugLevel = DebugLevel;
	}

	if (DebugLevel >= 2 && GEngine)
	{
		// Key 1: each frame replaces the previous message instead of stacking a new line.
		// 0 seconds: shown for one frame, so it disappears as soon as we stop sending it.
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("%s at %s"), *GetName(), *GetActorLocation().ToString()));
	}
}
```

Modules: `Core`, `CoreUObject`, `Engine` (already in every project's Build.cs).

The two delegate types decide what the command receives. `FConsoleCommandDelegate` gets nothing.
`FConsoleCommandWithArgsDelegate` gets the words after the name, already split at spaces.
`IConsoleManager.h` also has `FAutoConsoleCommandWithWorldAndArgs` for commands that need the
current world, for example to spawn something. The flags work the same as for CVars.

To see it: build, drag `ExConsoleTools` from the C++ Classes folder of the Content Browser into any
level, press **Play**, open the console with the backtick key and type `Ex.DebugLevel 1`, then
`Ex.DebugLevel 2`, then `Ex.Hello` and `Ex.Greet Ada Lovelace`. At level 2 a cyan line with the
actor's position appears at the top left of the viewport.

Expected Output Log (filter: `LogTemp`):

```text
LogTemp: Display: ExConsoleTools_0: Ex.DebugLevel changed from -1 to 0
LogTemp: Display: ExConsoleTools_0: Ex.DebugLevel changed from 0 to 1
LogTemp: Display: ExConsoleTools_0: Ex.DebugLevel changed from 1 to 2
LogTemp: Display: Ex.Hello: hello from a console command
LogTemp: Display: Ex.Greet: hello, Ada Lovelace (2 argument(s))
```

The actor's name (`ExConsoleTools_0` here) depends on how many you placed, and the editor may add
`UAID_...` characters to it. A CVar keeps its value until the editor closes, so the next Play
starts with the level you last set: the first line then says `from -1 to 2`.

### CVar or command, cheat or not

Use a **CVar** for a mode that stays on (draw the grid, log more), a **command** for a one-off
action (spawn, reset, print a report), and a **cheat manager** function (below) for actions on the
player's game, because it reaches the controller and pawn without searching. Mark every switch
that changes gameplay with `ECVF_Cheat`, so that players cannot use it in Shipping.

## Debug drawing

Debug drawing puts lines, boxes, spheres and text into the world for a frame or for a while. The
functions are in `DrawDebugHelpers.h` (Engine module):

| Function | Draws |
|---|---|
| `DrawDebugLine(World, Start, End, Color, bPersistent, LifeTime, Depth, Thickness)` | a line |
| `DrawDebugBox(World, Center, Extent, Color, ...)` | a box; `Extent` is half the size |
| `DrawDebugSphere(World, Center, Radius, Segments, Color, ...)` | a wire sphere |
| `DrawDebugPoint(World, Position, Size, Color, ...)` | a square dot, always facing you |
| `DrawDebugString(World, Location, Text, BaseActor, Color, Duration, bDrawShadow)` | text |
| `FlushPersistentDebugLines(World)` | removes everything persistent |

The lifetime arguments decide how long a shape stays:

- `bPersistentLines = false` with `LifeTime = -1`: one frame only. Draw it again every `Tick`.
  This is how you draw something that moves.
- `LifeTime = 5.0f`: stays for five seconds, then disappears.
- `bPersistentLines = true`: stays until `FlushPersistentDebugLines` or the end of play.

Text works differently. `DrawDebugString` is drawn by the player's HUD, so it needs a player
controller, and its last argument is a duration in seconds. When `BaseActor` is given, the location
is relative to that actor and the text follows it. Use a duration of `0` when you call it every
frame. A longer duration adds a new string each call, and they pile up.

All of these are development tools. In Shipping builds `ENABLE_DRAW_DEBUG` is `0`: the functions
still exist, but they do nothing. Wrap your drawing code in `#if ENABLE_DRAW_DEBUG` anyway. Then
the maths that only feeds the shapes is removed too, and nobody can mistake it for gameplay code.

`examples/ch19/ExDebugDraw.h`

```cpp
// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExDebugDraw.generated.h"

/**
 * Draws a box, a rotating line, a sphere and a label around itself every frame.
 * Debug shapes are for development only: they are compiled out of Shipping builds.
 *
 * Try it: drag AExDebugDraw into any level and press Play. In a Paper2D level, look at
 * it through the side-on (X-Z) view; the shapes are drawn in that plane.
 */
UCLASS()
class AExDebugDraw : public AActor
{
	GENERATED_BODY()

public:
	AExDebugDraw();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Half the size of the box, in world units. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	float BoxHalfSize = 32.0f;

	/** Length of the rotating line. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	float ArmLength = 48.0f;

	/** Turns per second of the line. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	float TurnsPerSecond = 0.5f;

private:
	float RunningTime = 0.0f;
};
```

`examples/ch19/ExDebugDraw.cpp`

```cpp
// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExDebugDraw.h"
#include "DrawDebugHelpers.h"

AExDebugDraw::AExDebugDraw()
{
	PrimaryActorTick.bCanEverTick = true;

	// An actor needs a root component to have a location.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AExDebugDraw::BeginPlay()
{
	Super::BeginPlay();

#if ENABLE_DRAW_DEBUG
	UE_LOG(LogTemp, Display, TEXT("%s: debug drawing is compiled in"), *GetName());

	// Persistent lines stay until FlushPersistentDebugLines (or the end of play).
	// Here: a cross marking where the actor started.
	const FVector Start = GetActorLocation();
	DrawDebugLine(GetWorld(), Start - FVector(8.0f, 0.0f, 0.0f), Start + FVector(8.0f, 0.0f, 0.0f), FColor::Magenta, /*bPersistentLines=*/true);
	DrawDebugLine(GetWorld(), Start - FVector(0.0f, 0.0f, 8.0f), Start + FVector(0.0f, 0.0f, 8.0f), FColor::Magenta, /*bPersistentLines=*/true);
#else
	UE_LOG(LogTemp, Display, TEXT("%s: debug drawing is compiled out (Shipping)"), *GetName());
#endif
}

void AExDebugDraw::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RunningTime += DeltaSeconds;

	// Everything inside #if is removed from builds without debug drawing, including
	// the maths that only exists to feed the shapes.
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	const FVector Center = GetActorLocation();

	// bPersistentLines false + LifeTime -1: drawn for this frame only, so we redraw
	// every Tick. That is how you draw something that moves.
	DrawDebugBox(World, Center, FVector(BoxHalfSize, 1.0f, BoxHalfSize), FColor::Green,
		/*bPersistentLines=*/false, /*LifeTime=*/-1.0f, /*DepthPriority=*/0, /*Thickness=*/1.0f);

	// A line turning in the X-Z plane (the plane Paper2D sprites live in).
	const float Angle = RunningTime * TurnsPerSecond * UE_TWO_PI;
	const FVector Tip = Center + FVector(FMath::Cos(Angle), 0.0f, FMath::Sin(Angle)) * ArmLength;
	DrawDebugLine(World, Center, Tip, FColor::Yellow, false, -1.0f, 0, 2.0f);
	DrawDebugSphere(World, Tip, 6.0f, /*Segments=*/12, FColor::Red, false, -1.0f, 0, 1.0f);

	// Text goes through the player's HUD. With an actor given, the location is relative
	// to it, so the label follows the actor. Duration 0: this frame only.
	DrawDebugString(World, FVector(0.0f, 0.0f, BoxHalfSize + 12.0f),
		FString::Printf(TEXT("%s  t=%.1fs"), *GetName(), RunningTime),
		this, FColor::White, /*Duration=*/0.0f, /*bDrawShadow=*/true);
#endif
}
```

Modules: `Core`, `CoreUObject`, `Engine`.

To see it: build, drag `ExDebugDraw` into a level and press **Play**. In GridSnake's levels the
board's camera looks along -Y, so the X-Z shapes face you: a green square, a yellow line turning
anticlockwise with a red sphere on its tip, a magenta cross where the actor started, and a white
label above the square. In a 3D level, look at it from the side. Shapes drawn with a Y thickness of
1 are hard to see from above.

Expected Output Log (filter: `LogTemp`):

```text
LogTemp: Display: ExDebugDraw_0: debug drawing is compiled in
```

In a Shipping build of the same project, the line would say `compiled out (Shipping)`. Except that
Shipping builds do not write logs by default either (chapter 20).

## The cheat manager

Some debug actions are about *the player's game*: grow the snake, win the level. A
`UCheatManager` is made for that. It is an object the player controller creates, and its
`UFUNCTION(Exec)` functions become console commands. Two properties make it the right home for
cheats:

- It is **not created in Shipping builds**. The engine's `UE_WITH_CHEAT_MANAGER` is off there, so
  the commands do not exist. Cheats cannot ship by accident.
- It knows its player controller (`GetPlayerController()`), so a cheat can reach the pawn, the
  world and the game mode without searching.

When does a non-Shipping game have one? In Play In Editor, always. In a Standalone or Development
build of a single-player game, the game mode allows cheats by default, and the controller creates
the cheat manager when it starts. For other cases there is the `EnableCheats` console command, and
the game mode's `AllowCheats` function decides. This can vary with the net mode. If your commands
are "unknown" in a Development build, type `EnableCheats` first.

Chapter 9 put an `Exec` function `GSGrow` on the snake itself. That works, because the console also
looks for Exec functions on the possessed pawn, but it ships with the game. It moves to the cheat
manager now.

## The Visual Logger

Logs are text in time order. Debug drawing is shapes in the world, but only *now*. The **Visual
Logger** records both, per actor, so that you can play, stop, and then scrub back through time to
see where things were and what they logged at each moment. It is ideal for "why did it die
there?"

Two macros (in `VisualLogger/VisualLogger.h`, Engine module) record entries:

```cpp
#include "VisualLogger/VisualLogger.h"

// A text entry for this actor at this moment.
UE_VLOG(this, LogGridSnake, Warning, TEXT("Died: %s"), Reason);

// A text entry plus a sphere at a location (radius 4, yellow).
UE_VLOG_LOCATION(this, LogGridSnake, Verbose, Board->CellToWorld(NextCell), 4.0f, FColor::Yellow,
	TEXT("Step %s -> %s"), *UEnum::GetValueAsString(CurrentDirection), *NextCell.ToString());
```

The first argument is the *owner*: the actor whose timeline the entry goes into. Then a log
category, a verbosity and a `printf` format, just like `UE_LOG`. There are shape variants too
(`UE_VLOG_SEGMENT`, `UE_VLOG_BOX` and others). `UE_VLOG_UELOG` writes the same line to both the
Visual Logger and the normal log.

The macros check whether the Visual Logger is recording before they do any work. When it is not
recording, the cost is a single test. They are compiled out when `ENABLE_VISUAL_LOG` is off, which
is the case in Shipping builds.

To use the tool:

1. Open it with **Tools > Debug > Visual Logger**. Older documentation shows **Window > Developer
   Tools > Visual Logger**. The menu path has moved between versions, so look in both places. The
   console command `VisLog` also opens it in many versions.
2. Click **Start** (the record button) on its toolbar.
3. Play, let the snake die, and stop playing. Then click **Stop** in the Visual Logger.
4. The left side lists actors that logged something (`BP_GSSnake_C_0` or similar). The timeline
   shows a mark for each entry. Click a mark or drag the time cursor: the log text for that moment
   appears in the panel below, and the shapes appear in the level viewport.
5. Filter with the category list (show only `LogGridSnake`) or the search box.

Recordings can be saved and loaded from the Visual Logger's toolbar. A tester can send you one
together with a bug report.

If the step entries do not appear, check the Visual Logger's own verbosity filter. Also raise the
category with `Log LogGridSnake Verbose` before recording, because the step entries are `Verbose`.

## Stats and Unreal Insights

"It feels slow" is not a measurement. Unreal has two built-in ways to measure: **stats**, live
numbers on screen, and **Unreal Insights**, a recorded timeline of everything that ran.

### Stat commands you already have

Chapter 1 used `stat fps` and `stat unit`. More of them:

| Command | Shows |
|---|---|
| `stat unit` | Frame, Game, Draw, GPU times in milliseconds |
| `stat unitgraph` | the same as a scrolling graph |
| `stat game` | time spent in game-thread work: ticks, world, components |
| `stat fps` | frames per second |
| `stat none` | hides every stat |

Read `stat unit` first. If **Game** is the largest, the game thread (your C++ and Blueprints) is
the bottleneck. If **Draw** or **GPU** is, it is rendering. For a 2D game, both should be tiny.

### Your own stat group

A stat group is one page of `stat <Name>`. GridSnake declares one in `GridSnake.h`, so every file
can add counters to it:

```cpp
#include "Stats/Stats.h"

/** Stat group for "stat GridSnake" in the console. Cycle stats are declared next to the code they time. */
DECLARE_STATS_GROUP(TEXT("GridSnake"), STATGROUP_GridSnake, STATCAT_Advanced);
```

Then a *cycle stat* (a timer) is declared where it is used, in `GSSnake.cpp`, and a scope in the
function starts and stops it:

```cpp
// Shows up under "stat GridSnake" (call count and time per frame).
DECLARE_CYCLE_STAT(TEXT("Snake Step"), STAT_GSSnakeStep, STATGROUP_GridSnake);

void AGSSnake::Step()
{
	SCOPE_CYCLE_COUNTER(STAT_GSSnakeStep);
	// ...
}
```

`SCOPE_CYCLE_COUNTER` creates an object whose constructor starts the timer and whose destructor
stops it. Every `return` in `Step` therefore stops it correctly.

Type `stat GridSnake` while playing. A table appears with a row **Snake Step**, its call count, and
inclusive and exclusive times. The snake steps a few times a second, not every frame, so the call
count flickers between 0 and 1 and the time is tiny. That is the honest answer: the snake's logic
costs next to nothing. Stats exist in Development builds and the editor, and are compiled out of
Shipping.

### Unreal Insights

Stats show averages now. **Unreal Insights** records a *trace*: every scope of every frame on every
thread, with exact start and end times. You open it afterwards and zoom in on the one slow frame.

To get your own code into the trace, add a CPU trace scope at the top of a function:
`TRACE_CPUPROFILER_EVENT_SCOPE(AGSSnake::Step);` (in `ProfilingDebugging/CpuProfilerTrace.h`, Core
module). The argument is a name, not a string; the function's own name is easy to find later. The
scope costs almost nothing unless the CPU channel is being traced.

Recording a trace:

1. In the editor's status bar (bottom right), open the **Trace** menu. Make sure the **CPU**
   channel (and **Frame**) is enabled, then choose **Start Trace**. The labels in this menu have
   changed between versions; look for the trace or "Unreal Insights" button in the status bar.
2. Play for a few seconds and steer the snake. Stop playing, then **Stop Trace** from the same
   menu.
3. Open Unreal Insights: **Tools > Unreal Insights** in the editor, or run the `UnrealInsights`
   program from the engine's `Engine/Binaries/<Platform>` folder. Its session browser lists the
   traces it knows. Open the newest one.

Alternatively, launch the game as **Standalone** with the command-line arguments
`-trace=cpu,frame,log`. In the Play options, use **Advanced Settings > Play in Standalone Game >
Additional Launch Parameters**, or run `UnrealEditor GridSnake.uproject -game -trace=cpu,frame,log`.
The trace goes to the trace server that Insights starts in the background. Start Insights first if
nothing shows up.

In the trace, **Timing Insights** opens with frames at the top, a timeline of threads in the
middle, and a **Timers** list. Type `AGSSnake::Step` in the timeline's or the Timers panel's search
box. Each step is a small block on the game thread, under the engine's tick scopes. Hover it to see
its exact duration. The Timers list gives the count and the total, average and maximum time for the
selected range.

`examples/ch19/ExStatsAndTrace.h`

```cpp
// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExStatsAndTrace.generated.h"

/**
 * Does some deliberately slow work every frame and measures it three ways:
 * a cycle stat ("stat Example"), a CPU trace scope (Unreal Insights) and, once at
 * BeginPlay, a structured log line with the time in milliseconds.
 *
 * Try it: drag AExStatsAndTrace into any level, press Play, open the console (`) and
 * type "stat Example". Raise ValuesPerFrame in the Details panel to make it slower.
 */
UCLASS()
class AExStatsAndTrace : public AActor
{
	GENERATED_BODY()

public:
	AExStatsAndTrace();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** How many values to sum each frame. */
	UPROPERTY(EditAnywhere, Category = "Profiling", meta = (ClampMin = "1"))
	int32 ValuesPerFrame = 1000000;

private:
	/** The slow function being measured. */
	uint64 SumValues(int32 Count);

	/** Kept so the compiler cannot throw the work away. */
	uint64 LastSum = 0;
};
```

`examples/ch19/ExStatsAndTrace.cpp`

```cpp
// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExStatsAndTrace.h"
#include "HAL/PlatformTime.h"
#include "Logging/StructuredLog.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Stats/Stats.h"

// A log category private to this file.
DEFINE_LOG_CATEGORY_STATIC(LogExStats, Log, All);

// A stat group is what "stat <Name>" shows: here "stat Example".
DECLARE_STATS_GROUP(TEXT("Example"), STATGROUP_Example, STATCAT_Advanced);

// One cycle counter in that group. The text is the row's label on screen.
DECLARE_CYCLE_STAT(TEXT("Sum Values"), STAT_ExSumValues, STATGROUP_Example);

AExStatsAndTrace::AExStatsAndTrace()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AExStatsAndTrace::BeginPlay()
{
	Super::BeginPlay();

	// FPlatformTime::Seconds() is a high-resolution clock: fine for a one-off timing.
	const double StartSeconds = FPlatformTime::Seconds();
	LastSum = SumValues(ValuesPerFrame);
	const double Ms = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;

	// Structured logging: the {Names} are filled in order by the values that follow.
	UE_LOGFMT(LogExStats, Log, "Summed {Count} values in {Ms} ms", ValuesPerFrame, Ms);
	UE_LOGFMT(LogExStats, Verbose, "Sum was {Sum}", LastSum);
}

void AExStatsAndTrace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	LastSum = SumValues(ValuesPerFrame);
}

uint64 AExStatsAndTrace::SumValues(int32 Count)
{
	// Both scopes end at the closing brace, so they time the whole function.
	// The cycle counter feeds "stat Example"; the trace scope is a named block in
	// Unreal Insights' timeline. Both cost almost nothing when nobody is looking,
	// and both are compiled out of Shipping builds by default.
	SCOPE_CYCLE_COUNTER(STAT_ExSumValues);
	TRACE_CPUPROFILER_EVENT_SCOPE(AExStatsAndTrace::SumValues);

	uint64 Sum = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// A cheap hash, so the compiler cannot replace the loop with a formula.
		Sum += (uint64(Index) * 2654435761ull) % 1000ull;
	}
	return Sum;
}
```

Modules: `Core`, `CoreUObject`, `Engine`.

`DEFINE_LOG_CATEGORY_STATIC` declares and defines a category in one line, visible only in this
`.cpp` file. It is handy for examples and small helpers. The stat group here is declared in the
`.cpp` because nothing else uses it. GridSnake declares its group in a header so all files can
share it.

To see it: build, drag `ExStatsAndTrace` into a level and press **Play**. Type `stat Example`: the
**Sum Values** row shows a call count of 1 and a time of a few milliseconds per frame. Then type
`stat unit` and watch **Game** go up by about the same amount. Set **Values Per Frame** to
`10000000` in the Details panel of the placed actor (in the level, before Play) and the frame rate
drops visibly. Record a trace as described above and search for `AExStatsAndTrace::SumValues`.

Expected Output Log (filter: `LogExStats`):

```text
LogExStats: Summed 1000000 values in <x> ms
```

`<x>` depends on your machine and build configuration: a few milliseconds in Development Editor,
noticeably more in DebugGame Editor, since your module is not optimized there. That difference is
worth seeing once, and it is why you never profile a DebugGame build. The `Sum was` line is
`Verbose`, so it only appears after `Log LogExStats Verbose`.

Profile Development builds (or the Test configuration), not the editor, when the numbers matter.
The editor adds its own work to every frame. Shipping builds have stats compiled out and, by
default, tracing off.

## A debugging checklist

Most Unreal bugs in a project like this are one of a few kinds. When something does not work, go
down the matching list before reaching for the debugger.

| Symptom | Check, in this order |
|---|---|
| None of my log lines show | Output Log search box and **Filters** menu; a `Verbose` line needs `Log LogGridSnake Verbose`; a `Warning` at the top of the function proves it runs; did the build or Live Coding patch succeed? |
| The actor does nothing | Is it in the Outliner while playing? Is it the Blueprint child (with its assets set) or the bare C++ class? `bCanEverTick = true`? A missing `Super::BeginPlay()`? Input: mapping context added (chapter 8), right `ETriggerEvent`, game paused or UI-only input mode (chapter 16)? |
| The widget is missing | Created *and* added to the viewport? Widget class set in the Blueprint defaults? A `BindWidgetOptional` name mismatch leaves the pointer null; check the names in the Designer. Hidden behind another widget or collapsed? |
| Overlaps do not fire | **Generate Overlap Events** on both? Responses set to *overlap* each other's object types (chapter 10)? `show collision` shows what physics sees. |
| The save does not load | Same slot name and user index on save and load? Does the file exist in `Saved/SaveGames/` (editor) or the per-user folder (packaged, chapter 20)? Every field a `UPROPERTY`? Did the class change, so `Cast` returns null? |
| It crashed | Read the first `Error` or `Fatal` line in `Saved/Logs/GridSnake.log`, then the callstack under it; the top lines in your module are where to look. `Assertion failed` names the expression, file and line. |

## Project: debug tools for GridSnake

The chapter 19 snapshot is `project/ch19/`. Compared with `project/ch18/`:

| File | Change |
|---|---|
| `Source/GridSnake/GridSnake.h` | `#include "Stats/Stats.h"` and the `STATGROUP_GridSnake` stat group |
| `Source/GridSnake/Core/GSCheatManager.h/.cpp` | **new**: the cheat manager |
| `Source/GridSnake/Core/GSPlayerController.cpp` | `CheatClass = UGSCheatManager::StaticClass();` |
| `Source/GridSnake/Gameplay/GSBoard.h/.cpp` | `GS.Debug.Grid` CVar; `Tick` and `DrawDebugGrid` |
| `Source/GridSnake/Snake/GSSnake.h/.cpp` | cycle stat, trace scope, `check`, `UE_LOGFMT`, `UE_VLOG`; the Exec `GSGrow` removed |

No Build.cs change: stats, tracing, structured logging and the Visual Logger are in `Core` and
`Engine`, and the cheat manager is in `Engine`.

### The cheat manager

`Source/GridSnake/Core/GSCheatManager.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GSCheatManager.generated.h"

/**
 * Console commands for testing. The player controller creates this object in
 * development builds only (Shipping builds have no cheat manager), so cheats
 * cannot ship by accident. Type the function name in the console: GSGrow 5
 */
UCLASS()
class UGSCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** Adds segments to the snake. */
	UFUNCTION(Exec)
	void GSGrow(int32 Amount = 5);

	/** Collects every gem in the level, which opens the exit. */
	UFUNCTION(Exec)
	void GSCollectAll();

	/** Finishes the level as if the snake had reached the open exit. */
	UFUNCTION(Exec)
	void GSWin();

	/** Unlocks every level in the level list and saves. */
	UFUNCTION(Exec)
	void GSUnlockAll();
};
```

`UFUNCTION(Exec)` is all it takes. The console parses the arguments by the parameter types, and a
default value makes the argument optional: `GSGrow` grows by 5, `GSGrow 10` by 10. Every name
starts with `GS` so that it is easy to find and cannot clash with the engine's own cheats (`God`,
`Fly`, `Ghost`, `Slomo` and others, which the base class provides).

`Source/GridSnake/Core/GSCheatManager.cpp`:

```cpp
#include "Core/GSCheatManager.h"
#include "GridSnake.h"
#include "Core/GSGameMode.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/GSGem.h"
#include "Save/GSSaveSubsystem.h"
#include "Snake/GSSnake.h"

namespace
{
	AGSSnake* GetSnake(UCheatManager& Cheats)
	{
		const APlayerController* PlayerController = Cheats.GetPlayerController();
		return PlayerController ? Cast<AGSSnake>(PlayerController->GetPawn()) : nullptr;
	}
}

void UGSCheatManager::GSGrow(int32 Amount)
{
	if (AGSSnake* Snake = GetSnake(*this))
	{
		Snake->Grow(Amount);
		UE_LOG(LogGridSnake, Log, TEXT("GSGrow: +%d segments"), Amount);
	}
}

void UGSCheatManager::GSCollectAll()
{
	AGSSnake* Snake = GetSnake(*this);
	if (!Snake)
	{
		return;
	}

	// Collect into an array first: collecting destroys the gem, and an actor
	// iterator should not run while the actors it visits are being destroyed.
	TArray<AGSGem*> Gems;
	for (TActorIterator<AGSGem> It(GetWorld()); It; ++It)
	{
		Gems.Add(*It);
	}

	for (AGSGem* Gem : Gems)
	{
		IGSSnakeInteractable::Execute_OnSnakeEntered(Gem, Snake);
	}
	UE_LOG(LogGridSnake, Log, TEXT("GSCollectAll: %d gem(s)"), Gems.Num());
}

void UGSCheatManager::GSWin()
{
	AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>();
	AGSSnake* Snake = GetSnake(*this);
	if (GameMode && Snake)
	{
		GameMode->OnExitReached(Snake);
	}
}

void UGSCheatManager::GSUnlockAll()
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	if (!LevelList || LevelList->Num() == 0)
	{
		return;
	}

	UGSSaveSubsystem* SaveSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGSSaveSubsystem>();
	SaveSubsystem->UnlockLevel(LevelList->Num() - 1);
	SaveSubsystem->Save();
	UE_LOG(LogGridSnake, Log, TEXT("GSUnlockAll: %d level(s) unlocked"), LevelList->Num());
}
```

Notes:

- Every cheat goes through the **same code path as the real game**. `GSCollectAll` calls the
  gem's interface function, exactly as the snake's overlap does (chapter 11), so scoring, the
  exit, music and sounds all react. `GSWin` calls `OnExitReached`. A cheat that set the score
  directly would test nothing.
- `TActorIterator<AGSGem>` (in `EngineUtils.h`) visits every gem in the world. Collecting a gem
  destroys it, so the loop first copies the gems into an array and then collects them. Changing a
  collection while iterating over it is a classic bug.
- The game mode ignores gems and the exit unless the phase is `Playing`. `GSWin` during the
  countdown does nothing. `GSCollectAll` during the countdown is worse: each gem still destroys
  itself, but no gem is counted, so the exit can never open. Wait for the countdown to end.
- `UnlockLevel(N)` unlocks everything up to level `N` (chapter 18), so unlocking the last level
  unlocks them all.

The player controller creates it. In `Core/GSPlayerController.cpp`:

```cpp
#include "Core/GSCheatManager.h"

AGSPlayerController::AGSPlayerController()
{
	// By default, possessing a pawn makes it the view target. The board's
	// camera should stay in charge, so we manage the view target ourselves.
	bAutoManageActiveCameraTarget = false;

	// Created automatically for a single-player game in non-Shipping builds;
	// its Exec functions become console commands.
	CheatClass = UGSCheatManager::StaticClass();
}
```

And the chapter 9 version is removed from `Snake/GSSnake.h/.cpp`: the `UFUNCTION(Exec) void
GSGrow(int32 Amount);` declaration and its definition are gone. `Grow` itself stays: the game mode
calls it for each gem.

### The debug grid

`Gameplay/GSBoard.h` gains two declarations:

```cpp
public:
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Draws every cell: red if blocked, green if free. Console: GS.Debug.Grid 1 */
	void DrawDebugGrid() const;
```

`Gameplay/GSBoard.cpp`: the includes and the CVar shown earlier in this chapter, then:

```cpp
AGSBoard::AGSBoard()
{
	// Ticks only to draw debug shapes when GS.Debug.Grid is on.
	PrimaryActorTick.bCanEverTick = true;
	// ... (components as before)
}

void AGSBoard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CVarGSDebugGrid.GetValueOnGameThread())
	{
		DrawDebugGrid();
	}
}

void AGSBoard::DrawDebugGrid() const
{
#if ENABLE_DRAW_DEBUG
	const FIntPoint Size = GetGridSize();
	const float Half = GetCellSize() * 0.45f;
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			const FColor Color = IsCellBlocked(Cell) ? FColor::Red : FColor::Green;
			// LifeTime -1 with bPersistentLines false: drawn for this frame only.
			// Offset towards the camera (+Y) so the lines are in front of the sprites.
			DrawDebugBox(GetWorld(), CellToWorld(Cell) + FVector(0.0f, 20.0f, 0.0f), FVector(Half, 1.0f, Half), Color,
				/*bPersistentLines=*/false, /*LifeTime=*/-1.0f, /*DepthPriority=*/0, /*Thickness=*/0.5f);
		}
	}
#endif
}
```

This is the most useful debug view in the project. It draws exactly what the snake believes:
`IsCellBlocked` is the same query `Step` uses (chapter 10). If a wall tile shows green, its
collision is wrong in the tile set. If an open cell shows red, something blocks the Grid channel
there. Each box is 90% of a cell (`0.45` half-size), so neighbouring boxes do not merge, and it
sits 20 units towards the camera so the sprites do not hide it.

A board ticking every frame for nothing would be wasteful, but here the cost is one CVar read. If
you prefer, you could switch the tick on only when the CVar changes. That is more code for
no visible gain.

### Measuring and recording the snake

`GridSnake.h` adds the include and the stat group shown in [Your own stat
group](#your-own-stat-group). `Snake/GSSnake.cpp` adds the includes `Logging/StructuredLog.h`,
`ProfilingDebugging/CpuProfilerTrace.h` and `VisualLogger/VisualLogger.h`, and the
`DECLARE_CYCLE_STAT` line shown in the same section. `AGSSnake::Step()` begins with the two
scopes and the assert, and records each step for the Visual Logger:

```cpp
void AGSSnake::Step()
{
	// Stats feed "stat GridSnake"; the trace scope shows each step as a named
	// block on the timeline in Unreal Insights.
	SCOPE_CYCLE_COUNTER(STAT_GSSnakeStep);
	TRACE_CPUPROFILER_EVENT_SCOPE(AGSSnake::Step);

	// check() stops the program in Debug and Development builds if it fails:
	// for things that must never happen. Tick only steps a live snake on a board.
	check(Board && Segments.Num() > 0);

	if (DirectionQueue.Num() > 0)
	{
		CurrentDirection = DirectionQueue[0];
		DirectionQueue.RemoveAt(0);
	}

	const FIntPoint NextCell = Segments[0] + GSGrid::ToOffset(CurrentDirection);

	// Recorded by the Visual Logger (Tools > Debug > Visual Logger) when it is
	// recording; costs next to nothing otherwise.
	UE_VLOG_LOCATION(this, LogGridSnake, Verbose, Board->CellToWorld(NextCell), 4.0f, FColor::Yellow,
		TEXT("Step %s -> %s"), *UEnum::GetValueAsString(CurrentDirection), *NextCell.ToString());

	// ... (the wall and self checks and the move, unchanged)
}
```

And `Die()` switches its log line to `UE_LOGFMT` and records the death:

```cpp
	// Before (chapter 9):
	// UE_LOG(LogGridSnake, Warning, TEXT("Snake %s at cell %s (length %d)"), Reason, *GetHeadCell().ToString(), Segments.Num());

	// Structured logging: {Name} placeholders are filled, in order, by the values
	// that follow, and are kept as named fields for tools that read the log.
	const FIntPoint Cell = GetHeadCell();
	UE_LOGFMT(LogGridSnake, Warning, "Snake {Reason} at cell ({X}, {Y}), length {Length}", FString(Reason), Cell.X, Cell.Y, Segments.Num());
	UE_VLOG(this, LogGridSnake, Warning, TEXT("Died: %s"), Reason);
```

The cell is now written as two numbers, `(3, 0)`, instead of `FIntPoint::ToString`'s
`X=3 Y=0`. Fields such as `{X}` stay numbers for tools that read the structured data.

### Editor steps

This chapter adds no assets. Build (close the editor and build from the IDE, because headers
changed), open the project, open `L_Level01` and press **Play**. Wait for the countdown to end,
then open the console with the backtick key and try each command:

| Command | What it does |
|---|---|
| `GS.Debug.Grid 1` | Draws every cell: red = blocked, green = free (`0` turns it off) |
| `GSGrow 10` | Adds 10 segments |
| `GSCollectAll` | Eats every gem; the exit opens |
| `GSWin` | Finishes the level |
| `GSUnlockAll` | Unlocks every level and saves |
| `stat GridSnake` | Shows the `Snake Step` cycle counter |
| `stat unit`, `stat fps` | Frame, game, draw and GPU times |
| `Log LogGridSnake Verbose` | Shows the Verbose lines of the project's log category |
| `show collision` | Draws collision shapes |

Then:

1. **Visual Logger**: **Tools > Debug > Visual Logger** (see the note on the menu path above),
   click **Start**, play until the snake dies, stop playing and click **Stop**. Scrub the
   timeline of the snake's row: every step left a yellow sphere labelled with the direction and
   the next cell, and the death is a `Warning` entry `Died: hit a wall`.
2. **Unreal Insights**: start a trace from the status bar's **Trace** menu with the **CPU**
   channel on (or launch Standalone with `-trace=cpu,frame,log`). Play for a few seconds, then
   stop the trace. Open it in Unreal Insights, and in **Timing Insights** search for
   `AGSSnake::Step`.

### Expected result

The grid overlay shows the border walls and any inner walls in red, and every other cell in green.
The snake can only enter green cells. In the console, the cheats log their lines and the game reacts
as if you had played: gems pop and play their sound, the exit opens, the level-complete screen
appears.

Expected Output Log (filter: `LogGridSnake`), for `GSGrow 10`, then `GSCollectAll` in a level
with three gems:

```text
LogGridSnake: GSGrow: +10 segments
LogGridSnake: Gem 1/3
LogGridSnake: Gem 2/3
LogGridSnake: Gem 3/3
LogGridSnake: All gems collected - the exit opens
LogGridSnake: GSCollectAll: 3 gem(s)
```

The exit actor's own `is now open` line also appears in that block. Its exact position depends on
when the exit hears the event.

For `GSUnlockAll` (the save line may come a moment later, because saving is asynchronous since
chapter 18):

```text
LogGridSnake: GSUnlockAll: 3 level(s) unlocked
LogGridSnake: Save to slot GridSnake succeeded
```

And when the snake dies by hitting the top wall, with the structured log line (the cell and length
are examples):

```text
LogGridSnake: Warning: Snake hit a wall at cell (3, 1), length 3
```

## Summary

- Debug your own code in the **DebugGame Editor** configuration: your module unoptimized, the
  engine fast. Start the editor with F5 from the IDE, or attach to `UnrealEditor`. Breakpoints,
  Locals, Watch and Call Stack work as in any C++ program. Close the editor before building from
  the IDE when Live Coding is on.
- Change log verbosity at run time with `Log <Category> <Verbosity>`, at startup with `-LogCmds`,
  permanently in `[Core.Log]`. `UE_LOGFMT` (`Logging/StructuredLog.h`) replaces `printf` specifiers
  with named fields. `GEngine->AddOnScreenDebugMessage` shows live values.
- `check`/`checkf` stop on broken invariants and vanish from Shipping, expression included.
  `verify` keeps the expression. `ensure`/`ensureMsgf` report once and keep running.
- `TAutoConsoleVariable` declares a CVar, read with `GetValueOnGameThread()`. `FAutoConsoleCommand`
  registers a command with a delegate. `ECVF_Cheat` hides either in Shipping.
- `DrawDebugBox`, `DrawDebugLine`, `DrawDebugSphere` and `DrawDebugString` (`DrawDebugHelpers.h`)
  draw for a frame, for a time, or persistently. Wrap them in `#if ENABLE_DRAW_DEBUG`.
- A `UCheatManager` subclass set as the controller's `CheatClass` turns `UFUNCTION(Exec)` functions
  into console cheats that never exist in Shipping. Cheats should take the real code paths.
- The Visual Logger records `UE_VLOG` and `UE_VLOG_LOCATION` entries per actor so you can scrub
  back in time.
- `DECLARE_STATS_GROUP`, `DECLARE_CYCLE_STAT` and `SCOPE_CYCLE_COUNTER` make a `stat <Group>` page.
  `TRACE_CPUPROFILER_EVENT_SCOPE` names a block in Unreal Insights. Record a trace from the
  status bar or with `-trace=cpu,frame`.
- GridSnake now has `GS.Debug.Grid`, `GSGrow`, `GSCollectAll`, `GSWin`, `GSUnlockAll`,
  `stat GridSnake`, an `AGSSnake::Step` trace scope, Visual Logger entries and a structured death
  log.

## Exercises

1. **Draw the queued turns.** Add a CVar `GS.Debug.Path` that, when on, draws a line from the
   snake's head through the turns waiting in `DirectionQueue`.

   <details><summary>Solution</summary>

   In `Snake/GSSnake.cpp`, add two includes and the CVar next to the cycle stat:

   ```cpp
   #include "DrawDebugHelpers.h"
   #include "HAL/IConsoleManager.h"

   static TAutoConsoleVariable<bool> CVarGSDebugPath(
   	TEXT("GS.Debug.Path"),
   	false,
   	TEXT("Draw the snake's queued turns: white = current direction, cyan = queued."),
   	ECVF_Cheat);
   ```

   At the top of `AGSSnake::Tick`, after `Super::Tick(DeltaSeconds);` and before the
   `if (!bMoving)` early return, so that it also draws while the snake waits:

   ```cpp
   #if ENABLE_DRAW_DEBUG
   	if (CVarGSDebugPath.GetValueOnGameThread() && Board && Segments.Num() > 0)
   	{
   		// Towards the camera, like the debug grid, so the sprites do not hide it.
   		const FVector Lift(0.0f, 25.0f, 0.0f);
   		FIntPoint Cell = Segments[0];

   		// Where the snake goes next if no turn is queued.
   		const FIntPoint Ahead = Cell + GSGrid::ToOffset(CurrentDirection);
   		DrawDebugLine(GetWorld(), Board->CellToWorld(Cell) + Lift, Board->CellToWorld(Ahead) + Lift,
   			FColor::White, false, -1.0f, 0, 1.0f);

   		for (const EGSDirection Direction : DirectionQueue)
   		{
   			const FIntPoint Next = Cell + GSGrid::ToOffset(Direction);
   			DrawDebugLine(GetWorld(), Board->CellToWorld(Cell) + Lift, Board->CellToWorld(Next) + Lift,
   				FColor::Cyan, false, -1.0f, 0, 2.0f);
   			Cell = Next;
   		}
   	}
   #endif
   ```

   Build, play and type `GS.Debug.Path 1`. The white line always points one cell ahead. Tap two
   arrow keys quickly: cyan segments appear for a moment, one per queued turn, and vanish as `Step`
   consumes them. The queue holds at most two turns (`MaxQueuedTurns`, chapter 9), so the cyan path is short
   and brief. That is what the buffer really does.

   </details>

2. **A speed cheat.** Add `GSSetSpeed <seconds>` that sets the time between two steps, for
   example `GSSetSpeed 0.5` for a slow snake.

   <details><summary>Solution</summary>

   In `Core/GSCheatManager.h`, inside the class:

   ```cpp
   	/** Sets the seconds between two steps. Ignores the level's speed curve until restart. */
   	UFUNCTION(Exec)
   	void GSSetSpeed(float Seconds = 0.15f);
   ```

   In `Core/GSCheatManager.cpp`:

   ```cpp
   void UGSCheatManager::GSSetSpeed(float Seconds)
   {
   	if (AGSSnake* Snake = GetSnake(*this))
   	{
   		// nullptr: no speed curve, so the interval stays the same as the snake grows.
   		Snake->ConfigureSpeed(Seconds, nullptr);
   		UE_LOG(LogGridSnake, Log, TEXT("GSSetSpeed: one step every %.2fs"), FMath::Max(Seconds, 0.02f));
   	}
   }
   ```

   `ConfigureSpeed` already clamps the interval to at least 0.02 seconds (chapter 13), and the
   snake reads the interval on every step, so the change applies at once. Restarting the level
   restores the level's own speed, because the game mode calls `ConfigureSpeed` in `StartPlay`.

   Expected Output Log (filter: `LogGridSnake`) for `GSSetSpeed 0.5`:

   ```text
   LogGridSnake: GSSetSpeed: one step every 0.50s
   ```

   </details>

3. **The most expensive function.** Record an Insights trace of a few seconds of play, with the
   `ExStatsAndTrace` actor in the level. Find which timer costs the most per frame.

   <details><summary>Solution</summary>

   1. Place `ExStatsAndTrace` in `L_Level01`. Start a trace with the **CPU** and **Frame** channels
      (status bar **Trace** menu, or Standalone with `-trace=cpu,frame`), play for five seconds
      and stop.
   2. Open the trace in Unreal Insights. In **Timing Insights**, drag across the frame graph at the
      top to select a range of frames, or select everything.
   3. In the **Timers** panel, sort by the **Incl** column (inclusive time: the timer plus
      everything it called) by clicking its header. Use **Excl** (exclusive: its own code only) to
      find the function that actually does the work.
   4. Near the top, below the frame and the engine's tick scopes, which include everything, you will
      find `AExStatsAndTrace::SumValues`, with a count of one per frame and an average of a few
      milliseconds. `AGSSnake::Step` is far down the list, with a few calls a second and
      microseconds each.

   The lesson: inclusive time tells you *where* the time is spent (follow the tree down).
   Exclusive time tells you *which* code spends it. Always measure before optimizing.

   </details>

4. **Make an ensure fire.** Add an `ensureMsgf` that fails in `AGSBoard::BeginPlay`. Find it in
   the log, play again, and explain why it does not report a second time. Then make it report every
   time.

   <details><summary>Solution</summary>

   At the end of `AGSBoard::BeginPlay` in `Gameplay/GSBoard.cpp`:

   ```cpp
   	// Deliberately fails: no GridSnake level is this wide.
   	ensureMsgf(GetGridSize().X > 1000, TEXT("%s is only %d cells wide"), *GetName(), GetGridSize().X);
   ```

   Build and press **Play**. The game keeps running. The Output Log shows an error containing
   `Ensure condition failed: GetGridSize().X > 1000`, your message, and a callstack that names
   `AGSBoard::BeginPlay`. With the debugger attached, execution stops on that line first. Press F5
   to continue.

   Stop and press **Play** again: no new report. An `ensure` reports only the first failure per call
   site per editor session, so a failing check in `Tick` cannot flood the log. Restarting the editor
   resets it. Replace `ensureMsgf` with `ensureAlwaysMsgf` and it reports on every Play. Remove the
   line when you are done: it is a broken invariant on purpose.

   </details>

---

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-19--debugging-and-profiling)
