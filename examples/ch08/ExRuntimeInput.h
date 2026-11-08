// Learn Unreal Engine 5 - chapter 8 example.
// Build.cs modules: Core, CoreUObject, Engine, InputCore, EnhancedInput.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExRuntimeInput.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * WASD steering without a single asset: the input action, the mapping context
 * and the modifiers are created with NewObject. Shows that input assets are
 * ordinary UObjects. Set it as the pawn to possess (Auto Possess Player = Player 0).
 */
UCLASS()
class AExRuntimeInput : public APawn
{
	GENERATED_BODY()

public:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

private:
	/** Creates the action and context the first time it is called. */
	void CreateInputObjects();

	void OnMove(const FInputActionValue& Value);
	void OnMoveReleased();

	// UPROPERTY keeps the garbage collector from deleting objects only C++ points to.
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Context;

	FVector2D LastValue = FVector2D::ZeroVector;
};
