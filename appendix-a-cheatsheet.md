# Appendix A — Cheat Sheet

A quick reference for Unreal Engine 5.8 C++ with Paper2D, one section per topic. Snippets are
fragments in the style of the GridSnake code, which (like the rest of this course) was checked
against Epic's 5.8 documentation but not compiled by the authors. Links point to the chapter with
the full explanation.

Previous: [Chapter 20](20-ship-it-and-make-your-own.md). Next:
[Appendix B — Project Reference](appendix-b-project-reference.md).

## Project files and the build

| File | Holds | Chapter |
|---|---|---|
| `GridSnake.uproject` | Engine version (`"EngineAssociation": "5.8"`), modules, enabled plugins (`Paper2D`) | [1](01-hello-unreal.md), [2](02-projects-modules-and-the-build.md) |
| `Source/GridSnake.Target.cs`, `GridSnakeEditor.Target.cs` | Game and editor targets: `TargetType`, `BuildSettingsVersion.V7`, `EngineIncludeOrderVersion.Unreal5_8` | [2](02-projects-modules-and-the-build.md) |
| `Source/GridSnake/GridSnake.Build.cs` | Module dependencies, PCH mode, include paths | [2](02-projects-modules-and-the-build.md) |
| `Source/GridSnake/GridSnake.h/.cpp` | `IMPLEMENT_PRIMARY_GAME_MODULE`, the `LogGridSnake` category | [2](02-projects-modules-and-the-build.md) |
| `Config/DefaultEngine.ini` | Maps & Modes, game instance, renderer settings, collision channels | [1](01-hello-unreal.md), [10](10-collision-in-2d.md) |
| `Config/DefaultGame.ini` | Project description and version, `UGSSettings`, packaging settings | [3](03-objects-and-reflection.md), [20](20-ship-it-and-make-your-own.md) |
| `Config/DefaultInput.ini` | Default input classes (Enhanced Input), console keys | [1](01-hello-unreal.md) |
| `Config/DefaultEditor.ini` | Editor-only settings, such as the Paper2D import defaults | [4](04-pixel-art-with-paper2d.md) |

Commit `Config/`, `Content/`, `Source/` and the `.uproject`. Never commit `Binaries/`,
`Intermediate/`, `Saved/` or `DerivedDataCache/`.

**Modules used by GridSnake** (`Build.cs`):

| Module | For | Chapter |
|---|---|---|
| `Core`, `CoreUObject`, `Engine`, `InputCore` | Everything | [2](02-projects-modules-and-the-build.md) |
| `EnhancedInput` | Input actions, mapping contexts, `UEnhancedInputComponent` | [2](02-projects-modules-and-the-build.md), [8](08-enhanced-input.md) |
| `DeveloperSettings` | `UDeveloperSettings` (Project Settings pages) | [3](03-objects-and-reflection.md) |
| `Paper2D` | Sprites, flipbooks, tile maps, grouped sprites | [5](05-actors-and-components.md) |
| `UMG` | `UUserWidget` and the widget classes | [15](15-a-hud-with-umg.md) |
| `MoviePlayer` (private) | The loading screen | [14](14-level-transitions-and-persistence.md) |
| `Slate`, `SlateCore` (private) | Slate widgets for the loading screen; UMG is built on them | [14](14-level-transitions-and-persistence.md) |

Public dependencies are visible to modules that include your headers; private ones only to your
`.cpp` files. A missing module shows up as `unresolved external symbol` (link error); a missing
include as `cannot open include file`.

**Building** ([chapter 2](02-projects-modules-and-the-build.md)):

| Task | How |
|---|---|
| Configuration for daily work | **Development Editor**, target `GridSnakeEditor` |
| Debug your own code | **DebugGame Editor**: your module unoptimized, the engine fast ([19](19-debugging-and-profiling.md)) |
| Regenerate project files | Right-click the `.uproject` > Generate Visual Studio project files; **Tools > Refresh Visual Studio/Xcode Project**; or `GenerateProjectFiles.sh -project=... -game` |
| Command-line build | `Build.bat GridSnakeEditor Win64 Development -Project=...` / `Build.sh GridSnakeEditor Linux Development -project=...` |
| Live Coding | **Ctrl+Alt+F11**. Function bodies only; restart for new classes, files, `UPROPERTY` layout or constructor default changes, or `Build.cs`/`.uproject` edits |
| Packaging | Platforms menu > *platform* > **Package Project**, or `RunUAT BuildCookRun` ([20](20-ship-it-and-make-your-own.md)) |

## Naming

| Prefix | Meaning | Example |
|---|---|---|
| `A` | Derives from `AActor` | `AGSSnake` |
| `U` | Derives from `UObject`, not an actor | `UGSSettings`, `UPaperSprite` |
| `F` | Any other struct or class | `FGSLevelInfo`, `FVector` |
| `E` | Enum | `EGSDirection` |
| `I` | Interface (the class you inherit) | `IGSSnakeInteractable` |
| `T` | Template | `TArray`, `TObjectPtr` |
| `b` | Boolean member | `bIsOpen` |

