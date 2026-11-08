# Chapter 14 — Level Transitions and Persistence

At the end of chapter 13 GridSnake opens the next map when you reach an exit. It works, but it is
abrupt: the picture freezes, then the new level appears. And it forgets everything. When the new
map opens, every actor in the old world is destroyed: the game mode, the game state with its score,
the player controller, the snake. Nothing you kept in them survives. This chapter is about what
happens during that map change and how to work with it:

- What a map change destroys, what survives it, and why.
- `OpenLevel`, what "travel" means, travel options, and the alternatives (multiplayer travel, level
  streaming, World Partition).
- **Game instance subsystems**: the right home for state that must outlive a map.
- **Camera fades**, so a level change looks deliberate.
- **Loading screens** drawn by the movie player while the game thread is busy loading.

GridSnake gains `UGSLevelFlowSubsystem`. It owns the *run*: which level of the list you are on and
the score banked from completed levels. It also owns the *transition*: fade to black, show
"Loading The Maze...", open the map, fade back in. The game mode shrinks to asking it for things.

Previous: [Chapter 13](13-data-driven-levels.md). Next: [Chapter 15](15-a-hud-with-umg.md).

## What a level change destroys

Chapter 7 introduced the gameplay framework's objects. Here they are again, grouped by who owns
them, because ownership decides what survives a map change:

```text
Engine (GEngine)                             - the whole process
 +- Game instance (UGSGameInstance)          - the whole game session
 |   +- game instance subsystems             - same as the game instance
 |   +- local player(s), viewport client     - same as the game instance
 +- World (UWorld): the map being played     - ONE map
     +- world subsystems                     - same as the world
     +- levels: the map's actors             - same as the world
         +- GameMode, GameState
         +- PlayerController (+ PlayerCameraManager, HUD), PlayerState
         +- Pawn (the snake), board, gems, exit, ...
             +- components
```

A map change replaces the whole `World` branch. Everything above it carries on. In detail:

| Object | Owned by | Survives a map change? |
|---|---|---|
| The engine and engine subsystems | the process | Yes |
| The game instance (`UGSGameInstance`) | the engine | Yes: one for the whole game session |
| Game instance subsystems | the game instance | Yes |
| Local players and their subsystems, the game viewport | the game instance | Yes |
| Settings objects such as `UGSSettings`, and other class default objects | their class | Yes |
| Static and global C++ variables | the module | Yes (and in the editor, even between Play sessions) |
| The `UWorld`, its levels, world subsystems | the engine's world context | **No**: a new world is created |
| Every actor: game mode, game state, player controller, player state, camera manager, HUD, pawn, everything placed or spawned | the world | **No**: each gets `EndPlay`, then is destroyed |
| Components | their actor | **No** |
| Timers set on the world's timer manager | the world | **No**: their objects are gone |
| UMG widgets added to the viewport (chapter 15) | the viewport | **No**: removed during the map change |
| Sounds playing in the world (chapter 17) | the world | Normally they stop |
| Assets the old map loaded | whoever references them | Only if something still references them; the rest may be freed |

Two lines in that table matter most:

- **The game instance survives.** It is created once when the game starts (or when you press Play
  in the editor) and destroyed when the game quits (or when you press Stop). It is the one
  gameplay object that sees every map.
- **The player controller does not.** Players often expect "the player" to persist, because the
  same person keeps playing. But `APlayerController` is an actor, so the new map gets a new one,
  logged in afresh with a new pawn. What persists is the `ULocalPlayer`: the engine's record that
  someone is sitting at this screen.

### Seeing it: a counter in three places

The next example counts in three places: in a game instance subsystem, in an actor member, and in a
static variable. Then it changes maps and watches which counts survive. Game instance subsystems
get a full section later in this chapter. For now: a `UGameInstanceSubsystem` subclass is an object
the engine creates automatically, one per game instance, and keeps exactly as long as the game
instance.

`examples/ch14/ExPersistentCounter.h`

```cpp
// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ExPersistentCounter.generated.h"

/** A counter that survives level changes because the game instance owns it. */
UCLASS()
class UExPersistentCounter : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Example")
	int32 Increment() { return ++Count; }

	UFUNCTION(BlueprintPure, Category = "Example")
	int32 GetCount() const { return Count; }

private:
	int32 Count = 0;
};

/**
 * Place one in two levels, ExMapA and ExMapB. Play ExMapA, then type
 * "open ExMapB" and "open ExMapA" in the console (`) to switch maps.
 * Each BeginPlay counts in three places and logs all three.
 */
UCLASS()
class AExCounterProbe : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Lives in this actor, which is destroyed with its level. */
	int32 ActorCount = 0;
};
```

`examples/ch14/ExPersistentCounter.cpp`

```cpp
// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExPersistentCounter.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Lives in the module: it survives level changes and, in the editor, even Stop and Play. */
	int32 GStaticCount = 0;

	const TCHAR* ReasonName(EEndPlayReason::Type Reason)
	{
		switch (Reason)
		{
		case EEndPlayReason::Destroyed:        return TEXT("Destroyed");
		case EEndPlayReason::LevelTransition:  return TEXT("LevelTransition");
		case EEndPlayReason::EndPlayInEditor:  return TEXT("EndPlayInEditor");
		case EEndPlayReason::RemovedFromWorld: return TEXT("RemovedFromWorld");
		case EEndPlayReason::Quit:             return TEXT("Quit");
		default:                               return TEXT("?");
		}
	}
}

void AExCounterProbe::BeginPlay()
{
	Super::BeginPlay();

	UExPersistentCounter* Counter = GetGameInstance()->GetSubsystem<UExPersistentCounter>();
	const int32 SubsystemCount = Counter->Increment();
	++ActorCount;
	++GStaticCount;

	UE_LOG(LogTemp, Log, TEXT("Probe on %s: subsystem %d, actor %d, static %d"),
		*UGameplayStatics::GetCurrentLevelName(this), SubsystemCount, ActorCount, GStaticCount);
}

void AExCounterProbe::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Log, TEXT("Probe on %s EndPlay: %s"),
		*UGameplayStatics::GetCurrentLevelName(this), ReasonName(EndPlayReason));

	Super::EndPlay(EndPlayReason);
}
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`. Game instance subsystems are part of `Engine`.

To see it:

1. Copy both files into your scratch project's module (chapter 7: a Blank C++ project, not
   GridSnake). New classes need a full build: close the editor and build from your IDE, then
   reopen the project.
2. Make two levels: **File > New Level > Basic**, then **File > Save Current Level As...** as
   `ExMapA`. Repeat for `ExMapB`. Save both in the same folder, for example `Content/Maps`. Basic
   levels have a floor, a sky and a light, which the fade example later in this chapter needs: a
   fade to black is invisible on an Empty Level, which is black already.
3. Drag `ExCounterProbe` from the Content Browser's **C++ Classes** folder into each level. Save.
4. Open `ExMapA` and press Play (Play In Editor).
5. Press the backtick key (`` ` ``) to open the console (chapter 1), type `open ExMapB` and press
   Enter.
6. Open the console again, type `open ExMapA`, press Enter.
7. Press Stop (or Esc). Then press Play again.

Expected Output Log (filter: `LogTemp`):

```text
LogTemp: Probe on ExMapA: subsystem 1, actor 1, static 1
LogTemp: Probe on ExMapA EndPlay: LevelTransition
LogTemp: Probe on ExMapB: subsystem 2, actor 1, static 2
LogTemp: Probe on ExMapB EndPlay: LevelTransition
LogTemp: Probe on ExMapA: subsystem 3, actor 1, static 3
LogTemp: Probe on ExMapA EndPlay: EndPlayInEditor
LogTemp: Probe on ExMapA: subsystem 1, actor 1, static 4
```

- **The actor count is always 1.** Each map has its own probe, and each probe is a new object: the
  `ExMapA` probe on the fifth line is not the one from the first line. Even returning to the same
  map gives you new actors, loaded fresh from the map file.
- **The subsystem count climbs: 1, 2, 3.** The subsystem belongs to the game instance, which lived
  through both map changes.
- **After Stop and Play, the subsystem count is 1 again.** Stop destroyed the game instance and
  its subsystems. Play created new ones. Inside the editor, one Play session is one "game".
- **The static count is 4.** A static variable belongs to the loaded C++ module, and in the editor
  the module stays loaded across Play sessions. That makes statics a trap for game state: a value
  left over from your last test run leaks into the next one, and a packaged game (one session per
  launch) behaves differently. Keep game state in objects with a clear owner.
- **`EndPlay` tells you why.** `EEndPlayReason::LevelTransition` means "the world is going away
  because a map is changing". You also get `EndPlayInEditor` (Stop), `Quit` (the game is closing),
  `Destroyed` (the actor itself was destroyed) and `RemovedFromWorld` (its streaming level was
  unloaded). Override `EndPlay`, not the destructor, for cleanup that must happen when an actor
  leaves play; the destructor runs later, whenever the garbage collector gets to it.
- **`open MapName`** is a console command that does the same as `OpenLevel`. It is the fastest way
  to test map changes. A short map name works as long as only one map has that name; otherwise give
  the full path, such as `open /Game/Maps/ExMapB`.

### Store data, not actors

The game instance and its subsystems survive. That makes them tempting places to keep a pointer to
"the snake" or "the board". Don't. After a map change that pointer points at a destroyed actor from
a world that no longer exists:

