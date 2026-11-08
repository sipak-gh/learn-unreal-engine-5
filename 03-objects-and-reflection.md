# Chapter 3 — Objects and Reflection

Chapter 2 wrote a C++ class with a `UCLASS()` macro and a `GENERATED_BODY()` line, and promised
to explain them later. This chapter does. Those macros connect your class to the **UObject
system**, the layer that makes Unreal C++ different from plain C++. Through it the engine knows,
at runtime, the name, type and flags of every member you mark. The Details panel, Blueprints, saving,
config files, console commands and the garbage collector all read that information. This chapter
covers the reflection macros and their most common specifiers, the class default object, garbage
collection and `TObjectPtr`, config properties, developer settings and console (`Exec`) commands.
GridSnake gains `UGSSettings`, its game rules under **Project Settings > Game > GridSnake**, and a
custom `UGSGameInstance` with a console command that prints any class's reflected members.

Previous: [Chapter 2](02-projects-modules-and-the-build.md). Next: [Chapter 4](04-pixel-art-with-paper2d.md).

## Why UObject

Plain C++ forgets almost everything about a class once the program is compiled. At runtime there
is no way to ask "what members does this class have?" or "set the member called `Count` to 10".
A game engine with a visual editor needs exactly that. The Details panel has to list a class's
members, the save system has to write them to disk, and a Blueprint has to call C++ functions by
name.

Unreal solves this with **reflection**: data that describes your classes and is available at runtime.
You do not write that data by hand. You mark classes and members with macros, and
**UnrealHeaderTool** (UHT, chapter 2) reads your headers before the compiler runs. It generates
code that registers every marked class, member and function with the engine. That generated code
is what `#include "Foo.generated.h"` pulls in and what `GENERATED_BODY()` expands to.

One set of metadata serves many systems:

| System | What it reads from reflection |
|---|---|
| Details panel | Which properties to show, their category, limits and display names |
| Blueprints | Which functions are nodes, which properties can be read or written |
| Serialization | Which properties to save in a level or asset, and their types |
| Config files | Which properties load from and save to `.ini` files |
| Garbage collector | Which pointers keep other objects alive |
| Console | Which functions can be typed as commands (`Exec`) |
| Networking | Which properties and functions replicate (not used in this course) |

All of this works only for classes derived from `UObject`, the root of the class tree. An actor
is a `UObject`; so are components, assets such as textures and sprites, Blueprints, and the
settings object this chapter writes. Plain structs and classes that do not derive from `UObject`
can still be used freely in Unreal C++. They just don't get these services.

### The three macros

A reflected class needs three things, which you saw in chapter 2's `AGSGameMode`:

```cpp
#include "GSGameMode.generated.h"   // last include in the header

UCLASS()                            // "UHT, reflect this class"
class AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()                // UHT's generated declarations go here
	// ...
};
```

- `UCLASS(...)` marks the class. Its parentheses take **class specifiers** such as `Abstract`,
  `Blueprintable` or `Config=Game`; you will meet several below.
- `GENERATED_BODY()` must be the first line in the class body. It expands to declarations UHT wrote:
  `StaticClass()`, the `Super` typedef you use in `Super::BeginPlay()`, constructors that the
  object system needs, and more.
- The `.generated.h` include must be the *last* include in the header, because the generated code
  relies on everything above it.

`USTRUCT()` and `UENUM()` do the same for structs and enums; chapter 7 uses `UENUM` for the snake's directions.
`UPROPERTY()` and `UFUNCTION()` mark members, and they are the subject of the next two sections.

### Prefixes

Unreal type names carry a one-letter prefix. Most of them are only a naming convention from the
[coding standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine),
but for `UObject` classes UHT enforces them:

| Prefix | Meaning | Examples |
|---|---|---|
| `A` | Derives from `AActor`: can be placed in or spawned into a level | `AActor`, `AGSGameMode` |
| `U` | Derives from `UObject` but is not an actor | `UObject`, `UGSSettings`, `UPaperSprite` |
| `F` | Any other struct or class, including `USTRUCT`s | `FString`, `FVector`, `FProperty` |
| `E` | Enum | `EEndPlayReason`, `EGSDirection` (chapter 7) |
| `I` | Interface | `IGSSnakeInteractable` (chapter 11) |
| `T` | Template | `TArray`, `TObjectPtr`, `TWeakObjectPtr` |
| `b` | Boolean variable (lower case, on members, not types) | `bWrapAround` |

The prefix is not part of the name the engine uses. The class `UGSSettings` is called `GSSettings`
inside the engine. Its full path is `/Script/GridSnake.GSSettings`: `/Script/` for C++ code, then the
module name, then the class name. You saw this form in chapter 2's `DefaultEngine.ini`, and you will
type the short name into a console command at the end of this chapter.

Name an actor class `UFoo` (or a non-actor `AFoo`) and UHT stops the build with an error naming
the prefix it expected.

### Objects live on the heap, created by the engine

You never create a `UObject` with `new` or as a local variable. The engine allocates it, registers
it, and frees it. You ask for one in one of these ways:

| Call | Creates | Where you use it |
|---|---|---|
| `NewObject<T>(Outer)` | Any `UObject` that is not an actor | Runtime objects, this chapter |
| `CreateDefaultSubobject<T>(Name)` | A component or sub-object owned by the class | Constructors only, chapter 5 |
| `GetWorld()->SpawnActor<T>(...)` | An actor in a level | Chapter 5 |
| Loading an asset | The asset's objects | Chapter 4 onwards |

You never `delete` one either: the garbage collector, covered after properties and functions,
destroys objects nothing references.

## Properties: UPROPERTY and its specifiers

`UPROPERTY(...)` in front of a member variable makes it a **property**. The specifiers inside the
parentheses say who may see and change it. Here is a property from this chapter's `UGSSettings`:

```cpp
/** How many gems must be collected before a level's exit opens. */
UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "1"))
int32 GemsPerLevel = 3;
```

### Editor visibility

These decide whether the property shows up in the Details panel, and whether it can be edited there.

| Specifier | Shown in the Details panel of | Editable |
|---|---|---|
| `EditAnywhere` | Class defaults (Blueprint editor) *and* placed instances | Yes |
| `EditDefaultsOnly` | Class defaults only | Yes |
| `EditInstanceOnly` | Placed instances only | Yes |
| `VisibleAnywhere` | Class defaults and instances | No, read-only |
| `VisibleDefaultsOnly` | Class defaults only | No |
| `VisibleInstanceOnly` | Instances only | No |
| (none of these) | Nowhere | No |

"Defaults" means the values a class starts with: what you edit in a Blueprint's **Class Defaults**.
"Instance" means one placed actor in a level, selected in the Outliner. A tuning value that should be
the same for every gem, such as its pickup sound, is `EditDefaultsOnly`. A value that differs per
placed actor, such as which level a door leads to, is `EditAnywhere` or `EditInstanceOnly`. Runtime
state you want to *watch* while debugging, such as the snake's current direction, is
`VisibleInstanceOnly`.