Asset prefixes in GridSnake: `L_` level, `BP_` Blueprint, `W_` widget, `IA_`/`IMC_` input action /
mapping context, `DA_` data asset, `C_` curve, `T_` texture, `S_` sprite, `FB_` flipbook, `TS_`/`TM_`
tile set / tile map, `SW_` sound wave, `MSS_` MetaSound source, `SC_` sound class, `SMix_` sound
mix.

A Blueprint class's path ends in `_C`: `/Game/GridSnake/Blueprints/BP_GSGameMode.BP_GSGameMode_C`.
A C++ class's path is `/Script/<Module>.<Name without prefix>`: `/Script/GridSnake.GSGameInstance`.

## Reflection macros

Every reflected type needs its macro, `GENERATED_BODY()` as the first line of the body, and
`#include "<File>.generated.h"` as the **last** include of its header.
([chapter 3](03-objects-and-reflection.md))

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSGem.generated.h"

UCLASS()
class AGSGem : public AActor
{
	GENERATED_BODY()

public:
	AGSGem();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

	UPROPERTY(EditDefaultsOnly, Category = "Gem")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, Category = "Gem", meta = (ClampMin = "0"))
	float BobHeight = 1.5f;
};
```

**`UCLASS` specifiers**

| Specifier | Effect | Chapter |
|---|---|---|
| `Abstract` | Cannot be placed or created directly; only its Blueprint children (GridSnake's widget parents) | [15](15-a-hud-with-umg.md) |
| `Blueprintable` / `NotBlueprintable` | Whether Blueprints may subclass it | [3](03-objects-and-reflection.md) |
| `BlueprintType` | Usable as a Blueprint variable type | [3](03-objects-and-reflection.md) |
| `Config = Game` | Loads `Config` properties from `[/Script/<Module>.<Class>]` in the Game ini | [3](03-objects-and-reflection.md) |
| `DefaultConfig` | Settings edited in Project Settings are written to `Config/Default*.ini` | [3](03-objects-and-reflection.md) |
| `meta = (DisplayName = "GridSnake")` | Label in the editor, e.g. the Project Settings page | [3](03-objects-and-reflection.md) |
| `MinimalAPI` | On `UINTERFACE`: export only what casting needs | [11](11-interfaces-delegates-and-game-rules.md) |

**`UPROPERTY` specifiers**

| Specifier | Effect |
|---|---|
| `EditAnywhere` | Editable in class defaults *and* on placed instances |
| `EditDefaultsOnly` | Editable in class defaults only (assets a Blueprint child picks) |
| `EditInstanceOnly` | Editable on placed instances only |
| `VisibleAnywhere` / `VisibleDefaultsOnly` / `VisibleInstanceOnly` | Shown, read-only (component pointers, runtime state) |
| `BlueprintReadOnly` / `BlueprintReadWrite` | Get node / Get and Set nodes |
| `BlueprintAssignable` | A dynamic multicast delegate Blueprints can bind to |
| `Category = "Rules"` | Groups the property in Details (required for exposed properties) |
| `Config` | Loaded from the class's ini section |
| `Transient` | Not saved with the level, asset or save game |
| `UPROPERTY()` | No exposure, but the garbage collector sees the reference |

**`UPROPERTY` metadata** (`meta = (...)`)

| Metadata | Effect | Chapter |
|---|---|---|
| `ClampMin = "1"`, `ClampMax = "10"` | Hard limits for typed and dragged values | [3](03-objects-and-reflection.md) |
| `UIMin`, `UIMax` | Slider range only | [3](03-objects-and-reflection.md) |
| `Units = "s"` | Unit label (`s`, `cm`, `deg`, `ms`...) | [3](03-objects-and-reflection.md) |
| `DisplayName`, `ToolTip` | Label and tooltip (a `/** */` comment also sets the tooltip) | [3](03-objects-and-reflection.md) |
| `EditCondition = "bWrapAround"` | Greyed out unless the bool is true | [3](03-objects-and-reflection.md) |
| `AllowPrivateAccess = "true"` | Lets Blueprints reach a `private` property | [3](03-objects-and-reflection.md) |
| `MakeEditWidget` | A 3D handle in the viewport for an `FVector` | [6](06-tile-maps-and-the-orthographic-camera.md) |
| `BindWidget` / `BindWidgetOptional` | The Widget Blueprint must (or may) contain a widget with this name | [15](15-a-hud-with-umg.md) |

**`UFUNCTION` specifiers**

| Specifier | Effect |
|---|---|
| `BlueprintCallable` | Node with execution pins; may change state |
| `BlueprintPure` | Node without execution pins; a getter |
| `BlueprintImplementableEvent` | Declared in C++, implemented in Blueprint |
| `BlueprintNativeEvent` | C++ default in `Name_Implementation`, a Blueprint may override |
| `Exec` | Console command, on: player controller, its pawn, HUD, cheat manager, game mode, game instance |
| `UFUNCTION()` | Needed for functions bound with `AddDynamic` or used by timers by name |

**Structs and enums**

```cpp
USTRUCT(BlueprintType)
struct FGSLevelInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UWorld> Map;
};

