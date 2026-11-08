// Learn Unreal Engine 5 - chapter 7 example.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExPossessionPawn.generated.h"

/**
 * A pawn that reports when a controller takes it over or lets it go.
 * Place it in a level and set Pawn > Auto Possess Player = Player 0.
 */
UCLASS()
class AExPossessionPawn : public APawn
{
	GENERATED_BODY()

public:
	/** Called on the server (in single player: always) when a controller possesses this pawn. */
	virtual void PossessedBy(AController* NewController) override;

	/** Called when the controller lets go of this pawn. */
	virtual void UnPossessed() override;

	/** Asks the current controller to let go of this pawn. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	void Release();
};
