# Chapter 7 — The Gameplay Framework

Until now, pressing Play in `L_Level01` gave you a level full of actors and a free-flying default
camera. Nobody was *playing*. This chapter introduces the classes Unreal uses to turn a level into
a game. The **GameMode** holds the rules. The **GameState** holds the shared state of the match.
The **PlayerController** stands for the player's will, the **Pawn** is the body it drives, and the
**PlayerState** and **HUD** fill in the rest. You will see in which order the engine creates and
wires them when a level starts, and which accessor finds each one. GridSnake gains its snake pawn
`AGSSnake`, a player controller that looks through the board's camera, and a game state that
counts gems.

Previous: [Chapter 6](06-tile-maps-and-the-orthographic-camera.md). Next:
[Chapter 8](08-enhanced-input.md).

## The cast of classes

Epic calls this set of classes the **Gameplay Framework**. Each class answers one question, and
the answers do not overlap:

| Class | Answers | Exists on | GridSnake (this chapter) |
|---|---|---|---|
| `UGameInstance` | What lives for the whole run of the program? | every machine | `UGSGameInstance` (chapter 3) |
| `AGameModeBase` | What are the rules? Who may join, where do they spawn? | the server only | `AGSGameMode` |
| `AGameStateBase` | What is the state of the match that everyone may see? | server and every client | `AGSGameState` |
| `APlayerController` | What does this player want? What do they see? | server and the owning client | `AGSPlayerController` |
| `APlayerState` | What should everyone know about this player (name, score)? | server and every client | engine default |
| `APawn` | What body is the player driving right now? | server and every client | `AGSSnake` |
| `AHUD` | How is the old canvas HUD drawn? | the owning client only | engine default (UMG in chapter 15) |

They form a tree of ownership. The game instance outlives every level; everything else belongs to
one world and dies with it:

```text
UGameInstance                       one per run of the game, survives OpenLevel
└── UWorld                          one per loaded level
    ├── AGameModeBase               rules; spawns everything below
    ├── AGameStateBase              shared match state (gems, score)
    │   └── PlayerArray             every player's APlayerState
    └── APlayerController           one per player
        ├── APlayerState            this player's public data
        ├── APlayerCameraManager    computes the view each frame
        ├── AHUD                    canvas HUD (optional)
        └── APawn                   the possessed body (may change, may be none)
```

Two pairs are easy to confuse, so it is worth stating the split plainly:

- **GameMode vs GameState.** The game mode *decides*; the game state *remembers*. The game mode
  awards a gem; the game state stores the count so the HUD can draw it. In a networked game the
  game mode exists only on the server, so clients cannot even see the rules, only their result.
- **PlayerController vs Pawn.** The controller is the player; the pawn is a body. When the snake
  dies and a new one spawns, the controller survives. Anything that belongs to the *player* rather
  than to one body (the camera, input contexts, menus) lives in the controller.

### Why bother in a single-player game

GridSnake has one player and no network code. It could put every rule in the snake. It does not,
because the split gives each piece of code one obvious home:

- The rules ("three gems open the exit", "a death restarts the level") go in the game mode. You
  can swap the game mode per level in World Settings without touching the snake.
- What the HUD shows (gems, score) goes in the game state. The HUD only reads it and never needs
  to find the game mode.
- Camera and input contexts go in the controller. They belong to the player, not to one body.
- The snake only knows how to be a snake.

The split costs almost nothing: the engine spawns these classes anyway, and you only choose the
subclasses.

## What happens when a level starts

When a level loads, for example when you press Play, the engine creates the framework objects in
a fixed order. The game mode page in Epic's documentation describes the steps. Simplified for a
single player:

1. **The world is created** and its level actors (board, gems, exit) are loaded. They exist, but
   none has had `BeginPlay` yet.
2. **The game mode is spawned.** The engine reads the class from World Settings > **GameMode
   Override**, or else from Project Settings > Maps & Modes > **Default GameMode**. Its `InitGame`
   runs first.
3. **The game state is spawned** by the game mode, using its `GameStateClass`.
4. **The player joins.** The game mode's `Login` spawns a `PlayerControllerClass` actor, and
   `PostLogin` runs. The controller gets its player state, camera manager and HUD.
5. **`HandleStartingNewPlayer`** decides what the new player starts as. The default calls
   `RestartPlayer`.
6. **`RestartPlayer`** finds a start spot (`FindPlayerStart`, which asks `ChoosePlayerStart`),
   spawns the `DefaultPawnClass` there (`SpawnDefaultPawnFor`) and makes the controller
   **possess** it.
7. **`StartPlay`** begins the match. Inside it, every actor in the world gets `BeginPlay`: the
   level actors and the controller and pawn spawned in steps 4–6.

Step 7 has a consequence you will rely on all through the course: **by the time any `BeginPlay`
runs, the game mode, game state, player controller and possessed pawn all exist.** The snake can
look for the board in its `BeginPlay`, and the controller can look for the board in its own,
without worrying about who was created first.

### Watching the order

The best way to believe a sequence is to log it. This game mode overrides five steps; each logs
and then calls the parent version, so nothing changes except the log.

`examples/ch07/ExFrameworkGameMode.h`

```cpp
// Learn Unreal Engine 5 - chapter 7 example.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ExFrameworkGameMode.generated.h"

/**
 * A game mode that changes nothing: every override logs and calls Super, so the
 * Output Log shows the order in which a level starts and a player joins.
 * Use it as World Settings > GameMode Override.
 */
UCLASS()
class AExFrameworkGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** First call on a new game mode, before any other actor is initialised for play. */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** A player controller exists and has joined. Safe to call functions on it. */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Decides what a new player starts as. The default restarts the player, which spawns a pawn. */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** Spawns the Default Pawn Class at the start spot. */
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	/** Starts the match: every actor's BeginPlay runs inside this call. */
	virtual void StartPlay() override;
};
```

`examples/ch07/ExFrameworkGameMode.cpp`

