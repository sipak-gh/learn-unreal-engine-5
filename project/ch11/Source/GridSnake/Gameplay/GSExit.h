// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/GSSnakeInteractable.h"
#include "GSExit.generated.h"

class UBoxComponent;
class UPaperFlipbook;
class UPaperFlipbookComponent;

/**
 * The level exit. Closed (and solid) until the game state reports that every
 * gem is collected; entering it then finishes the level.
 */
UCLASS()
class AGSExit : public AActor, public IGSSnakeInteractable
{
	GENERATED_BODY()

public:
	AGSExit();

	virtual void OnConstruction(const FTransform& Transform) override;

	// IGSSnakeInteractable
	virtual void OnSnakeEntered_Implementation(AGSSnake* Snake) override;

	UFUNCTION(BlueprintCallable, Category = "Exit")
	void SetOpen(bool bNewOpen);

	UFUNCTION(BlueprintPure, Category = "Exit")
	bool IsOpen() const { return bIsOpen; }

protected:
	virtual void BeginPlay() override;

	/** Root. Overlaps the snake's head when it enters the exit's cell. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Trigger;

	/** Blocks the Grid channel while the exit is closed, so a closed exit is a wall. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Blocker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Door;

	UPROPERTY(EditDefaultsOnly, Category = "Exit")
	TObjectPtr<UPaperFlipbook> ClosedFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "Exit")
	TObjectPtr<UPaperFlipbook> OpenFlipbook;

	/** Tick this on a placed exit to preview (or test) the open state. */
	UPROPERTY(EditAnywhere, Category = "Exit")
	bool bIsOpen = false;

private:
	UFUNCTION()
	void HandleAllGemsCollected();

	void UpdateVisuals();
	void UpdateCollision();
};
