// Chapter 2 example: logging at every verbosity from an actor.

#include "ExHelloActor.h"
#include "Engine/World.h"

// A log category visible only in this .cpp file. Default verbosity Log:
// Verbose and VeryVerbose lines are hidden until you raise it.
DEFINE_LOG_CATEGORY_STATIC(LogExHello, Log, All);

AExHelloActor::AExHelloActor()
{
	// This actor only logs; it never needs a per-frame Tick.
	PrimaryActorTick.bCanEverTick = false;
}

void AExHelloActor::BeginPlay()
{
	Super::BeginPlay();

	// %s needs a TCHAR pointer: the * operator turns an FString into one.
	UE_LOG(LogExHello, Log, TEXT("BeginPlay on %s in world %s"), *GetName(), *GetWorld()->GetMapName());
	UE_LOG(LogExHello, Display, TEXT("Display also goes to the console window of a game"));
	UE_LOG(LogExHello, Warning, TEXT("Warnings are yellow"));
	UE_LOG(LogExHello, Error, TEXT("Errors are red"));
	UE_LOG(LogExHello, Verbose, TEXT("Verbose lines are hidden until you raise the verbosity"));
}

void AExHelloActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// UEnum::GetValueAsString finds the reflected enum and returns "Type::Name".
	UE_LOG(LogExHello, Log, TEXT("EndPlay: %s"), *UEnum::GetValueAsString(EndPlayReason));

	Super::EndPlay(EndPlayReason);
}
