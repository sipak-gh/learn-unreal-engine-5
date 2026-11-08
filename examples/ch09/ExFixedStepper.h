// Chapter 9 example: a fixed-step simulation driven by the variable-length Tick.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExFixedStepper.generated.h"

/** Drop into a level and press Play: logs every frame's delta time and the steps it ran. */
UCLASS()
class AExFixedStepper : public AActor
{
	GENERATED_BODY()

public:
	AExFixedStepper();

	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Seconds of simulated time per step. Independent of the frame rate. */
	UPROPERTY(EditAnywhere, Category = "Stepper", meta = (ClampMin = "0.01"))
	float StepInterval = 0.1f;

	/** Log this many frames, then stay quiet. */
	UPROPERTY(EditAnywhere, Category = "Stepper", meta = (ClampMin = "0"))
	int32 FramesToLog = 20;

private:
	void Step();

	float Accumulator = 0.0f;
	float SimulatedTime = 0.0f;
	int32 FrameCount = 0;
	int32 StepCount = 0;
};
