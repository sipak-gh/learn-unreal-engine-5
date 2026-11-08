// Chapter 2 example: logging at every verbosity from an actor.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExHelloActor.generated.h"

/** Drop into a level and press Play: logs on BeginPlay and EndPlay. */
UCLASS()
class AExHelloActor : public AActor
{
	GENERATED_BODY()

public:
	AExHelloActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