UENUM(BlueprintType)
enum class EGSDirection : uint8
{
	Up,
	Down,
	Left,
	Right
};
```

`UEnum::GetValueAsString(Value)` gives `EGSDirection::Right` for logs.

## Objects, memory and references

| Task | Code | Chapter |
|---|---|---|
| Create a plain object | `NewObject<UMyThing>(Outer)` | [3](03-objects-and-reflection.md) |
| Create a component (constructor only) | `CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"))` | [5](05-actors-and-components.md) |
| Spawn an actor | `GetWorld()->SpawnActor<AGSFlipbookEffect>(Class, Location, FRotator::ZeroRotator, Params)` (check for `nullptr`) | [5](05-actors-and-components.md) |
| Read class defaults (CDO) | `GetDefault<UGSSettings>()`; mutable: `GetMutableDefault<T>()` | [3](03-objects-and-reflection.md) |
| Cast safely | `Cast<AGSSnake>(Actor)` returns `nullptr` if it is not one | [7](07-the-gameplay-framework.md) |
| Name for logs | `GetNameSafe(Ptr)` (works on `nullptr`) | [7](07-the-gameplay-framework.md) |
| Destroy | `Destroy()` now, `SetLifeSpan(Seconds)` later | [5](05-actors-and-components.md) |

| Reference | Keeps the object alive | Becomes `nullptr` when collected | Use for |
|---|---|---|---|
| `UPROPERTY() TObjectPtr<T>` | Yes | Yes | Members that own or need an object |
| `TWeakObjectPtr<T>` | No | Yes (`Get()`) | Observing an object owned elsewhere |
| `TSoftObjectPtr<T>` | No: a path until loaded | - | Assets loaded on demand: `LoadSynchronous()` |
| `TSubclassOf<T>` | The class | - | "Which class to spawn/create" properties |
| Raw `T*` member without `UPROPERTY` | No | No | Never, for a `UObject` |

Survives `OpenLevel`: the engine, the game instance and its subsystems, local players, CDOs and
settings, statics. Dies: the world, every actor (game mode, game state, controllers, pawns, HUD),
components and viewport widgets. Store IDs and data in persistent objects, never actor pointers.
([chapter 14](14-level-transitions-and-persistence.md))

## Strings and logging

| Type | For |
|---|---|
| `FString` | Mutable text: `FString::Printf(TEXT("%d"), N)`, `*Str` gives a `TCHAR*` |
| `FName` | Case-insensitive IDs, cheap to compare: level IDs, parameter names |
| `FText` | Player-facing text: `LOCTEXT("Key", "Gems {0}/{1}")`, `FText::Format`, `FText::AsNumber` |

```cpp
// GridSnake.h / GridSnake.cpp
DECLARE_LOG_CATEGORY_EXTERN(LogGridSnake, Log, All);
DEFINE_LOG_CATEGORY(LogGridSnake);

// A file-local category (give each one a unique name: unity builds merge .cpp files)
DEFINE_LOG_CATEGORY_STATIC(LogExHello, Log, All);

UE_LOG(LogGridSnake, Log, TEXT("Gem %d/%d"), GemsCollected, GemsRequired);
UE_LOG(LogGridSnake, Warning, TEXT("%s has no flipbook"), *GetName());

#include "Logging/StructuredLog.h"
UE_LOGFMT(LogGridSnake, Warning, "Snake {Reason} at cell ({X}, {Y})", FString(Reason), Cell.X, Cell.Y);
```

Verbosity from loud to quiet: `Fatal` (crashes), `Error`, `Warning`, `Display`, `Log`, `Verbose`,
`VeryVerbose`. `Verbose` and below are hidden by default: raise them with the console command
`Log LogGridSnake Verbose`, the command line `-LogCmds="LogGridSnake Verbose"`, or `[Core.Log]` in
an ini. Logs on disk: `Saved/Logs/<Project>.log`.
([chapters 2](02-projects-modules-and-the-build.md) and [19](19-debugging-and-profiling.md))

## Actors and components

Lifecycle of a spawned actor: constructor → `PostActorCreated` → `OnConstruction` →
`PostInitializeComponents` → `BeginPlay` → `Tick`... → `EndPlay` → `Destroyed`. Actors placed in a
level and duplicated for Play In Editor run the constructor but not `OnConstruction`.
([chapter 5](05-actors-and-components.md))

```cpp
AGSGem::AGSGem()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Root);
	Flipbook->SetRelativeLocation(FVector(0.0f, 5.0f, 0.0f)); // in front of the tile map
	Flipbook->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}
```

