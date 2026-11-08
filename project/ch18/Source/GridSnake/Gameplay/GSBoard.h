// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Snake/GSGridTypes.h"
#include "GSBoard.generated.h"

class UCameraComponent;
class UPaperTileMapComponent;

/**
 * One per level: draws the level's tile map, owns the orthographic camera that
 * frames it, and converts between grid cells and world positions.
 *
 * Cells are (X, Y) tile coordinates: X grows to the right, Y grows downwards,
 * (0, 0) is the top-left tile - the same numbers the Tile Map Editor shows.
 */
UCLASS()
class AGSBoard : public AActor
{
	GENERATED_BODY()

public:
	AGSBoard();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Width and height of the map, in tiles. */
	UFUNCTION(BlueprintPure, Category = "Grid")
	FIntPoint GetGridSize() const;

	/** Width of one tile in world units. GridSnake uses square tiles. */
	UFUNCTION(BlueprintPure, Category = "Grid")
	float GetCellSize() const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsInside(FIntPoint Cell) const;

	/** World-space centre of a cell, on the plane of the tile map's first layer. */
	UFUNCTION(BlueprintPure, Category = "Grid")
	FVector CellToWorld(FIntPoint Cell) const;

	/** The cell containing a world position (may be outside the map; check IsInside). */
	UFUNCTION(BlueprintPure, Category = "Grid")
	FIntPoint WorldToCell(const FVector& WorldLocation) const;

	/** True if any layer marked "Layer Collides" has a tile in this cell. Reads the tile map data directly. */
	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsWallTile(FIntPoint Cell) const;

	/**
	 * True if anything that blocks the Grid channel overlaps the cell: wall
	 * tiles (through their collision), closed exits, or any actor you set to
	 * block Grid.
	 * Asks the physics scene rather than the tile data.
	 */
	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsCellBlocked(FIntPoint Cell) const;

	/** Moves an actor to the centre of the cell it is in and returns that cell. */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FIntPoint SnapActorToCell(AActor* Actor) const;

	/** Which way the snake faces when this level starts. */
	UFUNCTION(BlueprintPure, Category = "Grid")
	EGSDirection GetStartDirection() const { return StartDirection; }

	UPaperTileMapComponent* GetTileMapComponent() const { return TileMapComponent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperTileMapComponent> TileMapComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	/** Set per level on the placed board: the direction the snake starts moving in. */
	UPROPERTY(EditAnywhere, Category = "Level")
	EGSDirection StartDirection = EGSDirection::Right;

	/** How far in front of the map the camera sits. Orthographic: this does not change the size. */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "10"))
	float CameraDistance = 500.0f;

	/** Extra space around the map, as a fraction (1.1 = 10% border). */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1"))
	float FramingMargin = 1.1f;

	/** The screen shape the framing is computed for. */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0.1"))
	float TargetAspectRatio = 16.0f / 9.0f;

private:
	void FrameCamera();
};
