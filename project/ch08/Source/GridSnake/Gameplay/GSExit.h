// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSExit.generated.h"

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

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
};
