// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExSpawner.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogExSpawn, Log, All);

AExSpawner::AExSpawner()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AExSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!ClassToSpawn)
	{
		UE_LOG(LogExSpawn, Warning, TEXT("%s: set Class To Spawn in the Details panel"), *GetName());
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	// Spawn even if the new actor's collision overlaps something.
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Location = GetActorLocation() + FVector(Spacing * Index, 0.0f, 0.0f);
		AActor* Spawned = GetWorld()->SpawnActor<AActor>(ClassToSpawn, Location, FRotator::ZeroRotator, Params);
		if (!Spawned)
		{
			UE_LOG(LogExSpawn, Warning, TEXT("Spawning %s failed"), *ClassToSpawn->GetName());
			continue;
		}

		if (LifeSpan > 0.0f)
		{
			Spawned->SetLifeSpan(LifeSpan);
		}
		UE_LOG(LogExSpawn, Log, TEXT("Spawned %s at %s"), *Spawned->GetName(), *Location.ToString());
	}
}