```cpp
// Learn Unreal Engine 5 - chapter 7 example.

#include "ExFrameworkGameMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogExFramework, Log, All);

void AExFrameworkGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UE_LOG(LogExFramework, Log, TEXT("InitGame map=%s"), *MapName);
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AExFrameworkGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Log first: Super::PostLogin is what calls HandleStartingNewPlayer.
	UE_LOG(LogExFramework, Log, TEXT("PostLogin %s"), *GetNameSafe(NewPlayer));
	Super::PostLogin(NewPlayer);
}

void AExFrameworkGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UE_LOG(LogExFramework, Log, TEXT("HandleStartingNewPlayer %s"), *GetNameSafe(NewPlayer));
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

APawn* AExFrameworkGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// Log after Super, so we can print the pawn it spawned.
	APawn* Pawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	UE_LOG(LogExFramework, Log, TEXT("SpawnDefaultPawnFor -> %s at %s (start spot %s)"),
		*GetNameSafe(Pawn),
		Pawn ? *Pawn->GetActorLocation().ToString() : TEXT("-"),
		*GetNameSafe(StartSpot));
	return Pawn;
}

void AExFrameworkGameMode::StartPlay()
{
	UE_LOG(LogExFramework, Log, TEXT("StartPlay"));
	Super::StartPlay();
}
```

To see it:

1. Copy both files into the `Source/<YourModule>/` folder of any UE 5.8 C++ project (a scratch
   project from the Blank template is fine; do not put examples into GridSnake). Build.
2. Open any level that has a **Player Start**. If it has none, add one from **Place Actors >
   Basic > Player Start**.
3. Open **Window > World Settings**. Under **Game Mode**, set **GameMode Override** =
   `ExFrameworkGameMode`.
4. Open **Window > Output Log** and type `LogExFramework` in its search box.
5. Press **Play**.

Expected Output Log (filter: `LogExFramework`):

```text
LogExFramework: InitGame map=<map name>
LogExFramework: PostLogin <PlayerController_0>
LogExFramework: HandleStartingNewPlayer <PlayerController_0>
LogExFramework: SpawnDefaultPawnFor -> <DefaultPawn_0> at X=<x> Y=<y> Z=<z> (start spot <PlayerStart_0>)
LogExFramework: StartPlay
```

- **The order is the point.** `InitGame` comes before any player exists. The pawn is spawned and
  possessed *before* `StartPlay`, which is why every `BeginPlay` can rely on it.
- **Names vary.** In PIE the map name usually has a `UEDPIE_0_` prefix; actor suffixes vary.
- **`_Implementation`.** `HandleStartingNewPlayer` and `SpawnDefaultPawnFor` are
  `BlueprintNativeEvent`s (chapter 3 previewed them): a Blueprint game mode may override them too.
  C++ overrides the `_Implementation` function, and calling `Super::..._Implementation` runs the
  engine's version.
- **Log before or after `Super`.** `PostLogin` logs *before* calling `Super`, because the parent
  version is what calls `HandleStartingNewPlayer`. `SpawnDefaultPawnFor` logs *after* `Super`,
  because only then does the pawn exist. Where you put the call to `Super` decides what you can
  observe and change.
- **`GetNameSafe`** returns `"None"` for a null pointer instead of crashing.
- The pawn is a `DefaultPawn` because `AGameModeBase` uses `ADefaultPawn` as its Default Pawn
  Class. That is the flying sphere you have been using since chapter 1.

### The start spot

`FindPlayerStart` looks for `APlayerStart` actors in the level. Their only job is to mark a
location and rotation. `ChoosePlayerStart` prefers starts that are not blocked by geometry and
picks one if there are several. If the level has none, `FindPlayerStart` logs a message on the
`LogGameMode` category (in UE 5.8 it mentions "NO PLAYERSTART") and falls back to the World
Settings actor, which sits at the origin.

When you use **Play from Here** (right-click in the viewport), the editor spawns a temporary
start at the clicked location instead. The Play button's **⋮** menu also has a **Spawn player
at** setting (the label may differ slightly in your version). With **Current Camera Location**
the editor spawns the pawn where the viewport camera is. For this course, keep it at **Default
Player Start**.

### Who is who at runtime

Once the game is running, every framework object can be reached from any actor. You will write
these accessors so often that it pays to see them in one place.

`examples/ch07/ExWhoIsWho.h`

```cpp
// Learn Unreal Engine 5 - chapter 7 example.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExWhoIsWho.generated.h"

/**
 * Drop it into any level and press Play: BeginPlay logs every framework
 * object of the running game and the accessor that finds it.
 */
UCLASS()
class AExWhoIsWho : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
```

`examples/ch07/ExWhoIsWho.cpp`

```cpp
// Learn Unreal Engine 5 - chapter 7 example.

#include "ExWhoIsWho.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogExWho, Log, All);

namespace
{
	/** "Name (Class)", or "none" for a null pointer. */
	FString Describe(const UObject* Object)
	{
		return Object ? FString::Printf(TEXT("%s (%s)"), *Object->GetName(), *Object->GetClass()->GetName())
		              : FString(TEXT("none"));
	}
}

void AExWhoIsWho::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	// Lives for the whole run of the game, across level changes.
	UE_LOG(LogExWho, Log, TEXT("GetGameInstance() -> %s"), *Describe(GetGameInstance()));

	// Only exists where the rules run: the server, or a single-player game.
	UE_LOG(LogExWho, Log, TEXT("World->GetAuthGameMode() -> %s"), *Describe(World->GetAuthGameMode()));

	// Exists everywhere: the state all players may read.
	UE_LOG(LogExWho, Log, TEXT("World->GetGameState() -> %s"), *Describe(World->GetGameState()));

	APlayerController* PC = World->GetFirstPlayerController();
	UE_LOG(LogExWho, Log, TEXT("World->GetFirstPlayerController() -> %s"), *Describe(PC));
	if (!PC)
	{
		return;
	}

	// Per-player data that everyone may read (name, score in a multiplayer game).
	UE_LOG(LogExWho, Log, TEXT("PC->GetPlayerState<APlayerState>() -> %s"), *Describe(PC->GetPlayerState<APlayerState>()));

	// The body the controller drives right now. May be null (spectating, dead).
	UE_LOG(LogExWho, Log, TEXT("PC->GetPawn() -> %s"), *Describe(PC->GetPawn()));

	// The old canvas HUD actor; UMG widgets (chapter 15) do not need it.
	UE_LOG(LogExWho, Log, TEXT("PC->GetHUD() -> %s"), *Describe(PC->GetHUD()));

	// Decides what the player sees each frame; follows the view target.
	UE_LOG(LogExWho, Log, TEXT("PC->PlayerCameraManager -> %s"), *Describe(PC->PlayerCameraManager.Get()));
	UE_LOG(LogExWho, Log, TEXT("PC->GetViewTarget() -> %s"), *Describe(PC->GetViewTarget()));
}
```

