// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/GSSnakeInteractable.h"
#include "GSExit.generated.h"

class UBoxComponent;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class USoundBase;

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

	/** Played once when the exit opens during play, then OpenFlipbook loops. */
	UPROPERTY(EditDefaultsOnly, Category = "Exit")
	TObjectPtr<UPaperFlipbook> OpeningFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "Exit")
	TObjectPtr<UPaperFlipbook> OpenFlipbook;

	/** Played when the exit opens during play (SW_ExitOpen). */
	UPROPERTY(EditDefaultsOnly, Category = "Exit")
	TObjectPtr<USoundBase> OpenSound;

	/** Tick this on a placed exit to preview (or test) the open state. */
	UPROPERTY(EditAnywhere, Category = "Exit")
	bool bIsOpen = false;

private:
	UFUNCTION()
	void HandleAllGemsCollected();

	UFUNCTION()
	void HandleDoorAnimFinished();

	void UpdateVisuals();
	void UpdateCollision();
};
