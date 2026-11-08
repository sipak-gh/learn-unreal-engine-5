# Chapter 18 — Saving Progress

GridSnake forgets everything when it closes. Every launch starts with all levels open, no best
scores, and the volume sliders back at their defaults. This chapter covers Unreal's save system:
`USaveGame` objects, save slots and where their files live, synchronous and asynchronous saving,
keeping old save files loadable as the game changes, and how one game instance subsystem can rely
on another. GridSnake gains locked levels that unlock as you finish them, a best score per level
and per run, a **Continue** button, saved volume settings, and a **Reset Progress** button.

Previous: [Chapter 17](17-sound-and-music.md). Next:
[Chapter 19](19-debugging-and-profiling.md) (debugging and profiling).

## What a game saves

Before any code, decide what belongs in a save file. For GridSnake:

| Data | Saved? | Why |
|---|---|---|
| Highest unlocked level | Yes | Progress the player earned |
| Best score per level, best full run | Yes | Something to beat next time |
| Music and SFX volume | Yes | A player's preference |
| Window mode, VSync | Already | `UGameUserSettings` writes them to `GameUserSettings.ini` (chapter 16) |
| The snake's position mid-level | No | A level takes a minute; restarting it is fine |
| The current run's score | No | A run is one sitting; leaving abandons it |

Saving *progress*, not a snapshot of the live world, keeps the save small. Restoring a level
exactly as the player left it would mean giving every actor a stable ID and rebuilding them all on
load; GridSnake does not need that. Volumes could also live in a `UGameUserSettings` subclass,
the engine's place for per-machine settings; here they go into the save game.

## USaveGame

A save game is a `UObject` subclass of `USaveGame` (in `GameFramework/SaveGame.h`, module
`Engine`) whose `UPROPERTY` fields are the data. `USaveGame` adds nothing itself. It is a marker
class that the save functions accept. The functions live in `UGameplayStatics`:

```cpp
// Kismet/GameplayStatics.h
static USaveGame* CreateSaveGameObject(TSubclassOf<USaveGame> SaveGameClass);
static bool SaveGameToSlot(USaveGame* SaveGameObject, const FString& SlotName, const int32 UserIndex);
static USaveGame* LoadGameFromSlot(const FString& SlotName, const int32 UserIndex);
static bool DoesSaveGameExist(const FString& SlotName, const int32 UserIndex);
static bool DeleteGameInSlot(const FString& SlotName, const int32 UserIndex);
```

- `CreateSaveGameObject` creates a new, empty object of your class. It returns a `USaveGame*`, so
  you `Cast` to your class.
- `SaveGameToSlot` serializes the object and writes it under a **slot name**. It blocks until the
  file is written and returns whether that worked.
- `LoadGameFromSlot` reads the slot and creates a *new* object holding the data, or returns
  `nullptr` if there is no such slot or it cannot be read.
- `DoesSaveGameExist` and `DeleteGameInSlot` do what their names say.

Which fields go into the file:

- **Every `UPROPERTY`**, of any type the reflection system can serialize: numbers, `bool`,
  `FString`, `FName`, `FText`, enums, structs with `UPROPERTY` members, `TArray`, `TMap` and
  `TSet` of those.
- **Except `Transient` ones**. `UPROPERTY(Transient)` means "never serialize". You do not need the
  `SaveGame` specifier you may see on other properties: that one marks fields of *actors and
  components* for a different kind of save, and a `USaveGame` writes all its properties anyway.
- **Plain C++ members are never saved.** The reflection system does not know they exist (chapter
  3), so serialization cannot see them.

The first example saves an object, loads it back, and shows these rules.

`examples/ch18/ExSaveGame.h`

```cpp
// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "ExSaveGame.generated.h"

/** The data to save. Only UPROPERTY fields are written to the file. */
UCLASS()
class UExSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Save")
	int32 Coins = 0;

	UPROPERTY(VisibleAnywhere, Category = "Save")
	FString PlayerName;

	UPROPERTY(VisibleAnywhere, Category = "Save")
	TArray<FName> Unlocked;

	/** Not a UPROPERTY: never saved, always 0 after loading. */
	int32 NotSaved = 0;
};

/**
 * Saves a UExSaveGame to slot "ExSlot" synchronously, loads it back and logs both
 * steps. Drop it in a level and press Play.
 */
UCLASS()
class AExSaveTester : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Save")
	FString SlotName = TEXT("ExSlot");

	/** Delete the file again in EndPlay, so every run starts without a save. */
	UPROPERTY(EditAnywhere, Category = "Save")
	bool bDeleteOnEndPlay = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
```

`examples/ch18/ExSaveGame.cpp`

```cpp
// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExSaveGame.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogExSave, Log, All);

void AExSaveTester::BeginPlay()
{
	Super::BeginPlay();

	const int32 UserIndex = 0;
	UE_LOG(LogExSave, Log, TEXT("Slot %s exists before saving: %s"), *SlotName,
		UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex) ? TEXT("true") : TEXT("false"));

	// 1. Create an object of our save class and fill it in.
	UExSaveGame* ToSave = Cast<UExSaveGame>(UGameplayStatics::CreateSaveGameObject(UExSaveGame::StaticClass()));
	ToSave->Coins = 42;
	ToSave->PlayerName = TEXT("Ada");
	ToSave->Unlocked = { FName(TEXT("L_Level01")), FName(TEXT("L_Level02")) };
	ToSave->NotSaved = 7;

	// 2. Serialize it and write the file. Blocks until the file is written.
	const bool bSaved = UGameplayStatics::SaveGameToSlot(ToSave, SlotName, UserIndex);
	UE_LOG(LogExSave, Log, TEXT("Saved slot %s: %s"), *SlotName, bSaved ? TEXT("true") : TEXT("false"));

	// 3. Read the file into a brand-new object. Null if missing or not readable.
	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	if (const UExSaveGame* Data = Cast<UExSaveGame>(Loaded))
	{
		UE_LOG(LogExSave, Log, TEXT("Loaded: %s, %d coins, %d unlocked (NotSaved = %d)"),
			*Data->PlayerName, Data->Coins, Data->Unlocked.Num(), Data->NotSaved);
		UE_LOG(LogExSave, Log, TEXT("Same object? %s"), Data == ToSave ? TEXT("yes") : TEXT("no"));
	}
	else
	{
		UE_LOG(LogExSave, Warning, TEXT("Could not load slot %s"), *SlotName);
	}
}

void AExSaveTester::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bDeleteOnEndPlay)
	{
		const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, 0);
		UE_LOG(LogExSave, Log, TEXT("Deleted slot %s: %s"), *SlotName, bDeleted ? TEXT("true") : TEXT("false"));
	}

	Super::EndPlay(EndPlayReason);
}
```

