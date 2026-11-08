# Chapter 13 — Data-Driven Levels

GridSnake has one level. Its rules (how fast the snake moves, what a gem is worth) are fine for one
level. They come from defaults in C++ and from `UGSSettings`. Three levels that each play
differently would need either a pile of `if (MapName == ...)` checks or a better idea: describe each
level as *data*, and let the code read it. This chapter covers the tools Unreal gives you for that:

- **Data assets**: assets whose fields you define in C++ and fill in the editor.
- **Hard and soft references**: what loads an asset, and how to avoid loading everything at once.
- **Curve assets**: a number that changes with another number, drawn in an editor.
- **Developer settings with asset paths**: how code finds "the" data asset without hard-coding a
  path.
- **DataTables**: a comparison, because many projects use them for the same job.

GridSnake gains `DA_LevelList`, the game's progression as one asset. It lists three levels, each
with a map, a display name, a speed, a gem value and an optional speed-up curve. Two new maps,
`L_Level02` and `L_Level03`, join `L_Level01`. Reaching the exit now opens the next map in the list.

Previous: [Chapter 12](12-flipbook-animation-from-cpp.md). Next:
[Chapter 14](14-level-transitions-and-persistence.md).

## Data, not code

A number in C++ needs a programmer, a compile and (often) an editor restart to change. A number in
an asset needs a designer, a text field and Ctrl+S. While you tune a game you change numbers all the
time, so they belong where changing them is cheap. There is a second reason. Code that reads "the
level's speed" from data works for a level that does not exist yet. Code that says
`if (Level == 2) Speed = 0.15f;` must change for every new level.

Unreal offers several places to keep game data. You have used two of them already:

| Where | Edited in | Good for | Weak at |
|---|---|---|---|
| Blueprint class defaults (ch05) | Blueprint editor, Class Defaults | Per-class tuning: `BP_GSGem`'s flipbook, `BP_GSSnake`'s input action | Lists of things; data shared by many classes |
| Config / developer settings (ch03) | Project Settings, `.ini` files | Project-wide switches: gems per level, which asset is "the" level list | Asset-heavy data; anything a designer adds many of |
| **Data assets** (this chapter) | Their own editor window, a Details panel | One "thing" with typed fields and asset references: a level list, an item, a character | Hundreds of similar rows |
| **DataTables** (this chapter) | A spreadsheet-like editor; CSV/JSON import | Many rows of the same struct, often made in a spreadsheet: enemy stats, loot tables, dialogue | Ordered lists; nested data; references are awkward to type in a CSV |
| Curve assets (this chapter) | A curve editor | One number as a function of another: speed by length, damage by level | Anything that is not a function |

All of them have one thing in common: you define the *shape* of the data in C++ (a `UCLASS` or a
`USTRUCT` with `UPROPERTY` fields) and reflection does the rest. The editor builds the forms, saving
writes the values into the asset, and loading reads them back. Chapter 3 showed reflection driving
the Details panel and config files. This chapter uses the same machinery for assets.

## Primary data assets

A data asset is an asset whose class you write. You derive from `UDataAsset` or, more commonly,
from `UPrimaryDataAsset` (both in `Engine/DataAsset.h`), add `UPROPERTY` fields, and build. The
editor can then create instances of your class in the Content Browser. Each instance is a `.uasset`
file with its own values, edited in a Details panel.

`UPrimaryDataAsset` adds one thing to `UDataAsset`: it is a *primary asset*. The **Asset Manager**
can find and load primary assets by an ID (a type plus a name) instead of a path. A game with many
items, say, can ask the Asset Manager "give me every asset of type `Item`" and load them in bulk.
You do not need the Asset Manager to use a data asset: a reference to it (this section and the next)
loads it like any other asset. Deriving from `UPrimaryDataAsset` costs nothing and keeps that door
open, so it is the usual choice.

### Example: an item definition

`examples/ch13/ExItemDefinition.h`

```cpp
// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "ExItemDefinition.generated.h"

class UTexture2D;

/**
 * One kind of item, edited as an asset instead of in code.
 * Create one with Content Browser > right-click > Miscellaneous > Data Asset >
 * ExItemDefinition, for example DA_Sword.
 */
UCLASS(BlueprintType)
class UExItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 Price = 10;

	/** Soft: saving the item stores the texture's path; the texture loads only on request. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** The asset manager's name for this asset: type "ExItem", name = the asset name. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

/** Place one in a level, set Item, press Play: it logs the item and loads its icon. */
UCLASS()
class AExItemInspector : public AActor
{
	GENERATED_BODY()

public:
	/** A hard reference: the item asset loads together with the level. */
	UPROPERTY(EditAnywhere, Category = "Item")
	TObjectPtr<UExItemDefinition> Item;

protected:
	virtual void BeginPlay() override;
};
```

`examples/ch13/ExItemDefinition.cpp`

```cpp
// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExItemDefinition.h"
#include "Engine/Texture2D.h"

FPrimaryAssetId UExItemDefinition::GetPrimaryAssetId() const
{
	// Every item shares one type, so an asset manager query for "ExItem"
	// finds them all, whatever their class or folder.
	static const FName ItemType(TEXT("ExItem"));
	return FPrimaryAssetId(FPrimaryAssetType(ItemType), GetFName());
}

void AExItemInspector::BeginPlay()
{
	Super::BeginPlay();

	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: set Item in the Details panel"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Item %s: \"%s\", price %d, id %s"),
		*Item->GetName(), *Item->DisplayName.ToString(), Item->Price,
		*Item->GetPrimaryAssetId().ToString());

	if (Item->Icon.IsNull())
	{
		UE_LOG(LogTemp, Log, TEXT("No icon set"));
		return;
	}

	// IsPending: the path is set but the object is not in memory (yet).
	UE_LOG(LogTemp, Log, TEXT("Icon %s, in memory: %s"),
		*Item->Icon.ToString(), Item->Icon.IsPending() ? TEXT("no") : TEXT("yes"));

	// Loads now and blocks until done. Fine for one small texture, not for a level.
	if (UTexture2D* Texture = Item->Icon.LoadSynchronous())
	{
		UE_LOG(LogTemp, Log, TEXT("Loaded %s (%dx%d)"), *Texture->GetName(), Texture->GetSizeX(), Texture->GetSizeY());
	}
}
```

Build.cs modules: `Core`, `CoreUObject`, `Engine` (every new C++ project has them).

To see it:

1. Copy both files into your module's source folder and build (or use Live Coding, Ctrl+Alt+F11,
   for a `.cpp`-only change; a new `UCLASS` is safest with a full build from the IDE, chapter 2).
2. In the Content Browser, right-click an empty area and choose **Miscellaneous > Data Asset**. A
   class picker opens (titled **Pick Class For Data Asset Instance**; the label may differ slightly
   in your version). Select `ExItemDefinition` and click **Select**. Name the new asset `DA_Sword`.
