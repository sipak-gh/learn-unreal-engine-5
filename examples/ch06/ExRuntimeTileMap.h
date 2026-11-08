// Example for chapter 6 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExRuntimeTileMap.generated.h"

class UPaperTileMapComponent;
class UPaperTileSet;

/**
 * Builds a tile map in code when play starts: an 8x8 checkerboard of two tiles
 * from TileSet. The map belongs to the component, not to an asset.
 */
UCLASS()
class AExRuntimeTileMap : public AActor
{
	GENERATED_BODY()

public:
	AExRuntimeTileMap();

	UPROPERTY(EditAnywhere, Category = "Tiles")
	TObjectPtr<UPaperTileSet> TileSet;

	/** Tile index (in TileSet) for the "white" squares. */
	UPROPERTY(EditAnywhere, Category = "Tiles", meta = (ClampMin = "0"))
	int32 TileA = 0;

	/** Tile index for the "black" squares. */
	UPROPERTY(EditAnywhere, Category = "Tiles", meta = (ClampMin = "0"))
	int32 TileB = 4;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPaperTileMapComponent> TileMapComponent;
};