To try it, copy both files into your project's `Source/<Module>/` folder, build, drag
`ExSaveTester` into a level and press **Play**.

Expected Output Log (filter: `LogExSave`), the first time:

```
LogExSave: Slot ExSlot exists before saving: false
LogExSave: Saved slot ExSlot: true
LogExSave: Loaded: Ada, 42 coins, 2 unlocked (NotSaved = 0)
LogExSave: Same object? no
```

Press **Play** again and the first line says `true`: the file from the first run is still there.
Tick **Delete On End Play** in the tester's Details to remove it each time you stop.

- `NotSaved` was 7 when the object was saved, and is 0 after loading. It is not a `UPROPERTY`, so
  it was never written; the loaded object has the class default.
- `Same object? no`: loading never hands back the object you saved. It creates a fresh one and
  fills it from the file. If you change `ToSave` afterwards, `Data` does not change, and the other
  way round.
- The `TArray<FName>` needed no extra code: containers of serializable types serialize themselves.
- Everything here is synchronous: `SaveGameToSlot` returns once the file is on disk. Fine at a
  menu; a possible hitch mid-gameplay on a slow disk. Async saving, below, fixes that.

### What a save file can hold

A save stores *values*. Object references are a trap:

- A `TObjectPtr<AActor>` field pointing at an actor in the level is written as the actor's path
  name (the save archive turns object references into strings). On load, the engine tries to find
  an object with that path. In a different session that actor may not exist, or may be a
  different instance, and you get `nullptr` or the wrong thing.
- Save **identifiers** instead: a level's asset name, a gem's grid cell, an item's `FName`. Then
  look the object up after loading. GridSnake keys its best scores by the map's asset name for
  exactly this reason.
- References to *assets* (a texture, a data asset) are safer, because asset paths are stable. A
  `TSoftObjectPtr` (chapter 13) makes the intent clear and does not load the asset until asked.

## Slots, user indices and files

A **slot** is a name you choose. Each slot is one file. `UserIndex` identifies a local player on
platforms that have per-user storage, such as consoles. On desktop platforms it is accepted but
typically does not change where the file goes; pass 0.

On desktop platforms the default save system writes `<SlotName>.sav` into a `SaveGames` folder
under the project's `Saved` directory:

| Running as | Folder |
|---|---|
| Play In Editor, Standalone from the editor | `<YourProject>/Saved/SaveGames/` |
| A packaged Windows build | `%LOCALAPPDATA%/<ProjectName>/Saved/SaveGames/` |

Other platforms put saves wherever their own save system dictates, which is why the API takes a
slot name and never a file path. Code that builds file paths by hand breaks on the first console
port.

Slot names become file names, so keep them to letters, digits and underscores. One slot for the
whole game (`"GridSnake"`) suits one player per machine; one slot per profile (`"GridSnake_1"`,
...) is exercise 2. Delete a slot's file by hand, with the game closed, to test the "no save yet"
path.

The `.sav` file is binary. It starts with a small header: a tag identifying it as a save file, the
engine and package versions it was written with, and the save class's path. Then come the
serialized properties, each with its name and type. `LoadGameFromSlot` reads the class path from
the header, creates an object of that class and deserializes into it. That is why loading returns a
`USaveGame*` you must cast: the file decides the class.

Two more functions skip the file system entirely:

```cpp
static bool SaveGameToMemory(USaveGame* SaveGameObject, TArray<uint8>& OutSaveData);
static USaveGame* LoadGameFromMemory(const TArray<uint8>& SaveData);
```

They produce and consume the same bytes as the slot functions. Use them to send a save to an
online service, to keep an in-memory "undo" copy, or in automated tests.

## Saving without a hitch

`AsyncSaveGameToSlot` and `AsyncLoadGameFromSlot` do the slow part on another thread and tell you
when they are done:

```cpp
// Kismet/GameplayStatics.h
DECLARE_DELEGATE_ThreeParams(FAsyncSaveGameToSlotDelegate, const FString& /*SlotName*/, const int32 /*UserIndex*/, bool /*bSuccess*/);
DECLARE_DELEGATE_ThreeParams(FAsyncLoadGameFromSlotDelegate, const FString& /*SlotName*/, const int32 /*UserIndex*/, USaveGame* /*LoadedGame*/);

static void AsyncSaveGameToSlot(USaveGame* SaveGameObject, const FString& SlotName, const int32 UserIndex,
    FAsyncSaveGameToSlotDelegate SavedDelegate = FAsyncSaveGameToSlotDelegate());
static void AsyncLoadGameFromSlot(const FString& SlotName, const int32 UserIndex,
    FAsyncLoadGameFromSlotDelegate LoadedDelegate);
```

How the save version works, step by step:

1. **On the game thread, during the call**, the object is serialized into a byte array. This is
   fast for a small object, and it means the call takes a snapshot.
2. **On a worker thread**, the bytes are written to disk. This is the slow part.
3. **Back on the game thread, later**, your delegate runs with the result.

Because of step 1, you may change or even discard the save object as soon as the call returns. The
file gets the values from the moment of the call.

These delegates are *single-cast, non-dynamic* (`DECLARE_DELEGATE_...`, chapter 11). You bind
them with `CreateUObject`, `CreateLambda` and friends, and the handler is a plain member function:
no `UFUNCTION` needed. `CreateUObject` holds a weak reference, so if the object is destroyed
before the save finishes, the callback is skipped instead of crashing.

`examples/ch18/ExAsyncSave.h`