3. Double-click `DA_Sword`. Its editor is a Details panel with an **Item** category. Set
   **Display Name** = `Sword`, **Price** = `25`, and pick any texture for **Icon** (in GridSnake,
   `T_Tiles`). Save.
4. Drag `ExItemInspector` from **C++ Classes** into a level. In its Details panel set **Item** =
   `DA_Sword`.
5. Press Play.

Expected Output Log (filter: `LogTemp`), with `T_Tiles` as the icon:

```text
LogTemp: Item DA_Sword: "Sword", price 25, id ExItem:DA_Sword
LogTemp: Icon /Game/GridSnake/Art/Textures/T_Tiles.T_Tiles, in memory: yes
LogTemp: Loaded T_Tiles (64x32)
```

- **The class defines the form, the asset holds the values.** `DisplayName`, `Price` and `Icon`
  appear in the asset's editor because they are `EditAnywhere` properties. `BlueprintReadOnly` lets
  Blueprints read them too. You can make a hundred items from this one class without touching C++.
- **`FText` for anything a player reads.** `DisplayName` is `FText`, not `FString`, so it can be
  translated later (chapter 3 introduced the three string types).
- **`GetPrimaryAssetId()`** returns the asset's ID as a type and a name; `ToString()` prints it as
  `Type:Name`. The default implementation in `UPrimaryDataAsset` derives the type from the class.
  The override shows the common pattern of naming the type yourself, so several item classes can
  share the type `ExItem`. To let the Asset Manager *find* assets by type, you also list the type
  under **Project Settings > Game > Asset Manager > Primary Asset Types to Scan**. This course does
  not need that, because it always reaches its data assets through references.
- **"in memory: yes" in the editor is normal.** `IsPending()` is true when the soft reference holds
  a path but the object is not loaded. In the editor, any asset you have opened, or that the Content
  Browser has shown a thumbnail of, may already be in memory, so you will often see `yes`. In a
  packaged game, or in a fresh editor session where nothing has touched the texture, you see `no`.
  The next section explains why.

### GridSnake's level list

GridSnake's data asset describes the whole progression. Each level is a struct, and the asset holds
an array of them in play order. This is a new folder, `Source/GridSnake/Data/`.

`Source/GridSnake/Data/GSLevelList.h` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSLevelList.generated.h"

class UCurveFloat;

/** Everything the game needs to know about one level. */
USTRUCT(BlueprintType)
struct FGSLevelInfo
{
	GENERATED_BODY()

	/** Soft reference: naming a map here does not load it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UWorld> Map;

	/** Shown on the HUD and in level select. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FText DisplayName;

	/** Seconds per step at the starting length. Smaller is faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (ClampMin = "0.02", Units = "s"))
	float StepInterval = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (ClampMin = "0"))
	int32 PointsPerGem = 100;

	/** Optional. X = snake length, Y = step interval multiplier (below 1 speeds the snake up). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TObjectPtr<UCurveFloat> SpeedCurve;

	/** Stable key for this level (the map's asset name, e.g. L_Level01). Used for saving in chapter 18. */
	FName GetLevelId() const { return FName(*Map.GetAssetName()); }
};

/**
 * The ordered list of levels: the game's progression, edited as data.
 * DA_LevelList is the one instance; UGSSettings points at it.
 */
UCLASS(BlueprintType)
class UGSLevelList : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels")
	TArray<FGSLevelInfo> Levels;

	/** Index of the level whose map has this asset name, or INDEX_NONE. */
	int32 FindLevelIndex(FName LevelId) const;

	/** nullptr if Index is out of range. */
	const FGSLevelInfo* GetLevel(int32 Index) const;

	int32 Num() const { return Levels.Num(); }
};
```

`Source/GridSnake/Data/GSLevelList.cpp` (new)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Data/GSLevelList.h"

int32 UGSLevelList::FindLevelIndex(FName LevelId) const
{
	return Levels.IndexOfByPredicate([LevelId](const FGSLevelInfo& Level)
	{
		return Level.GetLevelId() == LevelId;
	});
}

const FGSLevelInfo* UGSLevelList::GetLevel(int32 Index) const
{
	return Levels.IsValidIndex(Index) ? &Levels[Index] : nullptr;
}
```

- **A `USTRUCT` inside a `UCLASS`.** `FGSLevelInfo` is a plain struct with `GENERATED_BODY()` and
  `UPROPERTY` fields, the same as the structs in chapter 3. Because it is reflected, a
  `TArray<FGSLevelInfo>` shows in the editor as a list you can add to, remove from, reorder and
  expand. Each element shows its five fields. `BlueprintType` makes the struct usable as a Blueprint
  variable type, which chapter 16's level select menu needs.
- **One asset, many levels.** The list lives in one asset rather than one asset per level. Order
  matters (it *is* the progression), and one array in one asset makes the order visible and easy to
  change by dragging.
- **The map is a soft reference; the curve is a hard one.** That difference is the subject of the
  next section.
- **The level ID.** `GetLevelId()` turns the soft map reference into the map's asset name, such as
  `L_Level01`. `GetAssetName()` only reads the stored path; it does not load the map. The ID is
  stable as long as nobody renames the map, and it is what the game uses to find "the current
  level" and, in chapter 18, to save which levels are unlocked.
- **`FindLevelIndex`** uses `TArray::IndexOfByPredicate` with a lambda that captures the ID. It
  returns `INDEX_NONE` (-1) when no element matches.
- **`GetLevel` returns a pointer, or `nullptr`.** That makes "is there a level at this index?" and
  "give me that level" one call. The pointer points *into* the array, so it is only valid while the
  asset is alive and unchanged. Use it straight away; do not store it in a member. That rule matters
  more than it looks, as the next section shows.

## Hard and soft references

Every asset reference is one of two kinds:

- A **hard reference** is a pointer property: `TObjectPtr<UCurveFloat> SpeedCurve`,
  `TObjectPtr<UExItemDefinition> Item`, a `TSubclassOf<>` to a Blueprint class. When the object
  holding it is loaded, the engine loads the referenced asset too, first, so the pointer is valid
  by the time you read it. And as long as the holder is alive, the garbage collector will not
  free the target (chapter 3).
- A **soft reference** is `TSoftObjectPtr<T>` (or `TSoftClassPtr<T>` for classes). On disk and in
  memory it is a *path*, such as `/Game/GridSnake/Maps/L_Level02.L_Level02`, plus a weak pointer
  to the object if it happens to be loaded. Loading the holder loads nothing else. Nothing is kept
  alive by it.