Copy the pair into your scratch project, build, drag **ExWhoIsWho** from the Content Browser's C++
Classes folder into any level, and press Play.

Expected Output Log (filter: `LogExWho`), with the default game mode of a Blank project:

```text
LogExWho: GetGameInstance() -> <GameInstance_0> (GameInstance)
LogExWho: World->GetAuthGameMode() -> <GameModeBase_0> (GameModeBase)
LogExWho: World->GetGameState() -> <GameStateBase_0> (GameStateBase)
LogExWho: World->GetFirstPlayerController() -> <PlayerController_0> (PlayerController)
LogExWho: PC->GetPlayerState<APlayerState>() -> <PlayerState_0> (PlayerState)
LogExWho: PC->GetPawn() -> <DefaultPawn_0> (DefaultPawn)
LogExWho: PC->GetHUD() -> <HUD_0> (HUD)
LogExWho: PC->PlayerCameraManager -> <PlayerCameraManager_0> (PlayerCameraManager)
LogExWho: PC->GetViewTarget() -> <DefaultPawn_0> (DefaultPawn)
```

- The class names in parentheses are the reliable part. With your own game mode they become your
  classes, for example `GSGameState` or `BP_GSSnake_C`. The `_C` suffix marks a class generated
  from a Blueprint.
- **`GetAuthGameMode`** is named for *authority*. On a network client it returns null, because
  clients have no game mode. Code that must run everywhere reads the game state instead.
- **`GetFirstPlayerController`** is fine in single player. With several players, ask the pawn
  for its own controller (`Pawn->GetController()`).
- **`GetPlayerState<T>()`** is a template that casts for you. It returns null if the player state
  is not a `T`. The same pattern exists on pawns (`GetController<T>()`) and on the world
  (`GetGameState<T>()`).
- **The view target is the pawn** by default. GridSnake changes that later in this chapter.
- `UGameplayStatics` (`Kismet/GameplayStatics.h`) has static versions that Blueprints use:
  `GetGameMode(this)`, `GetGameState(this)`, `GetPlayerController(this, 0)`, `GetPlayerPawn(this, 0)`.

## GameMode vs GameState

GridSnake's game mode has existed since chapter 2, but it only logged a line. Now it names the
classes the engine should spawn. The game mode does this through class properties that the
constructor sets.

`Source/GridSnake/Core/GSGameMode.cpp` (changes from chapter 6)

```cpp
#include "Core/GSGameMode.h"
#include "GridSnake.h"
#include "Core/GSGameState.h"
#include "Core/GSPlayerController.h"
#include "Core/GSSettings.h"
#include "Engine/World.h"
#include "Snake/GSSnake.h"

AGSGameMode::AGSGameMode()
{
	// C++ defaults. BP_GSGameMode overrides Default Pawn Class with BP_GSSnake,
	// which knows the flipbook assets.
	GameStateClass = AGSGameState::StaticClass();
	PlayerControllerClass = AGSPlayerController::StaticClass();
	DefaultPawnClass = AGSSnake::StaticClass();
}
```

- **`StaticClass()`** returns the `UClass` that reflection generated for a C++ class. Class
  properties such as `GameStateClass` are `TSubclassOf<...>`. A `TSubclassOf<AGameStateBase>`
  accepts only `AGameStateBase` or a subclass of it, so a typo like
  `GameStateClass = AGSSnake::StaticClass()` is a compile error, not a crash at run time.
- **Why the includes matter.** `AGSGameState::StaticClass()` needs the full class declaration, so
  the `.cpp` includes `Core/GSGameState.h`; a forward declaration is not enough.
- **C++ default, Blueprint override.** The C++ constructor names `AGSSnake`. But `AGSSnake` does
  not know which flipbook to show, because assets belong in Blueprints (chapter 5). So
  `BP_GSGameMode` overrides **Default Pawn Class** with `BP_GSSnake`, which sets the flipbook. The
  game state and controller need no assets yet, so the Blueprint keeps the C++ defaults. This is
  the pattern for every class in this course: C++ sets safe defaults, and the Blueprint adds
  assets.

A pairing rule to remember: `AGameModeBase` goes with `AGameStateBase`, and `AGameMode` goes with
`AGameState`. `AGameMode` adds a match state machine (waiting to start, in progress, ended) that
GridSnake does not need; it will run its own simpler phases in chapter 11. Mixing the two families
does not work, so stay with the `...Base` pair. Epic's game mode page explains the difference.

### AGSGameState

The game state holds what every part of the game may read about the current level. For now that
is the gem count and the score.

`Source/GridSnake/Core/GSGameState.h` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GSGameState.generated.h"

/**
 * The state of the current level that everyone may read: gems, score.
 * The game mode changes it; the HUD (chapter 15) displays it.
 */
UCLASS()
class AGSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "GridSnake")
	int32 GetGemsCollected() const { return GemsCollected; }

	UFUNCTION(BlueprintPure, Category = "GridSnake")
	int32 GetGemsRequired() const { return GemsRequired; }

	UFUNCTION(BlueprintPure, Category = "GridSnake")
	int32 GetScore() const { return Score; }

protected:
	virtual void PostInitializeComponents() override;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 GemsCollected = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 GemsRequired = 3;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 Score = 0;
};
```

`Source/GridSnake/Core/GSGameState.cpp` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameState.h"
#include "GridSnake.h"
#include "Core/GSSettings.h"

void AGSGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// PostInitializeComponents runs when the game state is spawned, before any
	// actor's BeginPlay, so everyone who reads GemsRequired sees the real value.

	GemsRequired = UGSSettings::Get()->GemsPerLevel;
	UE_LOG(LogGridSnake, Log, TEXT("GameState ready: 0/%d gems"), GemsRequired);
}
```

- **Read-only from outside.** The counters are `protected`. Other code reads them through
  `BlueprintPure` getters. Only the game state, and in chapter 11 the game mode through functions
  the game state offers, may change them. When only one class writes a value, a wrong value has
  only one place to come from.
- **`VisibleInstanceOnly`** shows the value in the Details panel of the *running* instance. Select
  the game state in the Outliner during Play to watch it. It cannot be edited, and the class
  defaults do not show it. That is the right choice for values computed at run time.