```cpp
// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.
// Uses UExSaveGame from ExSaveGame.h (same folder).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExAsyncSave.generated.h"

class USaveGame;

/**
 * Saves with AsyncSaveGameToSlot, then loads with AsyncLoadGameFromSlot.
 * The log shows that BeginPlay carries on before the save has finished.
 */
UCLASS()
class AExAsyncSave : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Save")
	FString SlotName = TEXT("ExAsyncSlot");

protected:
	virtual void BeginPlay() override;

private:
	// Plain C++ handlers: these delegates are not dynamic, so no UFUNCTION is needed.
	void HandleSaved(const FString& InSlotName, const int32 InUserIndex, bool bSuccess);
	void HandleLoaded(const FString& InSlotName, const int32 InUserIndex, USaveGame* Loaded);
};
```

`examples/ch18/ExAsyncSave.cpp`

```cpp
// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExAsyncSave.h"
#include "ExSaveGame.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogExAsyncSave, Log, All);

void AExAsyncSave::BeginPlay()
{
	Super::BeginPlay();

	UExSaveGame* ToSave = Cast<UExSaveGame>(UGameplayStatics::CreateSaveGameObject(UExSaveGame::StaticClass()));
	ToSave->Coins = 7;
	ToSave->PlayerName = TEXT("Grace");

	// GFrameCounter counts engine frames; it shows when each line runs.
	UE_LOG(LogExAsyncSave, Log, TEXT("Frame %llu: calling AsyncSaveGameToSlot"), GFrameCounter);

	// The object is serialized now, on the game thread. The file is written on
	// another thread, and the delegate runs back on the game thread afterwards.
	UGameplayStatics::AsyncSaveGameToSlot(ToSave, SlotName, /*UserIndex=*/0,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &AExAsyncSave::HandleSaved));

	// Safe: the data was already copied out of ToSave.
	ToSave->Coins = 999;
	UE_LOG(LogExAsyncSave, Log, TEXT("Frame %llu: after the call, BeginPlay continues"), GFrameCounter);
}

void AExAsyncSave::HandleSaved(const FString& InSlotName, const int32 InUserIndex, bool bSuccess)
{
	UE_LOG(LogExAsyncSave, Log, TEXT("Frame %llu: save to %s (user %d) finished: %s"),
		GFrameCounter, *InSlotName, InUserIndex, bSuccess ? TEXT("true") : TEXT("false"));

	UGameplayStatics::AsyncLoadGameFromSlot(InSlotName, InUserIndex,
		FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &AExAsyncSave::HandleLoaded));
}

void AExAsyncSave::HandleLoaded(const FString& InSlotName, const int32 InUserIndex, USaveGame* Loaded)
{
	if (const UExSaveGame* Data = Cast<UExSaveGame>(Loaded))
	{
		UE_LOG(LogExAsyncSave, Log, TEXT("Frame %llu: loaded %s with %d coins"), GFrameCounter, *Data->PlayerName, Data->Coins);
	}
	else
	{
		UE_LOG(LogExAsyncSave, Warning, TEXT("Frame %llu: nothing loaded from %s"), GFrameCounter, *InSlotName);
	}
}
```

`ExAsyncSave.cpp` includes `ExSaveGame.h`, so copy the first example's files too. Place
`ExAsyncSave` in a level and press **Play**.

Expected Output Log (filter: `LogExAsyncSave`; the frame numbers vary):

```
LogExAsyncSave: Frame 1234: calling AsyncSaveGameToSlot
LogExAsyncSave: Frame 1234: after the call, BeginPlay continues
LogExAsyncSave: Frame 1236: save to ExAsyncSlot (user 0) finished: true
LogExAsyncSave: Frame 1238: loaded Grace with 7 coins
```

- The first two lines share a frame: `AsyncSaveGameToSlot` returned immediately and `BeginPlay`
  carried on. The "finished" line comes later, typically a frame or more later. Do not depend on
  the exact count.
- The loaded save has **7** coins, not 999. The object was serialized during the call, so setting
  `Coins = 999` afterwards changed only the object in memory, not the file.
- `GFrameCounter` is the engine's global frame count, a `uint64`, hence `%llu`.
- Start the load from the save's callback, as here. If you start a load of the same slot while a
  save is still being written, which one the file system sees first is up to timing.

When to use which:

| Situation | Use |
|---|---|
| Startup, before anything is on screen | Synchronous `LoadGameFromSlot`; you need the data before you can continue anyway |
| During gameplay or a transition | `AsyncSaveGameToSlot` |
| Quitting the game | Synchronous `SaveGameToSlot`, so the write finishes before the process exits |
| Huge saves, or loading while showing a menu | `AsyncLoadGameFromSlot` with a "loading" state |

GridSnake loads synchronously at startup and always saves asynchronously.

## Versioning

You will ship an update that changes the save class. Players' old files must keep working. How
Unreal serializes the properties decides what is safe.

A save object is written with **tagged property serialization**: each property is stored with its
name and type, and loading matches stored properties to the class's properties *by name*. So:

| Change | What happens to old files | Safe? |
|---|---|---|
| Add a property | Not in the file, so it keeps its default value | Yes, if the default is right for an old save |
| Remove a property | Stored data with no matching property is skipped | Yes (the data is lost) |
| Rename a property | Treated as remove + add: the old value is skipped, the new property gets its default | **No**, without a migration |
| Change a property's type | The stored tag no longer matches. Some numeric conversions are handled; don't rely on it | **No**, without a migration |
| Change what a value *means* | The bytes load fine but are wrong | **No**, without a migration |

There is a second rule that is easy to miss. Tagged serialization writes the *difference from the
class defaults*: a property whose value equals its default in the class default object is not
written at all, and on load it simply keeps the default. That is invisible in normal use, and it
decides how to store a version number.

The obvious version field is wrong:

```cpp
UPROPERTY()
int32 Version = CurrentVersion;   // Don't.
```

Today `CurrentVersion` is 1, so the field equals its default and is not even written. Next year
you set `CurrentVersion` to 2. An old file has no `Version` in it, so on load the field keeps its
*new* default, 2, and the old file claims to be current. The migration never runs.

The robust pattern:

- The field defaults to **0**, which means "written before this version existed".
- Whoever creates a fresh save sets it to `CurrentVersion` explicitly.
- After loading, if the version is lower than current, migrate one step at a time, set the version,
  and save again.

The third example simulates an update. Version 1 of the game stored `Score`; version 2 renamed it
to `BestScore`. The old field stays in the class as a legacy property, so old files can still be
read into it.

`examples/ch18/ExSaveMigration.h`