- A raw pointer (`AGSSnake*` without `UPROPERTY`) dangles: using it is undefined behaviour, usually
  a crash.
- A `UPROPERTY` pointer is cleared to `nullptr` by the garbage collector once the destroyed actor
  is collected (chapter 3 covered how the collector sees `UPROPERTY` pointers). Safe, but useless.
- A `TWeakObjectPtr` reports itself invalid. Also safe and also useless.

Worse, a strong reference from a long-lived object to anything in the old world can keep parts of
that world in memory. In development builds the engine checks after a map change that the old world
was really freed, and complains loudly (with a list of who still references it) when it was not.

What a persistent object should hold is **data**: numbers, names, IDs, soft references. GridSnake's
subsystem keeps an index into the level list and a score, never an actor. Each new world's actors
ask the subsystem for what they need, in their own `BeginPlay` or `StartPlay`.

If you need an actor of the *current* world from a subsystem, look it up when you need it (for
example with `UGameplayStatics::GetPlayerController(GetGameInstance(), 0)`), use it, and drop it.

### Where state should live

| State | Lifetime | Home |
|---|---|---|
| Gems collected in this level, this level's score, the phase | One attempt at one level | `AGSGameState` (dies with the map, which is what you want) |
| The current level of the run, the score banked from finished levels | One run, across maps | A game instance subsystem (this chapter) |
| Unlocked levels, best scores | Forever, across launches | A save game on disk (chapter 18) |
| Volume, resolution, key bindings | Forever, per machine | Game user settings and config (chapters 16 and 17) |
| Rules of the game: gems per level, the level list | Fixed while playing | Project settings and data assets (chapters 3 and 13) |

Notice that the game state is *meant* to reset. A restart after a death opens the same map again,
and the fresh game state gives the player a clean attempt for free. Persistence is something you opt
into, one piece of data at a time.

## OpenLevel and travel

"Travel" is Unreal's word for moving the player from one map to another. `UGameplayStatics` (header
`Kismet/GameplayStatics.h`) offers two functions for the single-player case:

```cpp
static void OpenLevel(const UObject* WorldContextObject, FName LevelName,
	bool bAbsolute = true, FString Options = FString(TEXT("")));

static void OpenLevelBySoftObjectPtr(const UObject* WorldContextObject,
	const TSoftObjectPtr<UWorld> Level, bool bAbsolute = true, FString Options = FString(TEXT("")));
```

- **`WorldContextObject`** is any object that can tell the function which world you mean: an
  actor, a component, or the game instance (whose `GetWorld()` returns its current world).
- **`LevelName`** is a map name, short (`L_Level02`) or long (`/Game/GridSnake/Maps/L_Level02`).
  A short name must be unique in the project. The soft-pointer version takes the map from a
  `TSoftObjectPtr<UWorld>`, like `FGSLevelInfo::Map`. It survives renames (the editor fixes the
  reference) and cannot contain a typo, so prefer it whenever you have a soft pointer.
- **`Options`** and **`bAbsolute`** are explained below.

In Blueprints the same functions are the **Open Level (by Name)** and **Open Level (by Object
Reference)** nodes (the labels may differ slightly in your version).

### OpenLevel does not open the level

The most important thing to know about `OpenLevel` is that it does not change the map when you
call it. It *requests* a travel: it records the destination with the engine (the engine function is
`UEngine::SetClientTravel`) and returns. Your code carries on, the rest of the frame runs normally,
and the engine performs the travel at the start of its next tick, before the world ticks again.

So after `OpenLevel` returns:

- `this`, the game mode, the snake and everything else are still alive for the rest of the frame.
  Code after the call runs, and nothing crashes.
- But nothing you do to the current world matters any more. Do not start effects, spawn actors or
  change state there and expect to see it.
- Calling `OpenLevel` twice in one frame means the second destination wins. GridSnake's subsystem
  guards against starting two transitions (later in this chapter).

If the map name is wrong, the travel fails and the engine logs a warning that the map could not be
found (the exact wording varies between versions). Search the Output Log for the map name when
"nothing happens" after an `OpenLevel`.

### What happens during the travel

When the engine processes the travel request, it loads the new map with `UEngine::LoadMap`. You do
not call `LoadMap` yourself, but knowing its order explains every rule in this chapter. Simplified,
and leaving out networking:

1. **`FCoreUObjectDelegates::PreLoadMap` is broadcast.** The documentation describes it as "sent
   at the very beginning of LoadMap"; it passes the map name. Loading screens start here.
2. **The old world ends play.** Every actor gets `EndPlay(EEndPlayReason::LevelTransition)`.
   Widgets are removed from the viewport. World subsystems are deinitialised. The world is cleaned
   up.
3. **Garbage collection** frees the old world and everything only it referenced.
4. **The new map is loaded from disk**, with every asset it hard-references (chapter 13's hard and
   soft references decide what that is). This is the slow part, and the game thread does nothing
   else meanwhile: no ticking, no rendering of the game.
