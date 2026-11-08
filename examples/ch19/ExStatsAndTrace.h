// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExStatsAndTrace.generated.h"

/**
 * Does some deliberately slow work every frame and measures it three ways:
 * a cycle stat ("stat Example"), a CPU trace scope (Unreal Insights) and, once at
 * BeginPlay, a structured log line with the time in milliseconds.
 *
 * Try it: drag AExStatsAndTrace into any level, press Play, open the console (`) and
 * type "stat Example". Raise ValuesPerFrame in the Details panel to make it slower.
 */
UCLASS()
class AExStatsAndTrace : public AActor
{
	GENERATED_BODY()

public:
	AExStatsAndTrace();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** How many values to sum each frame. */
	UPROPERTY(EditAnywhere, Category = "Profiling", meta = (ClampMin = "1"))
	int32 ValuesPerFrame = 1000000;

private:
	/** The slow function being measured. */
	uint64 SumValues(int32 Count);

	/** Kept so the compiler cannot throw the work away. */
	uint64 LastSum = 0;
};