```cpp
// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "ExSaveMigration.generated.h"

/**
 * Version 1 stored "Score". Version 2 renamed it to "BestScore".
 * Score stays as a legacy field so that version 1 files can still be read.
 */
UCLASS()
class UExVersionedSave : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 2;

	/** 0 = written before versioning existed. Set to CurrentVersion before saving. */
	UPROPERTY()
	int32 Version = 0;

	/** Version 1 only. Read during migration, then cleared. */
	UPROPERTY()
	int32 Score = 0;

	/** Version 2 and later. */
	UPROPERTY()
	int32 BestScore = 0;

	/** Upgrades this object step by step. Returns true if anything changed. */
	bool Migrate();
};

/** Writes a version 1 file, then loads and migrates it. Drop it in a level and press Play. */
UCLASS()
class AExSaveMigrationTester : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
```

`examples/ch18/ExSaveMigration.cpp`

```cpp
// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExSaveMigration.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogExMigration, Log, All);

namespace
{
	const TCHAR* MigrationSlot = TEXT("ExMigrationSlot");
}

bool UExVersionedSave::Migrate()
{
	const int32 Original = Version;

	if (Version < 2)
	{
		// v1 -> v2: Score became BestScore.
		BestScore = Score;
		Score = 0;
		Version = 2;
		UE_LOG(LogExMigration, Log, TEXT("Migrated v1 -> v2: BestScore = %d"), BestScore);
	}
	// A future "if (Version < 3) { ... Version = 3; }" goes here, after this step.

	return Version != Original;
}

void AExSaveMigrationTester::BeginPlay()
{
	Super::BeginPlay();

	// Pretend this file was written by last year's build: version 1, old field.
	UExVersionedSave* Old = Cast<UExVersionedSave>(UGameplayStatics::CreateSaveGameObject(UExVersionedSave::StaticClass()));
	Old->Version = 1;
	Old->Score = 250;
	UGameplayStatics::SaveGameToSlot(Old, MigrationSlot, 0);
	UE_LOG(LogExMigration, Log, TEXT("Wrote a version %d save with Score = %d"), Old->Version, Old->Score);

	// Today's build loads it.
	UExVersionedSave* Save = Cast<UExVersionedSave>(UGameplayStatics::LoadGameFromSlot(MigrationSlot, 0));
	if (!Save)
	{
		UE_LOG(LogExMigration, Warning, TEXT("No readable save - would start fresh"));
		return;
	}

	UE_LOG(LogExMigration, Log, TEXT("Loaded version %d (current is %d)"), Save->Version, UExVersionedSave::CurrentVersion);
	if (Save->Migrate())
	{
		// Write the upgraded data back, so the migration runs only once.
		UGameplayStatics::SaveGameToSlot(Save, MigrationSlot, 0);
	}

	UE_LOG(LogExMigration, Log, TEXT("Now version %d, BestScore %d"), Save->Version, Save->BestScore);
	UGameplayStatics::DeleteGameInSlot(MigrationSlot, 0);
}
```

Place `ExSaveMigrationTester` in a level and press **Play**.

Expected Output Log (filter: `LogExMigration`):

```
LogExMigration: Wrote a version 1 save with Score = 250
LogExMigration: Loaded version 1 (current is 2)
LogExMigration: Migrated v1 -> v2: BestScore = 250
LogExMigration: Now version 2, BestScore 250
```

- The first half plays "last year's build", writing `Version` 1 and `Score`. A real version 1
  class would not have had `BestScore` at all; that makes no difference, because a field equal to
  its default is not written anyway.
- `Migrate` is a chain of `if (Version < N)` steps, each upgrading exactly one version and in
  order. A version 1 file passes through every later step; a version 2 file skips the first. Never
  rewrite an old step: there are files in the wild that need it as it was.
- After migrating, the tester saves again, so the migration runs once per file, not on every
  launch.
- `Score` is reset to 0 after it is copied. Its default is 0, so the re-saved file no longer
  contains it at all.

## Subsystems that depend on each other

GridSnake will have three game instance subsystems: level flow (chapter 14), audio (chapter 17)
and a new save subsystem. The audio subsystem's `Initialize` needs the saved volumes, so the save
subsystem must have loaded the file first.

The engine creates and initializes a game instance's subsystems when the game instance starts, in
an order you must not rely on. If one subsystem needs another during `Initialize`, it says so:

```cpp
// Subsystems/SubsystemCollection.h (FSubsystemCollectionBase)
USubsystem* InitializeDependency(TSubclassOf<USubsystem> SubsystemClass);

template <typename TSubsystemClass>
TSubsystemClass* InitializeDependency();
```

`Collection.InitializeDependency<UGSSaveSubsystem>()` makes sure the save subsystem exists and has
finished its own `Initialize`, initializing it right now if necessary, and returns it.

- Call it from `Initialize` only. Afterwards every subsystem exists, and `GetSubsystem<T>()` is
  enough.
- Dependencies must not form a cycle. If A needs B and B needs A, neither can finish first;
  restructure so that one of them does not need the other during `Initialize`.

## GridSnake: saving progress

Everything that changed between `project/ch17` and `project/ch18`:

| File | Change |
|---|---|
| `Save/GSSaveGame.h` | New: the save object |
| `Save/GSSaveSubsystem.h/.cpp` | New: loads, answers questions, records progress, saves |
| `Core/GSAudioSubsystem.h/.cpp` | Reads the saved volumes; writes changes into the save |
| `Core/GSLevelFlowSubsystem.h/.cpp` | Records scores and unlocks in `CompleteLevel`, then saves |
| `UI/GSLevelButtonWidget.h/.cpp` | Locked or "Best N" |
| `UI/GSLevelSelectWidget.cpp` | Passes lock state and best score; focuses the first unlocked level |
| `UI/GSLevelEndWidget.cpp` | Shows the best score |
| `UI/GSMainMenuWidget.h/.cpp` | Continue button |
| `UI/GSMenuPanelWidget.h/.cpp` | `HandleBack()` hook |
| `UI/GSOptionsWidget.h/.cpp` | Reset Progress; saves when leaving the screen |

The new files go in a new `Save/` folder under `Source/GridSnake/`. The module's include path
already covers subfolders, so `#include "Save/GSSaveGame.h"` works, like `Core/` and `UI/`.

