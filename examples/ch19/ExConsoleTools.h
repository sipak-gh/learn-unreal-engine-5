// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExConsoleTools.generated.h"

/**
 * Reads the console variable Ex.DebugLevel every frame and reacts to it.
 * The .cpp also registers the console commands Ex.Hello and Ex.Greet, which need no actor.
 *
 * Try it: drag AExConsoleTools into any level, press Play, open the console (`) and type
 *   Ex.DebugLevel 1
 *   Ex.DebugLevel 2
 *   Ex.Hello
 *   Ex.Greet Ada Lovelace
 */
UCLASS()
class AExConsoleTools : public AActor
{
	GENERATED_BODY()

public:
	AExConsoleTools();

	virtual void Tick(float DeltaSeconds) override;

private:
	/** The value seen last frame. -1 so the first frame always reports. */
	int32 LastDebugLevel = -1;
};
