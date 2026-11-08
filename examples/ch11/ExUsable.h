// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ExUsable.generated.h"

/** The reflection half of the interface: UHT needs a UObject-derived type to describe. */
UINTERFACE(MinimalAPI, Blueprintable)
class UExUsable : public UInterface
{
	GENERATED_BODY()
};

/**
 * The C++ half: the functions an implementer provides. Anything a player can
 * "use" (lamps, levers, signs) implements it, in C++ or in a Blueprint.
 */
class IExUsable
{
	GENERATED_BODY()

public:
	/** C++ classes override Use_Implementation; Blueprints implement the "Use" event. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Example")
	void Use(AActor* User);
};