### The save object

`Save/GSSaveGame.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GSSaveGame.generated.h"

/**
 * Everything GridSnake remembers between sessions. A USaveGame is serialized
 * property by property, by name: every UPROPERTY that differs from its class
 * default ends up in the .sav file. Adding a field is safe; bump CurrentVersion
 * when the meaning of existing data changes.
 */
UCLASS()
class UGSSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	/**
	 * The version this save was written with; the save subsystem sets it.
	 * The default is 0, not CurrentVersion: a value equal to its default is not
	 * written, so an old file would otherwise load claiming to be current.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Save")
	int32 SaveVersion = 0;

	/** Levels 0..UnlockedLevelCount-1 can be played. */
	UPROPERTY(VisibleAnywhere, Category = "Progress")
	int32 UnlockedLevelCount = 1;

	/** Best level score per level id (the map's asset name, e.g. L_Level01). */
	UPROPERTY(VisibleAnywhere, Category = "Progress")
	TMap<FName, int32> BestScores;

	/** Best total for a whole run from the first level to the last. */
	UPROPERTY(VisibleAnywhere, Category = "Progress")
	int32 BestRunScore = 0;

	UPROPERTY(VisibleAnywhere, Category = "Options")
	float MusicVolume = 0.8f;

	UPROPERTY(VisibleAnywhere, Category = "Options")
	float SfxVolume = 1.0f;
};
```

- There is no `.cpp`: the class is only data.
- `SaveVersion` follows the versioning pattern above: default 0, set by whoever creates a fresh
  save. `CurrentVersion` is a `static constexpr`, part of the class rather than of each object, so
  it is never saved.
- `UnlockedLevelCount` 1 means a brand-new save has level 1 (index 0) open. Storing a count rather
  than a list of unlocked levels works because GridSnake's levels unlock strictly in order.
- `BestScores` is keyed by level *ID*, the map's asset name (`FGSLevelInfo::GetLevelId()`,
  prepared in chapter 13), not by index. Insert a new level at position 2 of `DA_LevelList` and the
  scores still belong to the right maps. The unlock count is index-based and would shift; that is
  the kind of change a version bump and a migration step exist for.
- `VisibleAnywhere` has nothing to do with saving; it would only matter if the object were shown
  in a Details panel. A bare `UPROPERTY()` is saved just the same.

### The save subsystem

`Save/GSSaveSubsystem.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GSSaveSubsystem.generated.h"

class UGSSaveGame;

/**
 * Owns the one save game: loads it when the game starts, answers questions
 * about progress, and writes it to disk after changes.
 * Saves go to Saved/SaveGames/GridSnake.sav (per user on most platforms).
 */
UCLASS()
class UGSSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	bool IsLevelUnlocked(int32 LevelIndex) const;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	int32 GetUnlockedLevelCount() const;

	/** True once the player has unlocked anything beyond the first level. */
	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	bool HasProgress() const;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	int32 GetBestScore(FName LevelId) const;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	int32 GetBestRunScore() const;

	void UnlockLevel(int32 LevelIndex);

	/** Records a level score. Returns true if it is a new best for that level. */
	bool SubmitLevelScore(FName LevelId, int32 Score);

	/** Records a finished run's total. Returns true if it is a new best. */
	bool SubmitRunScore(int32 Score);

	float GetMusicVolume() const;
	float GetSfxVolume() const;
	void SetVolumes(float MusicVolume, float SfxVolume);

	/** Forgets unlocked levels and scores; keeps the options. Saves immediately. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Save")
	void ResetProgress();

	/** Writes the save game to disk in the background. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Save")
	void Save();

	static const FString SlotName;
	static constexpr int32 UserIndex = 0;

private:
	void HandleSaveFinished(const FString& InSlotName, const int32 InUserIndex, bool bSuccess);

	/** Always valid after Initialize: loaded from disk, or a fresh one. */
	UPROPERTY(Transient)
	TObjectPtr<UGSSaveGame> SaveGame;
};
```

`Save/GSSaveSubsystem.cpp`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Save/GSSaveSubsystem.h"
#include "GridSnake.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GSSaveGame.h"

const FString UGSSaveSubsystem::SlotName(TEXT("GridSnake"));

void UGSSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		// Cast returns nullptr if the file holds some other class (or is damaged).
		SaveGame = Cast<UGSSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	}

	if (!SaveGame)
	{
		SaveGame = Cast<UGSSaveGame>(UGameplayStatics::CreateSaveGameObject(UGSSaveGame::StaticClass()));
		SaveGame->SaveVersion = UGSSaveGame::CurrentVersion;
		UE_LOG(LogGridSnake, Log, TEXT("No save game in slot %s - starting fresh"), *SlotName);
		return;
	}

	if (SaveGame->SaveVersion < UGSSaveGame::CurrentVersion)
	{
		// Upgrade older saves here, one version step at a time.
		SaveGame->SaveVersion = UGSSaveGame::CurrentVersion;
	}

	UE_LOG(LogGridSnake, Log, TEXT("Loaded save game: %d level(s) unlocked, best run %d"),
		SaveGame->UnlockedLevelCount, SaveGame->BestRunScore);
}

bool UGSSaveSubsystem::IsLevelUnlocked(int32 LevelIndex) const
{
	return LevelIndex >= 0 && LevelIndex < SaveGame->UnlockedLevelCount;
}

int32 UGSSaveSubsystem::GetUnlockedLevelCount() const
{
	return SaveGame->UnlockedLevelCount;
}

bool UGSSaveSubsystem::HasProgress() const
{
	return SaveGame->UnlockedLevelCount > 1;
}

int32 UGSSaveSubsystem::GetBestScore(FName LevelId) const
{
	const int32* Best = SaveGame->BestScores.Find(LevelId);
	return Best ? *Best : 0;
}

int32 UGSSaveSubsystem::GetBestRunScore() const
{
	return SaveGame->BestRunScore;
}

void UGSSaveSubsystem::UnlockLevel(int32 LevelIndex)
{
	SaveGame->UnlockedLevelCount = FMath::Max(SaveGame->UnlockedLevelCount, LevelIndex + 1);
}

bool UGSSaveSubsystem::SubmitLevelScore(FName LevelId, int32 Score)
{
	int32& Best = SaveGame->BestScores.FindOrAdd(LevelId);
	if (Score <= Best)
	{
		return false;
	}
	Best = Score;
	return true;
}

