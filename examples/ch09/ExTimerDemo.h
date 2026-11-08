// Chapter 9 example: looping, one-shot and next-tick timers.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExTimerDemo.generated.h"

/** Drop into a level and press Play: three kinds of timer log as they fire. */
UCLASS()
class AExTimerDemo : public AActor
{
	GENERATED_BODY()

public:
	AExTimerDemo();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Seconds between calls of the looping timer. */
	UPROPERTY(EditAnywhere, Category = "Timers", meta = (ClampMin = "0.01"))
	float LoopRate = 0.5f;

	/** Seconds until the one-shot timer fires and stops the loop. */
	UPROPERTY(EditAnywhere, Category = "Timers", meta = (ClampMin = "0.01"))
	float OneShotDelay = 1.75f;

private:
	void OnLoop();

	FTimerHandle LoopTimer;
	FTimerHandle OneShotTimer;
	int32 LoopCount = 0;
};