| Where | Do |
|---|---|
| Constructor | Create components, set defaults. No world, no other actors |
| `OnConstruction` | React to Details edits in the editor (C++ construction script) |
| `BeginPlay` | Find other actors, bind delegates, start timers |
| `Tick(DeltaSeconds)` | Per-frame work only; turn ticking off where not needed (`bCanEverTick = false`) |
| `EndPlay(Reason)` | Clean up; `Reason` is `LevelTransition`, `EndPlayInEditor`, `Destroyed`... |

Finding things: `UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass())`,
`GetPlayerController(this, 0)`, `GetPlayerPawn(this, 0)`, `GetWorld()->GetAuthGameMode()`,
`GetWorld()->GetGameState<AGSGameState>()`.

## Gameplay framework

| Class | Lives | Role in GridSnake | Chapter |
|---|---|---|---|
| `AGameModeBase` | One per level, server only | Rules: countdown, scoring, win and loss | [7](07-the-gameplay-framework.md), [11](11-interfaces-delegates-and-game-rules.md) |
| `AGameStateBase` | One per level | Facts everyone may read: gems, score, phase, with delegates | [7](07-the-gameplay-framework.md), [11](11-interfaces-delegates-and-game-rules.md) |
| `APlayerController` | One per player, per level | Input mapping, view target, HUD and menus | [7](07-the-gameplay-framework.md), [16](16-menus-pause-and-input-modes.md) |
| `APawn` | Possessed by a controller | The snake | [7](07-the-gameplay-framework.md) |
| `UGameInstance` | The whole session | Exec commands; owner of subsystems | [3](03-objects-and-reflection.md) |
| `UGameInstanceSubsystem` | The whole session | Level flow, audio, saving | [14](14-level-transitions-and-persistence.md) |
| `UCheatManager` | Non-Shipping builds | Cheat commands | [19](19-debugging-and-profiling.md) |

Pair `AGameModeBase` with `AGameStateBase`, or `AGameMode` with `AGameState`; do not mix them.
Override a `BlueprintNativeEvent` such as `PlayerCanRestart` as `PlayerCanRestart_Implementation`.

```cpp
// Subsystems: created with the game instance, found from anything that knows it
UGSSaveSubsystem* Save = GetGameInstance()->GetSubsystem<UGSSaveSubsystem>();

void UGSAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Makes sure the save subsystem is initialized first (call only in Initialize; no cycles).
	const UGSSaveSubsystem* SaveSubsystem = Collection.InitializeDependency<UGSSaveSubsystem>();
}
```

## Interfaces and delegates

```cpp
UINTERFACE(MinimalAPI, Blueprintable)
class UGSSnakeInteractable : public UInterface { GENERATED_BODY() };

class IGSSnakeInteractable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GridSnake")
	void OnSnakeEntered(AGSSnake* Snake);
};

// Implement OnSnakeEntered_Implementation; call it through Execute_:
if (OtherActor->Implements<UGSSnakeInteractable>())
{
	IGSSnakeInteractable::Execute_OnSnakeEntered(OtherActor, this);
}
```

`Implements<U...>()` also finds Blueprint implementations; `Cast<I...>` finds only C++ ones.
([chapter 11](11-interfaces-delegates-and-game-rules.md))

| Kind | Declare | Bind | Call |
|---|---|---|---|
| Single | `DECLARE_DELEGATE_OneParam(FOnX, int32)` | `BindUObject`, `BindLambda` | `ExecuteIfBound(5)` |
| Multicast | `DECLARE_MULTICAST_DELEGATE(FGSBackRequestedSignature)` | `AddUObject(this, &U::Fn)` (weak), `AddLambda` | `Broadcast()` |
| Dynamic | `DECLARE_DYNAMIC_DELEGATE_OneParam(FOnX, int32, Value)` | `BindDynamic(this, &U::Fn)` | `ExecuteIfBound(5)` |
| Dynamic multicast | `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSGemsChangedSignature, int32, Collected, int32, Required)` + `UPROPERTY(BlueprintAssignable)` | `AddDynamic(this, &U::Fn)` (`Fn` must be a `UFUNCTION()`); `AddUniqueDynamic` | `Broadcast(1, 3)` |

Bind in `BeginPlay` or `NativeConstruct`, not in the constructor. Unbind with `RemoveDynamic` or
`RemoveAll(this)`. Binding the same function twice with `AddDynamic` triggers an ensure.
`Broadcast` with no listeners is safe. Native delegates are invisible to Blueprints.

## Time and timers

```cpp
// Fixed steps from a variable frame time (chapter 9)
StepAccumulator += DeltaSeconds;
while (bMoving && StepAccumulator >= GetStepInterval())
{
	StepAccumulator -= GetStepInterval();
	Step();
}

// Timers
FTimerHandle CountdownTimer;
GetWorldTimerManager().SetTimer(CountdownTimer, this, &AGSGameMode::TickCountdown, 1.0f, /*bLoop=*/true);
GetWorldTimerManager().ClearTimer(CountdownTimer);
GetWorldTimerManager().SetTimer(LevelEndTimer,
	FTimerDelegate::CreateUObject(this, &AGSGameMode::ShowLevelEnd, Result), 1.5f, /*bLoop=*/false);
```

