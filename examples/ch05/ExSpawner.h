// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSpawner.generated.h"

/**
 * Spawns Count actors of ClassToSpawn in a row when play starts. Each spawned
 * actor removes itself after LifeSpan seconds.
 */
UCLASS()
class AExSpawner : public AActor
{
	GENERATED_BODY()

public:
	AExSpawner();

	/** Any actor class: a C++ class or a Blueprint such as BP_GSGem. */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	TSubclassOf<AActor> ClassToSpawn;

	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0", ClampMax = "100"))
	int32 Count = 3;

	/** Distance between two spawned actors, along +X. */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	float Spacing = 32.0f;

	/** Seconds until each spawned actor is destroyed. 0 keeps them forever. */
	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0"))
	float LifeSpan = 3.0f;

protected:
	virtual void BeginPlay() override;
};