Hard references are convenient: the asset is simply there. Their cost is that they chain. A level
hard-references a Blueprint, which hard-references a sound, which hard-references... Load one asset
and you load everything it can reach. For small assets that is fine and even desirable: GridSnake's
gem must have its flipbook ready the instant it spawns. For big assets that are only needed
sometimes, it is waste: memory, and loading time paid up front.

The `TSoftObjectPtr` operations you will use:

| Call | What it does |
|---|---|
| `IsNull()` | True if no path is set at all. |
| `IsPending()` | Path set, object not in memory. |
| `IsValid()` / `Get()` | Is it loaded? / The object if loaded, else `nullptr`. Never loads. |
| `LoadSynchronous()` | Loads now if needed (the game thread waits), returns the object. |
| `ToSoftObjectPath()` / `ToString()` | The path, for async loading or logging. |
| `GetAssetName()` | The last part of the path, e.g. `L_Level02`. No load. |

### Maps must be soft

A level list that hard-referenced its maps would load *every level* the moment anything touched the
list. Worse, a map is a `UWorld` with its own actors, which you do not want sitting in memory beside
the world being played. So the rule is absolute: **never hard-reference a `UWorld`**. Use
`TSoftObjectPtr<UWorld>`. In the editor a `TSoftObjectPtr<UWorld>` property shows a level picker,
and the travel functions in the next chapter take one directly.

`FGSLevelInfo::SpeedCurve`, on the other hand, is hard on purpose. A curve is tiny, the game needs it
the moment the level starts, and a hard reference from the list means "load the list, get its
curves free". That is a reasonable default: **soft for maps and for big or rarely used assets,
hard for small assets you always need with their owner.**

### When soft references are loaded and freed

A soft reference does not keep its target alive. If nothing else references the asset, the next
garbage collection may free it, and `Get()` returns `nullptr` again. A `TSoftObjectPtr` is safe
across this: it holds the path, so `LoadSynchronous()` simply loads the asset again. A *raw pointer*
you got from it is not safe. That is why `GetLevel()`'s pointer must be used and dropped, and why
anything that must stay loaded needs an owner: a `UPROPERTY` pointer (the snake's `SpeedCurve`
property later in this chapter), or a streamable handle (next).

In the editor you rarely see an asset freed, because the editor keeps assets it has loaded. A
packaged game frees them. Code that "works in the editor" but keeps a raw pointer to a soft-loaded
asset is a classic packaged-only crash.

### Loading synchronously and asynchronously

`LoadSynchronous()` blocks the game thread until the asset and its hard dependencies are loaded. For
a small asset that is already on disk that is a fraction of a millisecond, and simple code is worth
it. GridSnake loads its level list this way. For a large asset (a big texture, a long sound, a
character with many dependencies), a synchronous load is a hitch: the frame stalls until it is done.

The alternative is to *request* the load and get a callback when it is done. Unreal's
`FStreamableManager` (header `Engine/StreamableManager.h`) does this. The Asset Manager owns one for
the whole game, `UAssetManager::GetStreamableManager()`.

`examples/ch13/ExAsyncLoad.h`

```cpp
// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "ExAsyncLoad.generated.h"

class UTexture2D;

/**
 * Loads a texture in the background instead of blocking the game thread.
 * Place one in a level, pick any texture for SoftTexture, press Play.
 */
UCLASS()
class AExAsyncLoad : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Loading")
	TSoftObjectPtr<UTexture2D> SoftTexture;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnLoaded();

	/** The request. While this handle is alive, the streamable manager keeps the texture loaded. */
	TSharedPtr<FStreamableHandle> Handle;

	/** Once loaded, a UPROPERTY keeps the texture alive for as long as this actor is. */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LoadedTexture;

	uint64 RequestFrame = 0;
};
```

`examples/ch13/ExAsyncLoad.cpp`

```cpp
// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExAsyncLoad.h"
#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"

void AExAsyncLoad::BeginPlay()
{
	Super::BeginPlay();

	if (SoftTexture.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: pick a texture for SoftTexture"), *GetName());
		return;
	}

	RequestFrame = GFrameCounter;
	UE_LOG(LogTemp, Log, TEXT("Requested %s on frame %llu"), *SoftTexture.ToString(), RequestFrame);

	// The asset manager owns one streamable manager for the whole game.
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Handle = Streamable.RequestAsyncLoad(SoftTexture.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &AExAsyncLoad::OnLoaded));

	UE_LOG(LogTemp, Log, TEXT("BeginPlay continues without waiting"));
}

void AExAsyncLoad::OnLoaded()
{
	// The soft pointer now resolves: Get() returns the object without loading anything.
	LoadedTexture = SoftTexture.Get();
	UE_LOG(LogTemp, Log, TEXT("Loaded %s on frame %llu (%llu frames later)"),
		*GetNameSafe(LoadedTexture), GFrameCounter, GFrameCounter - RequestFrame);
}

void AExAsyncLoad::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Leaving before the load finished: drop the request.
	if (Handle.IsValid() && Handle->IsLoadingInProgress())
	{
		Handle->CancelHandle();
	}
	Handle.Reset();

	Super::EndPlay(EndPlayReason);
}
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`. The Asset Manager and the streamable manager are
both in `Engine`.

To see it: drag `ExAsyncLoad` into a level, set **Soft Texture** to a texture you have *not* opened
in this editor session (or restart the editor first), and press Play.

Expected Output Log (filter: `LogTemp`); frame numbers will differ:

```text
LogTemp: Requested /Game/GridSnake/Art/Textures/T_Tiles.T_Tiles on frame 1520
LogTemp: BeginPlay continues without waiting
LogTemp: Loaded T_Tiles on frame 1522 (2 frames later)
```

- **The request returns at once.** `RequestAsyncLoad` takes the path(s) to load and a delegate
  (chapter 11) to call when everything has loaded. `BeginPlay` carries on, and the game keeps
  running frames while the loader works.
- **`GFrameCounter`** is the engine's global frame number. It proves the callback ran on a later
  frame. A small texture takes a frame or two; a large asset with many dependencies takes longer.
- **If the asset is already in memory**, there is nothing to load. The callback may then run very
  soon, possibly before "BeginPlay continues" is printed, and the frame difference is 0. The code
  does not care which happens, and yours should not either: never assume the callback comes later.
- **The handle keeps the asset loaded.** `FStreamableHandle` is a shared pointer. While a handle is
  alive and active, the assets it loaded stay in memory. In `OnLoaded` the texture is also stored
  in a `UPROPERTY`, which keeps it alive for as long as the actor lives even without the handle.
- **`CreateUObject`** binds the delegate to this actor weakly. If the actor is destroyed before the
  load finishes, the callback is skipped instead of calling into a dead object. `EndPlay` still
  cancels the request, so the texture does not load for nobody.
