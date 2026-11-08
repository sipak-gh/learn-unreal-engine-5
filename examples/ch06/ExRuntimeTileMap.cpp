// Example for chapter 6 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExRuntimeTileMap.h"
#include "PaperTileMapComponent.h"
#include "PaperTileSet.h"

DEFINE_LOG_CATEGORY_STATIC(LogExRuntimeTiles, Log, All);

AExRuntimeTileMap::AExRuntimeTileMap()
{
	TileMapComponent = CreateDefaultSubobject<UPaperTileMapComponent>(TEXT("TileMap"));
	SetRootComponent(TileMapComponent);
}

void AExRuntimeTileMap::BeginPlay()
{
	Super::BeginPlay();

	const int32 TileCount = TileSet ? TileSet->GetTileCount() : 0;
	if (TileA >= TileCount || TileB >= TileCount)
	{
		UE_LOG(LogExRuntimeTiles, Warning, TEXT("%s: set Tile Set, and keep Tile A/B below its %d tiles"), *GetName(), TileCount);
		return;
	}

	// A new map owned by this component: 8x8 tiles of 16x16 pixels, 1 pixel per unit, one layer.
	// SetTile refuses to edit a tile map asset, so this is the way to change tiles at runtime.
	TileMapComponent->CreateNewTileMap(8, 8, 16, 16, 1.0f, true);

	int32 CountA = 0;
	for (int32 Y = 0; Y < 8; ++Y)
	{
		for (int32 X = 0; X < 8; ++X)
		{
			const bool bUseA = (X + Y) % 2 == 0;
			CountA += bUseA ? 1 : 0;

			FPaperTileInfo Info;
			Info.TileSet = TileSet;
			Info.PackedTileIndex = bUseA ? TileA : TileB;
			TileMapComponent->SetTile(X, Y, /*Layer=*/0, Info);
		}
	}

	// Collision is built from the tiles' collision shapes; rebuild it after editing cells.
	TileMapComponent->RebuildCollision();

	UE_LOG(LogExRuntimeTiles, Log, TEXT("%s built an 8x8 map: %d tiles of index %d, %d of index %d"),
		*GetName(), CountA, TileA, 64 - CountA, TileB);
}