Also `PauseTimer`, `UnPauseTimer`, `GetTimerRemaining`, `SetTimerForNextTick`. A rate ≤ 0 clears
the timer; timers bound to a destroyed object stop. World timers stop when the game is paused and
die with the world; the game instance's `GetTimerManager()` survives level changes.
([chapter 9](09-time-ticks-and-timers.md))

## Paper2D

Geometry: sprites lie in the **X-Z plane**; +X is right, +Z is up, Y is depth (the camera at +Y
looks down −Y). 1 pixel = 1 unit, tiles are 16 units. Tile (x, y) of a map at the origin is
centred at **(16x, 0, −16y)**; tile layer L sits at Y = −4·L. GridSnake's depth offsets: tile map
0, gems and exit +5, body +8, effects +9, head +10. In the editor, sprites face you in the
**Left/Right** orthographic views. ([chapters 4](04-pixel-art-with-paper2d.md) and
[6](06-tile-maps-and-the-orthographic-camera.md))

| Asset / component | Key API | Chapter |
|---|---|---|
| Texture settings for pixel art | Right-click > **Sprite Actions > Apply Paper2D Texture Settings** (Nearest filter, no mips, UserInterface2D compression, 2D Pixels group) | [4](04-pixel-art-with-paper2d.md) |
| `UPaperSpriteComponent` | `SetSprite`, `SetSpriteColor` | [4](04-pixel-art-with-paper2d.md) |
| `UPaperFlipbookComponent` | `SetFlipbook` (returns false if unchanged; follow with `PlayFromStart`), `PlayFromStart`, `SetLooping`, `SetPlayRate`, `SetSpriteColor`, `GetFlipbookLength`, `OnFinishedPlaying` (non-looping only) | [4](04-pixel-art-with-paper2d.md), [12](12-flipbook-animation-from-cpp.md) |
| `UPaperFlipbook` | Length = total frames ÷ Frames Per Second; `GetTotalDuration` | [12](12-flipbook-animation-from-cpp.md) |
| `UPaperGroupedSpriteComponent` | `AddInstance(Transform, Sprite, bWorldSpace, Color)`, `UpdateInstanceTransform`, `UpdateInstanceColor`, `ClearInstances`: one component, many sprites | [9](09-time-ticks-and-timers.md), [12](12-flipbook-animation-from-cpp.md) |
| `UPaperTileMapComponent` | `TileMap`, `GetMapSize(Width, Height, Layers)`, `GetTileCenterPosition(X, Y, Layer, bWorldSpace)`; `SetTile` needs an owned map (`MakeTileMapEditable`), then `RebuildCollision` | [6](06-tile-maps-and-the-orthographic-camera.md) |
| `UPaperTileMap` | `MapWidth`, `MapHeight`, `TileWidth`, `TileLayers` (`GetLayerCollides`, `GetCell(X, Y).IsValid()`), `GetTileCoordinatesFromLocalSpacePosition` | [6](06-tile-maps-and-the-orthographic-camera.md), [10](10-collision-in-2d.md) |

Sprite materials: `MaskedUnlitSpriteMaterial` (hard alpha, cheap, the default for pixel art),
`TranslucentUnlitSpriteMaterial` (fades, but sorting issues). Animations follow game events;
game logic never waits for an animation.

## Orthographic camera

```cpp
Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
Camera->SetOrthoWidth(VisibleWidth);              // world units across the screen
Camera->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f)); // Pitch, Yaw, Roll: look down -Y
PlayerController->SetViewTarget(Board);           // use the board's camera
```

The visible height is `OrthoWidth / AspectRatio`. In Details, rotation shows X = Roll, Y = Pitch,
Z = Yaw. ([chapter 6](06-tile-maps-and-the-orthographic-camera.md))

## Enhanced Input

```cpp
// Player controller: add the mapping context (BeginPlay)
if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
{
	InputSubsystem->AddMappingContext(DefaultMappingContext, /*Priority=*/0);
}

// Pawn: bind actions
void AGSSnake::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, this, &AGSSnake::OnSteer);
	}
}

void AGSSnake::OnSteer(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
}
```

| Concept | Remember | Chapter |
|---|---|---|
| Value types | Digital (bool, the default), Axis1D, Axis2D, Axis3D | [8](08-enhanced-input.md) |
| Modifiers | `Negate`, `Swizzle Input Axis Values` (YXZ turns X into Y), `Dead Zone` | [8](08-enhanced-input.md) |
| Trigger events | No trigger: `Started` once, `Triggered` every frame while non-zero, `Completed` on release | [8](08-enhanced-input.md) |
| Handler signatures | `void()`, `void(const FInputActionValue&)`, `void(const FInputActionInstance&)` | [8](08-enhanced-input.md) |
| Priority | Higher-priority contexts first; consumed keys do not reach lower ones | [8](08-enhanced-input.md) |
| Pause key | The action needs **Trigger when Paused** | [16](16-menus-pause-and-input-modes.md) |
| Controller's own actions | Bind in `SetupInputComponent()` | [16](16-menus-pause-and-input-modes.md) |

