// Chapter 9 example: one grouped sprite component drawing a 10x10 grid.
// Build.cs modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExGroupedSprites.generated.h"

class UPaperGroupedSpriteComponent;
class UPaperSprite;

/** Place in a level, pick a Sprite, press Play: random cells change colour. */
UCLASS()
class AExGroupedSprites : public AActor
{
	GENERATED_BODY()

public:
	AExGroupedSprites();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPaperGroupedSpriteComponent> Grid;

	/** Drawn once per cell. Any sprite works, e.g. S_SnakeBody. */
	UPROPERTY(EditAnywhere, Category = "Grid")
	TObjectPtr<UPaperSprite> Sprite;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", ClampMax = "100"))
	int32 Columns = 10;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", ClampMax = "100"))
	int32 Rows = 10;

	/** Distance between cell centres, in Unreal units. */
	UPROPERTY(EditAnywhere, Category = "Grid")
	float Spacing = 16.0f;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "0.05"))
	float RecolourInterval = 0.25f;

private:
	void RecolourRandomInstance();

	FTimerHandle RecolourTimer;
};
