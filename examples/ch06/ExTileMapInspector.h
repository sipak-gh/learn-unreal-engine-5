// Example for chapter 6 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExTileMapInspector.generated.h"

class UPaperTileMap;

/**
 * Reads a tile map asset and logs its size and layers. It changes nothing.
 * Drop it in any level, set Tile Map, and press Play.
 */
UCLASS()
class AExTileMapInspector : public AActor
{
	GENERATED_BODY()

public:
	AExTileMapInspector();

	UPROPERTY(EditAnywhere, Category = "Tiles")
	TObjectPtr<UPaperTileMap> TileMap;

protected:
	virtual void BeginPlay() override;
};