bool UGSSaveSubsystem::SubmitRunScore(int32 Score)
{
	if (Score <= SaveGame->BestRunScore)
	{
		return false;
	}
	SaveGame->BestRunScore = Score;
	return true;
}

float UGSSaveSubsystem::GetMusicVolume() const
{
	return SaveGame->MusicVolume;
}

float UGSSaveSubsystem::GetSfxVolume() const
{
	return SaveGame->SfxVolume;
}

void UGSSaveSubsystem::SetVolumes(float MusicVolume, float SfxVolume)
{
	SaveGame->MusicVolume = MusicVolume;
	SaveGame->SfxVolume = SfxVolume;
}

void UGSSaveSubsystem::ResetProgress()
{
	SaveGame->UnlockedLevelCount = 1;
	SaveGame->BestScores.Reset();
	SaveGame->BestRunScore = 0;
	Save();
}

void UGSSaveSubsystem::Save()
{
	// The object is serialized right away on the game thread; only the file
	// write happens in the background, so changing SaveGame afterwards is safe.
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SlotName, UserIndex,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UGSSaveSubsystem::HandleSaveFinished));
}

void UGSSaveSubsystem::HandleSaveFinished(const FString& InSlotName, const int32 InUserIndex, bool bSuccess)
{
	UE_LOG(LogGridSnake, Log, TEXT("Save to slot %s %s"), *InSlotName, bSuccess ? TEXT("succeeded") : TEXT("FAILED"));
}
```

How it works:

- **`Initialize` loads synchronously.** The game cannot show a level-select screen without
  knowing what is unlocked, and the file is tiny. The `Cast` returns `nullptr` both when the load
  fails and when the slot holds some other class, and both cases fall through to a fresh save.
- **A fresh save gets `SaveVersion = CurrentVersion`.** This is the line the versioning section
  insisted on. It is not written to disk yet: the first `Save()` does that.
- **The upgrade block** is where migration steps will go. At version 1 there is nothing to
  convert, so it only stamps the version. An older file is written back at the next `Save()`.
- **`SaveGame` is never null after `Initialize`**, so the query functions do not check it. One
  object lives for the whole session; every system reads and changes that one, and `Save()` writes
  it. Nothing else calls `LoadGameFromSlot`, so there are never two copies that disagree.
- **Changing and saving are separate.** `UnlockLevel`, `SubmitLevelScore` and `SetVolumes` only
  change memory. The caller decides when to write, so one level completion that changes three
  values costs one file write. `ResetProgress` is the exception: a player who presses Reset
  expects it to be final.
- `SubmitLevelScore` uses `TMap::FindOrAdd`, which returns a reference to the existing value or
  to a new zero-initialized one, so `Score <= Best` covers both cases.
- **`Save()` is asynchronous** and logs its result. The comment records the important fact from
  the async section: the data is captured during the call.

### Volumes

The audio subsystem from chapter 17 gets an `Initialize` that reads the saved volumes, using the
dependency mechanism. `Core/GSAudioSubsystem.cpp`:

```cpp
#include "Save/GSSaveSubsystem.h"
...
void UGSAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Subsystems are created in no particular order. Ask for the save subsystem
	// explicitly so that it has loaded the save game before we read it.
	const UGSSaveSubsystem* SaveSubsystem = Collection.InitializeDependency<UGSSaveSubsystem>();
	MusicVolume = SaveSubsystem->GetMusicVolume();
	SfxVolume = SaveSubsystem->GetSfxVolume();
}
...
void UGSAudioSubsystem::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->SetVolumes(MusicVolume, SfxVolume);
	ApplyVolumes();
}
```

`SetSfxVolume` gets the same line. The header declares the `Initialize` override and notes:
"Volume setters update the save game in memory; the options screen saves it."

Why not save in the setter? A slider fires `OnValueChanged` many times per second while it is
dragged. Writing a file each time would be wasteful even asynchronously. Instead the options
screen saves once, when the player leaves it. `UGSMenuPanelWidget` (chapter 16) gets a hook for
that, and `UGSOptionsWidget` overrides it:

```cpp
// UI/GSMenuPanelWidget.h: /** Called when Back is clicked, before OnBackRequested. */
	virtual void HandleBack() {}

// UI/GSMenuPanelWidget.cpp
void UGSMenuPanelWidget::HandleBackClicked()
{
	HandleBack();
	OnBackRequested.Broadcast();
}

// UI/GSOptionsWidget.cpp (declared as a protected override in the header)
void UGSOptionsWidget::HandleBack()
{
	GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->Save();
}
```

`HandleBack` is a plain C++ virtual with an empty default: a *template method*. The base class
keeps control of the order (the panel's own work first, then the event that switches panels),
and subclasses fill in only what they need. The level-select panel does not override it.

### Recording progress

Progress is recorded in exactly one place: `UGSLevelFlowSubsystem::CompleteLevel`, which already
ran when a level was finished (chapter 14). `Core/GSLevelFlowSubsystem.cpp`:

```cpp
#include "Save/GSSaveSubsystem.h"
...
bool UGSLevelFlowSubsystem::CompleteLevel(int32 LevelScore)
{
	RunScore += LevelScore;
	UE_LOG(LogGridSnake, Log, TEXT("Level %d complete: +%d, run score %d"), CurrentLevelIndex, LevelScore, RunScore);

	// Record progress: best score, the next level unlocked, or a finished run.
	UGSSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UGSSaveSubsystem>();
	const bool bHasNextLevel = HasNextLevel();
	if (const FGSLevelInfo* Level = GetCurrentLevelInfo())
	{
		SaveSubsystem->SubmitLevelScore(Level->GetLevelId(), LevelScore);
		if (bHasNextLevel)
		{
			SaveSubsystem->UnlockLevel(CurrentLevelIndex + 1);
		}
		else
		{
			SaveSubsystem->SubmitRunScore(RunScore);
		}
		SaveSubsystem->Save();
	}

	return bHasNextLevel;
}
```

- Completing level N unlocks level N + 1. `UnlockLevel` uses `FMath::Max`, so replaying an early
  level never relocks later ones.
- The best *run* is only recorded when the last level is finished.
- `GetCurrentLevelInfo()` is null on a map that is not in the level list, such as a test map, so
  nothing is recorded there. A listed level played directly from the editor does count, which is
  handy for testing unlocks.
- Crashing records nothing. Only finished levels count.

### The menus

The level-select screen asks the save subsystem about every button. In
`UGSLevelSelectWidget::Refresh` (`UI/GSLevelSelectWidget.cpp`), the loop becomes:

```cpp
		const FGSLevelInfo& Level = *LevelList->GetLevel(Index);
		const bool bUnlocked = SaveSubsystem->IsLevelUnlocked(Index);
		...
		LevelButton->Setup(Index, Level, bUnlocked, SaveSubsystem->GetBestScore(Level.GetLevelId()));
		...
		if (!FirstButton && bUnlocked)
		{
			FirstButton = LevelButton;
		}
