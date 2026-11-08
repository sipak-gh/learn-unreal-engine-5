// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSExit.generated.h"

class UBoxComponent;
class UPaperFlipbook;
class UPaperFlipbookComponent;

/**
 * The level exit. Closed until every gem is collected, then open; entering an
 * open exit finishes the level (chapter 11).
 */
UCLASS()
class AGSExit : public AActor
{
	GENERATED_BODY()

public:
	AGSExit();

	virtual void OnConstruction(const FTransform& Transform) override;

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
	void UpdateVisuals();
	void UpdateCollision();
};