5. **The new world is initialised.** World subsystems are created. The game mode class is chosen
   (from the travel options, then the map's World Settings override, then the project default) and
   the game mode is spawned. It reads the travel options in `InitGame`.
6. **Each local player logs in to the new game mode.** It gets a new player controller, which
   spawns its own camera manager, and the game mode spawns its pawn at a Player Start.
7. **The world begins play.** The game mode's `StartPlay` runs, which calls `BeginPlay` on every
   actor.
8. **`FCoreUObjectDelegates::PostLoadMapWithWorld` is broadcast** with the new `UWorld*` ("sent at
   the end of LoadMap"). Loading screens may finish from here.

Then the engine's next frame ticks the new world. The game instance and its subsystems watched all
of this from outside. They can follow along by binding to the two delegates.

### Example: watching map loads

`examples/ch14/ExTravelLogger.h`

```cpp
// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ExTravelLogger.generated.h"

/**
 * Logs every map load. Nothing to place or configure: the engine creates one
 * instance of every UGameInstanceSubsystem class together with the game
 * instance, and destroys it when the game instance shuts down.
 */
UCLASS()
class UExTravelLogger : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/** A plain member: it lives as long as the game instance, across every map change. */
	int32 LoadCount = 0;
};
```

`examples/ch14/ExTravelLogger.cpp`

```cpp
// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExTravelLogger.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogExTravel, Log, All);

void UExTravelLogger::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Global engine events: they fire for every map load, whoever started it.
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UExTravelLogger::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UExTravelLogger::HandlePostLoadMap);

	UE_LOG(LogExTravel, Log, TEXT("Travel logger created by %s"), *GetNameSafe(GetGameInstance()));
}

void UExTravelLogger::Deinitialize()
{
	// The delegates are static and outlive us: always unbind what you bound.
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	UE_LOG(LogExTravel, Log, TEXT("Travel logger going away after %d map load(s)"), LoadCount);
	Super::Deinitialize();
}

void UExTravelLogger::HandlePreLoadMap(const FString& MapName)
{
	++LoadCount;
	UE_LOG(LogExTravel, Log, TEXT("PreLoadMap %s (load #%d)"), *MapName, LoadCount);
}

void UExTravelLogger::HandlePostLoadMap(UWorld* LoadedWorld)
{
	UE_LOG(LogExTravel, Log, TEXT("PostLoadMap %s"), *GetNameSafe(LoadedWorld));
}
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`.

To see it: add the files to the scratch project that has `ExMapA` and `ExMapB` (the previous
example's probes can stay) and build from the IDE. Play `ExMapA`, then use the console: `open ExMapB`, then
`open ExMapA`. Press Stop.

Expected Output Log (filter: `LogExTravel`). The exact map strings depend on your folders and may
carry the PIE prefix described below:

```text
LogExTravel: Travel logger created by GameInstance_0
LogExTravel: PreLoadMap /Game/Maps/ExMapB (load #1)
LogExTravel: PostLoadMap ExMapB
LogExTravel: PreLoadMap /Game/Maps/ExMapA (load #2)
LogExTravel: PostLoadMap ExMapA
LogExTravel: Travel logger going away after 2 map load(s)
```

- **No setup.** The class exists, so the engine creates it. The first line names the game
  instance that created it: `GameInstance_0` with the engine's default class, or something like
  `GSGameInstance_0` in GridSnake.
- **The first map is not counted in PIE.** Play In Editor creates the first world by duplicating
  the map open in the editor (chapter 2), not by loading it with `LoadMap`, so there may be no
  `PreLoadMap` for it. A standalone or packaged game loads its first map through `LoadMap` like any
  other. Write code that does not depend on seeing the first map's events.
- **In PIE, map names may carry the `UEDPIE_0_` prefix** that marks Play In Editor copies, because
  maps opened during PIE are loaded as PIE copies too.
- **`FCoreUObjectDelegates`** (header `UObject/UObjectGlobals.h`) holds engine-wide delegates as
  static members. They are multicast delegates without reflection (chapter 11), so you bind with
  `AddUObject` and unbind with `RemoveAll(this)`.
- **Unbind in `Deinitialize`.** A static delegate outlives every object. `AddUObject` holds the
  object weakly, so a forgotten binding to a destroyed object is skipped rather than crashing, but
  it still sits in the delegate's list, and in the editor a new subsystem is created at every Play.
  Removing what you added is the habit to have.
- There is also `PreLoadMapWithContext`, which passes the world context as well as the name, for
  code that must tell several game instances apart (for example multiple PIE clients).

### Travel options

A travel destination is really a URL: a map name followed by options, each introduced by `?`:

```text
L_Level02?Difficulty=2?Hardcore
```

`OpenLevel`'s `Options` parameter is that part without the first `?`:

```cpp
UGameplayStatics::OpenLevel(this, FName("L_Level02"), /*bAbsolute=*/true, TEXT("Difficulty=2?Hardcore"));
```

The new world's game mode receives them. They are passed to `AGameModeBase::InitGame` and stored in
the game mode's `OptionsString` member. `UGameplayStatics` has helpers to read them:

```cpp
// In the new map's game mode, for example in StartPlay:
const int32 Difficulty = UGameplayStatics::GetIntOption(OptionsString, TEXT("Difficulty"), /*DefaultValue=*/1);
const bool bHardcore = UGameplayStatics::HasOption(OptionsString, TEXT("Hardcore"));
const FString Raw = UGameplayStatics::ParseOption(OptionsString, TEXT("Difficulty")); // "2"
```

The console takes options too: `open L_Level02?Difficulty=2`.

Some option names are understood by the engine itself. The one you are most likely to use is
`game=`, which overrides the game mode class for that map. It beats the map's **World Settings >
GameMode Override** and the project default (**Project Settings > Maps & Modes > Default
GameMode**). `listen` starts a listen server (multiplayer).

`bAbsolute` decides what happens to the options of the *current* URL. With `true` (the default) the
new URL starts clean. With `false` the current options are carried over and the new ones are
added. Options are an old and simple mechanism: they carry a few values into the next map's game
mode. For anything more than a handful of flags, a game instance subsystem is clearer, and it is
what GridSnake uses.

### Travel in multiplayer

`OpenLevel` is a client-side travel: this game leaves its map and opens another, on its own.
Multiplayer games use two related functions you will meet if you go further:

- `APlayerController::ClientTravel` sends one client to a map or a server address.
- `UWorld::ServerTravel` makes a server change map and brings every connected client along.

Server travel can also be **seamless**: the server first moves everyone to a small *transition
map*, unloads the old map, loads the new one in the background, and carries chosen actors (player
states, controllers) across. That avoids disconnecting players during the load. It is turned on in
the game mode (`bUseSeamlessTravel`) and only applies to server travel. GridSnake is single player
and uses none of this. The "Travelling in Multiplayer" page in this chapter's sources covers it.

### Level streaming: not changing the map at all

A full map change throws away the world. Sometimes you would rather keep it. **Level streaming**
splits a world into a *persistent level*, which is always loaded, and *sublevels* that you load and
unload while playing. The persistent level's actors (game mode, player controller, pawn, anything
placed in it) are never destroyed, and sublevels load in the background while the game runs.

| | `OpenLevel` (map change) | Level streaming |
|---|---|---|
| Old actors | All destroyed | Persistent level's actors stay; unloaded sublevels' actors go |
| Game mode, controller, pawn | New ones | The same ones |
| Loading | Blocks the game thread; hide it with a fade or loading screen | In the background while you play |
| Reset for free | Yes: the new map is pristine | No: you reset what needs resetting |
| Good for | Distinct levels, menus, "start over" | Rooms of one big area, seamless worlds, a menu over a live scene |

You set up sublevels in the **Levels** window (**Window > Levels**), and load them from Blueprints
with **Load Stream Level** and **Unload Stream Level**, or from C++ (exercise 4). GridSnake's levels
are separate puzzles, and starting each one from a clean map is exactly what it wants, so it uses
`OpenLevel`.

**World Partition** is the automatic version of streaming for very large worlds: the map is divided
into a grid, and cells load and unload around the player. New maps made from the **Open World**
template use it. A GridSnake board is smaller than a single World Partition cell, so it would gain
nothing. The maps in this course are made from **Empty Level** (chapter 1), which does not use World
Partition. If you ever build a large scrolling 2D world, read the World Partition page in this
chapter's sources.

## Game instance subsystems

You need an object that lives as long as the game instance. GridSnake already has a game instance
class, `UGSGameInstance` (chapter 3), so the obvious move is to add members to it. That works, and
older Unreal projects do exactly that, until the game instance has grown into a class that knows
about the level flow, the audio, the save files, the online services and the achievements. Every
feature edits the same file, and nothing can be reused in another project.

A **subsystem** is a small class with one job whose lifetime is tied to an engine object. You
subclass the right base class, and the engine does the rest:

- It creates one instance of your class automatically when its owner is created. No project
  setting, no spawning, no registration.
- It calls `Initialize` on it at creation and `Deinitialize` before the owner goes away.
- It hands it out to anyone who asks the owner with `GetSubsystem<T>()`.

There are five kinds, one per owner:

| Base class | Owner, and so lifetime | Example |
|---|---|---|
| `UEngineSubsystem` | The engine: the whole process, editor included | Something shared by the editor and every game session |
| `UEditorSubsystem` | The editor (editor modules only) | Editor tools |
| `UGameInstanceSubsystem` | The game instance: one game session, every map | GridSnake's level flow, audio (chapter 17) and saves (chapter 18) |
| `UWorldSubsystem` | One world: **dies with the map** | Per-level managers; `UTickableWorldSubsystem` can also tick |
| `ULocalPlayerSubsystem` | One local player: the session, every map | `UEnhancedInputLocalPlayerSubsystem` from chapter 8 |

You have used one already: chapter 8 asked the local player for the Enhanced Input subsystem to add
an input mapping context. The same pattern now runs in the other direction: you write the
subsystem, and the rest of the game asks for it.

A game instance subsystem is the right home for GridSnake's run because its lifetime is exactly
"from game start to quit, through every map", and it can hold its own state without touching
`UGSGameInstance`.

### Writing and using one

The smallest game instance subsystem is a class that derives from `UGameInstanceSubsystem` (header
`Subsystems/GameInstanceSubsystem.h`, in the `Engine` module). `UExPersistentCounter` earlier in
this chapter is one, and it does not even override `Initialize`. To get it:

```cpp
// From any actor or component (they all have GetGameInstance()):
UGSLevelFlowSubsystem* LevelFlow = GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();

// From anything that only has a world context object (and may run without a game instance,
// for example in the editor): the static version returns nullptr instead of crashing.
UGSLevelFlowSubsystem* Maybe = UGameInstance::GetSubsystem<UGSLevelFlowSubsystem>(
	UGameplayStatics::GetGameInstance(this));
```

In Blueprints, every game instance subsystem gets a getter node named after the class. Search for
the class name in the node menu, for example `GSLevel Flow Subsystem` (the exact label may differ
slightly in your version). Its `BlueprintCallable` and `BlueprintPure` functions are then available
like any object's.

The rules of the lifecycle:

- **`Initialize(FSubsystemCollectionBase& Collection)`** runs when the game instance initialises
  its subsystems, which happens inside `UGameInstance::Init`. In GridSnake that is inside
  `UGSGameInstance::Init`'s call to `Super::Init()`, so a subsystem's `Initialize` log is likely to
  appear *before* the game instance's own "initialised" log line. Call `Super::Initialize` first.
- **The order between subsystems is not defined.** If one needs another during `Initialize`, it
  asks for it with `Collection.InitializeDependency<T>()`; chapter 18 needs that. After
  initialisation, every subsystem exists and `GetSubsystem<T>()` is enough.
- **`Deinitialize()`** runs when the game instance shuts down: when the game quits, or when you
  press Stop in the editor. Unbind delegates and stop timers here. Call `Super::Deinitialize` last.
- **`ShouldCreateSubsystem(UObject* Outer) const`** can be overridden to return `false` and skip
  creation, for example on a dedicated server. A subsystem that may not exist must be null-checked
  wherever it is fetched. A class marked `UCLASS(Abstract)` is never created, which is how you write
  a base class for subsystems.
- **No ticking and no components.** A subsystem is a `UObject`, not an actor. For something
  periodic, use a timer on the game instance's timer manager (GridSnake does), or implement
  `FTickableGameObject` if it must run every frame.
- **It is a `UObject`.** `UPROPERTY` members are seen by the garbage collector, `UFUNCTION`s work
  in Blueprints, and `GetGameInstance()` returns the owner.

In the editor, each Play creates a new game instance and so new subsystems. The example output at
the start of this chapter showed it: the counter was back to 1 after Stop and Play.

## The level flow subsystem

GridSnake's version moves three things out of the game mode, which dies with each map:

- **Which level of the run this is.** Chapter 13 looked the map up in the level list in the game
  mode's `StartPlay` and kept the result in `AGSGameMode::LevelIndex`. That lookup moves into the
  subsystem.
- **The run score**: points banked from completed levels. Nothing kept it before; each level
  started at 0.
- **Opening maps**, with a fade out before and a fade in after, and a loading screen in between.

The game state still keeps *this level's* score. The HUD in chapter 15 shows the sum.

### GSLevelFlowSubsystem.h

`Source/GridSnake/Core/GSLevelFlowSubsystem.h` (new):

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GSLevelFlowSubsystem.generated.h"

struct FGSLevelInfo;

/**
 * Moves the player through the level list and remembers the run while maps
 * come and go. A game instance subsystem is created with the game instance
 * and lives until the game quits, so its members survive OpenLevel.
 *
 * Get it anywhere with GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>().
 */
UCLASS()
class UGSLevelFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Starts a new run at a level: resets the run score and loads the map. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	void StartRun(int32 LevelIndex);

	/** Banks a finished level's score into the run. Returns true if another level follows. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	bool CompleteLevel(int32 LevelScore);

	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	bool HasNextLevel() const;

	/** Loads the level after the current one. Does nothing after the last level. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	void OpenNextLevel();

	/** Loads the current level again. The run score is kept; the level's own score is lost. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	void RestartLevel();

	/**
	 * Called by the game mode when a level begins. Works out which level this
	 * is (so Play In Editor on any listed map works) and fades the view in.
	 */
	void NotifyLevelStarted(UWorld* World);

	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	int32 GetCurrentLevelIndex() const { return CurrentLevelIndex; }

	/** Points banked from completed levels in this run. */
	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	int32 GetRunScore() const { return RunScore; }

	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	bool IsTransitioning() const { return bTransitioning; }

	/** nullptr when the current map is not in the level list. */
	const FGSLevelInfo* GetCurrentLevelInfo() const;

	/** Seconds for the fade to and from black around a level change. */
	static constexpr float FadeSeconds = 0.4f;

private:
	/** Fades out, then opens the map. The fade hides the last frame of the old level. */
	void OpenLevelWithFade(const TSoftObjectPtr<UWorld>& Map);
	void OpenPendingLevel();

	/** Shows a loading screen while the engine loads a map (packaged/standalone games only). */
	void HandlePreLoadMap(const FString& MapName);

	TSoftObjectPtr<UWorld> PendingMap;
	FTimerHandle TransitionTimer;
	int32 CurrentLevelIndex = INDEX_NONE;
	int32 RunScore = 0;
	bool bTransitioning = false;
};
```

- **`struct FGSLevelInfo;`** is a forward declaration (chapter 5): the header only uses pointers to
  the struct, so it does not need to include `Data/GSLevelList.h`.
- **The public functions read like the game's flow**: start a run, complete a level, open the next
  one, restart this one. The game mode calls them now; a menu Blueprint can call them in chapter 16,
  which is why most are `UFUNCTION`s.
- **`GetCurrentLevelInfo` is not a `UFUNCTION`.** It returns a raw pointer to a struct inside the
  level list, and Blueprints cannot use struct pointers. C++ callers check it for `nullptr`, which
  means "the current map is not in the level list" (a test map).
- **`FadeSeconds` is a `static constexpr` constant.** Subsystems are created by the engine, not
  placed or derived as Blueprints, so there is no Details panel in which to edit a `UPROPERTY`. A
  value that should be editable belongs in `UGSSettings` (chapter 13 did that for the level list).
- **The state is plain data**: a soft pointer to the map being opened, a timer handle, an index,
  a score and a flag. No actor pointers, as the first section of this chapter advised.

### GSLevelFlowSubsystem.cpp

`Source/GridSnake/Core/GSLevelFlowSubsystem.cpp` (new):

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSLevelFlowSubsystem.h"
#include "GridSnake.h"
#include "Camera/PlayerCameraManager.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GridSnakeLevelFlow"

namespace
{
	/**
	 * The loading screen is drawn by the movie player on another thread while the
	 * game thread is busy loading, so it must be plain Slate: no UObjects, no UMG.
	 */
	TSharedRef<SWidget> MakeLoadingScreen(const FText& Message)
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::Black)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Message)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
				.ColorAndOpacity(FLinearColor::White)
			];
	}
}

void UGSLevelFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// PreLoadMap fires on the game thread just before a map starts loading.
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UGSLevelFlowSubsystem::HandlePreLoadMap);
	UE_LOG(LogGridSnake, Log, TEXT("Level flow subsystem initialised"));
}

void UGSLevelFlowSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);

	Super::Deinitialize();
}

void UGSLevelFlowSubsystem::StartRun(int32 LevelIndex)
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	const FGSLevelInfo* Level = LevelList ? LevelList->GetLevel(LevelIndex) : nullptr;
	if (!Level)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("StartRun: no level %d"), LevelIndex);
		return;
	}

	RunScore = 0;
	CurrentLevelIndex = LevelIndex;
	OpenLevelWithFade(Level->Map);
}

bool UGSLevelFlowSubsystem::CompleteLevel(int32 LevelScore)
{
	RunScore += LevelScore;
	UE_LOG(LogGridSnake, Log, TEXT("Level %d complete: +%d, run score %d"), CurrentLevelIndex, LevelScore, RunScore);
	return HasNextLevel();
}

bool UGSLevelFlowSubsystem::HasNextLevel() const
{
	// An unlisted map has index INDEX_NONE (-1), so its "next level" is the first one.
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	return LevelList && LevelList->GetLevel(CurrentLevelIndex + 1) != nullptr;
}

void UGSLevelFlowSubsystem::OpenNextLevel()
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	const FGSLevelInfo* Next = LevelList ? LevelList->GetLevel(CurrentLevelIndex + 1) : nullptr;
	if (!Next)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("OpenNextLevel: level %d was the last one"), CurrentLevelIndex);
		return;
	}

	++CurrentLevelIndex;
	OpenLevelWithFade(Next->Map);
}

void UGSLevelFlowSubsystem::RestartLevel()
{
	if (const FGSLevelInfo* Level = GetCurrentLevelInfo())
	{
		OpenLevelWithFade(Level->Map);
		return;
	}

	// Not a listed level (a test map, say): reload it by name.
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(GetGameInstance(), /*bRemovePrefixString=*/true);
	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*LevelName));
}

void UGSLevelFlowSubsystem::NotifyLevelStarted(UWorld* World)
{
	bTransitioning = false;

	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	const FName LevelId(*UGameplayStatics::GetCurrentLevelName(World, /*bRemovePrefixString=*/true));
	CurrentLevelIndex = LevelList ? LevelList->FindLevelIndex(LevelId) : INDEX_NONE;
	UE_LOG(LogGridSnake, Log, TEXT("Level %s started (index %d, run score %d)"), *LevelId.ToString(), CurrentLevelIndex, RunScore);

	// Start black and fade in, so the new level never pops onto the screen.
	if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController(World))
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeSeconds, FLinearColor::Black, /*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/false);
		}
	}
}

const FGSLevelInfo* UGSLevelFlowSubsystem::GetCurrentLevelInfo() const
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	return LevelList ? LevelList->GetLevel(CurrentLevelIndex) : nullptr;
}

void UGSLevelFlowSubsystem::OpenLevelWithFade(const TSoftObjectPtr<UWorld>& Map)
{
	if (bTransitioning)
	{
		return;
	}

	bTransitioning = true;
	PendingMap = Map;

	if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeSeconds, FLinearColor::Black, /*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/true);
		}
	}

	// The game instance's timer manager outlives every world, unlike GetWorldTimerManager().
	GetGameInstance()->GetTimerManager().SetTimer(TransitionTimer,
		FTimerDelegate::CreateUObject(this, &UGSLevelFlowSubsystem::OpenPendingLevel), FadeSeconds, /*bLoop=*/false);
}

void UGSLevelFlowSubsystem::OpenPendingLevel()
{
	UE_LOG(LogGridSnake, Log, TEXT("Opening %s"), *PendingMap.ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), PendingMap);
}

void UGSLevelFlowSubsystem::HandlePreLoadMap(const FString& MapName)
{
	// The movie player only runs in standalone and packaged games, never in
	// Play In Editor, so test the loading screen with Standalone Game.
	if (IsRunningDedicatedServer() || !IsMoviePlayerEnabled())
	{
		return;
	}

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.MinimumLoadingScreenDisplayTime = 1.0f;
	const FGSLevelInfo* Level = GetCurrentLevelInfo();
	const FText Message = Level
		? FText::Format(LOCTEXT("LoadingLevel", "Loading {0}..."), Level->DisplayName)
		: LOCTEXT("Loading", "Loading...");
	LoadingScreen.WidgetLoadingScreen = MakeLoadingScreen(Message);
	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

#undef LOCTEXT_NAMESPACE
```

Function by function:

- **`Initialize` and `Deinitialize`** bind and unbind `HandlePreLoadMap` to the engine's
  `PreLoadMap` delegate, exactly like `UExTravelLogger`. The loading screen is set up there; its
  section comes after the fades.
- **`StartRun`** resets the run score and opens a level of the list with a fade. The game mode
  calls it with 0 after the last level; a level select menu could call it with any index.
- **`CompleteLevel`** banks a level's score and logs it. It does not open anything: the game mode
  decides *when* to move on (after its `LevelEndDelay`). Keeping "record the result" separate from
  "go somewhere" also gives chapter 18 a single place to save progress.
- **`HasNextLevel`** asks the level list for the entry after the current one. `GetLevel` returns
  `nullptr` for an index out of range, so the last level has no next one. On an unlisted map the
  index is `INDEX_NONE` (-1), and its "next level" is level 0.
- **`OpenNextLevel`** moves the index on *before* the map opens, so that while the next map loads,
  the subsystem already describes it. The loading screen relies on that to show the right name.
- **`RestartLevel`** opens the same map again with a fade. The run score is untouched, and the new
  map comes with a new game state whose score is 0. So dying in level 2 costs you level 2's points,
  not level 1's. An unlisted map is reloaded by name, without a fade, as the game mode did
  before.
- **`NotifyLevelStarted`** is the new world reporting in. It clears `bTransitioning`, works out the
  index from the map's name and starts the fade in (next section).
- **`OpenLevelWithFade`** starts a fade to black, remembers the destination and sets a timer that
  calls `OpenPendingLevel` when the fade is complete. `bTransitioning` makes a second call during a
  transition do nothing, so a death and an exit in the same moment cannot open two maps.
- **`OpenPendingLevel`** logs and calls `OpenLevelBySoftObjectPtr`. The engine travels on its next
  tick.

#### The map is the truth

`NotifyLevelStarted` does not trust the index it set before the travel. It looks the new map up in
the level list again, by name, as chapter 13's game mode did. That has three benefits:

- **Play In Editor works on any map.** Open `L_Level02` and press Play: the subsystem is brand new
  (index -1), but the lookup finds index 1. Only the run score is wrong, 0 instead of whatever
  level 1 would have earned, because no run led here. That is expected.
- **The console works.** `open L_Level03` moves the index to 2 even though nothing called
  `OpenNextLevel`.
- **Mistakes do not accumulate.** `OpenNextLevel` increments the index even when
  `OpenLevelWithFade` ignores the call because a transition is already running. The next
  `NotifyLevelStarted` corrects the index from the map that actually opened.

#### The timer on the game instance

`OpenLevelWithFade` waits for the fade with a timer. The subsystem is not an actor, so it has no
`GetWorldTimerManager()`. It uses the game instance's timer manager instead, which the API
reference describes as the global timer manager, not tied to one world. The binding is
`FTimerDelegate::CreateUObject(this, &UGSLevelFlowSubsystem::OpenPendingLevel)`: a delegate
(chapter 11) bound to a `UObject`'s member function, held weakly, so it is skipped if the subsystem
is gone.

The rule behind the choice: **the timer belongs to the object that survives.** A world timer on the
game mode would also fire in time (`ExFadeTravel` below does exactly that, on an actor), because
the travel happens after the timer, never during it. But the subsystem started the transition, and
the subsystem is what remains when it ends.

Like every gameplay timer, it does not advance while the game is paused. Chapter 16 adds pausing
and has to keep that in mind.

### The game mode asks instead of deciding

`Source/GridSnake/Core/GSGameMode.h`:

```diff
 class AGSGameState;
 class AGSGem;
 class AGSSnake;
+class UGSLevelFlowSubsystem;
 ...
-	/** Restarts the current level from scratch by loading its map again. */
+	/** Asks the level flow subsystem to load this level again. */
 	void RestartLevel();
 
-	/** Loads the level after this one in the level list (or the first, after the last). */
+	/** Asks the level flow subsystem for the next level, or starts a new run after the last. */
 	void LoadNextLevel();
 
-	/** This map's position in the level list, or INDEX_NONE for a map that is not listed. */
-	UPROPERTY(VisibleInstanceOnly, Category = "Rules")
-	int32 LevelIndex = INDEX_NONE;
-
 	UPROPERTY(VisibleInstanceOnly, Category = "Rules")
 	int32 PointsPerGem = 100;
 
 	AGSGameState* GetGSGameState() const;
 	AGSSnake* GetSnake() const;
+	UGSLevelFlowSubsystem* GetLevelFlow() const;
```

`Source/GridSnake/Core/GSGameMode.cpp`:

```diff
 #include "Core/GSGameState.h"
+#include "Core/GSLevelFlowSubsystem.h"
 #include "Core/GSPlayerController.h"
 #include "Core/GSSettings.h"
 #include "Data/GSLevelList.h"
+#include "Engine/GameInstance.h"
 ...
 void AGSGameMode::StartPlay()
 {
+	// The subsystem outlives this game mode: it knows which level of the run
+	// this is and fades the view in. Tell it before Super::StartPlay() calls
+	// BeginPlay on every actor, so they can already ask which level this is.
+	UGSLevelFlowSubsystem* LevelFlow = GetLevelFlow();
+	LevelFlow->NotifyLevelStarted(GetWorld());
+
 	Super::StartPlay();
 
-	// Look this map up in the level list. bRemovePrefixString strips the
-	// "UEDPIE_0_" prefix that Play In Editor adds to map names.
 	const UGSSettings* Settings = UGSSettings::Get();
-	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
-	const FName LevelId(*UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true));
-	LevelIndex = LevelList ? LevelList->FindLevelIndex(LevelId) : INDEX_NONE;
-	const FGSLevelInfo* LevelInfo = LevelList ? LevelList->GetLevel(LevelIndex) : nullptr;
-
+	const FGSLevelInfo* LevelInfo = LevelFlow->GetCurrentLevelInfo();
 	PointsPerGem = LevelInfo ? LevelInfo->PointsPerGem : Settings->PointsPerGem;
-	UE_LOG(LogGridSnake, Log, TEXT("Level %s: list index %d, %d points per gem"), *LevelId.ToString(), LevelIndex, PointsPerGem);
 ...
 	Snake->StopMoving();
 	State->SetPhase(EGSLevelPhase::Completed);
-	UE_LOG(LogGridSnake, Log, TEXT("Level complete with %d points"), State->GetScore());
+	GetLevelFlow()->CompleteLevel(State->GetScore());
 
 	GetWorldTimerManager().SetTimer(LevelEndTimer, this, &AGSGameMode::LoadNextLevel, LevelEndDelay, /*bLoop=*/false);
 }
```

`RestartLevel` and `LoadNextLevel` lose their map lookups and log lines; what remains is a request
to the subsystem. They now read, together with the new `GetLevelFlow` helper:

```cpp
void AGSGameMode::RestartLevel()
{
	GetLevelFlow()->RestartLevel();
}

void AGSGameMode::LoadNextLevel()
{
	UGSLevelFlowSubsystem* LevelFlow = GetLevelFlow();
	if (LevelFlow->HasNextLevel())
	{
		LevelFlow->OpenNextLevel();
	}
	else
	{
		// Chapter 16 shows a "you win" screen here; for now, go round again.
		LevelFlow->StartRun(0);
	}
}

UGSLevelFlowSubsystem* AGSGameMode::GetLevelFlow() const
{
	return GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
}
```

- **`NotifyLevelStarted` comes before `Super::StartPlay()`.** `AGameModeBase::StartPlay` is what
  calls `BeginPlay` on every actor in the world. Reporting in first means any actor's `BeginPlay`
  (the HUD's, in chapter 15) can already ask the subsystem which level this is. The player
  controller and its camera manager exist already, because players log in before the world begins
  play (step 6 before step 7 of the `LoadMap` list), so the fade in can start here.
- **The game mode keeps its timing.** It still waits `LevelEndDelay` (1.5 s) on its own world timer
  after the exit or a death, so the player sees what happened. Only then does it ask the
  subsystem to move on. The world timer is fine here: the game mode is alive when it fires.
- **`GetLevelFlow` never returns `nullptr` in GridSnake.** The subsystem has no
  `ShouldCreateSubsystem` override, so every game instance has one. If you add one, check the
  result.
- **The `LevelIndex` property is gone.** The game mode no longer knows its place in the run; it
  asks. One owner per piece of state.

### The run, step by step

Finish level 1 in Play In Editor, and this is what happens, with times measured from reaching the
exit:

```text
t = 0.0 s    The snake reaches the exit. OnExitReached: phase Completed, CompleteLevel logs
             "Level 0 complete: +300, run score 300". World timer: LoadNextLevel in 1.5 s.
t = 1.5 s    LoadNextLevel: HasNextLevel is true, so OpenNextLevel: index 0 -> 1.
             OpenLevelWithFade: fade to black over 0.4 s (held), game instance timer 0.4 s.
t = 1.9 s    OpenPendingLevel logs "Opening .../L_Level02.L_Level02" and requests the travel.
next frame   LoadMap: PreLoadMap (loading screen, standalone only); level 1's actors end play;
             garbage collection; L_Level02 loads; a new game mode, game state, controller, snake.
             StartPlay -> NotifyLevelStarted: "Level L_Level02 started (index 1, run score 300)",
             fade from black over 0.4 s.
then         The 3 s countdown runs while the picture fades in. Level 2 plays.
```

Level 2's three gems are worth 150 each, so finishing it banks 450 for a run score of 750. Level 3
banks 600 (200 each), 1350 in all, and then `StartRun(0)` begins a new run at level 1 with the run
score back at 0. Die in level 2 instead, and `RestartLevel` fades out and reopens `L_Level02`: run
score still 300, level score 0.

## Fades

The cheapest way to make a level change look intentional is to fade the picture to black before it
and back from black after it. The **player camera manager** (`APlayerCameraManager`, chapter 7's
"who is who": every player controller spawns one, and it computes the final view) can draw a
full-screen colour over the view, with an opacity it animates for you:

```cpp
// Camera/PlayerCameraManager.h
void StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FLinearColor Color,
	bool bShouldFadeAudio = false, bool bHoldWhenFinished = false);
void StopCameraFade();
```

- **Alpha** is the colour's opacity: `0` is the clear game view, `1` is solid colour. A fade *out*
  goes from 0 to 1; a fade *in* goes from 1 to 0.
- **`Duration`** is in seconds of game time.
- **`bHoldWhenFinished`** keeps the final alpha on screen once the fade completes. Without it, the
  fade ends by switching itself off and the picture snaps back to clear. A fade out before a level
  change therefore needs `true`; a fade in does not care, since its final alpha is 0 anyway.
- **`bShouldFadeAudio`** fades the game's sound volume along with the picture. More on it below.
- **`StopCameraFade`** removes a fade at once.

Get the camera manager from the player controller (`PlayerController->PlayerCameraManager`) or with
`UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0)`. In Blueprints the node is
**Start Camera Fade**, with the camera manager as its target.

### Every world needs a fade in

The camera manager is an actor: it belongs to the player controller, which belongs to the world.
A map change destroys the faded-out camera manager, and the new world's player controller spawns a
new one that knows nothing about any fade. If you only fade out, the sequence is "fade to black,
freeze, *pop*: the new level at full brightness". So a fade out before a map change must be paired
with a fade in that the *new* world starts.

That is why `UGSLevelFlowSubsystem::NotifyLevelStarted`, which the game mode calls from `StartPlay`
before the first frame of the new level is drawn, starts a fade from 1 to 0. Starting at alpha 1
means the first frame is already black. Run the same code on a map you opened without a fade (Play
In Editor on `L_Level01`) and you get a short fade in at the start, which looks fine too.

### Why wait for the fade

`OpenLevelWithFade` does not call `OpenLevel` straight away. It waits `FadeSeconds` first. While
the engine loads the new map, the game thread does nothing else, so no new frames are drawn: the
window keeps showing whatever was drawn last (unless a loading screen covers it, next section). If
the fade has not reached 1 yet, that last frame is a half-faded game, frozen. Waiting for the full
duration makes the last frame black.

### Fading the sound

With `bShouldFadeAudio = true`, the camera manager scales the game's overall sound volume by the
same alpha, so sound dips with the picture. GridSnake passes `true` in both directions, so the
fade in restores the volume that the fade out took away.

Be careful with this flag across a map change. The volume it controls is not part of the world, so
it is not reset when the world is replaced. If you fade out with audio and the next map never fades
in with audio, the game can stay silent. Pair the flags: a fade out with `bShouldFadeAudio = true`
needs a fade in with `true` on the other side. Chapter 17 adds music that survives level changes
and comes back to this interaction.

### Example: fade, travel, fade in

This example does the whole transition in a single actor, with no subsystem: fade in when the
level starts, fade out on request, then travel.

`examples/ch14/ExFadeTravel.h`

```cpp
// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExFadeTravel.generated.h"

/**
 * Fades in when its level starts; Go() fades to black, then opens NextMap.
 * Place one in each of two levels, point their NextMap at each other, press
 * Play. With AutoGoSeconds > 0 it goes by itself; Go() can also be called
 * from a Blueprint, for example from a trigger's overlap event.
 */
UCLASS()
class AExFadeTravel : public AActor
{
	GENERATED_BODY()

public:
	/** Soft reference: the next map is loaded by the travel, not with this level. */
	UPROPERTY(EditAnywhere, Category = "Travel")
	TSoftObjectPtr<UWorld> NextMap;

	UPROPERTY(EditAnywhere, Category = "Travel", meta = (ClampMin = "0.05", Units = "s"))
	float FadeSeconds = 0.5f;

	/** Starts this level black and fades in. Put one of these in every map you travel to. */
	UPROPERTY(EditAnywhere, Category = "Travel")
	bool bFadeInOnBeginPlay = true;

	/** Calls Go() this long after BeginPlay. 0 = wait for a Blueprint to call Go(). */
	UPROPERTY(EditAnywhere, Category = "Travel", meta = (ClampMin = "0", Units = "s"))
	float AutoGoSeconds = 2.0f;

	UFUNCTION(BlueprintCallable, Category = "Travel")
	void Go();

protected:
	virtual void BeginPlay() override;

private:
	void Travel();

	FTimerHandle Timer;
	bool bGoing = false;
};
```

`examples/ch14/ExFadeTravel.cpp`

```cpp
// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExFadeTravel.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void AExFadeTravel::BeginPlay()
{
	Super::BeginPlay();

	// Every new world starts with a new, unfaded camera manager: fade in from black
	// before the first frame is drawn, so the level never pops onto the screen.
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (bFadeInOnBeginPlay && CameraManager)
	{
		CameraManager->StartCameraFade(1.0f, 0.0f, FadeSeconds, FLinearColor::Black,
			/*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/false);
	}

	if (AutoGoSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(Timer, this, &AExFadeTravel::Go, AutoGoSeconds, /*bLoop=*/false);
	}
}

void AExFadeTravel::Go()
{
	if (bGoing)
	{
		return;
	}
	if (NextMap.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: pick a NextMap"), *GetName());
		return;
	}
	bGoing = true;

	// From alpha 0 (clear) to 1 (solid black), and hold it there: the last
	// frame drawn before the load is black, not a frozen game.
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CameraManager->StartCameraFade(0.0f, 1.0f, FadeSeconds, FLinearColor::Black,
			/*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/true);
	}
	UE_LOG(LogTemp, Log, TEXT("Fading out over %.2f s"), FadeSeconds);

	// A world timer is fine here: it fires while this world, and this actor,
	// still exist. Travel happens after the timer, never during it.
	GetWorldTimerManager().SetTimer(Timer, this, &AExFadeTravel::Travel, FadeSeconds, /*bLoop=*/false);
}

void AExFadeTravel::Travel()
{
	UE_LOG(LogTemp, Log, TEXT("Opening %s"), *NextMap.ToString());

	// Only requests the travel; the engine loads the map at the start of the next frame.
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, NextMap);
	UE_LOG(LogTemp, Log, TEXT("OpenLevel returned; this actor lives until the next frame"));
}
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`.

To see it:

1. Copy both files into the scratch project with `ExMapA` and `ExMapB` (Basic levels, from the
   first example) and build from the IDE.
2. Open `ExMapA`. Drag `ExFadeTravel` from **C++ Classes** into the level. In the Details panel,
   under **Travel**, set **Next Map** = `ExMapB`. Save.
3. Open `ExMapB`. Place another `ExFadeTravel`, set **Next Map** = `ExMapA`, and set **Auto Go
   Seconds** = `0` so that the trip ends there. Save.
4. Open `ExMapA` and press Play. After two seconds the view fades to black, `ExMapB` opens and
   fades in, and stays.
5. Set **Auto Go Seconds** back to `2` in `ExMapB` and play again to watch the two maps alternate
   every 2.5 seconds or so, until you press Stop.

Expected Output Log (filter: `LogTemp`) for steps 1 to 4; the map path depends on your folders:

```text
LogTemp: Fading out over 0.50 s
LogTemp: Opening /Game/Maps/ExMapB.ExMapB
LogTemp: OpenLevel returned; this actor lives until the next frame
```

- **The third line proves the point of the "OpenLevel does not open the level" section.** The
  call returned, the actor ran its next line, and its world was still intact.
- **`bGoing` makes `Go` safe to call twice.** A trigger that overlaps twice, or a Blueprint that
  calls `Go` while the timer is still running, starts only one fade.
- **Each map needs its own `ExFadeTravel`** for the fade in, which is why `bFadeInOnBeginPlay` is
  a property of the actor that lives in the destination. GridSnake avoids that per-map chore by
  having the game mode, which every GridSnake map has, report to the subsystem.
- **`NextMap.ToString()`** prints the soft pointer's object path: the package
  (`/Game/Maps/ExMapB`), a dot, and the asset inside it (`ExMapB`).
- With the probes from the first example still placed, you also see their `LevelTransition` lines
  after `OpenLevel returned`, on the next frame.

## Loading screens

During step 4 of `LoadMap`, loading the package, the game thread is busy. GridSnake's maps load in
a fraction of a second, but a real game's levels can take many seconds. In that time nothing ticks
and the game's renderer is not fed new frames; to the player the game looks frozen, and an
operating system may even call it unresponsive.

The fix is the **movie player** (the `MoviePlayer` module). Despite its name it does two things: it
plays startup and loading movies, and it shows a *Slate* widget, Unreal's C++ UI framework that UMG
is built on. It draws on its own thread while the game thread loads, so it keeps animating and
responding however long the load takes.

Using it takes one call at the right moment. `HandlePreLoadMap` in the listing above fills in an
`FLoadingScreenAttributes` (header `MoviePlayer.h`) and passes it to
`GetMoviePlayer()->SetupLoadingScreen(...)`. `GetMoviePlayer()` returns the engine's movie player;
`SetupLoadingScreen` describes the loading screen that should cover the coming load, and the movie
player shows it while the map loads and takes it down afterwards. The right moment is `PreLoadMap`,
"the very beginning of LoadMap". A game instance subsystem is a natural place for the handler: it
exists before the first map loads and through every load after it.

### The attributes

`FLoadingScreenAttributes` collects the options. The ones you are most likely to use:

| Field | Meaning |
|---|---|
| `WidgetLoadingScreen` | The Slate widget to show. GridSnake's is a black border with centred text. |
| `MinimumLoadingScreenDisplayTime` | Show it at least this long, in seconds, even if loading finishes sooner. Avoids a one-frame flash. |
| `bAutoCompleteWhenLoadingCompletes` | Take it down when the map has loaded (and the minimum time has passed). |
| `bWaitForManualStop` | Keep it up until your code stops it, for example after the player presses a key. |
| `MoviePaths` | Movies to play, by file name without extension, from the project's `Content/Movies` folder. |
| `PlaybackType` | How several movies play: in order, looped, or with the last one looping while loading. |
| `bMoviesAreSkippable` | Whether a click or key press skips the movie. |
| `bAllowEngineTick` | Lets the engine keep ticking while the movie plays. Leave it off for map loads. |

GridSnake shows its widget for at least one second and lets it close by itself. There are no
movies. The minimum time is a trade-off: GridSnake's maps load almost instantly, so in a standalone
game every level change takes about one second longer than it has to, in exchange for a loading
screen that can be read. Try `0` and see whether you prefer the flash.

### Slate, briefly

The movie player's thread may not touch `UObject`s: the game thread is loading packages and
collecting garbage at the same time. A UMG widget (chapter 15) is a `UObject`, so the loading
screen is built from Slate widgets, which are plain C++ objects held by shared pointers. That is
also why the module now depends on `Slate` and `SlateCore`.

```cpp
TSharedRef<SWidget> MakeLoadingScreen(const FText& Message)
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor::Black)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(Message)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
			.ColorAndOpacity(FLinearColor::White)
		];
}
```

Slate's syntax is unusual but regular:

- **`SNew(SBorder)`** creates a widget and returns a builder. Widget classes start with `S`.
- **`.Name(Value)`** sets an *argument*. They can be chained in any order.
- **`[ ... ]`** fills the widget's *slot* with a child widget. A border has one slot; panels such
  as `SVerticalBox` add slots with `+ SVerticalBox::Slot()[ ... ]`.
- The result converts to `TSharedRef<SWidget>`, a shared pointer that is never null.

Here the `SBorder` fills the whole screen. Its `BorderImage` is a plain white brush from Slate's
core style, tinted black by `BorderBackgroundColor`, and it centres its one child: a 32-point bold
white `STextBlock`. The brush name `"WhiteBrush"` is looked up by name at run time. If your engine
version's core style does not have it, the border draws a placeholder pattern instead of black; in
that case use a solid colour brush that needs no lookup. Make it a `static const FSlateColorBrush
BlackBrush(FLinearColor::Black);` inside `MakeLoadingScreen` (header `Brushes/SlateColorBrush.h`),
pass `.BorderImage(&BlackBrush)`, and drop `BorderBackgroundColor`.

The message is an `FText`, so it can be translated (chapter 13 introduced `FText`):

- **`LOCTEXT("LoadingLevel", "Loading {0}...")`** declares a translatable text with a key and its
  source string. `LOCTEXT` needs a namespace, given by `#define LOCTEXT_NAMESPACE
  "GridSnakeLevelFlow"` near the top of the file and `#undef LOCTEXT_NAMESPACE` at the end.
- **`FText::Format(Pattern, Level->DisplayName)`** replaces `{0}` with the level's display name
  from the level list, giving "Loading The Maze...". Translators can move `{0}` within the sentence,
  which string concatenation would not allow.

The level list is where the name comes from: `GetCurrentLevelInfo()` already describes the
*destination*, because `OpenNextLevel` and `StartRun` set the index before they open the map. A map
opened another way (the `open` console command, or `RestartLevel` on an unlisted map) shows either
the old level's name or the plain `Loading...`, since the index is only corrected once the new map
has started.

### Not in Play In Editor

**The movie player does not run in Play In Editor.** There, `IsMoviePlayerEnabled()` returns false,
`HandlePreLoadMap` returns at its first line, and you will never see a loading screen however long
you look. This is the most common "my loading screen does not work" report. To see it, run the game
as its own process:

1. In the toolbar, open the drop-down next to the **Play** button and choose **Standalone Game**
   (chapter 1). Or, from a terminal, run `UnrealEditor GridSnake.uproject -game` with the path to
   your engine's `UnrealEditor` and to the project.
2. Play `L_Level01` through to the exit.
3. The screen fades to black, shows **Loading The Maze...** in white for about a second, then
   level 2 fades in.

A standalone game writes its log to `Saved/Logs/GridSnake.log` (or `GridSnake_2.log`), not to the
editor's Output Log (chapter 2). Adding `-log` to the command line also opens a log window.

In a standalone game the first map is loaded with `LoadMap` too, after the game instance and its
subsystems have been created. So you may see a plain **Loading...** before `L_Level01` (no level
is current yet, so there is no name). Depending on your project settings the engine may also show
its own startup movie or splash screen before that.

The other early return, `IsRunningDedicatedServer()`, matters only for multiplayer servers, which
have no screen to show anything on.

## Project milestone: a run across levels

Files changed in `project/ch14` compared with `project/ch13`:

| File | Change |
|---|---|
| `Source/GridSnake/Core/GSLevelFlowSubsystem.h/.cpp` | New: the run state, fades, the loading screen |
| `Source/GridSnake/Core/GSGameMode.h/.cpp` | Asks the subsystem; `LevelIndex` removed |
| `Source/GridSnake/GridSnake.Build.cs` | Depends on `MoviePlayer`, `Slate`, `SlateCore` |
| `ASSETS.md` | No new assets; editor steps for this chapter |

`Source/GridSnake/GridSnake.Build.cs`:

```diff
-		PrivateDependencyModuleNames.AddRange(new string[] { });
+		// Used only inside our .cpp files: the loading screen (MoviePlayer) and the
+		// Slate widget types it is built from.
+		PrivateDependencyModuleNames.AddRange(new string[] {
+			"MoviePlayer",
+			"Slate",
+			"SlateCore"
+		});
```

They are *private* dependencies (chapter 2) because only `GSLevelFlowSubsystem.cpp` includes their
headers. Nothing in a GridSnake header exposes a Slate or movie player type, so a module that
included GridSnake's headers would not need them.

### Editor steps

There are no new assets: the fade belongs to the camera manager, the loading screen is built in
C++, and the subsystem creates itself.

1. **Build.** Close the editor and build from your IDE: the module has a new class and new
   dependencies, which Live Coding cannot add (chapter 2). Reopen the project.
2. **Play `L_Level01` in the viewport.** Collect the three gems and reach the exit. After the
   1.5-second pause, the picture fades to black, `L_Level02` opens and fades in under the
   countdown.
3. **Check the run score in the log** (below). Then die on purpose in level 2: the level fades out
   and starts again, and the run score in the `started` line is still 300.
4. **Finish all three levels.** After level 3 the run starts over at level 1 with a run score of 0.
5. **See the loading screen.** Choose **Standalone Game** from the Play button's drop-down and
   play through level 1. **Loading The Maze...** appears for about a second between the fade out
   and the fade in. Remember: never in Play In Editor.

Expected Output Log (filter: `LogGridSnake`), from pressing Play to the start of level 2. Only the
level flow lines are shown; the countdown, gem, phase and snake lines from earlier chapters appear
between them:

```text
LogGridSnake: Level flow subsystem initialised
LogGridSnake: GSGameInstance initialised: 3 gems per level, step 0.15s
LogGridSnake: Level L_Level01 started (index 0, run score 0)
LogGridSnake: Level 0 complete: +300, run score 300
LogGridSnake: Opening /Game/GridSnake/Maps/L_Level02.L_Level02
LogGridSnake: Level L_Level02 started (index 1, run score 300)
```

- **The subsystem's line comes first** because subsystems are initialised inside
  `UGameInstance::Init`, which `UGSGameInstance::Init` calls before logging. The order of these two
  lines is an engine detail, not something to rely on.
- **`Level 0 complete: +300`**: three gems at 100 points on level 1.
- **The `Opening` line is 0.4 s after the fade starts**, and the next line comes from a different
  game mode in a different world. Only the subsystem saw both.

Then level 2 (150 per gem) ends with `Level 1 complete: +450, run score 750`, level 3 (200 per gem)
with `Level 2 complete: +600, run score 1350`, and `Opening /Game/GridSnake/Maps/L_Level01.L_Level01`
starts a new run: `Level L_Level01 started (index 0, run score 0)`.

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `Level flow subsystem initialised` never appears | The new class was not compiled into the running editor | Close the editor, build from the IDE, reopen |
| Linker errors mentioning `GetMoviePlayer`, `SBorder`, `STextBlock` or `FCoreStyle` | `Build.cs` lacks `MoviePlayer`, `Slate` or `SlateCore` | Add them to `PrivateDependencyModuleNames`, regenerate project files if your IDE needs it, rebuild |
| The screen fades to black and stays black | The travel failed (a map in the list was renamed, moved or deleted outside the editor), so the old level keeps its held fade and `bTransitioning` stays set | Look for a warning after the `Opening` line; fix the map in `DA_LevelList` |
| Fade out works, but the next level pops in at full brightness | That map's game mode is not `AGSGameMode`, so nothing calls `NotifyLevelStarted` | **World Settings > GameMode Override** = None or `BP_GSGameMode` |
| No loading screen | You are in Play In Editor | **Standalone Game**, or `-game` on the command line |
| The loading screen shows a checkered or odd pattern instead of black | The `"WhiteBrush"` style name is not found in your engine version | Use a `static const FSlateColorBrush` as described in "Slate, briefly" |
| The loading screen names the wrong level | The map was opened with the console or by name, not through `OpenNextLevel`/`StartRun` | Expected; the index is corrected when the map starts |
| Run score 0 when you press Play on `L_Level02` | Each Play creates a new game instance, and no run led to level 2 | Expected; play from `L_Level01` to test a run |
| Silence after a level change | A fade out with `bShouldFadeAudio = true` was not followed by a fade in with `true` | Pair the flags, or pass `false` in both (chapter 17) |
| A crash or `nullptr` after a level change in code that kept an actor pointer in a subsystem or the game instance | The actor was destroyed with its world | Keep data (indexes, names, scores) and look actors up in the current world |
| Nothing happens after `open MyMap` or `OpenLevel` | The name is wrong, or not unique | Read the warning in the Output Log; use the full path or a soft pointer |
| A value is wrong on the second Play but right on the first | It lives in a static variable, which survives Stop | Move it into a subsystem or another owned object |

## Summary

- A map change destroys the world and every actor in it: game mode, game state, player controller,
  camera manager, pawn. Each actor gets `EndPlay` with `EEndPlayReason::LevelTransition`.
- The game instance, its subsystems and the local players survive a map change. In the editor,
  each Play creates a new game instance; static variables survive even Stop.
- Persistent objects hold data, not pointers to actors.
- `OpenLevel` and `OpenLevelBySoftObjectPtr` only request a travel. The engine performs it on its
  next tick, in `UEngine::LoadMap`, between the `PreLoadMap` and `PostLoadMapWithWorld` delegates.
- Travel options (`?Name=Value`) reach the new game mode's `OptionsString`. Multiplayer games use
  `ClientTravel` and `ServerTravel`, optionally seamless. Level streaming keeps the persistent level
  and loads sublevels around it; World Partition automates that for big worlds.
- Game instance subsystems are created and destroyed with the game instance, found with
  `GetSubsystem<T>()`, and keep the game instance class small.
- `StartCameraFade` fades the view; hold the fade out (`bHoldWhenFinished`) and start a fade in from
  1 in the new world, whose camera manager is new.
- The movie player shows a Slate loading screen on its own thread while the map loads. Set it up in
  `PreLoadMap`. It never runs in Play In Editor.
- GridSnake's `UGSLevelFlowSubsystem` keeps the level index and run score across maps, opens maps
  with a fade and a loading screen, and treats the opened map's name as the truth.

## Exercises

### 1. Announce the level

When a level starts, show its display name ("The Maze") on screen for three seconds, using the
on-screen debug messages from chapter 1, and add it to the `started` log line.

<details><summary>Solution</summary>

In `UGSLevelFlowSubsystem::NotifyLevelStarted`, after the index is known (add
`#include "Engine/Engine.h"` for `GEngine`):

```cpp
	const FGSLevelInfo* Level = GetCurrentLevelInfo();
	const FString DisplayName = Level ? Level->DisplayName.ToString() : FString(TEXT("(unlisted)"));
	UE_LOG(LogGridSnake, Log, TEXT("Level %s \"%s\" started (index %d, run score %d)"),
		*LevelId.ToString(), *DisplayName, CurrentLevelIndex, RunScore);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::White, DisplayName);
	}
```

Replace the existing `UE_LOG` line with the new one. `GetCurrentLevelInfo` works here because the
index has just been set from the map name. Debug messages are drawn on top of the camera fade, so
the name is readable even while the level fades in. Chapter 15 replaces this with a proper HUD
widget.

</details>

### 2. Lives

Give the player three lives per run. Dying costs a life and restarts the level; losing the last
life starts a new run at level 1. Add a `GetLives` function Blueprints can read.

<details><summary>Solution</summary>

`GSLevelFlowSubsystem.h`, in the public section:

```cpp
	static constexpr int32 StartingLives = 3;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	int32 GetLives() const { return Lives; }
```

and in the private section:

```cpp
	int32 Lives = StartingLives;
```

`GSLevelFlowSubsystem.cpp`:

```cpp
void UGSLevelFlowSubsystem::StartRun(int32 LevelIndex)
{
	...
	RunScore = 0;
	Lives = StartingLives;
	CurrentLevelIndex = LevelIndex;
	OpenLevelWithFade(Level->Map);
}

void UGSLevelFlowSubsystem::RestartLevel()
{
	// A restart already under way must not cost a second life.
	if (bTransitioning)
	{
		return;
	}

	--Lives;
	UE_LOG(LogGridSnake, Log, TEXT("Lives left: %d"), Lives);
	if (Lives <= 0)
	{
		StartRun(0);
		return;
	}

	// ... the existing body
}
```

- `Lives` starts at `StartingLives` in its declaration, not only in `StartRun`, because Play In
  Editor on any level never calls `StartRun`. With `0` as the starting value, the first death in a
  PIE session would end the run.
- `StartingLives` must be declared before `Lives` in the class, since `Lives` uses it.
- The lives survive the restart for the same reason the run score does: they live in the
  subsystem, not in the game state that is rebuilt with the map.

</details>

### 3. A colour per level

Add a fade colour to each level in the level list. Fade out to the *destination* level's colour,
use it as the loading screen's background, and fade in from it.

<details><summary>Solution</summary>

`Data/GSLevelList.h`, in `FGSLevelInfo`:

```cpp
	/** Colour of the fade and the loading screen around this level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FLinearColor FadeColor = FLinearColor::Black;
```

`GSLevelFlowSubsystem.h`, private:

```cpp
	FLinearColor GetFadeColor() const;
```

`GSLevelFlowSubsystem.cpp`:

```cpp
FLinearColor UGSLevelFlowSubsystem::GetFadeColor() const
{
	const FGSLevelInfo* Level = GetCurrentLevelInfo();
	return Level ? Level->FadeColor : FLinearColor::Black;
}
```

Replace `FLinearColor::Black` with `GetFadeColor()` in both `StartCameraFade` calls. In
`OpenLevelWithFade` the index already points at the destination (`OpenNextLevel` and `StartRun` set
it before calling), and for a restart it is the current level, which is also the destination. In
`NotifyLevelStarted` it is the level that just started.

For the loading screen, give `MakeLoadingScreen` a second parameter, `const FLinearColor& Color`,
use it for `BorderBackgroundColor`, and pass `GetFadeColor()` from `HandlePreLoadMap`. Set a dark
blue on `The Maze` in `DA_LevelList` and play through level 1. Keep the colours dark so the white
text stays readable.

</details>

### 4. Stream a room instead

In the scratch project, keep `ExMapA` loaded and stream `ExMapB` in beside it at run time, then out
again, with `ULevelStreamingDynamic`. With an `ExCounterProbe` in each map: which `EndPlay` reason
does `ExMapB`'s probe report, and does `ExMapA`'s probe end play at all? Would GridSnake's levels be
better as streamed levels?

<details><summary>Solution</summary>

Add an actor class `AExRoomLoader` to `ExMapA` (header `Engine/LevelStreamingDynamic.h`). In its
class declaration: `UPROPERTY(EditAnywhere) TSoftObjectPtr<UWorld> Room;`,
`UPROPERTY() TObjectPtr<ULevelStreamingDynamic> LoadedRoom;`, a `UFUNCTION() void HandleRoomShown();`
and a `void UnloadRoom();`. Then:

```cpp
void AExRoomLoader::BeginPlay()
{
	Super::BeginPlay();

	bool bSuccess = false;
	LoadedRoom = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(this, Room,
		FVector(3000.0f, 0.0f, 0.0f), FRotator::ZeroRotator, bSuccess);
	if (bSuccess && LoadedRoom)
	{
		LoadedRoom->OnLevelShown.AddDynamic(this, &AExRoomLoader::HandleRoomShown);
	}

	FTimerHandle UnloadTimer;
	GetWorldTimerManager().SetTimer(UnloadTimer, this, &AExRoomLoader::UnloadRoom, 5.0f, false);
}

void AExRoomLoader::HandleRoomShown()
{
	UE_LOG(LogTemp, Log, TEXT("Room is visible"));
}

void AExRoomLoader::UnloadRoom()
{
	if (LoadedRoom)
	{
		LoadedRoom->SetIsRequestingUnloadAndRemoval(true);
		LoadedRoom = nullptr;
	}
}
```

`LoadLevelInstanceBySoftObjectPtr` streams a *copy* of the map in at an offset, asynchronously:
`bSuccess` only says the request was made, and `OnLevelShown` (a dynamic multicast delegate with no
parameters) fires once the room is visible. `SetIsRequestingUnloadAndRemoval(true)` unloads it.
For sublevels set up in the **Levels** window, `UGameplayStatics::LoadStreamLevel` and
`UnloadStreamLevel` (or the Blueprint nodes) do the same by name.

`ExMapB`'s probe logs its `BeginPlay` when the room appears (the subsystem count goes up as usual)
and `EndPlay: RemovedFromWorld` when it goes. `ExMapA`'s probe does not end play until you press
Stop: the persistent level, its game mode and the player controller were never replaced.

For GridSnake, a map change is the better fit. Each level is a separate puzzle with its own board,
gems and Player Start; the game mode and game state are meant to start fresh; a restart is "load
the same map again"; and the maps are so small that loading costs nothing. Streaming would make
sense if levels were rooms of one connected world, or if a menu had to stay alive over them.

</details>

---

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-14--level-transitions-and-persistence)
