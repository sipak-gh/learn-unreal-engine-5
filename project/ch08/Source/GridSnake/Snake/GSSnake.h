// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Snake/GSGridTypes.h"
#include "GSSnake.generated.h"

class AGSBoard;
class UBoxComponent;
class UInputAction;
class UPaperFlipbookComponent;
struct FInputActionValue;

/**
 * The player's pawn. The actor sits on the head's cell; the rest of the
 * snake is drawn by components (chapter 9).
 */
UCLASS()
class AGSSnake : public APawn
{
	GENERATED_BODY()

public:
	AGSSnake();

	UFUNCTION(BlueprintPure, Category = "Snake")
	EGSDirection GetDirection() const { return CurrentDirection; }

	UFUNCTION(BlueprintPure, Category = "Snake")
	FIntPoint GetHeadCell() const { return HeadCell; }

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	/** Root. Used for overlaps from chapter 10; until then it has no collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Head;

	/** IA_Steer: an Axis2D action. Keys and sticks are mapped to it in IMC_Snake. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SteerAction;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	TObjectPtr<AGSBoard> Board;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	FIntPoint HeadCell = FIntPoint::ZeroValue;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	EGSDirection CurrentDirection = EGSDirection::Right;

private:
	void OnSteer(const FInputActionValue& Value);

	/** Moves the actor to the centre of the cell it was spawned in. */
	void SnapToBoard();

	void UpdateHeadRotation();
};