## Collision

| Concept | Remember | Chapter |
|---|---|---|
| Responses | Ignore < Overlap < Block; two components use the weaker of their responses | [10](10-collision-in-2d.md) |
| Overlap events | `SetGenerateOverlapEvents(true)` on both, and both must at least Overlap | [10](10-collision-in-2d.md) |
| Profiles | Set the profile first (`SetCollisionProfileName`), then per-channel overrides (`SetCollisionResponseToChannel`) | [10](10-collision-in-2d.md) |
| Visual only | `SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName)` | [5](05-actors-and-components.md) |
| Custom channel | Project Settings > Engine > Collision > New Trace Channel "Grid" (default Ignore) = `ECC_GameTraceChannel1`, aliased as `ECC_GSGrid` | [10](10-collision-in-2d.md) |
| 2D boxes | Give boxes some Y depth so they meet things on other layers | [10](10-collision-in-2d.md) |
| Overlap callback | Override `NotifyActorBeginOverlap(AActor* OtherActor)`, or bind `OnComponentBeginOverlap` | [10](10-collision-in-2d.md) |

Query names are `LineTrace`/`Sweep`/`Overlap` + `Test`/`Single`/`Multi` + `ByChannel`/
`ByObjectType`/`ByProfile`:

```cpp
const float HalfSize = GetCellSize() * 0.25f;     // a quarter cell: never touches a neighbour
const FCollisionShape Probe = FCollisionShape::MakeBox(FVector(HalfSize, 50.0f, HalfSize));
FCollisionQueryParams Params(SCENE_QUERY_STAT(GSCellBlocked), /*bTraceComplex=*/false);
const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
	CellToWorld(Cell), FQuat::Identity, ECC_GSGrid, Probe, Params);
```

## Data and assets

| Task | Code | Chapter |
|---|---|---|
| A data asset class | `UCLASS(BlueprintType) class UGSLevelList : public UPrimaryDataAsset` with `UPROPERTY` arrays of `USTRUCT`s | [13](13-data-driven-levels.md) |
| An asset path in settings | `UPROPERTY(Config, EditAnywhere) TSoftObjectPtr<UGSLevelList> LevelList;` | [13](13-data-driven-levels.md) |
| Load it | `LevelList.LoadSynchronous()`; async: `UAssetManager::GetStreamableManager().RequestAsyncLoad(...)` and keep the handle | [13](13-data-driven-levels.md) |
| A curve | `UCurveFloat* Curve; Curve->GetFloatValue(Length)` | [13](13-data-driven-levels.md) |
| A map's short name | `Map.GetAssetName()`; current level: `UGameplayStatics::GetCurrentLevelName(this, true)` (strips `UEDPIE_0_`) | [13](13-data-driven-levels.md) |

## Levels and loading

```cpp
UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), Map);      // travel next tick
UGameplayStatics::OpenLevel(this, FName("L_Level02"), /*bAbsolute=*/true, TEXT("Difficulty=2?Hardcore"));

PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.4f, FLinearColor::Black,
	/*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/true);

FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UGSLevelFlowSubsystem::HandlePreLoadMap);
FLoadingScreenAttributes LoadingScreen;
LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
LoadingScreen.MinimumLoadingScreenDisplayTime = 1.0f;
LoadingScreen.WidgetLoadingScreen = MakeLoadingScreen(Message); // plain Slate, no UObjects
GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
```

Read level options in `InitGame` with `UGameplayStatics::ParseOption`, `GetIntOption` or
`HasOption`. Every new world needs its own fade-in. The loading screen does **not** show in Play In
Editor: test with Standalone Game. ([chapter 14](14-level-transitions-and-persistence.md))

## UMG

```cpp
// A C++ parent for a Widget Blueprint
UCLASS(Abstract)
class UGSHUDWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GemsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> GemIconBox;
};

// Create and show it (player controller)
HUDWidget = CreateWidget<UGSHUDWidget>(this, HUDWidgetClass);
HUDWidget->AddToViewport();
```

| Task | Code | Chapter |
|---|---|---|
| Text | `GemsText->SetText(FText::Format(LOCTEXT("Gems", "Gems {0}/{1}"), Collected, Required))` | [15](15-a-hud-with-umg.md) |
| Show/hide | `SetVisibility(ESlateVisibility::Collapsed / Visible / HitTestInvisible)`; `HitTestInvisible` for display-only widgets | [15](15-a-hud-with-umg.md) |
| Remove | `RemoveFromParent()` | [15](15-a-hud-with-umg.md) |
| Buttons | `PlayButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandlePlayClicked)` (once, in `NativeConstruct` or `NativeOnInitialized`) | [16](16-menus-pause-and-input-modes.md) |
| Other events | `Slider->OnValueChanged`, `CheckBox->OnCheckStateChanged` | [16](16-menus-pause-and-input-modes.md), [17](17-sound-and-music.md) |
| Panels | `PanelSwitcher->SetActiveWidgetIndex(1)`; `Box->AddChild(Widget)` | [16](16-menus-pause-and-input-modes.md) |
| Focus | `Button->SetFocus()` after setting the input mode | [16](16-menus-pause-and-input-modes.md) |
| Display settings | `UGameUserSettings* S = UGameUserSettings::GetGameUserSettings(); S->SetFullscreenMode(EWindowMode::WindowedFullscreen); S->SetVSyncEnabled(true); S->ApplySettings(false);` (no effect in PIE) | [16](16-menus-pause-and-input-modes.md) |

