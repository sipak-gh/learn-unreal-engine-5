// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MGPawn2D.generated.h"

class UBoxComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class UInputAction;
class UPaperFlipbook;
class UPaperFlipbookComponent;
struct FInputActionValue;

/**
 * A free-moving top-down 2D pawn: a box for collision, a flipbook for looks, an
 * orthographic camera, and simple movement locked to the X-Z plane. Make a Blueprint
 * child (BP_MGPawn2D) to set the flipbooks and the move action.
 *
 * For a platformer (gravity, jumping) start from APaperCharacter instead (chapter 20).
 */
UCLASS()
class AMGPawn2D : public APawn
{
	GENERATED_BODY()

public:
	AMGPawn2D();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> Movement;

	/** An Axis2D input action (IA_Move): X is right, Y is up. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> IdleFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> RunFlipbook;

private:
	void OnMove(const FInputActionValue& Value);
};
