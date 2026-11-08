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