- **When to use which.** Use async loading for anything large or anything you load during play,
  where a hitch would show. Use `LoadSynchronous()` for small assets, at moments where a short wait
  is invisible (level start, behind a loading screen), and for code that is simpler that way.
  GridSnake's assets are all small, so it never needs async loading. The map change in the next
  chapter is a much bigger synchronous load, and it hides it behind a fade and a loading screen.

### Seeing references: the Reference Viewer

The editor can draw the reference graph for you:

1. In the Content Browser, right-click an asset and choose **Reference Viewer** (shortcut
   Alt+Shift+R).
2. The selected asset is in the middle. Assets that reference it are on the left; assets it
   references are on the right.
3. Hard and soft references are drawn differently. In current versions soft references are drawn
   as dashed or differently coloured lines (the exact styling may differ in your version). The
   viewer's settings let you show or hide each kind.

Next to it, **Size Map** (right-click > **Size Map**, Alt+Shift+M) shows how much memory an asset
pulls in with everything it hard-references. Exercise 4 uses both on `DA_LevelList`.

## Curves

Some data is not a single number but a relationship: "the longer the snake, the faster it moves".
You could write that as a formula in C++, but then a designer cannot tune its shape. A **curve
asset** stores the relationship as keys on a graph. `UCurveFloat` (header `Curves/CurveFloat.h`)
maps one float to another: `GetFloatValue(X)` returns the curve's value at X, interpolating
between keys.

GridSnake's third level uses one. In `C_SpeedUp`, X is the snake's length (segments, head included)
and Y is a *multiplier* on the level's step interval:

| Length | Multiplier | Interval with a 0.15 s base |
|---|---|---|
| 3 (start) | 1.0 | 0.150 s |
| 21 or 22 | about 0.75 | about 0.113 s |
| 40 and more | 0.5 | 0.075 s |

Each gem adds two segments (chapter 11's `GrowthPerGem`), so the snake speeds up as it grows. Past
the last key the curve keeps the last key's value by default, so the speed stops increasing at
length 40. A multiplier instead of absolute seconds means the same curve can speed up any level,
whatever its base interval.

### The snake reads the curve

The snake's fixed `StepInterval` from chapter 9 becomes a base interval plus an optional curve,
configured per level by the game mode.

`Source/GridSnake/Snake/GSSnake.h` (changed parts)

```cpp
class UCurveFloat;

// public:
	/** Sets the speed for this level. Curve (optional): X = length, Y = interval multiplier. */
	void ConfigureSpeed(float BaseInterval, UCurveFloat* Curve);

	/** Seconds per step right now, after applying the speed curve. */
	UFUNCTION(BlueprintPure, Category = "Snake")
	float GetStepInterval() const;

// private: StepInterval is replaced by these two
	/** Seconds per step at the starting length. Set by the game mode from the level list. */
	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	float BaseStepInterval = 0.15f;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	TObjectPtr<UCurveFloat> SpeedCurve;
```

`Source/GridSnake/Snake/GSSnake.cpp` (changed parts)

```diff
+#include "Curves/CurveFloat.h"
 ...
 	// in BeginPlay
-	StepInterval = UGSSettings::Get()->DefaultStepInterval;
+	BaseStepInterval = UGSSettings::Get()->DefaultStepInterval;
 ...
 	// in Tick
 	StepAccumulator += DeltaSeconds;
-	while (bMoving && StepAccumulator >= StepInterval)
+	float Interval = GetStepInterval();
+	while (bMoving && StepAccumulator >= Interval)
 	{
-		StepAccumulator -= StepInterval;
+		StepAccumulator -= Interval;
 		Step();
+		Interval = GetStepInterval();
 	}
 ...
 	// in StartMoving
 	UE_LOG(LogGridSnake, Log, TEXT("Snake starts moving %s, one step every %.2fs"),
-		*UEnum::GetValueAsString(CurrentDirection), StepInterval);
+		*UEnum::GetValueAsString(CurrentDirection), GetStepInterval());
```

```cpp
void AGSSnake::ConfigureSpeed(float BaseInterval, UCurveFloat* Curve)
{
	BaseStepInterval = FMath::Max(BaseInterval, 0.02f);
	SpeedCurve = Curve;
}

float AGSSnake::GetStepInterval() const
{
	const float Multiplier = SpeedCurve ? SpeedCurve->GetFloatValue(float(Segments.Num())) : 1.0f;
	// Never step faster than 50 times a second, whatever the curve says.
	return FMath::Max(BaseStepInterval * Multiplier, 0.02f);
}
```

- **No curve, no change.** With `SpeedCurve` null the multiplier is 1, so levels 1 and 2 behave
  exactly as before with their own base interval.
- **The interval is re-read after every step.** A step can grow the snake, which changes the
  interval. The fixed-step loop from chapter 9 must use the new value for the *next* step, or a
  long frame that owes several steps would take them all at the old speed.
- **A floor of 0.02 s.** Whatever the data says, the snake never steps more than 50 times a
  second. Data is edited by people; code that consumes it should clamp values that could break the
  game. The `ClampMin` metadata on `FGSLevelInfo::StepInterval` stops bad values in the editor, and
  the code clamps again for values that arrive any other way (a curve, a Blueprint call).
- **`SpeedCurve` is a `UPROPERTY`.** The snake holds a hard, reflected pointer to the curve, so the
  curve cannot be freed while the snake uses it, even if the level list itself were freed.

## Pointing code at data

The code needs to find `DA_LevelList`. Three ways, from worst to best:

1. **A hard-coded path**, such as `LoadObject<UGSLevelList>(nullptr,
   TEXT("/Game/GridSnake/Data/DA_LevelList.DA_LevelList"))`. It works until someone moves or renames
   the asset; then it fails at run time, quietly, and no tool warns you.
2. **A property on a Blueprint**, such as a `LevelList` property on `BP_GSGameMode`. Better: the
   editor tracks the reference and fixes it on rename. But every class that needs the list needs its
   own property or a way to reach the game mode, and the game mode is gone between levels (next
   chapter).
3. **A soft reference in project settings.** `UGSSettings` (chapter 3) is a developer settings class:
   a config-backed object that shows under **Project Settings** and is readable from anywhere with
   `GetDefault<>()`. Give it a `TSoftObjectPtr<UGSLevelList>`. The editor shows an asset picker, the
   path is stored in `DefaultGame.ini`, and any code can reach the list.

GridSnake uses the third.

`Source/GridSnake/Core/GSSettings.h` (changed parts)

```diff
 #include "Engine/DeveloperSettings.h"
 #include "GSSettings.generated.h"

+class UGSLevelList;
+
 ...
-	/** Seconds between two snake steps unless a level overrides it. */
+	/** Seconds between two snake steps in maps that are not in the level list. */
 	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "0.02", Units = "s"))
 	float DefaultStepInterval = 0.15f;

-	/** Score awarded for each gem. */
+	/** Score awarded for each gem in maps that are not in the level list. */
 	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "0"))
 	int32 PointsPerGem = 100;

+	/** The levels, in play order. */
+	UPROPERTY(Config, EditAnywhere, Category = "Levels")
+	TSoftObjectPtr<UGSLevelList> LevelList;
+
 	/** The class default object holds the values loaded from the .ini files. */
 	static const UGSSettings* Get() { return GetDefault<UGSSettings>(); }
+
+	/** Loads the level list on first use (a small asset, so loading synchronously is fine). */
+	static const UGSLevelList* GetLevelList();
 };
```

`Source/GridSnake/Core/GSSettings.cpp` (new: until now the class was header-only)

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSSettings.h"
#include "GridSnake.h"
#include "Data/GSLevelList.h"

const UGSLevelList* UGSSettings::GetLevelList()
{
	const UGSLevelList* List = Get()->LevelList.LoadSynchronous();
	if (!List)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("No level list - set Project Settings > Game > GridSnake > Level List"));
	}
	return List;
}
```

`Config/DefaultGame.ini` (the editor writes this line for you in the editor steps below)

```ini
[/Script/GridSnake.GSSettings]
...
; GridSnake (ch13)
LevelList=/Game/GridSnake/Data/DA_LevelList.DA_LevelList
```

- **A forward declaration in the header.** `GSSettings.h` only names `UGSLevelList` inside a
  template argument, so `class UGSLevelList;` is enough there. `TSoftObjectPtr<T>` does not need
  the full type until you dereference it. The `.cpp` includes the real header. Every file that
  includes `GSSettings.h` is spared the include, which keeps builds fast (chapter 2).
- **Why `LoadSynchronous` here.** The list is a few hundred bytes plus three tiny curves. The first
  call loads it; later calls find it in memory and return at once. If a packaged game's garbage
  collector frees the list between levels (nothing holds it with a hard reference), the next call
  loads it again, which costs about as much as the first. Simple and correct.
- **A loud warning on misconfiguration.** If the setting is empty, the function returns `nullptr`
  and logs exactly where to fix it. Every caller checks for `nullptr` and falls back to the
  project-wide defaults, so an unconfigured project still plays its one level.
- **The asset path format.** A soft object path is `/Game/<folder>/<Package>.<Object>`: the package
  (the `.uasset` file, without extension) and the object inside it. For assets, the two names are
  the same. `/Game/` is the project's `Content` folder.
- **Moving the asset.** If you rename or move `DA_LevelList` in the editor, the editor leaves a
  *redirector* at the old path (a small stub asset that forwards to the new one), so references keep
  working. Config files
  are not assets, so re-pick the asset in Project Settings after a move to keep the ini tidy. Fixing
  up redirectors (right-click the folder > **Fix Up Redirectors**; the label may differ slightly in
  your version) then removes the old path.
- **Packaging.** A packaged game only contains assets the cooker knows are needed. Chapter 20 adds
  the `Data` folder to the always-cook list so the level list (reachable only through an ini line)
  is certainly included.

## Building levels 2 and 3

The data side is ready. The game mode now uses it: at the start of a level it looks up the current
map in the list, configures the snake, and awards that level's points per gem; at the exit it opens
the next map in the list.

`Source/GridSnake/Core/GSGameMode.h` (changed parts)

```cpp
	/** Restarts the current level from scratch by loading its map again. */
	void RestartLevel();

	/** Loads the level after this one in the level list (or the first, after the last). */
	void LoadNextLevel();

	/** This map's position in the level list, or INDEX_NONE for a map that is not listed. */
	UPROPERTY(VisibleInstanceOnly, Category = "Rules")
	int32 LevelIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, Category = "Rules")
	int32 PointsPerGem = 100;