```

with `SaveSubsystem` fetched once before the loop. Each button shows the answer
(`UI/GSLevelButtonWidget.cpp`):

```cpp
void UGSLevelButtonWidget::Setup(int32 InLevelIndex, const FGSLevelInfo& Level, bool bUnlocked, int32 BestScore)
{
	LevelIndex = InLevelIndex;
	NameText->SetText(FText::Format(LOCTEXT("LevelButton", "{0}. {1}"), LevelIndex + 1, Level.DisplayName));

	// A disabled button cannot be clicked or focused.
	Button->SetIsEnabled(bUnlocked);

	if (DetailText)
	{
		DetailText->SetText(bUnlocked
			? FText::Format(LOCTEXT("Best", "Best {0}"), BestScore)
			: LOCTEXT("Locked", "Locked"));
	}
}
```

- A disabled button is greyed out by its style and skipped by keyboard and gamepad navigation, so
  locking needs no special input code. Focus must go to an *enabled* button, hence
  `!FirstButton && bUnlocked`.
- `DetailText` is `BindWidgetOptional` (chapter 16), so the code checks it. In chapter 16 it was
  collapsed; now it has something to say. If your `W_LevelButton` has no `DetailText`, it still
  works but shows no best score or lock label.
- The level list is rebuilt in `Refresh`, every time the panel opens, so it always shows the
  current save.

The level-end screen (`UI/GSLevelEndWidget.cpp`) now formats
`"Level score {0}   Best {1}   Total {2}"`, fetching the best score with `GetBestScore(Level->GetLevelId())`. `CompleteLevel` ran
before the screen appeared, so a new record already shows as the best.

The main menu gets a `BindWidget` `UButton* ContinueButton`, bound to `HandleContinueClicked`
in `NativeOnInitialized` (`UI/GSMainMenuWidget.h/.cpp`):

```cpp
void UGSMainMenuWidget::ShowMainPanel()
{
	PanelSwitcher->SetActiveWidget(MainPanel);

	// Progress can change on the options screen (Reset Progress), so check every time.
	const bool bCanContinue = GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->HasProgress();
	ContinueButton->SetVisibility(bCanContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	(bCanContinue ? ContinueButton : PlayButton)->SetFocus();
}
...
void UGSMainMenuWidget::HandleContinueClicked()
{
	const int32 LastUnlocked = GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->GetUnlockedLevelCount() - 1;
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->StartRun(LastUnlocked);
}
```

- `Collapsed` removes the button from the layout, so the menu has no gap when it is hidden.
  `Hidden` would keep its space.
- Continue starts a run at the furthest unlocked level. Such a run skips the earlier levels, so
  its total is lower than a full run's, yet it is still submitted as a run when the last level is
  finished. That only matters while no full run exists; a stricter game would remember where the
  run started. Exercise 3 offers a different meaning for Continue.

Finally, `UGSOptionsWidget` gets a `BindWidget` `UButton* ResetProgressButton` whose click handler
is one line: `GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->ResetProgress();`. A real game
would ask "Are you sure?" first; that is a small widget of its own.

### Editor steps

These follow `project/ch18/ASSETS.md`. The save object needs no asset: it is created from C++.

1. **Build** (close the editor, build, reopen).
2. **`W_MainMenu`:** add a **Button** named exactly **ContinueButton** as the *first* button in
   `MainPanel` (under the title, above `PlayButton`), with a **Text** child "Continue". Copy the Play button and rename the copy to
   match the style. Give it the `SW_UIClick` pressed sound like the others.
3. **`W_Options`:** add a **Button** named **ResetProgressButton** with the text "Reset Progress",
   just above `BackButton`.
4. **`W_LevelButton`:** if you skipped the optional **DetailText** in chapter 16, add a **Text**
   named **DetailText** under the name text now.
5. Compile and save the three widget Blueprints. The first two will not compile until the new
   `BindWidget` names exist.

### Testing it

Delete `Saved/SaveGames/GridSnake.sav` in your project folder if it exists, open `L_MainMenu`
and press **Play**. There is no Continue button, and **Select Level** shows level 1 with "Best 0"
and the others locked. Press **Play** and finish level 1. The level-end screen shows
`Level score 300   Best 300   Total 300`; back in the menu, Continue is there and focused, and level 2 is
unlocked. Then stop Play In Editor and press **Play** again.

Expected Output Log (filter: `LogGridSnake`; other lines omitted). The first two lines may swap,
because subsystems initialize in no fixed order:

```
LogGridSnake: No save game in slot GridSnake - starting fresh
LogGridSnake: Level flow subsystem initialised
...
LogGridSnake: Level 0 complete: +300, run score 300
LogGridSnake: Save to slot GridSnake succeeded
                                          (Play stopped and started again)
LogGridSnake: Loaded save game: 2 level(s) unlocked, best run 0
```

"best run 0" is correct: the best *run* is only recorded when the final level is finished.

More checks:

- Move the volume sliders and press **Back**: another `Save to slot GridSnake succeeded`. Restart
  Play and the sliders and music keep your values.
- **Options > Reset Progress** saves, and Continue disappears. The volumes stay.
- `GridSnake.sav` is a few hundred bytes of binary, but in a hex editor you can spot the class
  path and property names such as `UnlockedLevelCount`: tagged serialization, visible.
- `Save to slot ... FAILED` means the file could not be written, usually because another program
  has it open.

## Summary

- A save game is a `USaveGame` subclass whose `UPROPERTY` fields are the data. Plain members and
  `Transient` properties are not saved. Save identifiers, not pointers to level objects.
- `CreateSaveGameObject`, `SaveGameToSlot`, `LoadGameFromSlot`, `DoesSaveGameExist` and
  `DeleteGameInSlot` in `UGameplayStatics` cover the basics. Loading always creates a new object;
  `Cast` it, and treat `nullptr` as "no usable save".
- Slots are names, not paths. On desktop they become `Saved/SaveGames/<Slot>.sav`.
- `AsyncSaveGameToSlot` serializes during the call, writes on a worker thread, and calls a
  single-cast delegate on the game thread afterwards. Load synchronously at startup; save
  asynchronously during play; save synchronously when quitting.
- Properties are matched by name. Adding and removing are safe; renaming, retyping and changing
  meaning need a version number and migration steps.
- Values equal to their class defaults are not written. So a version field defaults to 0 and is
  set to the current version explicitly.
- `Collection.InitializeDependency<T>()` in `Initialize` guarantees another subsystem is ready.
- One subsystem owns the one save object; others change it through it and choose when to save.

GridSnake is now a complete game: it plays, sounds right, and remembers. Chapter 19 turns to the
tools for finding out what a game is doing when it misbehaves.

## Exercises

1. **Total gems.** Count every gem the player has ever collected, and show it on the level-select
   screen.

   <details><summary>Solution</summary>

   In `Save/GSSaveGame.h`, append a field:

   ```cpp
   	UPROPERTY(VisibleAnywhere, Category = "Stats")
   	int32 TotalGemsCollected = 0;
   ```

   No version bump: an old file has no such field, so it loads as 0, which is the right answer for
   a save from before the feature existed. In `UGSSaveSubsystem`:

   ```cpp
   void UGSSaveSubsystem::AddGemCollected()
   {
   	++SaveGame->TotalGemsCollected;
   }

   int32 UGSSaveSubsystem::GetTotalGemsCollected() const
   {
   	return SaveGame->TotalGemsCollected;
   }
   ```

   Call `GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->AddGemCollected()` in
   `AGSGameMode::OnGemCollected`. Do not save there: the next `Save()`, at the end of the level,
   writes it (gems from a crashed level ride along with it). To display the total, add a
   `BindWidgetOptional` `UTextBlock* TotalGemsText` to `UGSLevelSelectWidget` and set it in
   `Refresh`.
   </details>

2. **Three save slots.** Let the player pick one of three profiles from the main menu.

   <details><summary>Solution</summary>

   Replace the constant slot with a current one and move the loading code out of `Initialize`
   into a new function:

   ```cpp
   // GSSaveSubsystem.h: public
   	/** Loads (or creates) profile 0, 1 or 2. Saves go to that profile from now on. */
   	UFUNCTION(BlueprintCallable, Category = "GridSnake|Save")
   	void SelectProfile(int32 ProfileIndex);
   // private
   	FString CurrentSlot = TEXT("GridSnake_0");

   // GSSaveSubsystem.cpp
   void UGSSaveSubsystem::SelectProfile(int32 ProfileIndex)
   {
   	CurrentSlot = FString::Printf(TEXT("GridSnake_%d"), FMath::Clamp(ProfileIndex, 0, 2));
   	SaveGame = nullptr;
   	// ...the old body of Initialize after Super::Initialize, with CurrentSlot for SlotName...
   }
   ```

   `Initialize` calls `SelectProfile(0)`; `Save()` uses `CurrentSlot`. A new main-menu panel with
   three buttons calls `SelectProfile` and then `ShowMainPanel()`. The volumes are now per
   profile, which is odd for a machine setting: move them to their own `"GridSnake_Settings"`
   slot, or have the audio subsystem re-read them after a profile change.
   </details>

3. **Continue where you left off.** Make Continue start the level the player last *played*, not
   the furthest unlocked one.

   <details><summary>Solution</summary>

   Add `UPROPERTY(VisibleAnywhere, Category = "Progress") FName LastPlayedLevel;` to
   `UGSSaveGame`, with `SetLastPlayedLevel(FName)` and `GetLastPlayedLevel()` on the subsystem.
   In `UGSLevelFlowSubsystem::NotifyLevelStarted`, when `GetCurrentLevelInfo()` is valid, call
   `SetLastPlayedLevel(Level->GetLevelId())` and `Save()`. In
   `UGSMainMenuWidget::HandleContinueClicked`, look the ID up with the level list's
   `FindLevelIndex` (include `Core/GSSettings.h` and `Data/GSLevelList.h`):

   ```cpp
   	const UGSSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UGSSaveSubsystem>();
   	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
   	int32 Start = LevelList ? LevelList->FindLevelIndex(SaveSubsystem->GetLastPlayedLevel()) : INDEX_NONE;
   	if (!SaveSubsystem->IsLevelUnlocked(Start))
   	{
   		Start = SaveSubsystem->GetUnlockedLevelCount() - 1;
   	}
   	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->StartRun(Start);
   ```

   Storing the ID, not the index, finds the right level even after the list is reordered. A level
   that no longer exists gives `INDEX_NONE`, which `IsLevelUnlocked` rejects, so Continue falls
   back to the furthest unlocked level.
   </details>

4. **An unreadable save.** Find out what GridSnake does when the save slot holds something it
   cannot use, and make it log a warning.

   <details><summary>Solution</summary>

   Create the problem safely with the first example: place `ExSaveTester` in a level, set its
   **Slot Name** to `GridSnake`, and press Play once. The slot now holds a `UExSaveGame`. Remove
   the tester and play again: `LoadGameFromSlot` succeeds, the `Cast<UGSSaveGame>` returns
   `nullptr`, and you see `No save game in slot GridSnake - starting fresh`. The first save then
   overwrites the file.

   That message is misleading: there *was* a save. In `Initialize`:

   ```cpp
   	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
   	{
   		SaveGame = Cast<UGSSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
   		if (!SaveGame)
   		{
   			UE_LOG(LogGridSnake, Warning, TEXT("Save game in slot %s could not be used - starting fresh"), *SlotName);
   		}
   	}
   ```

   A damaged file takes the same path whenever `LoadGameFromSlot` rejects it. A careful game
   would also keep the bad file rather than overwrite it.
   </details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-18--saving-progress)
