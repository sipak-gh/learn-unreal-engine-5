// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GSSnakeInteractable.generated.h"

class AGSSnake;

UINTERFACE(MinimalAPI, Blueprintable)
class UGSSnakeInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the snake's head can enter: gems, exits, and whatever you invent
 * next (keys, portals, speed pads). The snake does not need to know the type.
 */
class IGSSnakeInteractable
{
	GENERATED_BODY()

public:
	/** Called when the snake's head enters this actor's cell. Implement in C++ (_Implementation) or Blueprint. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GridSnake")
	void OnSnakeEntered(AGSSnake* Snake);
};