```

`Source/GridSnake/Core/GSGameMode.cpp` (changed parts)

```diff
 #include "Core/GSSettings.h"
+#include "Data/GSLevelList.h"
 #include "Gameplay/GSGem.h"
 ...
 void AGSGameMode::StartPlay()
 {
 	Super::StartPlay();

-	UE_LOG(LogGridSnake, Log, TEXT("GridSnake game mode started on map %s (collect %d gems)"),
-		*GetWorld()->GetMapName(), UGSSettings::Get()->GemsPerLevel);
+	// Look this map up in the level list. bRemovePrefixString strips the
+	// "UEDPIE_0_" prefix that Play In Editor adds to map names.
+	const UGSSettings* Settings = UGSSettings::Get();
+	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
+	const FName LevelId(*UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true));
+	LevelIndex = LevelList ? LevelList->FindLevelIndex(LevelId) : INDEX_NONE;
+	const FGSLevelInfo* LevelInfo = LevelList ? LevelList->GetLevel(LevelIndex) : nullptr;

-	// The player's pawn was spawned when the player joined, before StartPlay.
+	PointsPerGem = LevelInfo ? LevelInfo->PointsPerGem : Settings->PointsPerGem;
+	UE_LOG(LogGridSnake, Log, TEXT("Level %s: list index %d, %d points per gem"), *LevelId.ToString(), LevelIndex, PointsPerGem);
+
+	// The player's pawn was spawned when the player joined, and has begun play
+	// in Super::StartPlay(), so it is safe to configure now.
 	if (AGSSnake* Snake = GetSnake())
 	{
 		Snake->OnDied.AddDynamic(this, &AGSGameMode::HandleSnakeDied);
+		Snake->ConfigureSpeed(
+			LevelInfo ? LevelInfo->StepInterval : Settings->DefaultStepInterval,
+			LevelInfo ? LevelInfo->SpeedCurve.Get() : nullptr);
 	}
 ...
 void AGSGameMode::OnGemCollected(AGSGem* Gem, AGSSnake* Snake)
 ...
-	State->AddGem(UGSSettings::Get()->PointsPerGem);
+	State->AddGem(PointsPerGem);
 	Snake->Grow(GrowthPerGem);
 ...
 void AGSGameMode::OnExitReached(AGSSnake* Snake)
 ...
 	UE_LOG(LogGridSnake, Log, TEXT("Level complete with %d points"), State->GetScore());

-	// Chapter 13 loads the next level from data; for now, play this one again.
-	GetWorldTimerManager().SetTimer(LevelEndTimer, this, &AGSGameMode::RestartLevel, LevelEndDelay, /*bLoop=*/false);
+	GetWorldTimerManager().SetTimer(LevelEndTimer, this, &AGSGameMode::LoadNextLevel, LevelEndDelay, /*bLoop=*/false);
 }
