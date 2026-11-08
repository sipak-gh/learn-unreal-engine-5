// Learn Unreal Engine 5 - chapter 7 example.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExWhoIsWho.generated.h"

/**
 * Drop it into any level and press Play: BeginPlay logs every framework
 * object of the running game and the accessor that finds it.
 */
UCLASS()
class AExWhoIsWho : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
