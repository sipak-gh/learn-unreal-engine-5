// Example for chapter 6 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExTileMapInspector.h"
#include "PaperTileLayer.h"
#include "PaperTileMap.h"

DEFINE_LOG_CATEGORY_STATIC(LogExTiles, Log, All);

AExTileMapInspector::AExTileMapInspector()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AExTileMapInspector::BeginPlay()
{
	Super::BeginPlay();

	if (!TileMap)
	{
		UE_LOG(LogExTiles, Warning, TEXT("%s: set Tile Map in the Details panel"), *GetName());
		return;
	}

	UE_LOG(LogExTiles, Log, TEXT("%s: %dx%d tiles of %dx%d, %d layers"), *TileMap->GetName(),
		TileMap->MapWidth, TileMap->MapHeight, TileMap->TileWidth, TileMap->TileHeight, TileMap->TileLayers.Num());

	// Where Paper2D puts tile centres, in the map's local space (layer 0).
	UE_LOG(LogExTiles, Log, TEXT("  tile (0,0) centre %s, tile (1,1) centre %s"),
		*TileMap->GetTileCenterInLocalSpace(0, 0, 0).ToString(), *TileMap->GetTileCenterInLocalSpace(1, 1, 0).ToString());

	// Index 0 is the layer at the top of the Tile Map Editor's Layers panel.
	for (int32 LayerIndex = 0; LayerIndex < TileMap->TileLayers.Num(); ++LayerIndex)
	{
		const UPaperTileLayer* Layer = TileMap->TileLayers[LayerIndex];
		if (!Layer)
		{
			continue;
		}

		// Every layer has the map's size, so MapWidth x MapHeight covers all its cells.
		int32 Painted = 0;
		for (int32 Y = 0; Y < TileMap->MapHeight; ++Y)
		{
			for (int32 X = 0; X < TileMap->MapWidth; ++X)
			{
				// An empty cell has no tile set, so IsValid() is false.
				if (Layer->GetCell(X, Y).IsValid())
				{
					++Painted;
				}
			}
		}

		UE_LOG(LogExTiles, Log, TEXT("  layer %d '%s' collides=%s painted=%d"), LayerIndex, *Layer->LayerName.ToString(),
			Layer->GetLayerCollides() ? TEXT("true") : TEXT("false"), Painted);
	}
}