```

And the new function:

```cpp
void AGSGameMode::LoadNextLevel()
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	if (!LevelList || LevelIndex == INDEX_NONE)
	{
		RestartLevel();
		return;
	}

	int32 NextIndex = LevelIndex + 1;
	if (!LevelList->GetLevel(NextIndex))
	{
		UE_LOG(LogGridSnake, Log, TEXT("That was the last level - starting over"));
		NextIndex = 0;
	}

	const FGSLevelInfo* Next = LevelList->GetLevel(NextIndex);
	UE_LOG(LogGridSnake, Log, TEXT("Opening level %d: %s"), NextIndex, *Next->Map.ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Next->Map);
}
```

- **Which level am I?** The game mode does not get told; it works it out from the map's name. That
  makes every entry point work the same way: playing `L_Level02` straight from the editor, arriving
  from level 1, or (in chapter 16) arriving from the level select menu. `GetCurrentLevelName` with
  `bRemovePrefixString = true` strips the `UEDPIE_0_` prefix that Play In Editor adds to the copy of
  the map it plays, so `L_Level01` matches in PIE and in a packaged game.
- **Unlisted maps still work.** A test map that is not in the list gets `LevelIndex = INDEX_NONE`,
  `LevelInfo = nullptr`, and the project-wide defaults from `UGSSettings`. At its exit,
  `LoadNextLevel` restarts it. Making the data optional is what keeps quick test maps cheap.
- **Configure after `Super::StartPlay()`.** `Super::StartPlay()` makes every actor begin play, and
  the snake's `BeginPlay` sets `BaseStepInterval` from the project default. Calling
  `ConfigureSpeed` afterwards overrides that default with the level's value. The countdown, which
  only starts the snake moving three seconds later, comes after both.
- **`SpeedCurve.Get()`** turns the `TObjectPtr` into the raw pointer the function takes.
- **`OpenLevelBySoftObjectPtr`** is in `UGameplayStatics` (`Kismet/GameplayStatics.h`, used since
  chapter 11 for `OpenLevel`). It takes the soft map reference directly, so there is no name to
  build by hand. It does *not* load the map immediately; the next chapter is all about what
  happens instead. For now: the current level ends, the next one starts.
- **After the last level** the index wraps to 0, so the game loops. Chapter 14 moves this logic
  into a subsystem that also keeps a score across levels, and chapter 16 replaces the loop with a
  "you win" screen.

### Editor steps

These steps follow `project/ch13/ASSETS.md`.

1. **Build.** Close the editor and build from your IDE: the module gains new classes, a new
   folder and a new source file (`GSSettings.cpp`), which Live Coding is not good at. Reopen the
   project.
2. **Create the data folder.** In the Content Browser, inside `Content/GridSnake`, right-click >
   **New Folder**, name it `Data`.
3. **Create the speed curve.** In `Data`, right-click > **Miscellaneous > Curve**. A class picker
   opens (**Pick Curve Class**; the label may differ slightly in your version). Select
   **CurveFloat** and click **Select**. Name the asset `C_SpeedUp`.
4. **Add the curve's keys.** Double-click `C_SpeedUp` to open the curve editor.
   - Right-click in the graph area and choose **Add Key** (in some versions, middle-click or
     Shift+click on the curve adds a key; the option may differ slightly in your version).
   - With the new key selected, type its time and value into the two number fields in the curve
     editor's toolbar: time `3`, value `1.0`.
   - Add a second key: time `40`, value `0.5`.
   - Press **F** (frame all) to see both keys. The line falls from 1.0 at 3 to 0.5 at 40. Save.
5. **Create the level list.** In `Data`, right-click > **Miscellaneous > Data Asset**, select
   `GSLevelList` in the class picker, click **Select**, and name it `DA_LevelList`.
6. **Fill in level 1.** Double-click `DA_LevelList`. Next to **Levels**, click **+** to add an
   element and expand **Index [ 0 ]**:
   - **Map**: pick `L_Level01` (the picker lists only levels).
   - **Display Name**: `First Steps`.
   - **Step Interval**: `0.18`. **Points Per Gem**: `100`. **Speed Curve**: leave **None**.
   - Save.
7. **Make the tile maps for levels 2 and 3.** In `Art/Tiles`, select `TM_Level01` and press Ctrl+D
   twice (or right-click > **Duplicate**). Rename the copies `TM_Level02` and `TM_Level03`. Open
   each and repaint its `Walls` layer (chapter 6):
   - `TM_Level02`, "The Maze": inner walls that form corridors. Keep corridors at least one free
     cell wide, and check that every gem and the exit can be reached.
   - `TM_Level03`, "Long Snake": an open floor with few inner walls. This level is about the snake
     getting long and fast, so give it room. You may also change **Map Width** and **Map Height**
     in the tile map's Details panel; the board frames whatever size it is given.
   - On both, keep a free row of cells for the snake's start and keep a border cell for the exit.
8. **Make the maps.** In `Maps`, select `L_Level01`, press Ctrl+D twice, and rename the copies
   `L_Level02` and `L_Level03`. Duplicating a level copies everything in it: board, gems, exit,
   Player Start, and its World Settings.
9. **Point each new map at its tile map.** Open `L_Level02`. Select the `GSBoard` actor, select its
   **TileMap** component in the Details panel, and set **Tile Map** = `TM_Level02`. The board's
   camera reframes immediately. Move the three gems, the exit and the **Player Start** onto free
   cells of the new layout (they snap to cell centres on Play). Save. Repeat for `L_Level03` with
   `TM_Level03`.
10. **Add levels 2 and 3 to the list.** Back in `DA_LevelList`, add two more elements:

    | # | Map | Display Name | Step Interval | Points Per Gem | Speed Curve |
    |---|---|---|---|---|---|
    | 0 | `L_Level01` | First Steps | 0.18 | 100 | None |
    | 1 | `L_Level02` | The Maze | 0.15 | 150 | None |
    | 2 | `L_Level03` | Long Snake | 0.15 | 200 | `C_SpeedUp` |

    Save. The order of the array is the order of play; drag an element's handle to reorder.
11. **Point the project at the list.** **Edit > Project Settings > Game > GridSnake**. In the new
    **Levels** category, set **Level List** = `DA_LevelList`. The editor writes the `LevelList=`
    line into `Config/DefaultGame.ini` at once.
12. **Play.** Open `L_Level01` and press Play. Collect the three gems and enter the exit.

Expected Output Log (filter: `LogGridSnake`), playing `L_Level01` to its exit:

```text
LogGridSnake: Level L_Level01: list index 0, 100 points per gem
LogGridSnake: Countdown: 3
LogGridSnake: Countdown: 2
LogGridSnake: Countdown: 1
LogGridSnake: Snake starts moving EGSDirection::Right, one step every 0.18s
...
LogGridSnake: Level complete with 300 points
LogGridSnake: Opening level 1: /Game/GridSnake/Maps/L_Level02.L_Level02
LogGridSnake: Level L_Level02: list index 1, 150 points per gem
```

After the exit of `L_Level03`:

```text
LogGridSnake: Level complete with 600 points
LogGridSnake: That was the last level - starting over
LogGridSnake: Opening level 0: /Game/GridSnake/Maps/L_Level01.L_Level01
```

- The other lines from earlier chapters (board size, view target, gems, phases) still appear
  between these; `...` stands for them. The start direction in the "Snake starts moving" line
  depends on the board's **Start Direction**.
- `L_Level02`'s first line shows its own points per gem: the new game mode in the new map looked
  itself up. Each gem now adds 150 to the score.
- In `L_Level03`, watch the snake speed up as it eats. The "one step every 0.15s" line is printed
  at the start only; the interval shrinks afterwards.
- Press Play on `L_Level02` directly: the log starts with `Level L_Level02: list index 1`. Any
  listed map plays with its own settings, wherever you start.
- If you see `No level list - set Project Settings > Game > GridSnake > Level List`, step 11 is
  missing. The level still plays, with index -1 and the defaults.

## DataTables for comparison

A **DataTable** (`UDataTable`, header `Engine/DataTable.h`) is the other common way to hold game
data. It is a table: every row has the same `USTRUCT` type, and every row has a unique name. Tables
are edited in a spreadsheet-like editor in Unreal, and can be imported from (and re-imported from)
CSV or JSON files. That last point is their strength. A designer keeps enemy stats in a spreadsheet,
exports CSV, and re-imports it.

The row struct derives from `FTableRowBase`:

`examples/ch13/ExEnemyRow.h`

```cpp
// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "ExEnemyRow.generated.h"