- **Why `PostInitializeComponents` and not `BeginPlay`.** The game state is spawned in step 3 of
  the start-up sequence, long before `StartPlay`. Reading the setting there means the value is
  correct before *any* `BeginPlay` runs. If the HUD or a gem read `GemsRequired` in their own
  `BeginPlay`, they would not depend on whether the game state's `BeginPlay` happened to run
  first. Chapter 5 placed `PostInitializeComponents` in the actor lifecycle. This is a typical use:
  initialise state that other actors read in their `BeginPlay`.
- **`UGSSettings::Get()`** is the developer-settings object from chapter 3, so the gem count
  comes from Project Settings > Game > GridSnake.

## The snake as a Pawn

A **pawn** is any actor a controller can possess. The engine offers two bases:

- **`APawn`** is the minimal possessable actor: no shape and no movement.
- **`ACharacter`** is a pawn with a capsule, a skeletal mesh slot and the
  `UCharacterMovementComponent`. That component handles walking, jumping, falling, swimming and
  network prediction.

GridSnake's snake moves in whole cells, one step at a time, on a flat board. It has no gravity, no
slopes and no physics. Character movement would fight that at every step, so `AGSSnake` is a
plain `APawn` and does its own grid movement (chapter 9). This is the usual choice for grid
games, card games and most 2D games that are not platformers.

### Grid types

Before the pawn, the project needs a vocabulary for directions. It lives in its own header, so the
board, the snake and later the level data can share it.

`Source/GridSnake/Snake/GSGridTypes.h` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GSGridTypes.generated.h"

/** A step on the grid. Up is towards the top of the screen (tile row - 1). */
UENUM(BlueprintType)
enum class EGSDirection : uint8
{
	Up,
	Down,
	Left,
	Right
};

namespace GSGrid
{
	/** The cell offset of one step. Tile rows grow downwards, so Up is Y - 1. */
	inline FIntPoint ToOffset(EGSDirection Direction)
	{
		switch (Direction)
		{
		case EGSDirection::Up:    return FIntPoint(0, -1);
		case EGSDirection::Down:  return FIntPoint(0, 1);
		case EGSDirection::Left:  return FIntPoint(-1, 0);
		case EGSDirection::Right: return FIntPoint(1, 0);
		}
		return FIntPoint::ZeroValue;
	}

	inline bool AreOpposite(EGSDirection A, EGSDirection B)
	{
		return ToOffset(A) + ToOffset(B) == FIntPoint::ZeroValue;
	}

	/**
	 * Rotation that turns a sprite drawn facing right (+X) to face Direction.
	 * Sprites lie in the X-Z plane, so turning them is a pitch (around Y).
	 */
	inline FRotator ToSpriteRotation(EGSDirection Direction)
	{
		switch (Direction)
		{
		case EGSDirection::Up:    return FRotator(90.0f, 0.0f, 0.0f);
		case EGSDirection::Down:  return FRotator(-90.0f, 0.0f, 0.0f);
		case EGSDirection::Left:  return FRotator(180.0f, 0.0f, 0.0f);
		case EGSDirection::Right: return FRotator::ZeroRotator;
		}
		return FRotator::ZeroRotator;
	}
}
```

- **`UENUM(BlueprintType)`** makes the enum visible to reflection. You get a drop-down in the
  Details panel, a Blueprint type, and names at run time: `UEnum::GetValueAsString(EGSDirection::Up)`
  returns `"EGSDirection::Up"`. A reflected enum must be `uint8`-based when Blueprints use it, and
  the header needs its `.generated.h` include even though it declares no class.
- **A namespace of `inline` functions**, not a class. The grid math needs no state and no
  reflection, so it stays plain C++. Plain C++ is the easiest code in the project to read and to
  test. `inline` lets the functions live in a header that many `.cpp` files include.
- **`ToOffset`** encodes the course's tile convention from chapter 6: X grows to the right and
  Y grows *downwards*, the way the Tile Map Editor numbers rows. So Up is `(0, -1)`.
- **`AreOpposite`** uses a neat fact: two directions are opposite exactly when their offsets add
  up to zero. Chapter 8 uses it to stop the snake from reversing into its own neck.
- **`ToSpriteRotation`** turns the head sprite. Sprites lie in the X-Z plane, and the camera looks
  down −Y, so a turn on screen is a rotation *around Y*. In Unreal's `FRotator(Pitch, Yaw, Roll)`,
  that is **pitch**. Positive pitch tilts +X towards +Z, so a sprite drawn facing right faces up.
  Facing left uses pitch 180°, which also turns the sprite upside down. For a symmetric top-down
  head that is invisible. For lopsided art you would mirror it instead (a scale of −1 on X).
- The `return` after each `switch` keeps compilers quiet: an `enum class` can hold other numbers,
  so they may warn that the function can end without returning.

### GSSnake.h

`Source/GridSnake/Snake/GSSnake.h` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Snake/GSGridTypes.h"
#include "GSSnake.generated.h"

class AGSBoard;
class UBoxComponent;
class UPaperFlipbookComponent;

/**
 * The player's pawn. The actor sits on the head's cell; the rest of the
 * snake is drawn by components (chapter 9).
 */
UCLASS()
class AGSSnake : public APawn
{
	GENERATED_BODY()

public:
	AGSSnake();

	UFUNCTION(BlueprintPure, Category = "Snake")
	EGSDirection GetDirection() const { return CurrentDirection; }

	UFUNCTION(BlueprintPure, Category = "Snake")
	FIntPoint GetHeadCell() const { return HeadCell; }

protected:
	virtual void BeginPlay() override;

	/** Root. Used for overlaps from chapter 10; until then it has no collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Head;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	TObjectPtr<AGSBoard> Board;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	FIntPoint HeadCell = FIntPoint::ZeroValue;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	EGSDirection CurrentDirection = EGSDirection::Right;

private:
	/** Moves the actor to the centre of the cell it was spawned in. */
	void SnapToBoard();

	void UpdateHeadRotation();
};
```

- **The actor is the head.** The snake's location is the head's cell. The body, added in chapter
  9, will be drawn by a component, not by separate actors. One actor with one transform keeps
  movement simple: moving the snake means moving one actor.
- **`Board` is a `UPROPERTY`.** A raw pointer to another actor would not be seen by the garbage
  collector or the Details panel. `TObjectPtr` plus `UPROPERTY` gives both (chapter 3).
