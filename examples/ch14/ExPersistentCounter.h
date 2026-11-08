// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ExPersistentCounter.generated.h"

/** A counter that survives level changes because the game instance owns it. */
UCLASS()
class UExPersistentCounter : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Example")
	int32 Increment() { return ++Count; }

	UFUNCTION(BlueprintPure, Category = "Example")
	int32 GetCount() const { return Count; }

private:
	int32 Count = 0;
};

/**
 * Place one in two levels, ExMapA and ExMapB. Play ExMapA, then type
 * "open ExMapB" and "open ExMapA" in the console (`) to switch maps.
 * Each BeginPlay counts in three places and logs all three.
 */
UCLASS()
class AExCounterProbe : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Lives in this actor, which is destroyed with its level. */
	int32 ActorCount = 0;
};
