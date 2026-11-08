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