/**
 * One row of an enemy table. The row's name (the first CSV column, "Bat") is
 * the key, so it is not a field here. Import ExEnemies.csv with this row type.
 */
USTRUCT(BlueprintType)
struct FExEnemyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1"))
	int32 Health = 1;

	/** Units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float Speed = 100.0f;
};

/** Place one in a level, set Table to DT_ExEnemies, press Play. */
UCLASS()
class AExEnemyTableReader : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Enemies")
	TObjectPtr<UDataTable> Table;

protected:
	virtual void BeginPlay() override;
};
```

`examples/ch13/ExEnemyRow.cpp`

```cpp
// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExEnemyRow.h"

void AExEnemyTableReader::BeginPlay()
{
	Super::BeginPlay();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: set Table in the Details panel"), *GetName());
		return;
	}

	// Row names, in the order they appear in the table.
	TArray<FString> Names;
	for (const FName& RowName : Table->GetRowNames())
	{
		Names.Add(RowName.ToString());
	}
	UE_LOG(LogTemp, Log, TEXT("%s has %d rows: %s"), *Table->GetName(), Names.Num(), *FString::Join(Names, TEXT(", ")));

	// The context string only appears in warnings, to say who was looking.
	static const TCHAR* Context = TEXT("ExEnemyRow");

	if (const FExEnemyRow* Bat = Table->FindRow<FExEnemyRow>(FName("Bat"), Context))
	{
		UE_LOG(LogTemp, Log, TEXT("Bat: %d HP, speed %.0f"), Bat->Health, Bat->Speed);
	}

	// A missing row returns nullptr; passing false skips the LogDataTable warning.
	const FExEnemyRow* Dragon = Table->FindRow<FExEnemyRow>(FName("Dragon"), Context, /*bWarnIfRowMissing=*/false);
	UE_LOG(LogTemp, Log, TEXT("Dragon: %s"), Dragon ? TEXT("found") : TEXT("no such row"));
}
```

`examples/ch13/ExEnemies.csv`

```text
---,Health,Speed
Bat,10,300
Slime,25,80
Knight,60,150
```

Build.cs modules: `Core`, `CoreUObject`, `Engine`.

To see it:

1. Add the two source files to your module and build. The row struct must exist before you import,
   because the import asks for it.
2. Drag `ExEnemies.csv` from your file manager into the Content Browser (or click **Import** in the
   Content Browser toolbar and pick the file).
3. An import options dialog appears (**DataTable Options**; the label may differ slightly in your
   version). Set **Import As** to **DataTable** and choose `ExEnemyRow` as the row type (the field
   is labelled along the lines of **Choose DataTable Row Type**). Click **Apply**.
4. Rename the new asset `DT_ExEnemies` and double-click it: a grid with three rows, and a panel to
   edit the selected row.
5. Drag `ExEnemyTableReader` into a level, set **Table** = `DT_ExEnemies`, and press Play.

Expected Output Log (filter: `LogTemp`):

```text
LogTemp: DT_ExEnemies has 3 rows: Bat, Slime, Knight
LogTemp: Bat: 10 HP, speed 300
LogTemp: Dragon: no such row
```

- **The first CSV column is the row name.** Its header cell can say anything (`---` is the usual
  convention); the other headers must match the struct's property names. The row name is the key,
  which is why `FExEnemyRow` has no `Name` field.
- **`FindRow<T>(RowName, ContextString, bWarnIfRowMissing)`** returns a pointer to the row or
  `nullptr`. The context string is only used in the warning it logs for a missing row, so you can
  tell who asked. Pass `false` as the third argument when a missing row is a normal case.
- **`GetRowNames()`** lists the keys. `GetAllRows<T>()` and `ForeachRow<T>()` walk the rows
  themselves.
- **The returned pointer points into the table**, like `GetLevel()`. Use it and drop it.
- **Re-import** (right-click > **Reimport**) picks up an edited CSV. The table remembers its source
  file.

### Why GridSnake keeps a data asset

The level list could be a DataTable with `FGSLevelInfo` as the row struct (deriving from
`FTableRowBase`). Exercise 2 tries it. For this list, a data asset is the better fit:

- **Order is the point.** A data asset's array has an obvious order that you change by dragging. A
  DataTable's rows have an order too (`GetRowNames()` returns rows in table order), but tables are
  designed for lookup by name, and their editors are less suited to reordering a sequence.
- **References.** `Map` and `SpeedCurve` are asset references. In a data asset they are pickers. In
  a CSV they are path strings typed by hand, which is where typos live.
- **One list, few rows.** DataTables shine with many rows edited outside Unreal. Three levels edited
  inside Unreal do not need that.

Use DataTables when the data really is a table: dozens or hundreds of rows of the same shape,
often owned by a designer with a spreadsheet. Use data assets for "things" with structure and
references. Many games use both, for example an item data asset per item plus a DataTable of drop
rates.

## Summary

- Keep tuning numbers and lists in data, not code: designers can change them without a compile, and
  code that reads data handles content that does not exist yet.
- A data asset is an instance of your own `UDataAsset`/`UPrimaryDataAsset` subclass, created with
  **Miscellaneous > Data Asset**. Reflected properties become its editor. A `USTRUCT` array gives
  you an editable list.
- Hard references (`TObjectPtr`) load with their owner and keep the target alive. Soft references
  (`TSoftObjectPtr`) are paths: they load on request and keep nothing alive.
- Maps are always soft (`TSoftObjectPtr<UWorld>`). Small assets you always need with their owner
  (GridSnake's curves) can be hard.
- `LoadSynchronous()` blocks; `FStreamableManager::RequestAsyncLoad()` calls you back later and
  returns a handle that keeps the assets loaded. Do not keep raw pointers to soft-loaded data.
- A `UCurveFloat` turns one number into another; `GetFloatValue(X)` reads it. A multiplier curve
  can be shared by levels with different base values.
- A soft reference in a developer settings class, saved in `DefaultGame.ini`, is a robust way for
  code to find "the" data asset.
- The game mode finds its level in the list by map name, configures the snake and the gem value,
  and opens the next map with `UGameplayStatics::OpenLevelBySoftObjectPtr`.
- DataTables hold many rows of one struct, keyed by row name, imported from CSV or JSON. Prefer
  them for big tables and data assets for structured, ordered, reference-heavy data.

## Exercises

1. **Par time.** Give each level a par time in seconds. When the snake reaches the exit, log the
   time taken and whether the player beat par.

   <details><summary>Solution</summary>

   Add a field to `FGSLevelInfo` in `Data/GSLevelList.h`:

   ```cpp
   	/** Seconds from "Go!" to the exit that count as a good run. 0 = no par time. */
   	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (ClampMin = "0", Units = "s"))
   	float ParTimeSeconds = 0.0f;
   ```

   The game mode needs to remember when play started and the par time. In `GSGameMode.h`, private:

   ```cpp
   	float PlayStartTime = 0.0f;
   	float ParTimeSeconds = 0.0f;
   ```

   In `StartPlay`, after `LevelInfo` is known:

   ```cpp
   	ParTimeSeconds = LevelInfo ? LevelInfo->ParTimeSeconds : 0.0f;
   ```

   In `TickCountdown`, where the countdown ends and the phase becomes `Playing`:

   ```cpp
   	PlayStartTime = GetWorld()->GetTimeSeconds();
   ```

   In `OnExitReached`, after the level-complete log:

   ```cpp
   	const float Taken = GetWorld()->GetTimeSeconds() - PlayStartTime;
   	if (ParTimeSeconds > 0.0f)
   	{
   		UE_LOG(LogGridSnake, Log, TEXT("Time %.1fs, par %.1fs: %s"), Taken, ParTimeSeconds,
   			Taken <= ParTimeSeconds ? TEXT("beat par") : TEXT("over par"));
   	}
   ```

   `GetTimeSeconds()` is game time: it stops while the game is paused (chapter 16) and scales with
   time dilation, which is what a par time should measure. Rebuild, then set a par time on each
   entry of `DA_LevelList`. Existing entries get the default, 0, until you edit them: new struct
   fields never break saved assets.

   </details>

2. **The level list as a DataTable.** Convert the level list to a DataTable and note what you gain
   and what breaks.

   <details><summary>Solution</summary>

   Make the row struct derive from `FTableRowBase` and include `Engine/DataTable.h`:

   ```cpp
   USTRUCT(BlueprintType)
   struct FGSLevelInfo : public FTableRowBase
   ```

   Create the table: right-click > **Miscellaneous > Data Table**, pick `GSLevelInfo` as the row
   structure, and name it `DT_Levels`. Add three rows named `L_Level01`, `L_Level02`, `L_Level03`
   and fill in the same values. Point a `TSoftObjectPtr<UDataTable>` setting at it instead of the
   list.

   The lookups become:

   ```cpp
   const FGSLevelInfo* Info = Table->FindRow<FGSLevelInfo>(LevelId, TEXT("GridSnake"));
   TArray<FName> Order = Table->GetRowNames(); // index = position in Order
   ```

   Gained: lookup by name is built in (`FindRow` instead of `FindLevelIndex`), and the table can be
   exported to and imported from CSV. Lost or awkward:

   - The order of play is now the table's row order, which is less visible. Inserting a level
     between two others means reordering rows in the table editor.
   - The row name duplicates the map name, and nothing keeps them in sync.
   - In CSV, `Map` and `SpeedCurve` are typed asset paths, with no picker to catch typos.
   - Every caller that used an index needs `GetRowNames()` and `IndexOfByKey`.

   For a short ordered list with asset references, the data asset is simpler. Revert to it.

   </details>

3. **A tint per level.** Let each level tint its tile map, for example a cold blue for "The Maze".

   <details><summary>Solution</summary>

   Add to `FGSLevelInfo`:

   ```cpp
   	/** Multiplies the tile map's colours. White = unchanged. */
   	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
   	FLinearColor TileTint = FLinearColor::White;
   ```

   In `AGSGameMode::StartPlay`, after `LevelInfo` is known, find the board and tint its tile map
   component. `UPaperTileMapComponent::SetTileMapColor` sets the colour the whole map is multiplied
   by:

   ```cpp
   #include "EngineUtils.h"
   #include "Gameplay/GSBoard.h"
   #include "PaperTileMapComponent.h"

   	if (LevelInfo)
   	{
   		for (AGSBoard* Board : TActorRange<AGSBoard>(GetWorld()))
   		{
   			Board->GetTileMapComponent()->SetTileMapColor(LevelInfo->TileTint);
   		}
   	}
   ```

   `AGSBoard::GetTileMapComponent()` is the getter chapter 6 gave the board. Set **Tile Tint** to
   (0.7, 0.8, 1.0) on level 2. The game mode already includes `Kismet/GameplayStatics.h`; the
   module already depends on `Paper2D`.

   </details>

4. **Prove the maps are not loaded.** Use the editor's tools to show that loading `DA_LevelList`
   does not load the three maps, while it does load `C_SpeedUp`.

   <details><summary>Solution</summary>

   1. Right-click `DA_LevelList` > **Reference Viewer**. On the right (what it references) you see
      `L_Level01`, `L_Level02`, `L_Level03` and `C_SpeedUp`. The three map links are drawn as soft
      references and the curve link as a hard one (dashed versus solid, or a different colour; the
      exact styling may differ in your version). In the viewer's settings, turn off soft references:
      the maps disappear, the curve stays.
   2. Right-click `DA_LevelList` > **Size Map**. The total is tiny: the list plus the curve. The
      maps, with all their actors and tile maps, are not included, because Size Map follows hard
      references only.
   3. For contrast, open the Reference Viewer on `L_Level01`. It references `TM_Level01`, the gem and
      exit Blueprints and their art through hard references: loading the map loads all of that,
      which is exactly why the list must not hard-reference it.

   </details>

---

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-13--data-driven-levels)