- **Forward declarations** (`class AGSBoard;`) keep the header light. Only the `.cpp` includes
  the full headers. `GSGridTypes.h` *is* included, because the header uses `EGSDirection` by value.

### GSSnake.cpp

`Source/GridSnake/Snake/GSSnake.cpp` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Snake/GSSnake.h"
#include "GridSnake.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbookComponent.h"

AGSSnake::AGSSnake()
{
	PrimaryActorTick.bCanEverTick = false;

	// A grid pawn never turns with the controller; we rotate the head sprite ourselves.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->InitBoxExtent(FVector(4.0f, 16.0f, 4.0f));
	Collision->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetRootComponent(Collision);

	Head = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Head"));
	Head->SetupAttachment(Collision);
	// In front of gems (+5) and the tile map (0).
	Head->SetRelativeLocation(FVector(0.0f, 10.0f, 0.0f));
	Head->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}
```

- **`bUseControllerRotation...`** A pawn can copy the controller's control rotation, as
  first-person shooters do. The snake sets all three flags to `false`, whatever the parent's
  defaults are: a turned actor would tilt its sprites out of the X-Z plane. Facing is shown by
  rotating the `Head` component instead.
- **A box as the root.** The root is a `UBoxComponent` with the `NoCollision` profile for now.
  Chapter 10 gives it an overlap profile, so the head can touch gems and walls. Its extent is
  4 units in X and Z, small inside the 16-unit cell. In Y it is 16, deep enough to reach the
  gems' and walls' planes. A shape as the root gives the actor a clean transform and, later, a
  collision volume that does not depend on the art.
- **The head draws at Y = +10.** This is the draw-order convention from chapter 4: tile map 0,
  gems +5, head +10. The camera sits at +Y, so larger Y is nearer to it and draws on top.
- **No flipbook here.** The C++ class creates the component but does not set its flipbook. The
  asset is chosen in `BP_GSSnake`, as with the gem and exit in chapter 5.

```cpp
void AGSSnake::BeginPlay()
{
	Super::BeginPlay();

	// Level actors already exist when a spawned pawn begins play.
	Board = Cast<AGSBoard>(UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass()));
	if (!Board)
	{
		UE_LOG(LogGridSnake, Error, TEXT("%s: no GSBoard in this level - place one"), *GetName());
		return;
	}

	CurrentDirection = Board->GetStartDirection();
	SnapToBoard();
	UpdateHeadRotation();
}
```

- **Finding the board.** `UGameplayStatics::GetActorOfClass` returns the first actor of a class in
  the world, or null. It walks the world's actors, so call it once and keep the result.
- **Fail loudly.** A level without a board is a level-design mistake. An `Error` line names the
  problem and the fix, and the function returns before it would dereference null.
- **The start direction belongs to the level**, so it is read from the board (next section).

```cpp
void AGSSnake::SnapToBoard()
{
	HeadCell = Board->WorldToCell(GetActorLocation());
	if (!Board->IsInside(HeadCell))
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s spawned outside the board at %s; using cell (1, 1)"),
			*GetName(), *GetActorLocation().ToString());
		HeadCell = FIntPoint(1, 1);
	}

	// The Player Start's rotation would otherwise tilt the sprites out of the X-Z plane.
	SetActorLocationAndRotation(Board->CellToWorld(HeadCell), FRotator::ZeroRotator);
	UE_LOG(LogGridSnake, Log, TEXT("Snake starts at cell %s facing %s"),
		*HeadCell.ToString(), *UEnum::GetValueAsString(CurrentDirection));
}

void AGSSnake::UpdateHeadRotation()
{
	Head->SetRelativeRotation(GSGrid::ToSpriteRotation(CurrentDirection));
}
```

- **Snap, do not trust.** The Player Start may sit a few units off a cell centre, so the snake
  converts its position to a cell (`WorldToCell`, chapter 6) and moves to that cell's exact centre
  (`CellToWorld`). From now on the cell is the truth; the world position is derived from it.
- **Outside the board** (a Player Start off the map), the snake warns and uses cell (1, 1), just
  inside the usual wall border, so the game stays playable while you fix the level.
- **Rotation reset.** The game mode spawns the pawn with the Player Start's yaw. A yaw turns an
  X-Z sprite edge-on to the camera, so the snake resets the actor rotation to zero.
- **`FIntPoint::ToString()`** prints `X=12 Y=7`. **`UEnum::GetValueAsString`** prints
  `EGSDirection::Right`. Both work because the types are reflected.

## PlayerStart and the start direction

A **Player Start** marks where the player's pawn spawns. Its arrow shows a facing, which the game
mode passes on as the pawn's spawn yaw. That arrow is no use to GridSnake. On a board in the X-Z
plane, yaw turns things into or out of the screen, never up or down, and the snake throws the
rotation away anyway. The starting direction is a property of the *level* instead. Each placed
board carries it.

`Source/GridSnake/Gameplay/GSBoard.h` (changes from chapter 6)

```cpp
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Snake/GSGridTypes.h"
#include "GSBoard.generated.h"

// ...

	/** Which way the snake faces when this level starts. */
	UFUNCTION(BlueprintPure, Category = "Grid")
	EGSDirection GetStartDirection() const { return StartDirection; }

// ...

	/** Set per level on the placed board: the direction the snake starts moving in. */
	UPROPERTY(EditAnywhere, Category = "Level")
	EGSDirection StartDirection = EGSDirection::Right;