`VisibleAnywhere` on a component pointer looks odd at first: why make a pointer read-only? The
pointer itself cannot change, but the component it points to still shows its own editable
properties in the Details panel. That is the usual specifier for components, and chapter 5 uses it.

### Blueprint access

| Specifier | Blueprint graphs can |
|---|---|
| `BlueprintReadOnly` | Read it (a "Get" node) |
| `BlueprintReadWrite` | Read and write it (Get and Set nodes) |
| (neither) | Not see it at all |

Editor visibility and Blueprint access are independent. A property can be editable in the Details
panel but invisible to graphs, or the other way round. Blueprint access requires the member to be
`public` or `protected`. A `private` member needs `meta = (AllowPrivateAccess = "true")` before
Blueprints may touch it.

`Category = "Rules"` groups the property under a heading in the Details panel and in the Blueprint
node menu. A `|` makes sub-categories: `Category = "GridSnake|Levels"`. Give every exposed
property a category; it keeps the Details panel readable.

### Metadata

The `meta = (...)` part holds **metadata**: hints for the editor that do not change how the code
runs. Some common keys:

| Metadata | Effect in the editor |
|---|---|
| `ClampMin = "1"`, `ClampMax = "10"` | The value cannot be typed or dragged outside the range |
| `UIMin`, `UIMax` | Limits only the slider range; typed values can go outside |
| `Units = "s"` | Shows a unit next to the number (`s`, `cm`, `deg`, `ms`, ...) |
| `DisplayName = "Gems"` | Label to show instead of the generated one |
| `EditCondition = "bWrapAround"` | Greyed out unless the named bool is true |
| `ToolTip = "..."` | Tooltip; a `/** */` comment above the property does the same |

