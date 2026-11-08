// Chapter 9 example: a fixed-step simulation driven by the variable-length Tick.

#include "ExFixedStepper.h"

DEFINE_LOG_CATEGORY_STATIC(LogExStep, Log, All);

AExFixedStepper::AExFixedStepper()
{
	PrimaryActorTick.bCanEverTick = true;
	// TG_PrePhysics is the default group; set here only to show where it lives.
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void AExFixedStepper::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	++FrameCount;

	// Bank the frame's time, then spend it in whole steps. Whatever is left
	// over (less than one step) waits for the next frame.
	Accumulator += DeltaSeconds;
	int32 StepsThisFrame = 0;
	while (Accumulator >= StepInterval)
	{
		Accumulator -= StepInterval;
		Step();
		++StepsThisFrame;
	}

	if (FrameCount <= FramesToLog)
	{
		UE_LOG(LogExStep, Log, TEXT("Frame %d: dt=%.3f steps=%d (left over %.3f)"),
			FrameCount, DeltaSeconds, StepsThisFrame, Accumulator);
	}
}

void AExFixedStepper::Step()
{
	++StepCount;
	SimulatedTime += StepInterval;

	if (FrameCount <= FramesToLog)
	{
		UE_LOG(LogExStep, Log, TEXT("  step %d (simulated time %.2fs)"), StepCount, SimulatedTime);
	}
}