```

- **`EditAnywhere`** lets you set it per placed board (per level) and on the class defaults. The
  Details panel shows it as **Level > Start Direction**, a drop-down of the four enum values.
- The header now includes `Snake/GSGridTypes.h` because the property uses the enum by value.
- The Player Start still decides *where* the snake starts. The board decides *which way it
  faces*. Both are level data, so both live in the level.

Placing a start on a cell centre uses the arithmetic from chapter 6. Paper2D centres tile (0, 0)
on the map's origin, so cell (x, y) is centred at (16x, 0, −16y), and cell (12, 7) is centred at
(192, 0, −112): 12·16 = 192 and 7·16 = 112. You do not have to be exact, because `SnapToBoard`
uses the cell that contains the spawn point. A cell reaches 8 units from its centre in each
direction, so every point from the cell's top-left corner (184, 0, −104) up to, but not
including, its bottom-right corner (200, 0, −120) counts as cell (12, 7). The asset table for
this chapter uses the centre, which is the safer choice if you type the numbers yourself.

> **The "BADsize" label.** A Player Start is sized for a standing character, so in a small 2D
> level its capsule may overlap wall tiles. The editor then draws a warning label on it (shown as
> "BADsize" in recent versions). The snake has no collision yet and ignores the capsule, so you can
> ignore the label in this project.

## The PlayerController and view targets

The player controller is the player's representative in the world. In GridSnake it has two jobs:
the view (now), and input and screens (chapters 8, 15 and 16).

`Source/GridSnake/Core/GSPlayerController.h` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GSPlayerController.generated.h"

/**
 * The player's will. Owns the view (the board's fixed camera, not the pawn);
 * later also input contexts and screens.
 */
UCLASS()
class AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

protected:
	virtual void BeginPlay() override;
};
```

`Source/GridSnake/Core/GSPlayerController.cpp` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSPlayerController.h"
#include "GridSnake.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"

AGSPlayerController::AGSPlayerController()
{
	// By default, possessing a pawn makes it the view target. The board's
	// camera should stay in charge, so we manage the view target ourselves.
	bAutoManageActiveCameraTarget = false;
}

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (AActor* Board = UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass()))
	{
		SetViewTarget(Board);
		UE_LOG(LogGridSnake, Log, TEXT("View target: %s"), *Board->GetName());
	}
}
```

### What a view target is

Every player controller owns a **player camera manager**, which decides what the player sees
each frame. It asks one actor, the **view target**, for a point of view by calling the actor's
`CalcCamera`. The default `AActor::CalcCamera` looks for an active `UCameraComponent` on the actor
and, if it finds one, uses that camera's location, rotation, projection mode and ortho width. If
it finds none, it uses the actor's "eyes": its location and rotation, with a perspective view.

The board from chapter 6 has exactly such a camera component: orthographic, framing the whole
map, looking down −Y. So making the *board* the view target is all it takes to play through that
camera. No camera actor and no extra component on the pawn are needed.

### Why `bAutoManageActiveCameraTarget = false`

`APlayerController` has a flag, `bAutoManageActiveCameraTarget`, that is `true` by default. While
it is on, every possession makes the possessed pawn the view target (unless a placed Camera Actor
is set to auto-activate for this player). That is what you want in a third-person game, where the
camera rides on the character. It is wrong for GridSnake:

- The snake has no camera component, so the view would come from the pawn's "eyes". That is a
  perspective view along the pawn's +X axis, looking at the sprites edge-on.
- Possession happens again whenever a pawn is possessed later, for example after a spectating
  phase. Each time, the automatic behaviour would steal the view back from the board.

Turning the flag off in the constructor makes the controller leave the view target alone. The
controller sets it once in `BeginPlay`, which runs after the initial possession (step 6 vs step
7). When exactly the engine would apply its automatic choice no longer matters.

### The rest of BeginPlay

- **`IsLocalController()`** is true for a controller that belongs to a player on *this* machine.
  A server also holds remote players' controllers, which must not set its view. In single player
  the check always passes; it documents intent.
- **`SetViewTarget(Board)`** switches instantly. `SetViewTargetWithBlend(Board, 0.5f)` would blend
  over half a second, which is useful for cut-aways. Chapter 6's `ExOrthoCamera` example used it.
- The log prints the board's internal object name (such as `GSBoard_0`, possibly with a long
  `_UAID_...` suffix), not its Outliner label. Names in the expected logs are illustrative.

## Possession, up close

`Possess` and `UnPossess` are functions on the controller. The pawn hears about them through two
virtual functions. Overriding them is the standard way for a pawn to react to gaining or losing a
player, for example to show or hide a highlight.

`examples/ch07/ExPossessionPawn.h`

```cpp
// Learn Unreal Engine 5 - chapter 7 example.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExPossessionPawn.generated.h"

/**
 * A pawn that reports when a controller takes it over or lets it go.
 * Place it in a level and set Pawn > Auto Possess Player = Player 0.
 */
UCLASS()
class AExPossessionPawn : public APawn
{
	GENERATED_BODY()

public:
	/** Called on the server (in single player: always) when a controller possesses this pawn. */
	virtual void PossessedBy(AController* NewController) override;

	/** Called when the controller lets go of this pawn. */
	virtual void UnPossessed() override;

