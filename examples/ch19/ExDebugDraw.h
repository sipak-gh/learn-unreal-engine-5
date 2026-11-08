// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExDebugDraw.generated.h"

/**
 * Draws a box, a rotating line, a sphere and a label around itself every frame.
 * Debug shapes are for development only: they are compiled out of Shipping builds.
 *
 * Try it: drag AExDebugDraw into any level and press Play. In a Paper2D level, look at
 * it through the side-on (X-Z) view; the shapes are drawn in that plane.
 */
UCLASS()
class AExDebugDraw : public AActor
{
	GENERATED_BODY()

public:
	AExDebugDraw();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Half the size of the box, in world units. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	float BoxHalfSize = 32.0f;

	/** Length of the rotating line. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	float ArmLength = 48.0f;

	/** Turns per second of the line. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	float TurnsPerSecond = 0.5f;

private:
	float RunningTime = 0.0f;
};
