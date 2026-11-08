// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExFadeTravel.generated.h"

/**
 * Fades in when its level starts; Go() fades to black, then opens NextMap.
 * Place one in each of two levels, point their NextMap at each other, press
 * Play. With AutoGoSeconds > 0 it goes by itself; Go() can also be called
 * from a Blueprint, for example from a trigger's overlap event.
 */
UCLASS()
class AExFadeTravel : public AActor
{
	GENERATED_BODY()

public:
	/** Soft reference: the next map is loaded by the travel, not with this level. */
	UPROPERTY(EditAnywhere, Category = "Travel")
	TSoftObjectPtr<UWorld> NextMap;

	UPROPERTY(EditAnywhere, Category = "Travel", meta = (ClampMin = "0.05", Units = "s"))
	float FadeSeconds = 0.5f;

	/** Starts this level black and fades in. Put one of these in every map you travel to. */
	UPROPERTY(EditAnywhere, Category = "Travel")
	bool bFadeInOnBeginPlay = true;

	/** Calls Go() this long after BeginPlay. 0 = wait for a Blueprint to call Go(). */
	UPROPERTY(EditAnywhere, Category = "Travel", meta = (ClampMin = "0", Units = "s"))
	float AutoGoSeconds = 2.0f;

	UFUNCTION(BlueprintCallable, Category = "Travel")
	void Go();

protected:
	virtual void BeginPlay() override;

private:
	void Travel();

	FTimerHandle Timer;
	bool bGoing = false;
};