	/** Asks the current controller to let go of this pawn. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	void Release();
};
```

`examples/ch07/ExPossessionPawn.cpp`

```cpp
// Learn Unreal Engine 5 - chapter 7 example.

#include "ExPossessionPawn.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY_STATIC(LogExPossess, Log, All);

void AExPossessionPawn::PossessedBy(AController* NewController)
{
	// Super sets Controller, so GetController() is valid after this line.
	Super::PossessedBy(NewController);
	UE_LOG(LogExPossess, Log, TEXT("%s possessed by %s"), *GetName(), *GetNameSafe(GetController()));
}

void AExPossessionPawn::UnPossessed()
{
	// Log before Super: Super clears Controller.
	UE_LOG(LogExPossess, Log, TEXT("%s unpossessed (controller was %s)"), *GetName(), *GetNameSafe(GetController()));
	Super::UnPossessed();
}

void AExPossessionPawn::Release()
{
	if (AController* MyController = GetController())
	{
		UE_LOG(LogExPossess, Log, TEXT("%s asks %s to let go"), *GetName(), *MyController->GetName());
		MyController->UnPossess();
	}
	else
	{
		UE_LOG(LogExPossess, Log, TEXT("%s has no controller"), *GetName());
	}
}
```

To see it:

1. Copy the pair into your scratch project and build.
2. Drag **ExPossessionPawn** into a level. In its Details panel, under **Pawn**, set **Auto
   Possess Player** = **Player 0**. A placed pawn with this setting is possessed by the first
   local player instead of a new pawn being spawned.
3. To call `Release`, open the level Blueprint (**Blueprints** toolbar button > **Open Level
   Blueprint**; the menu may be labelled slightly differently in your version). Select the placed
   pawn in the viewport, right-click in the graph and choose **Create a Reference to
   ExPossessionPawn**. Then wire **Event BeginPlay** → **Delay** (2 seconds) → **Release**, with
   the reference as the target.
4. Press Play and wait two seconds.

Expected Output Log (filter: `LogExPossess`):

```text
LogExPossess: <ExPossessionPawn_0> possessed by <PlayerController_0>
LogExPossess: <ExPossessionPawn_0> asks <PlayerController_0> to let go
LogExPossess: <ExPossessionPawn_0> unpossessed (controller was <PlayerController_0>)
```

- **`PossessedBy` runs on the server only.** In single player, that is your game. Code that must
  also run on a client that owns the pawn uses `NotifyControllerChanged()` (chapter 8 uses it in
  an example) or, in Blueprints, the Possessed/Unpossessed events. `PossessedBy` is enough for
  GridSnake, which never runs a network game.
- **After `UnPossess`**, the pawn stays in the world with no controller. It gets no input, but it
  still ticks and still renders. The controller keeps existing too, with no pawn. This is how
  spectating works, and exercise 4 uses it.
- **The camera after release.** With default settings, if the released pawn was the view target,
  the controller makes *itself* the view target, so the view stays where it was. GridSnake's
  board view is unaffected, because the board, not the pawn, is the view target.

## Building the milestone

The C++ side is done: two new framework classes, the pawn, the grid types and the board's start
direction. Now the editor side. These steps are `project/ch07/ASSETS.md` in narrative form.

1. **Build.** New classes were added, so close the editor and build `GridSnakeEditor` from your
   IDE, then reopen the project (chapter 2). Live Coding can add classes in many cases, but a
   restart is the safe choice when new `UCLASS`es appear.
2. **Create `BP_GSSnake`.**
   - In the Content Browser, open **C++ Classes > GridSnake > Snake**.
   - Right-click `GSSnake` and choose **Create Blueprint class based on GSSnake**.
   - Name it `BP_GSSnake` and save it in `/Game/GridSnake/Blueprints`.
   - In the Blueprint editor's **Components** panel, select **Head**. In the Details panel, under
     **Sprite**, set **Source Flipbook** = `FB_SnakeHead_Idle`.
   - **Compile** and **Save**.
3. **Point the game mode at it.**
   - Open `BP_GSGameMode` and click **Class Defaults** in the toolbar.
   - Under **Classes**, set **Default Pawn Class** = `BP_GSSnake`.
   - Check that **Game State Class** shows `GSGameState` and **Player Controller Class** shows
     `GSPlayerController`. You did not set them; they come from the C++ constructor, because a
     Blueprint subclass starts with its parent's defaults.
   - Compile and save.
4. **Place a Player Start** in `L_Level01`.
   - **Place Actors > Basic > Player Start** (or the **+** button in the main toolbar >
     **Basic**). Drag it into the level.
   - Set its location inside a free floor cell, for example (192, 0, −112), the centre of cell
     (12, 7). Leave free cells to the *left* of it too: in
     chapter 9 the body trails behind a snake heading right.
   - Its rotation does not matter.
5. **Set the start direction.** Select the placed `GSBoard`. In the Details panel, under **Level**,
   set **Start Direction** = **Right**.
6. **Play.**
   - The view is the board's camera: the whole map, framed, orthographic.
   - The snake's head appears on the Player Start's cell, facing right.
   - Open the **Outliner** (called the World Outliner in older versions) while playing. Next to
     the level's actors you will find the spawned `GSGameState`, `GSPlayerController`,
     `BP_GSSnake_C`, a `PlayerState` and a `PlayerCameraManager`.
   - Select the `GSGameState` entry: its Details panel shows **Gems Collected** 0,
     **Gems Required** 3, **Score** 0.

Expected Output Log (filter: `LogGridSnake`):

```text
LogGridSnake: GameState ready: 0/3 gems
LogGridSnake: Board <GSBoard_0>: 24x14 cells of 16 units
LogGridSnake: View target: <GSBoard_0>
LogGridSnake: Snake starts at cell X=12 Y=7 facing EGSDirection::Right
LogGridSnake: GridSnake game mode started on map <UEDPIE_0_L_Level01> (collect 3 gems)
```

- **Order.** The game state line comes first, because the game state is spawned in step 3, long
  before `StartPlay`. The next three lines are `BeginPlay`s, which all run *inside*
  `Super::StartPlay()`. The game mode's own line comes last, because `AGSGameMode::StartPlay` logs
  only after `Super::StartPlay()` returns.
- **Among the `BeginPlay`s**, the order follows the order in which actors sit in the level. Level
  actors (the board) come first, then actors spawned in order: controller, then pawn. Your log may
  order them differently if your level differs, and the code does not depend on it. The snake
  only needs the board to *exist*, not to have begun play.
- The board name and the map name are illustrative. If the gem and exit actors from chapter 5
  log anything in your setup, those lines appear among the `BeginPlay`s too.

### When it goes wrong

| Symptom | Cause | Fix |
|---|---|---|
| The view is still the free camera | `L_Level01` overrides the game mode, or the project default is not `BP_GSGameMode` | Check World Settings > **GameMode Override** (should be None or `BP_GSGameMode`) and Project Settings > Maps & Modes |
| The view is right but no snake is visible | Default Pawn Class is still the C++ `GSSnake`, which has no flipbook | Set **Default Pawn Class** = `BP_GSSnake` in `BP_GSGameMode` |
| `no GSBoard in this level - place one` | The level has no board | Place `GSBoard` (chapter 6) |
| `spawned outside the board ... using cell (1, 1)` | A Player Start off the map, or Play spawns at the camera location | Place a Player Start on a floor cell; set the Play menu's spawn option to the default Player Start |
| The head shows edge-on or tilted | Code that rotates the actor (not the `Head` component) | Rotate only `Head`; keep the actor at zero rotation |

## Summary

- The Gameplay Framework splits a game into **GameMode** (rules, server only), **GameState**
  (shared state), **PlayerController** (the player), **PlayerState** (public per-player data),
  **Pawn** (the body) and **HUD**. Each piece of GridSnake now has an obvious home.
- When a level starts, the engine runs `InitGame`, spawns the game state, logs the player in,
  starts them (`HandleStartingNewPlayer` → `RestartPlayer` → `SpawnDefaultPawnFor` → possession),
  and only then calls `StartPlay`, which runs every `BeginPlay`.
- The game mode chooses the classes through `GameStateClass`, `PlayerControllerClass` and
  `DefaultPawnClass`. C++ sets defaults; Blueprint subclasses swap in classes that know assets.
- Each framework object has an accessor: `GetAuthGameMode`, `GetGameState`,
  `GetFirstPlayerController`, `GetPlayerState<T>`, `GetPawn`, plus the `UGameplayStatics`
  versions.
- `AGSSnake` is a plain `APawn`. It finds the board, snaps to a cell, faces the level's start
  direction and turns its head sprite with a pitch.
- A view target is any actor; its camera component supplies the view. `AGSPlayerController`
  turns off `bAutoManageActiveCameraTarget` and makes the board the view target, so possession
  never steals the camera.

## Exercises

1. **A player state.** Add `AGSPlayerState`, a subclass of `APlayerState`. Make the game mode use
   it, give the player the name "Snake" when they join, and log the name from the snake's
   `BeginPlay`.

<details><summary>Solution</summary>

`Source/GridSnake/Core/GSPlayerState.h`:

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GSPlayerState.generated.h"

UCLASS()
class AGSPlayerState : public APlayerState
{
	GENERATED_BODY()
};
```

`APlayerState` already stores a name (`SetPlayerName`, `GetPlayerName`), so the subclass needs no
members yet. It is still worth having: it is the place where per-player data such as lives would
go. In `AGSGameMode.h`, declare `virtual void PostLogin(APlayerController* NewPlayer) override;`.
Then in `AGSGameMode.cpp`:

```cpp
#include "Core/GSPlayerState.h"
#include "GameFramework/PlayerController.h"

AGSGameMode::AGSGameMode()
{
	// ... the existing three lines ...
	PlayerStateClass = AGSPlayerState::StaticClass();
}

void AGSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (AGSPlayerState* PS = NewPlayer->GetPlayerState<AGSPlayerState>())
	{
		PS->SetPlayerName(TEXT("Snake"));
	}
}
```

In `AGSSnake::BeginPlay`, after `Super::BeginPlay()`:

```cpp
	if (const APlayerState* PS = GetPlayerState())
	{
		UE_LOG(LogGridSnake, Log, TEXT("Player: %s"), *PS->GetPlayerName());
	}
```

Include `GameFramework/PlayerState.h` in `GSSnake.cpp`. The player state is created with the
controller during login, and the pawn receives it when it is possessed. Both happen before
`BeginPlay`, so the pointer is valid there. Expected: `LogGridSnake: Player: Snake`. Check that
`BP_GSGameMode` shows **Player State Class** = `GSPlayerState`; it inherits the new default.

</details>

2. **Face the arrow.** Instead of the board's Start Direction, make the snake face the way the
   Player Start's arrow points. What is the problem with that idea on an X-Z board, and how would
   you work around it?

<details><summary>Solution</summary>

The game mode spawns the pawn with the Player Start's **yaw** (pitch and roll are dropped). Yaw
rotates around Z. On a board in the X-Z plane, that turns the arrow towards +X (right), −X (left)
or into and out of the screen (±Y). It can never point up or down on screen. So you need a
convention for the two depth directions, for example "yaw +90 means Up, yaw −90 means Down".
Read the yaw *before* `SnapToBoard` resets the rotation:

```cpp
namespace
{
	EGSDirection DirectionFromYaw(float Yaw)
	{
		// Round to the nearest quarter turn: 0, 1, 2 or 3.
		const int32 Quarter = (FMath::RoundToInt(FRotator::NormalizeAxis(Yaw) / 90.0f) + 4) % 4;
		switch (Quarter)
		{
		case 1:  return EGSDirection::Up;   // yaw +90 (by convention)
		case 2:  return EGSDirection::Left; // yaw 180
		case 3:  return EGSDirection::Down; // yaw -90 (by convention)
		default: return EGSDirection::Right;
		}
	}
}

// In BeginPlay, replacing the Board->GetStartDirection() line:
	CurrentDirection = DirectionFromYaw(GetActorRotation().Yaw);
```

`NormalizeAxis` maps the yaw to (−180, 180], so the division gives −2 to 2, and `+ 4) % 4` folds
that into 0–3. It works, but the convention is invisible in the editor: the arrow points into the
screen, and nothing says "Up". That is why the project stores an explicit **Start Direction** on
the board.

</details>

3. **No Player Start.** Delete the Player Start from `L_Level01` and press Play. What happens,
   and which code handles it?

<details><summary>Solution</summary>

`FindPlayerStart` finds no `APlayerStart`. It logs a line on `LogGameMode` and falls back to the
World Settings actor at the origin, so the snake spawns at (0, 0, 0). The origin is the top-left
corner of the tile map, which `WorldToCell` rounds down to cell (0, 0). That cell is *inside* the
board, so no warning appears, and the snake sits on the corner wall:

```text
LogGridSnake: Snake starts at cell X=0 Y=0 facing EGSDirection::Right
```

If the editor's Play options spawn the pawn at the camera location instead, the cell depends on
where your viewport camera was. If that cell is outside the map, `SnapToBoard` prints its warning
and uses cell (1, 1). Either way the game keeps running, which is the point of the fallback. Put
the Player Start back, and check `LogGameMode` whenever a pawn appears in an odd place.

</details>

4. **Spectate.** Give `AGSPlayerController` a function `StopPlaying()` that lets go of the snake
   but keeps the board in view. Call it from the console to test it. Why does the view not jump?

<details><summary>Solution</summary>

In `GSPlayerController.h`:

```cpp
public:
	/** Lets go of the current pawn. The board stays in view. */
	UFUNCTION(Exec)
	void StopPlaying();
```

In `GSPlayerController.cpp`:

```cpp
void AGSPlayerController::StopPlaying()
{
	if (APawn* OldPawn = GetPawn())
	{
		UnPossess();
		UE_LOG(LogGridSnake, Log, TEXT("Released %s; view target is still %s"),
			*OldPawn->GetName(), *GetNameSafe(GetViewTarget()));
	}
}
```

`Exec` functions on the player controller can be run from the console (chapter 3): press the
backtick key during Play and type `StopPlaying`.

Expected Output Log:

```text
LogGridSnake: Released <BP_GSSnake_C_0>; view target is still <GSBoard_0>
```

The view does not move because `UnPossess` only resets the view target if the released pawn *was*
the view target. Here the board is, and with `bAutoManageActiveCameraTarget` off, nothing else
touches it. The snake stays on the board, frozen and visible, with no controller.

</details>

---

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-7--the-gameplay-framework)