Input modes ([chapter 16](16-menus-pause-and-input-modes.md)):

```cpp
FInputModeUIOnly InputMode;                 // menus: the game sees no keys
InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
SetInputMode(InputMode);                    // set the mode first...
bShowMouseCursor = true;
MainMenuWidget = CreateWidget<UGSMainMenuWidget>(this, MainMenuWidgetClass);
MainMenuWidget->AddToViewport();            // ...then add the widget, which focuses its first button

SetInputMode(FInputModeGameAndUI());        // pause menu: unhandled keys still reach the game
SetInputMode(FInputModeGameOnly());         // gameplay
SetPause(true);                             // ticks and world timers stop; widgets keep working
```

Prefer delegates (events) over property bindings, which run every frame.

## Audio

| Task | Code | Chapter |
|---|---|---|
| One-shot 2D sound | `UGameplayStatics::PlaySound2D(this, PickupSound)` (UI sounds play while paused) | [17](17-sound-and-music.md) |
| Music that survives level changes | `CreateSound2D(World, Music, 1.0f, 1.0f, 0.0f, nullptr, /*bPersistAcrossLevelTransition=*/true, /*bAutoDestroy=*/false)`; check for `nullptr` | [17](17-sound-and-music.md) |
| Fade | `MusicComponent->FadeIn(1.0f)`, `FadeOut(Duration, 0.0f)` | [17](17-sound-and-music.md) |
| MetaSound parameter | `MusicComponent->SetFloatParameter(TEXT("Intensity"), 0.5f)` (wrong names are silently ignored) | [17](17-sound-and-music.md) |
| Volumes | `SetSoundMixClassOverride(this, Mix, Class, Volume, 1.0f, 0.0f, true)` then `PushSoundMixModifier(this, Mix)` | [17](17-sound-and-music.md) |

A sound only follows a mix if its **Sound Class** is set: the first thing to check when a slider
does nothing. Camera fades with `bShouldFadeAudio` also fade persistent music.

## Saving

```cpp
UCLASS()
class UGSSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(VisibleAnywhere, Category = "Save")
	int32 SaveVersion = 0;   // every non-Transient UPROPERTY is saved
};

if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
{
	SaveGame = Cast<UGSSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
}
else
{
	SaveGame = Cast<UGSSaveGame>(UGameplayStatics::CreateSaveGameObject(UGSSaveGame::StaticClass()));
}
UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SlotName, UserIndex,
	FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UGSSaveSubsystem::HandleSaveFinished));
```

Loading returns a **new** object. Properties are saved by name, so adding and removing is safe;
renaming, retyping or changing meaning needs a version number and migration. Files: `Saved/SaveGames/<Slot>.sav`
(packaged Windows: `%LOCALAPPDATA%/<Project>/Saved/SaveGames/`). Delete with
`UGameplayStatics::DeleteGameInSlot`. ([chapter 18](18-saving-progress.md))

## Debugging

```cpp
check(Board && Segments.Num() > 0);        // stops the program (not in Shipping)
verify(DoSomething());                     // like check, but the expression still runs in Shipping
if (!ensure(Board)) { return; }            // logs a callstack once, continues

static TAutoConsoleVariable<bool> CVarGSDebugGrid(
	TEXT("GS.Debug.Grid"), false, TEXT("Draw the board's cells: red = blocked, green = free."), ECVF_Cheat);
if (CVarGSDebugGrid.GetValueOnGameThread()) { /* draw */ }

#if ENABLE_DRAW_DEBUG
DrawDebugBox(GetWorld(), Center, Extent, FColor::Red, /*bPersistentLines=*/false, /*LifeTime=*/-1.0f,
	/*DepthPriority=*/0, /*Thickness=*/0.5f);
#endif

DECLARE_STATS_GROUP(TEXT("GridSnake"), STATGROUP_GridSnake, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Snake Step"), STAT_GSSnakeStep, STATGROUP_GridSnake);
SCOPE_CYCLE_COUNTER(STAT_GSSnakeStep);

UE_VLOG(this, LogGridSnake, Warning, TEXT("Died: %s"), Reason);          // Visual Logger
GEngine->AddOnScreenDebugMessage(/*Key=*/1, 2.0f, FColor::Yellow, TEXT("Hello"));
```

