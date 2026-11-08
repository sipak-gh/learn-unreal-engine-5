// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ExTravelLogger.generated.h"

/**
 * Logs every map load. Nothing to place or configure: the engine creates one
 * instance of every UGameInstanceSubsystem class together with the game
 * instance, and destroys it when the game instance shuts down.
 */
UCLASS()
class UExTravelLogger : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/** A plain member: it lives as long as the game instance, across every map change. */
	int32 LoadCount = 0;
};
