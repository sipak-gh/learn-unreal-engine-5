// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Snake/GSGridTypes.h"
#include "GSSnake.generated.h"

class AGSBoard;
class UBoxComponent;
class UPaperFlipbookComponent;

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

protected:
	virtual void BeginPlay() override;

	/** Root. Used for overlaps from chapter 10; until then it has no collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Head;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	TObjectPtr<AGSBoard> Board;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	FIntPoint HeadCell = FIntPoint::ZeroValue;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	EGSDirection CurrentDirection = EGSDirection::Right;

private:
	/** Moves the actor to the centre of the cell it was spawned in. */
	void SnapToBoard();

	void UpdateHeadRotation();
};
