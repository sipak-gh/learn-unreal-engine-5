// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GSPlayerController.generated.h"

/**
 * The player's will. Owns the view (the board's fixed camera, not the pawn);
 * later also input contexts and screens.
 */
UCLASS()
class AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

protected:
	virtual void BeginPlay() override;
};