State is a CVar (`TAutoConsoleVariable`); an action is a command (`FAutoConsoleCommand`) or an
`Exec` function on the cheat manager, which exists only in non-Shipping builds
(`UE_WITH_CHEAT_MANAGER`). Register it in the player controller's constructor:
`CheatClass = UGSCheatManager::StaticClass();`.
([chapter 19](19-debugging-and-profiling.md))

## Console commands

Open the console with the backtick/tilde key (twice for the expanded console). Tab completes.
Typing a CVar alone prints its value and help; `CVar value` sets it for this session.

| Command | Does | Chapter |
|---|---|---|
| `stat fps`, `stat unit` | Frame rate; frame, game, draw and GPU times (again to hide) | [1](01-hello-unreal.md) |
| `stat game`, `stat GridSnake` | Gameplay timings; GridSnake's `Snake Step` counter | [19](19-debugging-and-profiling.md) |
| `t.MaxFPS 30` | Cap the frame rate (0 = unlimited) | [1](01-hello-unreal.md), [9](09-time-ticks-and-timers.md) |
| `r.ScreenPercentage 50` | Render resolution (100 resets) | [1](01-hello-unreal.md) |
| `HighResShot 2` | Screenshot into `Saved/Screenshots/` | [1](01-hello-unreal.md) |
| `ShowFlag.Bounds 1`, `ShowFlag.Tonemapper 0` | Show bounds; compare colours without tone mapping | [1](01-hello-unreal.md), [4](04-pixel-art-with-paper2d.md) |
| `show collision` | Draw collision shapes | [10](10-collision-in-2d.md) |
| `slomo 0.5`, `pause`, `dumpticks` | Time dilation; pause; list ticking objects | [9](09-time-ticks-and-timers.md) |
| `showdebug enhancedinput` | Live input actions and mappings | [8](08-enhanced-input.md) |
| `open L_Level02` | Travel to a map by short name | [14](14-level-transitions-and-persistence.md) |
| `Log LogGridSnake Verbose` | Change a log category's verbosity | [2](02-projects-modules-and-the-build.md), [19](19-debugging-and-profiling.md) |
| `help`, `DumpConsoleCommands` | Print help text; list every command | [1](01-hello-unreal.md) |
| `GSDumpClass GSGameMode` | GridSnake: print a class's reflected properties and functions (no `A`/`U` prefix) | [3](03-objects-and-reflection.md) |
| `GS.Debug.Grid 1` | GridSnake: draw every cell, red blocked, green free | [19](19-debugging-and-profiling.md) |
| `GSGrow 10`, `GSCollectAll`, `GSWin`, `GSUnlockAll` | GridSnake cheats (non-Shipping) | [19](19-debugging-and-profiling.md) |

## Editor shortcuts

| Keys | Does | Chapter |
|---|---|---|
| Play button | Play In Editor; **Esc** stops (it is the editor's Stop key, so test pause with **P**) | [1](01-hello-unreal.md), [16](16-menus-pause-and-input-modes.md) |
| **Shift+F1** | Free the mouse while playing | [1](01-hello-unreal.md) |
| **F8** | Eject from the player to fly around while playing | [1](01-hello-unreal.md) |
| **Alt+S** | Simulate | [1](01-hello-unreal.md) |
| **Ctrl+Space** | Open the Content Drawer | [1](01-hello-unreal.md) |
| **Ctrl+S** | Save the current level and assets | [1](01-hello-unreal.md) |
| **Ctrl+D** | Duplicate the selected asset or actor | [13](13-data-driven-levels.md) |
| **Alt+Shift+R** / **Alt+Shift+M** | Reference Viewer / Size Map for the selected asset | [13](13-data-driven-levels.md) |
| **Ctrl+Alt+F11** | Live Coding compile | [2](02-projects-modules-and-the-build.md) |
| **Alt+Enter** | Toggle fullscreen in a running game | [20](20-ship-it-and-make-your-own.md) |
| IDE: **F5**, **Ctrl+F5**, **Ctrl+Alt+P** | Debug the editor, run without debugger, attach to `UnrealEditor` (Visual Studio) | [2](02-projects-modules-and-the-build.md), [19](19-debugging-and-profiling.md) |
| IDE: **F10**, **F11**, **Shift+F11** | Step over, into, out | [19](19-debugging-and-profiling.md) |

## Packaging

| Setting (`[/Script/UnrealEd.ProjectPackagingSettings]`) | GridSnake value |
|---|---|
| `BuildConfiguration` | `PPBC_Shipping` (no console, cheats, `check`, debug draw, stats or logs); Development for test builds |
| `+MapsToCook` | The menu and the three levels (empty list = all maps) |
| `+DirectoriesToAlwaysCook` | `/Game/GridSnake/Data`, `Audio`, `UI`: assets reached only through config or soft references |
| `UsePakFile`, `bUseIoStore` | `True` |

The pipeline is build → cook → stage → package → archive. Not cooked means not in the game.
([chapter 20](20-ship-it-and-make-your-own.md))

---

Sources for this appendix: [REFERENCES.md](REFERENCES.md#appendix-a--cheat-sheet)