Without `DisplayName` the editor makes a label from the variable name: it drops the `b` of booleans
and splits words at capitals, so `DefaultStepInterval` becomes **Default Step Interval**.
Metadata exists only in editor builds. A packaged game has no `ClampMin`, so validate values in
code if a wrong value would break something at runtime. The full list is on the
[metadata specifiers](https://dev.epicgames.com/documentation/en-us/unreal-engine/metadata-specifiers-in-unreal-engine)
page.

### Storage: Config, Transient and the empty UPROPERTY()

| Specifier | Meaning |
|---|---|
| `Config` | Loaded from the class's `.ini` section; `SaveConfig()` writes it back. Needs `UCLASS(Config=...)` |
| `Transient` | Not saved with a level or asset; starts as the default every time |
| `UPROPERTY()` | No editor or Blueprint exposure. Still visible to the garbage collector |

The last row matters more than it looks. A bare `UPROPERTY()` on an object pointer exposes nothing,
but it tells the garbage collector "this object is in use". Most `UPROPERTY()` lines in GridSnake are
bare for exactly that reason.

### Types a property can have

Reflection understands sized numbers and `bool` (`uint8`, `int32`, `int64`, `float`, `double`;
Blueprints are happiest with `int32`, `float` and `bool`), the string types from chapter 2, engine
structs such as `FVector` and `FLinearColor`, your own `USTRUCT`s and `UENUM`s, object references
(`TObjectPtr<T>`, `TSubclassOf<T>`, `TSoftObjectPtr<T>`), and `TArray`, `TMap` and `TSet` of those.
A `std::vector`, an `int`, or a pointer to a non-`UObject` class cannot be a property; UHT names the
unsupported type in its error.

### The specifiers GridSnake uses

By the end of the course, GridSnake uses this small set. You do not need many more for a whole game.

| Specifiers | Used for | First seen |
|---|---|---|
| `Config, EditAnywhere, Category = ...` | Rules in `UGSSettings` | This chapter |
| `VisibleAnywhere, BlueprintReadOnly, Category = "Components"` | Component pointers on actors | Chapter 5 |
| `EditDefaultsOnly, Category = ...` | Assets a Blueprint child picks (flipbooks, sounds, widget classes) | Chapter 5 |
| `EditAnywhere, Category = ..., meta = (ClampMin ...)` | Per-instance tuning (camera margin, which level an exit loads) | Chapter 6 |
| `VisibleInstanceOnly, Category = ...` | Runtime state to watch while debugging (snake segments, gem count) | Chapter 7 |
| `BlueprintAssignable` | Events that widgets and Blueprints bind to | Chapter 11 |
| `Transient` | Runtime-only pointers, such as created widgets | Chapter 15 |
| `UPROPERTY()` | Object pointers that only need to be kept alive | This chapter |

## Functions: UFUNCTION

`UFUNCTION(...)` makes a member function known to reflection. Without it, a function is ordinary C++:
callable from C++, invisible to everything else.

| Specifier | Meaning |
|---|---|
| `BlueprintCallable` | Appears as a node with execution pins (the white arrows); may change state |
| `BlueprintPure` | Appears as a node without execution pins; must not change state, like a getter |
| `BlueprintImplementableEvent` | Declared in C++, *implemented* in a Blueprint; C++ calls it |
| `BlueprintNativeEvent` | Has a C++ default (`Name_Implementation`) that a Blueprint may override |
| `Exec` | Can be typed in the console, on the classes listed below |
| (none) | Reflected but not exposed; needed for functions bound to delegates (chapter 11) |

A `BlueprintPure` function is called again every time a node that uses its result runs, so keep it
cheap and free of side effects. Mark C++ getters `const` and `BlueprintPure`; mark anything that
does something `BlueprintCallable`. Chapter 11 uses `BlueprintNativeEvent` for "the snake entered
this tile" handlers, where C++ gives the default behaviour and a Blueprint can override it.

### Exec: functions you can type into the console

An `Exec` function becomes a console command: its name is the command and its parameters are parsed
from the rest of the line. You can type `GSDumpClass GSSettings` and Unreal calls
`GSDumpClass(TEXT("GSSettings"))` on the right object.

"The right object" is the catch. The engine does not search every object for `Exec` functions. It
offers each typed command to a fixed chain of objects. On the local player's side these are:

- the **PlayerController**,
- its **Pawn**,
- its **HUD**,
- its **CheatManager** (cheats are enabled in editor and development builds),
- the **GameMode** (only where the game mode exists: the server, or single player),
- the **GameInstance**.

An `Exec` function on any other class compiles fine and is silently never called. Typing its name
gives *Command not recognized* in the console. If a command does nothing, first check that its class
is in this list and that an instance exists while you type it. GridSnake puts its debug commands on
the game instance (this chapter) and on a cheat manager (chapter 19).

Shipping builds strip most console access, so never use `Exec` for anything a player needs.

## Classes and the class default object

Every reflected class has a matching **`UClass`** object, created at startup by the code UHT
generated. A `UClass` is the reflection data: the class's name, its parent, its flags, its list of
properties and functions. There are two ways to get one:

```cpp
UClass* Static = UGSSettings::StaticClass();   // the class named in the code, at compile time
UClass* Dynamic = SomeObject->GetClass();       // the actual class of an object, at runtime
```

`GetClass()` is the runtime one. If `SomeObject` is declared as `UObject*` but really is a
`UGSSettings`, `GetClass()` returns `UGSSettings`'s class. If it is an instance of the Blueprint
`BP_GSGameMode`, it returns the Blueprint's generated class, `BP_GSGameMode_C`.

### The class default object (CDO)

Each `UClass` owns one special instance of its class: the **class default object**, or **CDO**. The
engine creates it at startup by running your constructor once, then loading any `Config` properties
from the `.ini` files. The CDO is the template for every other instance: a new object starts as a
copy of its class's CDO.

The CDO is how Unreal knows a property's default value. When the Details panel shows a "reset to default" arrow
next to a property, it is comparing the instance against the CDO. When a level is
saved, only properties that differ from the CDO are written.

You read the CDO with `GetDefault<T>()`, which returns a `const T*`:

```cpp
const UGSSettings* Settings = GetDefault<UGSSettings>();
int32 Gems = Settings->GemsPerLevel;
```

`GetMutableDefault<T>()` returns a non-const pointer. Changing the CDO changes the default for all
objects created afterwards, which is rarely what you want at runtime. Exercise 3 explores it.

### Constructors run for the CDO

Because the engine runs your constructor to build the CDO, **the constructor runs at startup, before
any level exists.** It also runs again for every instance. So a `UObject` constructor must only set
defaults and create sub-objects. It must not:

- call `GetWorld()` (there is no world for the CDO; you get `nullptr`),
- spawn actors, start timers or play sounds,
- read other objects that may not be loaded yet.

Do that work in `BeginPlay` for actors (chapter 5) or `Init` for a game instance (below). A crash
on editor startup, inside your constructor, almost always means the constructor touched the world.
The call stack in your IDE shows your constructor below a function with `DefaultObject`
in its name, such as `UClass::CreateDefaultObject`.

### NewObject and the Outer

`NewObject<T>()` creates a non-actor object at runtime:

```cpp
UMyThing* Thing = NewObject<UMyThing>(this);                      // Outer = this
UMyThing* Named = NewObject<UMyThing>(this, TEXT("FirstThing"));  // with an explicit name
UMyThing* Loose = NewObject<UMyThing>();                          // Outer = the transient package
```

The first argument is the **Outer**: the object that contains the new one. Every `UObject` except a
package has an Outer, and the chain of Outers forms its path name, like a folder path. A sprite
asset's Outer is its package; a component's Outer is its actor. An Outer gives the object a place
in that hierarchy and a unique name inside it, so two objects with the same Outer cannot share a
name.

An Outer does **not** keep the object alive. People often assume it does. The next section shows
what does.

### Example: reflecting over an object

This example creates a small `UObject` at runtime and inspects it with the reflection API, the same
way the Details panel does. Like chapter 2's examples, it goes into any UE 5.8 C++ project. A
separate `Sandbox` project is the cleanest place; replace `Sandbox` with your module name in the
expected output if you use another project.

`examples/ch03/ExReflectedThing.h`

```cpp
// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "ExReflectedThing.generated.h"

/**
 * A plain UObject (not an actor) with a few reflected members. Nothing places it
 * in a level; AExReflectionDemo creates one at runtime and inspects it.
 */
UCLASS(BlueprintType)
class UExReflectedThing : public UObject
{
	GENERATED_BODY()

public:
	UExReflectedThing();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thing", meta = (ClampMin = "0"))
	int32 Count = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thing")
	FString Label = TEXT("Hello");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thing")
	TArray<FName> Tags;

	/** Not a UPROPERTY: invisible to reflection, the editor, Blueprints and the GC. */
	int32 Hidden = 42;

	UFUNCTION(BlueprintCallable, Category = "Thing")
	FString Describe() const;
};

/** Drop it into any level and press Play: it lists UExReflectedThing's members. */
UCLASS()
class AExReflectionDemo : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	/** UPROPERTY so that the garbage collector knows this actor keeps Thing alive. */
	UPROPERTY()
	TObjectPtr<UExReflectedThing> Thing;
};
```

`examples/ch03/ExReflectedThing.cpp`

```cpp
// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExReflectedThing.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogExReflect, Log, All);

UExReflectedThing::UExReflectedThing()
{
	// Constructors set defaults. This one also runs for the class default object.
	Tags = { TEXT("Shiny"), TEXT("Small") };
}

FString UExReflectedThing::Describe() const
{
	return FString::Printf(TEXT("%s x%d"), *Label, Count);
}

void AExReflectionDemo::BeginPlay()
{
	Super::BeginPlay();

	// The Outer (this actor) is the object's owner for naming and lifetime grouping.
	Thing = NewObject<UExReflectedThing>(this);
	const UClass* Class = Thing->GetClass();
	UE_LOG(LogExReflect, Log, TEXT("%s is a %s, outer %s"),
		*Thing->GetName(), *Class->GetName(), *GetNameSafe(Thing->GetOuter()));

	// Walk the reflected properties. Hidden is not one of them.
	for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FProperty* Property = *It;

		// GetCPPType returns "TArray" and puts "<FName>" into the extended text.
		FString ExtendedType;
		const FString Type = Property->GetCPPType(&ExtendedType);

		FString Value;
		Property->ExportTextItem_Direct(Value, Property->ContainerPtrToValuePtr<void>(Thing.Get()),
			nullptr, nullptr, PPF_None);

		UE_LOG(LogExReflect, Log, TEXT("Property %s (%s%s) = %s"),
			*Property->GetName(), *Type, *ExtendedType, *Value);
	}

	for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		UE_LOG(LogExReflect, Log, TEXT("Function %s"), *It->GetName());
	}

	// Write a property by name, the way the Details panel and the config system do.
	const FName CountName = GET_MEMBER_NAME_CHECKED(UExReflectedThing, Count);
	if (const FIntProperty* CountProperty = CastField<FIntProperty>(Class->FindPropertyByName(CountName)))
	{
		CountProperty->SetPropertyValue_InContainer(Thing.Get(), 10);
	}
	UE_LOG(LogExReflect, Log, TEXT("After writing Count by name: %s"), *Thing->Describe());
}
```

Build with the editor closed, start it, drag **ExReflectionDemo** from **C++ Classes** into the
level, filter the Output Log by `LogExReflect` and press **Play**.

Expected Output Log (filter: `LogExReflect`), in PIE:

```text
LogExReflect: ExReflectedThing_0 is a ExReflectedThing, outer <ExReflectionDemo_...>
LogExReflect: Property Count (int32) = 3
LogExReflect: Property Label (FString) = Hello
LogExReflect: Property Tags (TArray<FName>) = ("Shiny","Small")
LogExReflect: Function Describe
LogExReflect: After writing Count by name: Hello x10
```

(Unreal could not run where this course was written, so this is derived from the code; details
may differ slightly.)

- `<ExReflectionDemo_...>` is the name the editor gave the placed actor. The object's own name,
  `ExReflectedThing_0`, was generated by `NewObject`: class name without prefix, then a number.
- The properties should appear in declaration order. The text form of `Tags` comes from the array
  property's export code; the exact quoting of the names may differ. `Label` is unquoted because
  `PPF_None` asks for the plain value.
- `Hidden` is missing. It is an ordinary C++ member, so reflection does not know it exists. The same
  is true for the Details panel, Blueprints, saving, and the garbage collector.
- `UClass::GetName()` prints `ExReflectedThing`, without the `U`. That is the engine name.

How it works:

- **`TFieldIterator<FProperty>`** walks the properties of a class. `ExcludeSuper` skips inherited
  ones; without it, the loop would also list everything `UObject` declares.
- **`FProperty`** describes one property: name, type, flags, and where it sits inside an object.
  Properties are not `UObject`s (they are lighter `FField`s), so you cast them with `CastField<T>`,
  not `Cast<T>`. `FIntProperty` is the property class for `int32`; there is one per type.
- **`ContainerPtrToValuePtr`** turns "pointer to the object" into "pointer to this property inside
  it". **`ExportTextItem_Direct`** turns the value at that address into text. `_Direct` means the
  pointer points at the value itself. This is how the engine writes config files and copy-paste
  text.
- **`GetCPPType`** gives the C++ type name. For templates it splits the name: `TArray` in the
  return value, `<FName>` in the out-parameter.
- **`UFunction`**s *are* `UObject`s, and the second loop lists them the same way. `Describe` is
  there because it is a `UFUNCTION`.
- **`GET_MEMBER_NAME_CHECKED(Class, Member)`** makes an `FName` from a member name, and fails to
  compile if the member does not exist. That beats typing `TEXT("Count")` and finding a typo at
  runtime. `FindPropertyByName` looks the property up, and `SetPropertyValue_InContainer` writes
  through it.

You will rarely write reflection loops in gameplay code, but doing it once shows that the Details
panel, Blueprints and config are ordinary code walking these lists. GridSnake's `GSDumpClass`
command, at the end of this chapter, is this example made into a tool.

## Garbage collection and object pointers

Unreal frees `UObject`s with a **garbage collector** (GC). Every so often it runs a
*mark and sweep*:

1. **Mark.** Start from the **root set**: objects that must stay alive no matter what, such as the
   loaded levels, the game instance, and anything explicitly added to the root. Follow every
   reference the GC can see from those objects, and from the objects they reference, and so on.
   Mark everything reached.
2. **Sweep.** Every object that was not marked is unreachable. Call its `BeginDestroy`, later
   `FinishDestroy`, then free its memory.

"Every reference the GC can see" means object pointers in `UPROPERTY` members, including inside
`UPROPERTY` containers and structs (plus references reported by overriding `AddReferencedObjects`,
which this course does not need). A pointer member *without* `UPROPERTY` is invisible. The GC does not know it exists, so it does not
keep anything alive, and the GC does not clear it when its object is destroyed. It keeps pointing
at freed memory: a **dangling pointer**.

Actors are a special case that is easy to reason about. A level holds a reference to each of its
actors, so an actor lives until it is destroyed or its level is unloaded, whatever your pointers do.
When you call `Destroy()` on an actor, the engine marks it as **garbage**. The next collection frees
it and sets the `UPROPERTY` references that pointed at it to `nullptr`.

The GC runs at level loads and at an interval set under **Project Settings > Engine > Garbage
Collection** (about a minute by default; see the
[garbage collection settings](https://dev.epicgames.com/documentation/en-us/unreal-engine/garbage-collection-settings-in-the-unreal-engine-project-settings)).
So dangling-pointer bugs often hide during a short test and crash much later.

### Which pointer to use

| Pointer | Keeps the object alive | Knows when it is gone | Use it for |
|---|---|---|---|
| `UPROPERTY() TObjectPtr<T>` | Yes | Yes: set to `nullptr` once collected | Members that own or need an object |
| `TWeakObjectPtr<T>` | No | Yes: `Get()` returns `nullptr` | Members that observe an object owned elsewhere |
| `T*` local variable or parameter | No (doesn't need to) | No | Short use inside one function |
| `T*` member without `UPROPERTY` | No | No | Never, for a `UObject` |

**`TObjectPtr<T>`** is UE5's type for object pointers in `UPROPERTY` members. It behaves like `T*`:
you can use `->`, compare it with `nullptr`, and pass it where a `T*` is expected. In editor builds
it adds tracking used by the editor's asset loading; in packaged builds it compiles to a plain
pointer. The coding standard asks for `TObjectPtr` in headers for `UPROPERTY` members, and plain
`T*` for locals and function parameters. `Thing.Get()` returns the raw pointer when a function
needs exactly a `T*`, as the reflection example did.

**`TWeakObjectPtr<T>`** stores something like an index and serial number rather than a pointer. It
never keeps the object alive, and asking it for the object after the object is destroyed returns
`nullptr` instead of garbage. Use it when you want to remember an object without owning it.

**`IsValid(Object)`** is the check to use before touching an object you did not just create. It is
`false` for `nullptr` *and* for an object already marked as garbage. The second part matters: an
actor that was destroyed this frame is still in memory until the next collection, and a plain
`!= nullptr` test cannot tell.

### Example: watching the garbage collector

This example makes three objects, holds each in a different kind of pointer, forces a collection,
and checks what survived.

`examples/ch03/ExGcDemo.h`

```cpp
// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "ExGcDemo.generated.h"

/** A tiny object that logs when the garbage collector destroys it. */
UCLASS()
class UExGcPayload : public UObject
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;
};

/**
 * Creates three payloads and keeps them in three kinds of pointer, forces a
 * garbage collection, then checks which survived. Drop it into a level and Play.
 */
UCLASS()
class AExGcDemo : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	void CheckPointers();

	/** A strong, GC-visible reference: keeps its object alive. */
	UPROPERTY()
	TObjectPtr<UExGcPayload> Kept;

	/** A weak reference: does not keep its object alive, but knows when it is gone. */
	TWeakObjectPtr<UExGcPayload> Weak;

	/** A raw pointer without UPROPERTY: the GC cannot see it. Never dereference it after a GC. */
	UExGcPayload* Raw = nullptr;

	FTimerHandle CheckTimer;
};
```

`examples/ch03/ExGcDemo.cpp`

```cpp
// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExGcDemo.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExGc, Log, All);

void UExGcPayload::BeginDestroy()
{
	UE_LOG(LogExGc, Log, TEXT("%s: BeginDestroy"), *GetName());
	Super::BeginDestroy();
}

void AExGcDemo::BeginPlay()
{
	Super::BeginPlay();

	// Explicit names make the log easy to read. The Outer (this) does not keep them alive.
	Kept = NewObject<UExGcPayload>(this, TEXT("KeptPayload"));
	Weak = NewObject<UExGcPayload>(this, TEXT("WeakPayload"));
	Raw = NewObject<UExGcPayload>(this, TEXT("RawPayload"));
	UE_LOG(LogExGc, Log, TEXT("Created three payloads; requesting a full garbage collection"));

	// The collection runs on the next engine tick, not inside this call.
	GEngine->ForceGarbageCollection(true);

	GetWorldTimerManager().SetTimer(CheckTimer, this, &AExGcDemo::CheckPointers, 0.5f, false);
}

void AExGcDemo::CheckPointers()
{
	UE_LOG(LogExGc, Log, TEXT("Kept valid: %s, Weak valid: %s"),
		IsValid(Kept) ? TEXT("true") : TEXT("false"),
		Weak.IsValid() ? TEXT("true") : TEXT("false"));

	// Comparing the address with nullptr is all we may do: the memory behind it is gone.
	UE_LOG(LogExGc, Log, TEXT("Raw is still non-null: %s (it dangles)"),
		Raw != nullptr ? TEXT("true") : TEXT("false"));
	Raw = nullptr;
}
```

Place **ExGcDemo** in a level and press **Play**.

Expected Output Log (filter: `LogExGc`), in PIE. This depends on timing; see the notes below:

```text
LogExGc: Created three payloads; requesting a full garbage collection
LogExGc: WeakPayload: BeginDestroy
LogExGc: RawPayload: BeginDestroy
LogExGc: Kept valid: true, Weak valid: false
LogExGc: Raw is still non-null: true (it dangles)
```

and, when you stop PIE and the editor cleans up the play world:

```text
LogExGc: KeptPayload: BeginDestroy
```

- `ForceGarbageCollection(true)` only *requests* a full collection; the engine runs it at its next
  opportunity, normally within a frame. The 0.5 s timer gives it time. The two `BeginDestroy`
  lines may appear in either order.
- `KeptPayload` survives because a `UPROPERTY` of a live actor references it. `WeakPayload` and
  `RawPayload` are collected, even though their Outer is that live actor.
- `Weak.IsValid()` is `false`: the weak pointer noticed. `Raw` still holds the old address. Calling
  `Raw->GetName()` here would be *undefined behaviour*: it might print garbage, crash at once, or
  appear to work and crash ten minutes later, after the memory was reused. The example only compares
  the address with `nullptr`, which is all you can safely do with a dangling pointer, and then clears
  it.

The fix for a raw member pointer is always one of the first two rows of the table: add
`UPROPERTY()` and use `TObjectPtr` if this object should keep the other alive, or use
`TWeakObjectPtr` if it should not. Then check with `IsValid` before use. Exercise 4 makes an actor
version of this bug.

## Config and developer settings

The `.ini` files in `Config/` are not only for engine settings. Any `UObject` class can load
properties from them. Two specifiers do it:

- `UCLASS(Config = Game)` names the config file *category*: `Game` means the `Game.ini` hierarchy,
  `Engine` means `Engine.ini`, `Input` means `Input.ini`, and so on.
- `UPROPERTY(Config)` marks each property to load and save.

The **section** name is the class's path: `[/Script/<Module>.<ClassName>]`. When the class's CDO
is created, the engine reads that section and overwrites the constructor defaults with any values it
finds. Every new object then copies them from the CDO.

### The config hierarchy

"The `Game.ini` hierarchy" means several files, read in order, each overriding the ones before:

1. The engine's `Engine/Config/BaseGame.ini`.
2. Your project's `Config/DefaultGame.ini`, which you commit to version control.
3. Platform-specific files, such as `Config/Windows/WindowsGame.ini`, if they exist.
4. The per-user, per-machine file under `Saved/Config/`, which you do not commit.

`SaveConfig()` writes an object's `Config` properties to the *last* file, the one under `Saved/`.
In the editor that file is `Saved/Config/<Platform>Editor/Game.ini`, where `<Platform>Editor` is
`WindowsEditor` on Windows (the folder name differs by platform and engine version; look inside
`Saved/Config/` to find yours). Values there win over `DefaultGame.ini`, but they live only on your
machine.

### Example: a value that survives restarts

`examples/ch03/ExConfigObject.h`

```cpp
// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "ExConfigObject.generated.h"

/**
 * Config = Game without DefaultConfig: SaveConfig() writes to the per-user
 * Saved/Config/<Platform>/Game.ini, not to the project's Config/DefaultGame.ini.
 */
UCLASS(Config = Game)
class UExConfigObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config)
	int32 HighScore = 0;
};

/** Drop it into a level and press Play several times: HighScore goes up by one each time. */
UCLASS()
class AExConfigDemo : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
```

`examples/ch03/ExConfigObject.cpp`

```cpp
// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExConfigObject.h"

DEFINE_LOG_CATEGORY_STATIC(LogExConfig, Log, All);

void AExConfigDemo::BeginPlay()
{
	Super::BeginPlay();

	// The class default object (CDO) was filled from the .ini files when the class loaded.
	const UExConfigObject* Defaults = GetDefault<UExConfigObject>();
	UE_LOG(LogExConfig, Log, TEXT("HighScore read from config: %d"), Defaults->HighScore);

	// A new object starts as a copy of the CDO. Change it and save its Config properties.
	UExConfigObject* Copy = NewObject<UExConfigObject>();
	Copy->HighScore = Defaults->HighScore + 1;
	Copy->SaveConfig();

	// Reload the CDO from the (now updated) config cache so the next Play sees the new value.
	GetMutableDefault<UExConfigObject>()->ReloadConfig();

	UE_LOG(LogExConfig, Log, TEXT("Saved HighScore=%d to section [%s]"),
		Copy->HighScore, *UExConfigObject::StaticClass()->GetPathName());
}
```

Place **ExConfigDemo** in a level and press **Play** three times.

Expected Output Log (filter: `LogExConfig`), first Play, in a project whose module is `Sandbox`:

```text
LogExConfig: HighScore read from config: 0
LogExConfig: Saved HighScore=1 to section [/Script/Sandbox.ExConfigObject]
```

The second Play reads 1 and saves 2; the third reads 2 and saves 3. Restart the editor and the
count continues. Open `Saved/Config/WindowsEditor/Game.ini` (or your platform's folder) and you
should find:

```ini
[/Script/Sandbox.ExConfigObject]
HighScore=3
```

- The engine keeps config files in a cache in memory (`GConfig`). `SaveConfig()` updates the cache
  and writes the file. `ReloadConfig()` refreshes the CDO from the cache; without it, the next Play
  in the same editor session would read the stale CDO and print the old value again.
- This is a fine way to remember *editor* or *developer* preferences. It is not how to save player
  progress: config files are plain text that players can edit, and chapter 18 uses `USaveGame`
  instead.

### DefaultConfig and developer settings

Settings that belong to the *project*, such as game rules, should go into `Config/DefaultGame.ini`
so everyone on the team gets them. The class specifier **`DefaultConfig`** does that: it makes the
editor save the class's config properties to the `Default*.ini` file instead of the per-user one.

Then there is the question of how to *edit* them. You could type values into the `.ini` file, but
Unreal has a better way. Derive the class from **`UDeveloperSettings`**, and the editor
automatically adds a page for it to **Project Settings**, with every `Config, EditAnywhere`
property on it. Changing a value there updates the CDO immediately and saves the `.ini` file.

`UDeveloperSettings` lives in the `DeveloperSettings` module (its header is
`Engine/DeveloperSettings.h`, despite the folder name). A module that derives from it must list
`DeveloperSettings` in its `Build.cs`. If you forget, the header may still be found through
another module's dependencies, but the link fails with *unresolved external symbol* errors that
name `UDeveloperSettings` functions. Chapter 2 showed how to read those.

The page appears in **Project Settings** (not **Editor Preferences**), under a heading named after
the config file (`Config = Game` gives **Game**; override `GetCategoryName()` to change it), with
the class's `DisplayName` metadata as its title.

## Project: GridSnake's settings

GridSnake's rules are three numbers that several classes need: how many gems open the exit, how
fast the snake steps, and how many points a gem is worth. They become a developer settings class.

`project/ch03/Source/GridSnake/Core/GSSettings.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GSSettings.generated.h"

/**
 * Project-wide game rules. Appears under Project Settings > Game > GridSnake and
 * is saved to Config/DefaultGame.ini. Read it anywhere with UGSSettings::Get().
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GridSnake"))
class UGSSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** How many gems must be collected before a level's exit opens. */
	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "1"))
	int32 GemsPerLevel = 3;

	/** Seconds between two snake steps unless a level overrides it. */
	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "0.02", Units = "s"))
	float DefaultStepInterval = 0.15f;

	/** Score awarded for each gem. */
	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "0"))
	int32 PointsPerGem = 100;

	/** The class default object holds the values loaded from the .ini files. */
	static const UGSSettings* Get() { return GetDefault<UGSSettings>(); }
};
```

- `Config = Game, DefaultConfig`: the section lives in `Config/DefaultGame.ini`.
- `meta = (DisplayName = "GridSnake")` names the Project Settings page. Without it the page would be named after
  the class, something like **GS Settings**.
- Each property is `Config` (load and save) and `EditAnywhere` (shown on the page), in the
  category **Rules**. `ClampMin` stops anyone from setting zero gems; `Units = "s"` shows the step
  interval as seconds.
- `Get()` is a one-line convenience: `UGSSettings::Get()->GemsPerLevel` reads better than
  `GetDefault<UGSSettings>()->GemsPerLevel`, and it returns `const`, so callers cannot change the
  rules by accident.
- For now the header has no `.cpp` file. The class needs no code of its own.

The section in `Config/DefaultGame.ini`:

```ini
[/Script/GridSnake.GSSettings]
; GridSnake (ch03): Project Settings > Game > GridSnake. Written when you change a value
; there (DefaultConfig); the C++ defaults apply until then.
GemsPerLevel=3
DefaultStepInterval=0.150000
PointsPerGem=100
```

The snapshot includes it so that the rules are visible in one file. The editor writes the same
section the first time you change a value on the settings page. Until then, the C++ defaults
apply. When the editor rewrites the section it may drop the comments and use its own number format
(the `0.150000` above is what the engine's float export produces).

`GridSnake.Build.cs` gains the module:

```csharp
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"DeveloperSettings"
		});
```

And `AGSGameMode::StartPlay` (in `GSGameMode.cpp`, which now includes `Core/GSSettings.h`) reads
one value, to prove the settings work from C++:

```cpp
	UE_LOG(LogGridSnake, Log, TEXT("GridSnake game mode started on map %s (collect %d gems)"),
		*GetWorld()->GetMapName(), UGSSettings::Get()->GemsPerLevel);
```

Designers can now tune the game in **Project Settings** without touching code, and the values sit in
a text file that diffs well. Chapter 13 adds per-level data assets, with these values as fallbacks.

## A custom GameInstance

A **game instance** (`UGameInstance`) is the object that represents one run of the game. It is
created when the game starts, before the first level loads, and destroyed when the game quits.
Levels come and go and every actor in them is destroyed on a level change, but the game instance
stays. That makes it the natural home for anything that must outlive a level: which level comes
next, the score of the current run, the loaded save game. Chapter 14 moves those duties into *game
instance subsystems*, small objects that the game instance creates and owns, so that the game
instance class itself stays short. (The
[subsystems](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine)
page explains the idea if you want to read ahead.)

In the editor, each **Play** session creates a fresh game instance and destroys it when you stop.
Outside PIE the editor has no game instance at all.

`project/ch03/Source/GridSnake/Core/GSGameInstance.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GSGameInstance.generated.h"

/**
 * Created when the game starts and destroyed when it quits: it survives every
 * level change. Later chapters hang level progression and saving off it.
 */
UCLASS()
class UGSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** Console: GSDumpClass GSSettings - lists a class's reflected properties and functions. */
	UFUNCTION(Exec)
	void GSDumpClass(const FString& ClassName);
};
```

- **`Init`** runs once, after the game instance is created and before the first level starts.
  It is the game instance's `BeginPlay`. Call `Super::Init()` first.
- **`Shutdown`** runs once when the game ends (or PIE stops). Call `Super::Shutdown()` last, after
  your own cleanup.
- **`GSDumpClass`** is an `Exec` function, and the game instance is one of the classes that receive
  console commands, so typing `GSDumpClass ...` reaches it.

To make the engine use this class instead of the default `UGameInstance`, set
**Project Settings > Project > Maps & Modes > Game Instance > Game Instance Class** to
`GSGameInstance`. The editor writes it to `Config/DefaultEngine.ini`, next to chapter 2's game mode:

```ini
[/Script/EngineSettings.GameMapsSettings]
; ...
; GridSnake (ch03): Game Instance Class
GameInstanceClass=/Script/GridSnake.GSGameInstance
```

The value is the class path from the [prefixes](#prefixes) section: `/Script/`, module, class name
without the `U`. Unlike the game mode, which is set per level in **World Settings** if you want,
the game instance is one per project.

## Reflection in action: GSDumpClass

Here is the implementation. `Init` and `Shutdown` only log; `GSDumpClass` is the reflection example
from earlier, turned into a tool that works on any loaded class.

`project/ch03/Source/GridSnake/Core/GSGameInstance.cpp`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameInstance.h"
#include "GridSnake.h"
#include "Core/GSSettings.h"
#include "UObject/UnrealType.h"

void UGSGameInstance::Init()
{
	Super::Init();

	const UGSSettings* Settings = UGSSettings::Get();
	UE_LOG(LogGridSnake, Log, TEXT("GSGameInstance initialised: %d gems per level, step %.2fs"),
		Settings->GemsPerLevel, Settings->DefaultStepInterval);
}

void UGSGameInstance::Shutdown()
{
	UE_LOG(LogGridSnake, Log, TEXT("GSGameInstance shutting down"));

	Super::Shutdown();
}

void UGSGameInstance::GSDumpClass(const FString& ClassName)
{
	const UClass* Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::NativeFirst);
	if (!Class)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("No loaded class named '%s' (omit the A/U prefix)"), *ClassName);
		return;
	}

	// The class default object (CDO) holds every property's default value.
	const UObject* ClassDefaultObject = Class->GetDefaultObject();
	UE_LOG(LogGridSnake, Log, TEXT("Class %s (parent %s)"), *Class->GetName(), *GetNameSafe(Class->GetSuperClass()));

	for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FProperty* Property = *It;

		FString DefaultValue;
		Property->ExportTextItem_Direct(DefaultValue, Property->ContainerPtrToValuePtr<void>(ClassDefaultObject), nullptr, nullptr, PPF_None);

		UE_LOG(LogGridSnake, Log, TEXT("  property %s %s = %s%s%s"),
			*Property->GetCPPType(), *Property->GetName(), *DefaultValue,
			Property->HasAnyPropertyFlags(CPF_Edit) ? TEXT(" [editable]") : TEXT(""),
			Property->HasAnyPropertyFlags(CPF_Config) ? TEXT(" [config]") : TEXT(""));
	}

	for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const UFunction* Function = *It;
		UE_LOG(LogGridSnake, Log, TEXT("  function %s%s%s"), *Function->GetName(),
			Function->HasAnyFunctionFlags(FUNC_BlueprintCallable) ? TEXT(" [BlueprintCallable]") : TEXT(""),
			Function->HasAnyFunctionFlags(FUNC_Exec) ? TEXT(" [Exec]") : TEXT(""));
	}
}
```

What is new compared with `ExReflectedThing`:

- **`FindFirstObject<UClass>(Name, Options)`** finds a loaded object by its short name. Classes
  are objects too, so this finds a class from the string the user typed. `NativeFirst` prefers a
  C++ class if a Blueprint class has the same name. The name is the engine name, without the
  `A`/`U` prefix; the warning says so, because typing `UGSSettings` is the obvious mistake. Only
  *loaded* classes are found: every C++ class in a loaded module, but only the Blueprint classes
  something has loaded.
- **The CDO** is the object whose values are printed, so the output shows each property's
  *default* value. For `GSSettings` the CDO is the live settings object, so you see the current
  values from the `.ini` file.
- **Property flags.** Each specifier becomes a bit in the property's flags: `EditAnywhere` sets
  `CPF_Edit`, `Config` sets `CPF_Config`, `BlueprintReadOnly` sets `CPF_BlueprintVisible` and
  `CPF_BlueprintReadOnly`, and so on. `HasAnyPropertyFlags` tests them. Function specifiers do the
  same with `FUNC_` flags, and class specifiers with `CLASS_` flags (exercise 2).

### Try it

Build, start the editor, and press **Play**. Click into the game viewport, then press the
**`` ` ``** key (backtick, left of 1 on a US keyboard) to open the console. Other keyboard layouts
may need another key; see **Project Settings > Engine > Input > Console > Console Keys**. Type
`GSDumpClass GSSettings`.

Expected Output Log (filter: `LogGridSnake`):

```text
LogGridSnake: Class GSSettings (parent DeveloperSettings)
LogGridSnake:   property int32 GemsPerLevel = 3 [editable] [config]
LogGridSnake:   property float DefaultStepInterval = 0.150000 [editable] [config]
LogGridSnake:   property int32 PointsPerGem = 100 [editable] [config]
```

The properties should come in declaration order, and the float's text is the engine's export
format; glance at both when you run it. Then `GSDumpClass GSGameInstance`, `GSDumpClass GSGameMode`
(no reflected members of its own yet, so only the class line) and `GSDumpClass UGSSettings` print:

```text
LogGridSnake: Class GSGameInstance (parent GameInstance)
LogGridSnake:   function GSDumpClass [Exec]
LogGridSnake: Class GSGameMode (parent GameModeBase)
LogGridSnake: Warning: No loaded class named 'UGSSettings' (omit the A/U prefix)
```

Engine classes work too: try `GSDumpClass GameInstance` to see how much the base class declares,
or `GSDumpClass PaperSprite` once chapter 4 has loaded some sprites.

## Project: chapter 3 milestone

The changes since `project/ch02/`:

| File | Change |
|---|---|
| `Source/GridSnake/GridSnake.Build.cs` | `"DeveloperSettings"` added to the public dependencies |
| `Source/GridSnake/Core/GSSettings.h` | new: `UGSSettings` developer settings |
| `Source/GridSnake/Core/GSGameInstance.h`, `.cpp` | new: `UGSGameInstance` with `GSDumpClass` |
| `Source/GridSnake/Core/GSGameMode.cpp` | logs `GemsPerLevel` from the settings |
| `Config/DefaultEngine.ini` | `GameInstanceClass=/Script/GridSnake.GSGameInstance` |
| `Config/DefaultGame.ini` | new `[/Script/GridSnake.GSSettings]` section |

This chapter adds no assets. The editor steps, from `project/ch03/ASSETS.md`:

1. Close the editor. Add the new files, update `GridSnake.Build.cs`, and build **GridSnakeEditor**
   (**Development Editor**) from your IDE. New classes and a new module dependency both need a
   full build, not Live Coding. Start the editor. **C++ Classes > GridSnake > Core** now lists
   `GSSettings` and `GSGameInstance`.
2. **Project Settings > Project > Maps & Modes > Game Instance Class** = `GSGameInstance`. (If you
   copied the snapshot's `Config/` folder, it is already set.)
3. Open **Project Settings > Game > GridSnake**. The **Rules** category shows **Gems Per Level**,
   **Default Step Interval** (with an `s` unit) and **Points Per Gem**. Try typing `0` into Gems Per
   Level: the clamp puts it back to 1.
4. Press **Play**. Open the console and run `GSDumpClass GSGameMode` (ASSETS.md's check). Press
   **Esc** to stop.

Expected Output Log (filter: `LogGridSnake`):

```text
LogGridSnake: GSGameInstance initialised: 3 gems per level, step 0.15s
LogGridSnake: GridSnake game mode started on map UEDPIE_0_L_Level01 (collect 3 gems)
LogGridSnake: Class GSGameMode (parent GameModeBase)
LogGridSnake: GSGameInstance shutting down
```

The game instance line comes first: it is initialised before the level starts. `GSGameMode` has no
reflected members of its own yet, so the dump is one line; chapter 9 gives it some. Change **Gems
Per Level** to 5 in Project Settings and play again: both lines say 5, and `Config/DefaultGame.ini`
holds `GemsPerLevel=5`. Set it back to 3 so your project matches the snapshot.

If the game instance line is missing, or `GSDumpClass` gives *Command not recognized*, the **Game
Instance Class** setting did not stick: check `DefaultEngine.ini`. The command lives on
`UGSGameInstance`, so it only exists when that class is the game instance.

## Summary

- **Reflection** is runtime data about classes, generated by UHT from `UCLASS`, `UPROPERTY`,
  `UFUNCTION`, `USTRUCT` and `UENUM`. The Details panel, Blueprints, saving, config, the GC and the
  console all read it. Only `UObject`-derived classes get it.
- `A` = actor, `U` = other `UObject`, `F` = plain type, `E` enum, `I` interface, `T` template. The
  engine name drops the prefix: `UGSSettings` is `/Script/GridSnake.GSSettings`.
- Property specifiers: `EditAnywhere` / `EditDefaultsOnly` / `EditInstanceOnly` / `Visible...` for
  the Details panel; `BlueprintReadOnly` / `BlueprintReadWrite` for graphs; `Category`; metadata
  such as `ClampMin` and `Units` (editor only); `Config`; `Transient`.
- Function specifiers: `BlueprintCallable`, `BlueprintPure`, `BlueprintImplementableEvent`,
  `BlueprintNativeEvent`, `Exec`. `Exec` works only on the player controller, pawn, HUD, cheat
  manager, game mode and game instance.
- Each class has a `UClass` (`StaticClass()`, `GetClass()`) and a **CDO** built by running the
  constructor at startup. Constructors set defaults only: no world, no spawning.
- `NewObject<T>(Outer)` creates objects. The Outer names and groups them; it does not keep them alive.
- The **GC** keeps what is reachable through `UPROPERTY` references. Use `UPROPERTY()
  TObjectPtr<T>` for members that keep an object, `TWeakObjectPtr<T>` for members that observe one,
  raw `T*` only in locals, and `IsValid()` before use. A raw member pointer dangles.
- `UCLASS(Config=Game)` + `UPROPERTY(Config)` load from `[/Script/Module.Class]`; `SaveConfig()`
  writes to `Saved/Config/...`. `DefaultConfig` + `UDeveloperSettings` gives a Project Settings
  page that saves to `Config/Default*.ini`.
- The game instance lives for the whole game run; `Init` and `Shutdown` bracket it. GridSnake's
  `UGSGameInstance` is set in **Maps & Modes** and offers `GSDumpClass`.

## Exercises

1. **A new rule.** Add a `bWrapAround` setting ("the snake leaves one edge and enters at the
   opposite one") to `UGSSettings` and make it appear in Project Settings. You do not need to
   implement the rule.

   <details><summary>Solution</summary>

   Add to `GSSettings.h`, after `PointsPerGem`:

   ```cpp
   /** If true, the snake leaves one edge of the board and enters at the opposite edge. */
   UPROPERTY(Config, EditAnywhere, Category = "Rules")
   bool bWrapAround = false;
   ```

   Build and restart the editor. **Project Settings > Game > GridSnake > Rules** shows a **Wrap Around** check box: the
   editor dropped the `b`. Nothing else is needed; `UDeveloperSettings` builds the page from the
   reflection data. Ticking it writes `bWrapAround=True` into `Config/DefaultGame.ini`, and
   `GSDumpClass GSSettings` prints a fourth line, `property bool bWrapAround = True [editable]
   [config]` (or `False`). Remove the property afterwards so your project matches the snapshot.

   </details>

2. **Class flags.** Make `GSDumpClass` print whether the class is abstract, is a config class,
   and can be used as a Blueprint parent. Test it on `MeshComponent` (abstract) and `GSSettings`.

   <details><summary>Solution</summary>

   Add after the `Class %s (parent %s)` line:

   ```cpp
   TArray<FString> Flags;
   if (Class->HasAnyClassFlags(CLASS_Abstract)) { Flags.Add(TEXT("Abstract")); }
   if (Class->HasAnyClassFlags(CLASS_Config)) { Flags.Add(TEXT("Config")); }
   if (Class->HasAnyClassFlags(CLASS_DefaultConfig)) { Flags.Add(TEXT("DefaultConfig")); }
   #if WITH_EDITOR
   // "Blueprintable" is stored as metadata (IsBlueprintBase), which only editor builds have.
   // It is inherited, so walk up to the nearest class that says yes or no.
   for (const UClass* C = Class; C; C = C->GetSuperClass())
   {
   	if (C->HasMetaData(TEXT("IsBlueprintBase")))
   	{
   		if (C->GetBoolMetaData(TEXT("IsBlueprintBase"))) { Flags.Add(TEXT("Blueprintable")); }
   		break;
   	}
   }
   #endif
   UE_LOG(LogGridSnake, Log, TEXT("  flags: %s"), *FString::Join(Flags, TEXT(", ")));
   ```

   `UCLASS(Abstract)` sets `CLASS_Abstract`, `Config=` sets `CLASS_Config` and `DefaultConfig`
   sets `CLASS_DefaultConfig`; `Blueprintable` and `NotBlueprintable` become the `IsBlueprintBase`
   metadata. `GSDumpClass MeshComponent` lists `Abstract`; `GSDumpClass GSSettings` lists `Config,
   DefaultConfig`. Config flags are inherited, so engine classes may show `Config` too. Treat
   `Blueprintable` as a hint: the editor's own "can I make a Blueprint of this?" test checks more.

   </details>

3. **Changing settings at runtime.** What happens if code does
   `GetMutableDefault<UGSSettings>()->GemsPerLevel = 5;` during play? Is the change saved?

   <details><summary>Solution</summary>

   It changes the CDO, which is the object `UGSSettings::Get()` returns. Every reader sees 5 from
   then on, for the rest of the process. In the editor that includes the editor itself after you stop
   PIE, because PIE and the editor share CDOs: the Project Settings page shows 5 too. Nothing is
   written to disk, so after a restart the value is back to what the `.ini` file says.

   To save it, call a function that writes config:

   ```cpp
   UGSSettings* Settings = GetMutableDefault<UGSSettings>();
   Settings->GemsPerLevel = 5;
   Settings->TryUpdateDefaultConfigFile();   // writes Config/DefaultGame.ini
   ```

   `TryUpdateDefaultConfigFile()` (on `UObject`) writes the class's config properties to its
   `Default*.ini` file, which is what the Project Settings page does. `SaveConfig()` would write the
   per-user `Saved/Config/...` file instead. Neither is right for player choices in a shipped game:
   the `Config/` folder is packaged into read-only files. Chapter 16 stores options in game user
   settings and chapter 18 stores progress in a save game.

   </details>

4. **A dangling actor pointer.** In an actor, keep a raw `AActor* Target` member (no
   `UPROPERTY`). In `BeginPlay`, spawn an actor into it with
   `GetWorld()->SpawnActor<AActor>(AActor::StaticClass())` (chapter 5 explains spawning), destroy it
   with `Target->Destroy()`, request a GC, and on a timer half a second later log
   `Target->GetName()`. What happens, and how do you fix it?

   <details><summary>Solution</summary>

   `Destroy()` marks the actor as garbage and removes it from the level. The GC frees it. `Target`
   still holds the old address, so `Target->GetName()` reads freed memory: undefined behaviour. It may
   print nonsense, crash with an access violation, or seem to work until the memory is reused.

   Fix: make the GC aware of the pointer and check it before use. Declare it as
   `UPROPERTY() TObjectPtr<AActor> Target;` (the GC sets it to `nullptr` once the actor is
   collected) or as `TWeakObjectPtr<AActor> Target;` (it never keeps the actor alive), and test it:

   ```cpp
   if (IsValid(Target.Get()))
   {
   	UE_LOG(LogTemp, Log, TEXT("%s"), *Target->GetName());
   }
   ```

   `IsValid` returns `false` both before the GC (the actor is marked as garbage) and after it
   (`nullptr`). A destroyed actor is collected even if a `UPROPERTY` still references it, so the
   strong pointer does not "save" it; it only stops the pointer from dangling.

   </details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-3--objects-and-reflection)
