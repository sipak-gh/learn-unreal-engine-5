// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExUser.generated.h"

/**
 * Uses every actor in the level that implements IExUsable, half a second after
 * play starts. It never names a concrete class: lamps, signs and anything
 * added later are all reached the same way.
 */
UCLASS()
class AExUser : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Example")
	void UseEverything();

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle Timer;
};
