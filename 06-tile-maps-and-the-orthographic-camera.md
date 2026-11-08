# Chapter 6 — Tile Maps and the Orthographic Camera

GridSnake is played on a grid, and Paper2D has a tool built for grids: the **tile map**. This
chapter turns the tile sheet from chapter 4 into a **tile set** and gives the wall tiles
collision. It then paints a two-layer **tile map** for the first level and works out exactly where
each tile ends up in the world. It also covers **orthographic cameras**, which show a flat 2D
scene with no perspective and can frame a whole map at any screen shape. GridSnake gains
`AGSBoard`, the actor at the centre of every level. It draws the level's tile map, converts
between grid cells and world positions, answers "is this cell a wall?", and owns the camera that
frames the board.

Previous: [Chapter 5](05-actors-and-components.md). Next:
[Chapter 7](07-the-gameplay-framework.md) (the gameplay framework).

As in every chapter, nothing here was compiled or run by the authors (see the README). Code was
checked against the UE 5.8 API reference, and the tile coordinate rules against Paper2D's source.
Where a book like this would show program output, you get an *Expected Output Log* block: the
lines the code's `UE_LOG` calls print, with the parts that vary marked as such.

## Tiles, tile sets and tile maps

Chapter 4 imported `T_Tiles`, a 64×32 pixel texture holding eight 16×16 tiles in two rows:

```text
+------+------+------+------+
|  0   |  1   |  2   |  3   |   row 0: floor tiles
+------+------+------+------+
|  4   |  5   |  6   |  7   |   row 1: wall tiles
+------+------+------+------+
```

You could place 336 sprite actors by hand for a 24×14 level, but that is slow to edit, slow to
draw and hard to query from code. Paper2D offers two asset types instead:

| Asset | Class | What it holds |
|---|---|---|
| Tile set | `UPaperTileSet` | A texture cut into a grid of equal tiles, plus per-tile data such as collision shapes |
| Tile map | `UPaperTileMap` | A grid of cells, in one or more layers; each cell names a tile from a tile set, or is empty |

A tile map is drawn by a single component, `UPaperTileMapComponent`. It renders every layer in one
go and builds one collision body for all the tiles that have collision. To draw a level, you place
one actor with that component and point the component at the map.

The numbers in the picture are **tile indices**. A tile set numbers its tiles left to right, top
to bottom, starting at 0. The tile map stores, for each cell, the tile set and the index. That is
why the art layout from chapter 4 matters: floor tiles are indices 0 to 3 and wall tiles are 4 to
7, and the rest of the course relies on that.

## Tile sets

A tile set is made from a texture in a few clicks.

1. In the Content Browser, open `Art/Tiles` (or wherever `T_Tiles` lives, see
   `project/ch04/ASSETS.md`).
2. Right-click `T_Tiles` and choose **Sprite Actions > Create Tile Set** (the label may differ
   slightly in your version). A new asset appears next to the texture.
3. Rename it `TS_Dungeon`. The `TS_` prefix is the usual one for tile sets; tile maps use `TM_`.
4. Double-click it to open the **Tile Set Editor**.

The editor shows the texture with a grid drawn over it. Its **Details** panel holds the settings
that define the grid:

- **Tile Size**: the width and height of one tile in pixels. Set it to 16 × 16. Older versions of
  the editor showed separate *Tile Width* and *Tile Height* fields; the meaning is the same.
- **Margin**: pixels to skip around the edge of the texture. Leave it at 0.
- **Spacing**: pixels between neighbouring tiles. Leave it at 0. Tile sheets that leave a
  one-pixel gap between tiles, to stop colours bleeding at the edges, use this.

With a 16 × 16 tile size, a 64 × 32 texture gives exactly eight tiles. If the grid lines do not
fall on the tile edges, the tile size is wrong. Fix it before you paint any map: a tile map stores
tile *indices*, so changing the grid afterwards changes which picture each index refers to.

### Per-tile collision

Each tile in a tile set can carry its own collision shapes. A tile with no shape has no collision
at all, even when it is painted on a layer that collides. GridSnake's walls need collision, so the
four wall tiles get a box each:

1. Click tile 4 in the tile set editor. The **Single Tile Editor** shows the selected tile on its
   own.
2. In the toolbar, click **Add Box** (the label may differ slightly in your version). A box shape
   appears and fills the whole tile.
3. Repeat for tiles 5, 6 and 7.
4. Save the tile set.

The shapes are 2D outlines. When the tile map builds its collision, it extrudes them into 3D
boxes, with a depth set by the tile map's **Collision Thickness** (next section). The floor tiles
0 to 3 get no shape: the snake moves over them.

The Single Tile Editor also has a per-tile **User Data Name** field for tagging tiles ("lava",
"ice"). GridSnake does not use it: its rules come from the layer a tile is painted on.

## Tile maps and layers

A tile map is a separate asset. One tile set can feed many tile maps, and each GridSnake level gets
its own map.

1. In `Art/Tiles`, click **Add > Paper2D > Tile Map** (Epic's tile map page lists it under
   **Add > Miscellaneous**; the menu has moved between versions). Name it `TM_Level01` and
   double-click it to open the **Tile Map Editor**.
2. In the Details panel, under **Setup**, set:
   - **Map Width** = 24 and **Map Height** = 14. These are counts of tiles, not pixels.
   - **Tile Width** = 16 and **Tile Height** = 16, the tile set's tile size.
   - **Projection Mode** = Orthogonal. The other modes are isometric and hexagonal layouts.
3. Leave **Pixels Per Unreal Unit** at 1 (one art pixel is one world unit, as for the sprites in
   chapter 4) and **Separation Per Layer** at 4, explained next.

A 24 × 14 map of 16-pixel tiles is 384 × 224 world units. That is the whole playing field.

### Layers

A tile map has one or more **layers** (`UPaperTileLayer`). Every layer has the same width and
height as the map, and every cell of every layer holds either a tile or nothing. Layers stack:
where a front layer has a tile, it covers the layers behind it.

The **Layers** panel of the Tile Map Editor lists them. Layer order in that list is the order of
the `TileLayers` array in the asset:

- The layer at the **top** of the list is index 0, and it is drawn **in front**. Each following
  layer is drawn further back.
- **Add New Layer** inserts the new layer *above* the selected one. Other commands bring a layer
  forward or send it backward (the labels may differ slightly in your version).

Paper2D does not sort layers with a special draw order. It moves them apart in depth: layer `L` is
placed `L × Separation Per Layer` units further from the viewer than layer 0. With a separation of
4, layer 0 sits at Y = 0 in the map's local space, layer 1 at Y = −4, layer 2 at Y = −8. A camera
in front of the map (at positive Y, looking towards −Y) sees layer 0 first. That is also why
chapter 5 put the gem and exit sprites at Y = +5: in front of every layer of the map.

GridSnake uses two layers:

| Index | Name | Contents | Layer Collides |
|---|---|---|---|
| 0 | `Walls` | the border and inner walls, wall tiles 4–7 | on |
| 1 | `Floor` | every cell, floor tiles 0–3 | **off** |

Set them up like this:

1. A new tile map starts with one layer. Select it in the Layers panel, double-click its name, and
   rename it `Floor`.
2. With `Floor` selected, find **Layer Collides** in the Details panel and **untick** it.
3. Click **Add New Layer** (the label may differ slightly in your version). The new layer appears
   above `Floor`. Rename it `Walls`.
4. With `Walls` selected, check that **Layer Collides** is ticked.

Step 2 matters. Every new layer starts with Layer Collides ticked. Floor tiles have no collision
shapes, so physics would not care, but `AGSBoard::IsWallTile` treats *any* painted cell on a
colliding layer as a wall. A colliding `Floor` makes every cell a wall, and from chapter 9 the
snake would refuse to move.

### Painting

The Tile Map Editor works like a small paint program. The **Toolbox** has the tools, and the
**Active Tile Set** panel shows the tiles you can paint with.

1. Pick `TS_Dungeon` as the active tile set and click a tile to make it the brush.
2. Select the `Floor` layer. With the **Fill** tool, click anywhere: every cell gets the floor
   tile. Use the **Paint** tool to add a few of the other floor tiles for variety.
3. Select the `Walls` layer. Paint a border of wall tiles around the map, then a few inner walls.
   The **Erase** tool clears cells on the selected layer.

On the `Walls` layer, keep cell (12, 7) and the cells to its left free for the snake's start in
chapter 7, plus three cells for the gems and one border cell for the exit. The editor shows the
cell under the cursor. Its viewport menus can also show the tile grid and **Collision** (the labels
may differ slightly in your version). Save the map.

### Tile map collision

The Details panel's **Collision** section controls the single collision body the component builds:

- **Collision Thickness**: how deep, in world units, the extruded tile shapes are. The default is
  50. Chapter 10 traces against the walls and may raise it.
- **Sprite Collision Domain**: **Use 3D Physics** (the default), **Use 2D Physics**, or **None**.
  Keep 3D. The 2D option is experimental, and the overlap and trace queries in chapter 10 are 3D
  physics queries.

Epic's documentation marks tile maps as experimental in places. That is one reason `AGSBoard`
does not ask physics whether a cell is a wall: it reads the tile map *data*, which always says
exactly what you painted.

### Inspecting a tile map from C++

The first example reads a tile map asset and prints what it contains. It changes nothing, so you
can point it at `TM_Level01` safely.

`examples/ch06/ExTileMapInspector.h`

```cpp
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
```

`examples/ch06/ExTileMapInspector.cpp`

```cpp
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
```

Add both files to a project whose `.Build.cs` lists `Paper2D` (GridSnake's has since chapter 5),
build, drag **ExTileMapInspector** into a level, set its **Tile Map** to `TM_Level01`, and press
Play.

Expected Output Log (filter: `LogExTiles`; `<n>` depends on your painting, and the vector
formatting may differ slightly, for example `-0.000`):

```text
LogExTiles: TM_Level01: 24x14 tiles of 16x16, 2 layers
LogExTiles:   tile (0,0) centre X=0.000 Y=0.000 Z=0.000, tile (1,1) centre X=16.000 Y=0.000 Z=-16.000
LogExTiles:   layer 0 'Walls' collides=true painted=<n>
LogExTiles:   layer 1 'Floor' collides=false painted=336
```

- **The map's public fields.** `MapWidth`, `MapHeight`, `TileWidth`, `TileHeight` and `TileLayers`
  are public `UPROPERTY` members of `UPaperTileMap`. They are the same values you set in the
  Details panel. Reading them from C++ is normal; writing them is not, because the layers would
  no longer match the map size. Use the editor, or the component functions in the last section.
- **`TileLayers` is an array of `UPaperTileLayer*`.** Each layer is a `UObject` stored inside the
  map asset. `LayerName` is an `FText`, the type Unreal uses for text that a user sees and that may
  be translated. `ToString()` turns it into an `FString` for the log.
- **`GetCell(X, Y)`** returns an `FPaperTileInfo`: a tile set pointer and a packed tile index. An
  empty cell has no tile set, and `IsValid()` is false for it. `GetTileIndex()` would give the
  index (0 to 7 for `TS_Dungeon`).
- **The check.** 336 is 24 × 14: the floor fill reached every cell. A lower number means some
  floor cells are empty. A `Floor` line with `collides=true` means you skipped the Layer Collides
  step above, and `layer 0 'Floor'` means the layers are in the wrong order: select `Walls` and
  bring it forward.
- **The tile centres** are the subject of the next section.

## Tile space and world space

A tile map has its own coordinates, **tile space**: whole numbers (X, Y), where X counts columns
from the left and Y counts rows from the top. Cell (0, 0) is the top-left tile, cell (23, 13) is
the bottom-right one of `TM_Level01`. These are the numbers the Tile Map Editor shows under the
cursor, and GridSnake uses them for every grid position from here on.

The world has different axes. Chapter 4 explained that Paper2D draws in the **X–Z plane**: X grows
to the right and Z grows *up*. Rows in tile space grow *down*. So moving one row down in tile
space moves −16 units in Z, and the bottom rows of every level have negative Z.

Where is tile (0, 0)? Paper2D's answer, for an orthogonal map, is that the **centre** of tile
(0, 0) is the map's local origin. With 16-unit tiles:

| | Local position |
|---|---|
| Centre of tile (x, y), layer 0 | (16x, 0, −16y) |
| Top-left corner of the whole map | (−8, 0, 8) |
| Bottom-right corner of `TM_Level01` | (376, 0, −216) |
| Tile (x, y) covers | X from 16x − 8 to 16x + 8, Z from −16y + 8 down to −16y − 8 |
| Layer `L` | the same, at Y = −4L |

The inspector's second log line shows this directly: tile (0, 0) is centred at the origin, and
tile (1, 1) one tile right and one tile down. That line is in the example so you can check the
rule in your engine version instead of trusting this table. If it ever prints something else, the
code below still works, because it never hard-codes the formula. It asks the engine.

Two functions convert between the spaces:

- `UPaperTileMapComponent::GetTileCenterPosition(TileX, TileY, LayerIndex, bWorldSpace)` returns
  the centre of a tile. With `bWorldSpace` false you get the map's local position. With true the
  result goes through the component's transform, so it is correct wherever the actor is placed,
  rotated or scaled. A sibling, `GetTileCornerPosition`, returns the top-left corner instead.
- `UPaperTileMap::GetTileCoordinatesFromLocalSpacePosition(Position, OutTileX, OutTileY)` goes the
  other way: it takes a *local* position and writes the tile coordinates into its two `int32&`
  out-parameters. It rounds **down** (it floors), so a point just left of the map gives X = −1,
  not 0. It does not check bounds: positions outside the map give coordinates outside the map.

There is no world-space version of the reverse function. You convert world to local yourself with
the component's transform, then call it. `AGSBoard::WorldToCell` does exactly that.

### `FIntPoint`

A cell is two integers, and Unreal has a struct for that: `FIntPoint`, with public `int32` members
`X` and `Y`. GridSnake uses it as its cell type everywhere:

```cpp
FIntPoint Cell(12, 7);
Cell.X += 1;                                      // one column right
const FIntPoint Below = Cell + FIntPoint(0, 1);   // one row down
const bool bSame = (Cell == Below);               // false
UE_LOG(LogTemp, Log, TEXT("%s"), *Cell.ToString()); // "X=13 Y=7"
```

It supports `+`, `-` and `==`, works as a `TMap` or `TSet` key, and is a Blueprint type, so
`BlueprintPure` functions can take and return it. `X=13 Y=7` is the format later logs show.

## AGSBoard

Every GridSnake level has exactly one board. It is the only actor that knows the level's tile map,
and every other actor asks it about the grid. That keeps the tile map details in one class: the
snake (chapter 7) thinks in cells and asks the board for world positions, never the reverse.

The milestone adds one class in a new pair of files. Nothing else changes compared with chapter 5.

`project/ch06/Source/GridSnake/Gameplay/GSBoard.h`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	UPaperTileMapComponent* GetTileMapComponent() const { return TileMapComponent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperTileMapComponent> TileMapComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

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
```

The header follows the pattern from chapter 5:

- **Two components**, both `VisibleAnywhere`: the tile map component and a camera. You can select
  them and edit *their* properties in the Details panel (this is how you set the tile map), but you
  cannot replace them.
- **No `UPaperTileMap` property on the board.** The map is a property of the component
  (`UPaperTileMapComponent::TileMap`), so the board reads it from there. One source of truth.
- **The grid API is `BlueprintPure` and `const`.** Pure functions have no execution pins in
  Blueprint and must not change anything. Every one of them only reads.
- **Three camera settings** are `EditAnywhere`, so each placed board can tune its framing. They
  have `ClampMin` metadata: the Details panel will not accept a margin below 1 or a distance
  below 10.
- **`GetTileMapComponent()`** is a plain inline C++ getter, with no `UFUNCTION`, for later C++
  code. The component classes are forward-declared; only the `.cpp` includes their headers.

`project/ch06/Source/GridSnake/Gameplay/GSBoard.cpp`

```cpp
// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSBoard.h"
#include "GridSnake.h"
#include "Camera/CameraComponent.h"
#include "PaperTileLayer.h"
#include "PaperTileMap.h"
#include "PaperTileMapComponent.h"

AGSBoard::AGSBoard()
{
	PrimaryActorTick.bCanEverTick = false;

	TileMapComponent = CreateDefaultSubobject<UPaperTileMapComponent>(TEXT("TileMap"));
	SetRootComponent(TileMapComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(TileMapComponent);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	// Paper2D draws sprites in the X-Z plane. Yaw -90 makes the camera look down
	// -Y, so +X is screen right and +Z is screen up.
	Camera->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
}

void AGSBoard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Re-frame whenever the actor or its tile map changes in the editor.
	FrameCamera();
}

void AGSBoard::BeginPlay()
{
	Super::BeginPlay();

	const FIntPoint Size = GetGridSize();
	UE_LOG(LogGridSnake, Log, TEXT("Board %s: %dx%d cells of %.0f units"), *GetName(), Size.X, Size.Y, GetCellSize());
}

FIntPoint AGSBoard::GetGridSize() const
{
	int32 Width = 0;
	int32 Height = 0;
	int32 NumLayers = 0;
	TileMapComponent->GetMapSize(Width, Height, NumLayers);
	return FIntPoint(Width, Height);
}

float AGSBoard::GetCellSize() const
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	return TileMap ? TileMap->TileWidth * TileMap->GetUnrealUnitsPerPixel() : 0.0f;
}

bool AGSBoard::IsInside(FIntPoint Cell) const
{
	const FIntPoint Size = GetGridSize();
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Size.X && Cell.Y < Size.Y;
}

FVector AGSBoard::CellToWorld(FIntPoint Cell) const
{
	return TileMapComponent->GetTileCenterPosition(Cell.X, Cell.Y, /*LayerIndex=*/0, /*bWorldSpace=*/true);
}

FIntPoint AGSBoard::WorldToCell(const FVector& WorldLocation) const
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (!TileMap)
	{
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}

	const FVector LocalLocation = TileMapComponent->GetComponentTransform().InverseTransformPosition(WorldLocation);
	FIntPoint Cell;
	TileMap->GetTileCoordinatesFromLocalSpacePosition(LocalLocation, Cell.X, Cell.Y);
	return Cell;
}

bool AGSBoard::IsWallTile(FIntPoint Cell) const
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (!TileMap || !IsInside(Cell))
	{
		return false;
	}

	for (const UPaperTileLayer* Layer : TileMap->TileLayers)
	{
		if (Layer && Layer->GetLayerCollides() && Layer->GetCell(Cell.X, Cell.Y).IsValid())
		{
			return true;
		}
	}
	return false;
}

void AGSBoard::FrameCamera()
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (!TileMap || TileMap->MapWidth <= 0 || TileMap->MapHeight <= 0)
	{
		return;
	}

	// Centre the camera between the first and last tile, in the actor's local space.
	const FVector FirstTile = TileMapComponent->GetTileCenterPosition(0, 0, 0, /*bWorldSpace=*/false);
	const FVector LastTile = TileMapComponent->GetTileCenterPosition(TileMap->MapWidth - 1, TileMap->MapHeight - 1, 0, /*bWorldSpace=*/false);
	const FVector Centre = (FirstTile + LastTile) * 0.5f;
	Camera->SetRelativeLocation(FVector(Centre.X, CameraDistance, Centre.Z));

	// Ortho Width is the visible width in world units. Fit whichever of the
	// map's width or height is the tighter fit for the target aspect ratio.
	const float UnitsPerPixel = TileMap->GetUnrealUnitsPerPixel();
	const float MapWidth = TileMap->MapWidth * TileMap->TileWidth * UnitsPerPixel;
	const float MapHeight = TileMap->MapHeight * TileMap->TileHeight * UnitsPerPixel;
	Camera->SetOrthoWidth(FMath::Max(MapWidth, MapHeight * TargetAspectRatio) * FramingMargin);
}
```

The file in order:

### The constructor

- **The tile map component is the root.** The board's location *is* the map's location. Placing
  the board at the origin puts the centre of tile (0, 0) at the world origin.
- **No tick.** `bCanEverTick = false`: the board never changes on its own. Chapter 5 explained why
  turning tick off is the right default.
- **The camera is attached to the tile map component**, so it moves with the board. Its projection
  mode and rotation are set here, in the constructor, because they never change. Its *location*
  and *ortho width* depend on the tile map, which is not known in the constructor. They are set in
  `FrameCamera`. The next section explains the yaw of −90 degrees.
- **The component name `TileMap`** is what the Components list of the Details panel shows. To
  assign the map you select that component, not the actor.

### `OnConstruction`

Chapter 5 introduced `OnConstruction`: it runs in the editor every time a placed actor is created,
moved, or has a property changed, including a property of one of its components. So assigning
`TM_Level01` re-runs `FrameCamera`, and the camera updates immediately. In PIE the placed board
keeps the camera values computed in the editor.

### `BeginPlay`

One log line, so you can see that the board is in the level and which map size it has. It uses
`LogGridSnake` from chapter 3. `%.0f` prints the float cell size with no decimals: `16`.

### `GetGridSize` and `GetCellSize`

`UPaperTileMapComponent::GetMapSize` fills three `int32&` out-parameters: width, height and number
of layers. The board only needs the first two. A fresh tile map component is never quite empty:
in the Paper2D source, it creates a small map of its own (4 × 4 tiles of 32 pixels, no layers)
until you assign an asset. So a board without `TM_Level01` reports a 4 × 4 grid, not 0 × 0. The
`!TileMap` checks in this file guard against a map being cleared to none explicitly.

`GetCellSize` returns tile pixels times `GetUnrealUnitsPerPixel()` (`1 / PixelsPerUnrealUnit`):
16 units. Computing it instead of writing 16 keeps the board correct for maps with other settings.

### `IsInside`

A plain bounds check; the last column is `Size.X - 1`. `IsWallTile` calls it first, because
`GetCell` on a layer expects valid coordinates.

### `CellToWorld`

One call: the centre of the tile, in world space, on layer 0, the front layer at the map's Y = 0.
Actors' own small Y offsets (+5 for gems) put them in front of it.

### `WorldToCell`

The reverse needs two steps:

1. `GetComponentTransform()` is the component's world transform. `InverseTransformPosition`
   undoes it, giving the position in the component's local space. With the board at the origin
   they are equal; if you move the board, this step keeps the answer right.
2. `GetTileCoordinatesFromLocalSpacePosition` writes the tile coordinates into two `int32&`
   parameters, here straight into the `FIntPoint`'s `X` and `Y`.

With no map, the function returns `(INDEX_NONE, INDEX_NONE)`, Unreal's "no index" constant (−1):
outside any map. Callers check `IsInside`, as the header comment says.

### `IsWallTile`

This is the function behind "can the snake move there?" in chapter 9. A cell is a wall if **any
layer that collides has a tile in it**:

`GetLayerCollides()` is the **Layer Collides** checkbox, and `GetCell(X, Y).IsValid()` means
"something is painted here", as in the inspector example.

The function never looks at the tile index or at the tile's collision shape. Paint any tile on a
colliding layer and that cell becomes a wall. That keeps level design simple: the `Walls` layer
*is* the wall data. It is also why the `Floor` layer must not collide.

Reading the data instead of tracing against physics works in the editor, costs a few array
reads, and gives exactly what the designer painted. Chapter 10 adds physics checks for actors,
such as the closed exit, that are not tiles.

### `FrameCamera`

The camera is centred on the map, then zoomed to fit it. The next section explains the zoom.

The centre is the midpoint between the centres of the first tile (0, 0) and the last tile
(MapWidth − 1, MapHeight − 1). The map is symmetric around that point, so the camera is centred
no matter where Paper2D puts the map's origin. For `TM_Level01`:

```text
first tile centre   (0, 0, 0)
last tile centre    (23 * 16, 0, -13 * 16) = (368, 0, -208)
midpoint            (184, 0, -104)
camera location     (184, 500, -104)      (relative to the board)
```

The positions are *local* (`bWorldSpace` false), because the camera is attached to the tile map
component and its location is relative to it. The camera's Y is `CameraDistance`, 500 units in
front of the map, towards the viewer.

## Orthographic cameras

A normal game camera uses a **perspective** projection: things further away look smaller. A 2D
game wants every tile the same size, wherever it is on screen. An **orthographic** projection
does that: it projects everything straight onto the screen, so size no longer depends on distance,
only on how much of the world the view covers. In Unreal that is one number, **Ortho Width**: the
width of the visible area, in world units.

`UCameraComponent` supports both projections:

- `SetProjectionMode(ECameraProjectionMode::Orthographic)` switches it (the Details panel shows it
  as **Projection Mode**).
- `SetOrthoWidth(float)` sets the visible width. The Details panel shows it as **Ortho Width**. A
  smaller width shows less of the world, so everything looks bigger: it zooms in.
- The visible *height* follows from the screen's shape: height = width ÷ aspect ratio. On a
  1920 × 1080 screen (16:9), an ortho width of 384 shows 384 × 216 units. That assumes the default
  aspect ratio axis constraint, *Maintain X-Axis FOV*, set in Project Settings. With *Maintain
  Y-Axis FOV*, Unreal keeps the height fixed instead, and the width follows the screen.

Because size does not depend on distance, `CameraDistance` on the board does not change the
picture. It only has to put the camera in front of everything it should see. 500 units leaves
plenty of room for the map's layers (Y = 0 to −4) and the sprites in front of them (Y = +5).

### Which way the camera looks

A camera looks along its component's forward direction, its local +X axis. Paper2D draws in the X–Z
plane, seen from the +Y side, so the camera must look towards −Y. A yaw of −90 degrees turns +X
(forward) into −Y. The camera's right-hand axis then points along world +X and its up axis along
+Z:

| Camera axis | Direction with yaw −90 | On screen |
|---|---|---|
| forward (+X local) | world −Y | into the screen |
| right (+Y local) | world +X | to the right |
| up (+Z local) | world +Z | up |

So world X is screen right and world Z is screen up, which is what the tile coordinates assume. A
camera at yaw +90 would look at the map from behind, and the level would appear mirrored.

In the constructor this is `FRotator(0.0f, -90.0f, 0.0f)`. `FRotator`'s constructor takes pitch,
yaw, roll, in that order. The Details panel shows the same rotation as **Rotation** X (roll) = 0,
Y (pitch) = 0, Z (yaw) = −90, so be careful when you type a rotation by hand.

### Fitting the map on screen

The camera must show the whole map on a 16:9 screen, with a small border. Two conditions:

- The visible width must cover the map's width: `OrthoWidth >= MapWidth`.
- The visible height must cover the map's height. The visible height is `OrthoWidth / Aspect`, so
  `OrthoWidth >= MapHeight * Aspect`.

The smallest width that satisfies both is the larger of the two, and the margin adds 10 percent:

```cpp
Camera->SetOrthoWidth(FMath::Max(MapWidth, MapHeight * TargetAspectRatio) * FramingMargin);
```

For `TM_Level01`:

```text
MapWidth                     24 * 16          = 384
MapHeight * Aspect           14 * 16 * 16/9   = 398.2
OrthoWidth                   398.2 * 1.1      = 438.0
visible area at 16:9         438.0 x 246.4
```

The height was the tight side here: 14 rows at 16:9 need more width than 24 columns do. A wide,
flat map would make the width the tight side. The `Max` picks whichever applies.

`TargetAspectRatio` is a guess about the player's screen. On a narrower screen (4:3) the width
stays 438 and the visible height grows to 328.5: the map still fits. On a wider one (21:9) the
height shrinks to about 188 and the top and bottom rows are cut off. So set it to the *widest*
shape you support. Alternatively, tick **Constrain Aspect Ratio** on the camera component and set
its **Aspect Ratio**: the engine then adds black bars on other screen shapes. GridSnake leaves it
off, so the picture fills the screen.

### Near and far clip planes

A camera only draws what lies between its near and far clip planes. Recent engine versions
compute them automatically for orthographic cameras (**Auto Calculate Ortho Planes**,
`bAutoCalculateOrthoPlanes`), and GridSnake keeps the defaults. If parts of the map vanish in the
board camera's view, untick it and set **Ortho Near Clip Plane** to 0 and **Ortho Far Clip Plane**
to something larger than `CameraDistance`, such as 2000 (the labels may differ slightly in your
version).

### A camera as an actor of its own

`AGSBoard` keeps its camera as a component, because framing depends on the map. A camera can also
be an actor of its own. This example is the smallest orthographic camera actor that takes over the
player's view.

`examples/ch06/ExOrthoCamera.h`

```cpp
// Example for chapter 6 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExOrthoCamera.generated.h"

class UCameraComponent;

/**
 * An orthographic camera that takes over the player's view when play starts.
 * Place it at Y = 500 with rotation (0, -90, 0) to look at Paper2D sprites.
 */
UCLASS()
class AExOrthoCamera : public AActor
{
	GENERATED_BODY()

public:
	AExOrthoCamera();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Visible width of the view in world units (1 unit = 1 pixel in this course). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "1"))
	float OrthoWidth = 512.0f;

	/** Seconds to blend from the current view to this camera. */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0"))
	float BlendTime = 0.5f;

	/** Changes the visible width. Smaller values zoom in. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ZoomTo(float NewWidth);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;
};
```

`examples/ch06/ExOrthoCamera.cpp`

```cpp
// Example for chapter 6 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExOrthoCamera.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AExOrthoCamera::AExOrthoCamera()
{
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Camera);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
}

void AExOrthoCamera::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Apply the property to the component so the editor preview matches.
	Camera->SetOrthoWidth(OrthoWidth);
}

void AExOrthoCamera::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no player controller to take over"), *GetName());
		return;
	}

	// The view target is the actor the player's camera manager looks through.
	// An actor with a camera component supplies that component's view.
	PlayerController->SetViewTargetWithBlend(this, BlendTime);
	UE_LOG(LogTemp, Log, TEXT("%s is now the view target (ortho width %.0f)"), *GetName(), OrthoWidth);
}

void AExOrthoCamera::ZoomTo(float NewWidth)
{
	OrthoWidth = FMath::Max(1.0f, NewWidth);
	Camera->SetOrthoWidth(OrthoWidth);
	UE_LOG(LogTemp, Log, TEXT("%s ortho width %.0f"), *GetName(), OrthoWidth);
}
```

To try it, drag **ExOrthoCamera** into `L_Level01`. In the Details panel set its **Location** to
(184, 500, −104), the board's centre, and its **Rotation** Z (yaw) to −90. Then press **Play**
(not Simulate: Simulate has no player controller).

Expected Output Log (filter: `LogTemp`; the actor name is illustrative):

```text
LogTemp: ExOrthoCamera_0 is now the view target (ortho width 512)
```

The view blends from the default pawn's camera to the map, seen flat. Delete the actor afterwards:
chapter 7 makes the *board* the view target, and two actors fighting over the view would be
confusing.

- **The camera is the root component**, so you rotate the actor itself. The board, by contrast,
  rotates only its camera component.
- **A property mirrored onto a component.** `OrthoWidth` lives on the actor and `OnConstruction`
  copies it to the camera, as the exit's flipbook was handled in chapter 5.
- **View targets.** Every player controller has a *camera manager* that decides what the player
  sees. It looks through a **view target**, an actor. By default that is the pawn the controller
  possesses. `SetViewTargetWithBlend` switches to another actor and blends over `BlendTime`
  seconds. When the view target has an active camera component, the camera manager uses that
  component's location, rotation, projection and ortho width. The blend from a perspective view
  to an orthographic one may look abrupt; that is expected.
- **`GetFirstPlayerController()`** is the local player's controller in a single-player game. In
  PIE, the player is logged in before actors begin play, so it is ready in `BeginPlay`. Chapter 7
  explains the player controller and its camera properly.
- **`ZoomTo`** is `BlueprintCallable`, so a Blueprint such as the Level Blueprint can call it.
  `FMath::Max` keeps the width positive; zero would make the projection invalid.

## Placing the board

With the tile set, the tile map and the class ready, the level can get its board. These steps
match `project/ch06/ASSETS.md`.

1. Build the project (or use Live Coding) so the editor knows `AGSBoard`. The class appears under
   **C++ Classes/GridSnake/Gameplay** in the Content Browser.
2. Open `L_Level01`. Delete the test sprites from chapter 4 if they are still there. Keep the
   gems and the exit.
3. Drag `GSBoard` into the level. In the Details panel set its **Location** to (0, 0, 0) and leave
   the rotation at zero.
4. In the Details panel's component list, select the **TileMap** component. Under its tile map
   settings, set **Tile Map** to `TM_Level01`. The map appears in the viewport.
5. Select the **Camera** component. Its **Ortho Width** now reads about 438, and its location
   (184, 500, −104). `OnConstruction` computed both when you assigned the map. If the values have
   not updated, move the board slightly and undo the move: that re-runs `OnConstruction`.
6. Move the three gems and the exit onto free cells. Nothing snaps them yet (chapter 10 adds
   that), so type their locations. A cell centre is (16x, 0, −16y). For example, cell (3, 2) is
   (48, 0, −32). With the viewport's grid snap set to 16, dragging also lands on cell centres. The
   exit goes on a gap in the border wall.
7. Save the level.

There is no `BP_GSBoard`: the board has no class-wide art to pick, since the tile map is a
per-level choice made on the placed actor, so the C++ class is placed directly.

### Looking through the board's camera

Pressing Play still shows the default pawn's view, because nothing has made the board the view
target yet. That is chapter 7's job. You can check the framing in the editor already:

- **Camera preview.** Select the board's **Camera** component. The viewport shows a small preview
  of what the camera sees in one corner (the preview may be disabled in your editor settings).
- **Piloting.** Right-click the board in the **Outliner** and choose **Pilot 'GSBoard'** (the label
  may differ slightly in your version). The viewport now looks through the board's camera
  component: you see the whole map, flat, with a small border. While piloting, moving the viewport
  moves the actor, so do not drag around. To stop, click the **Eject** button at the top left of
  the viewport.

### Expected Output Log

Press Play (or Simulate) with the Output Log open.

Expected Output Log (filter: `LogGridSnake`; actor names are illustrative, and the board and exit
lines may come in either order):

```text
LogGridSnake: GSGameInstance initialised: 3 gems per level, step 0.15s
LogGridSnake: Board GSBoard_0: 24x14 cells of 16 units
LogGridSnake: BP_GSExit_C_0 starts closed
LogGridSnake: GridSnake game mode started on map UEDPIE_0_L_Level01 (collect 3 gems)
```

- The board and the exit both log from `BeginPlay`, which the game mode's `StartPlay` dispatches
  before printing its own line (chapter 5). Do not rely on the order among actors.
- `24x14` and `16` come from the map asset. If you see `4x4 cells of 32 units`, the board still
  uses the component's own placeholder map (see `GetGridSize` above): select the **TileMap**
  component and set **Tile Map**.

## Changing tiles at runtime

`TM_Level01` is an asset. The board draws it, but never changes it, and a game should not change
it: edits to an asset at runtime would change it for every level that uses it and, in the editor,
could end up saved. Paper2D enforces this. `UPaperTileMapComponent::SetTile` only works on a
component that **owns** its tile map, and a map shared as an asset is not owned by any component.

A component gets a map of its own in two ways:

- `CreateNewTileMap(...)` creates a new, empty map inside the component.
- `MakeTileMapEditable()` copies the assigned asset into a private map owned by the component.
  Changes then affect only that component, and the asset stays untouched. `OwnsTileMap()` tells you
  which case you are in.

This example builds a map from nothing.

`examples/ch06/ExRuntimeTileMap.h`

```cpp
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
```

`examples/ch06/ExRuntimeTileMap.cpp`

```cpp
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
```

Drag **ExRuntimeTileMap** into a level, away from the board (for example at X = 500), set its
**Tile Set** to `TS_Dungeon`, and Simulate (**Alt+S**). In the Left or Right view (whichever has
+X to the right), a checkerboard of floor and wall tiles appears where the actor is. In the editor it is empty: the map only exists once
play has started.

Expected Output Log (filter: `LogExRuntimeTiles`; the actor name is illustrative):

```text
LogExRuntimeTiles: ExRuntimeTileMap_0 built an 8x8 map: 32 tiles of index 0, 32 of index 4
```

- **`CreateNewTileMap(MapWidth, MapHeight, TileWidth, TileHeight, PixelsPerUnrealUnit,
  bCreateLayer)`** creates the map with the same settings as the asset's Details panel. The last
  argument asks for one empty layer, which `SetTile` then fills.
- **`FPaperTileInfo`** is one cell: the tile set and a tile index. The index is called *packed*
  because its top bits can hold flip and rotate flags for the tile. A plain index such as 4 has no
  flags set.
- **The guard** uses `UPaperTileSet::GetTileCount()`: 8 for `TS_Dungeon`. An index past the end
  would draw nothing. With no tile set the count is 0 and the guard catches that too.
- **`SetTile` does not update collision.** The API reference says so, and asks you to call
  `RebuildCollision()` once after a batch of edits. Run the console command `show collision` while
  simulating: only the 32 squares of tile 4, which has a box, show collision.
- **This is not GridSnake's path.** GridSnake's levels never change shape while playing. This is
  the escape hatch for procedural levels or destructible walls.

## Summary

- A **tile set** (`UPaperTileSet`) cuts a texture into numbered tiles and stores per-tile data,
  such as collision shapes. `TS_Dungeon` has floor tiles 0–3 and wall tiles 4–7, with a box on
  each wall tile.
- A **tile map** (`UPaperTileMap`) is a grid of cells in one or more **layers**. Layer 0 is at the
  top of the Layers panel and drawn in front; each further layer sits `Separation Per Layer` units
  further back. Layers collide by default; GridSnake's `Floor` layer must not.
- One **`UPaperTileMapComponent`** draws the whole map and builds its (3D by default) collision.
- **Tile space** has (0, 0) at the top-left and Y growing down; world Z grows up. Paper2D centres
  tile (0, 0) on the map's local origin, so tile (x, y) is centred at (16x, 0, −16y) for 16-unit
  tiles. `GetTileCenterPosition` and `GetTileCoordinatesFromLocalSpacePosition` convert between the
  spaces, and the second one floors. `FIntPoint` is the cell type.
- **`AGSBoard`** owns the level's tile map and an orthographic camera. It converts cells to world
  positions and back, checks bounds, and reads the map's data to decide which cells are walls.
- An **orthographic camera** draws without perspective; **Ortho Width** is the visible width in
  world units. Fitting a map means `Max(MapWidth, MapHeight × Aspect) × Margin`. Yaw −90 looks
  down −Y at Paper2D's X–Z plane.
- A player's view goes through a **view target**; `SetViewTargetWithBlend` switches it. Chapter 7
  makes the board the view target.
- `SetTile` only edits maps a component owns (`CreateNewTileMap`, `MakeTileMapEditable`), and
  needs `RebuildCollision` afterwards.

The project now has a real level: a painted, framed board with gems and an exit on it. Chapter 7
adds the gameplay framework (game mode, player controller, pawn) and puts the snake on the board.

## Exercises

1. **A decoration layer.** Add a `Decor` layer between `Walls` and `Floor` to `TM_Level01`, and
   paint a few extra floor tiles on it (cracks, moss: any of tiles 1–3). Check with the inspector
   example that the board's walls did not change.

<details><summary>Solution</summary>

In the Tile Map Editor, select `Floor` and click **Add New Layer**. The new layer is inserted above
the selected one, so the order becomes `Walls` (0), the new layer (1), `Floor` (2). Rename it
`Decor` and **untick Layer Collides**, since new layers collide by default. Paint on it.

Draw order follows from the depth: `Walls` at Y = 0, `Decor` at Y = −4, `Floor` at Y = −8. Added
above `Walls` instead, decor would draw over the walls.

The inspector now prints `3 layers`, then `layer 1 'Decor' collides=false painted=<m>` between
the `Walls` and `Floor` lines, and the `Walls` count is unchanged.

`AGSBoard` needs no change. `IsWallTile` skips layers that do not collide, and `CellToWorld` still
uses layer 0, the `Walls` plane at Y = 0. With Layer Collides left on, every decor cell would turn
into a wall.

</details>

2. **WorldToCell by hand.** Write `FIntPoint AGSBoard::WorldToCellManual(const FVector&
   WorldLocation) const` that computes the cell without `GetTileCoordinatesFromLocalSpacePosition`,
   using only the cell size and the centre of tile (0, 0). Make it correct for points left of and
   above the map, where the coordinates are negative.

<details><summary>Solution</summary>

Declare it in `GSBoard.h` next to `WorldToCell`, then:

```cpp
FIntPoint AGSBoard::WorldToCellManual(const FVector& WorldLocation) const
{
	const float Cell = GetCellSize();
	if (Cell <= 0.0f)
	{
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}

	const FVector Local = TileMapComponent->GetComponentTransform().InverseTransformPosition(WorldLocation);
	// Tile (0, 0) is centred on Origin, so its left and top edges are half a cell away.
	const FVector Origin = TileMapComponent->GetTileCenterPosition(0, 0, 0, /*bWorldSpace=*/false);
	const double Column = (Local.X - Origin.X) / Cell + 0.5;
	const double Row = -(Local.Z - Origin.Z) / Cell + 0.5;   // rows grow towards -Z
	return FIntPoint(FMath::FloorToInt32(Column), FMath::FloorToInt32(Row));
}
```

The `+ 0.5` moves from "measured from the centre of tile (0, 0)" to "measured from its left (or
top) edge", in tiles. Row is negated because tile rows grow down while Z grows up.

The important choice is `FloorToInt32` instead of a cast. Converting a floating-point number to
`int32` with a cast **truncates**: it drops the fraction, rounding towards zero. Take a point 10
units left of the centre of tile (0, 0), at local X = −10. That is outside the map, whose left edge
is at −8:

```text
Column = -10 / 16 + 0.5 = -0.125
(int32)-0.125            =  0    wrong: claims column 0, inside the map
FMath::FloorToInt32(-0.125) = -1  right: one column left of the map
```

For positive numbers truncating and flooring agree, which is why the bug hides until something
wanders off the left or top edge. Paper2D's own function floors, so `WorldToCell` and
`WorldToCellManual` should agree everywhere. `FVector` components are `double` in UE5, which is why
`Column` and `Row` are `double`; `FloorToInt32` has overloads for both `float` and `double`.

</details>

3. **Pixel-perfect framing.** Pixel art looks best when every art pixel covers the same whole
   number of screen pixels. Change the board so that, when play starts, it picks the largest whole
   scale at which the map still fits the game viewport, and sets the ortho width to match.

<details><summary>Solution</summary>

With 1 unit per pixel, an ortho width of `W` on a viewport `V` screen pixels wide shows `V / W`
screen pixels per art pixel. For a whole number `Scale`, set `W = V / Scale`. The largest scale at
which the map fits is the smaller of the two whole ratios, and never less than 1.

In `GSBoard.h`, declare `void MakePixelPerfect();` in the private section. In `GSBoard.cpp`, add
`#include "Engine/GameViewportClient.h"` and:

```cpp
void AGSBoard::MakePixelPerfect()
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!TileMap || !Viewport)
	{
		return;
	}

	FVector2D ViewportSize;
	Viewport->GetViewportSize(ViewportSize);
	if (ViewportSize.X <= 0.0 || ViewportSize.Y <= 0.0)
	{
		return;
	}

	const float UnitsPerPixel = TileMap->GetUnrealUnitsPerPixel();
	const double MapWidth = TileMap->MapWidth * TileMap->TileWidth * UnitsPerPixel;
	const double MapHeight = TileMap->MapHeight * TileMap->TileHeight * UnitsPerPixel;

	// Whole screen pixels per art pixel, as large as possible with the whole map visible.
	const int32 Scale = FMath::Max(1, FMath::FloorToInt32(FMath::Min(ViewportSize.X / MapWidth, ViewportSize.Y / MapHeight)));
	const float NewOrthoWidth = static_cast<float>(ViewportSize.X / Scale);
	Camera->SetOrthoWidth(NewOrthoWidth);
	UE_LOG(LogGridSnake, Log, TEXT("Pixel-perfect scale %d, ortho width %.0f"), Scale, NewOrthoWidth);
}
```

Call `MakePixelPerfect()` at the end of `BeginPlay`. On a 1920 × 1080 viewport the ratios are
1920 / 384 = 5 and 1080 / 224 ≈ 4.8, so the scale is 4 and the ortho width 480: the view shows
480 × 270 units, and every art pixel is 4 × 4 screen pixels.

```text
LogGridSnake: Pixel-perfect scale 4, ortho width 480
```

Notes:

- `GetWorld()->GetGameViewport()` returns the viewport of the world's game instance, which is the
  PIE window in the editor. `GEngine->GameViewport` also works in a single-player game, but with
  several PIE windows it is only one of them.
- The PIE viewport size is whatever size the editor gives it, so the scale differs between the
  editor and a full-screen game. That is expected.
- `FVector2D` holds `double`s in UE5, so the division is done in `double` and then explicitly
  narrowed to the `float` that `SetOrthoWidth` takes.
- This assumes the default *Maintain X-Axis FOV* constraint, and does not react to the window
  being resized.

</details>

4. **A second level.** Make `TM_Level02`, a bigger map of 32 × 18 tiles, and a level `L_Level02`
   whose board uses it. Work out the ortho width the board will pick before you look.

<details><summary>Solution</summary>

1. In the Content Browser, select `TM_Level01` and press **Ctrl+D** (duplicate). Name the copy
   `TM_Level02`.
2. Open it and set **Map Width** = 32 and **Map Height** = 18. Every layer is resized with the map.
   Repaint: fill the new floor cells, move the border walls to the new edges, and add new inner
   walls.
3. Duplicate `L_Level01` as `L_Level02` (in `Maps`) and open it. Select the board's **TileMap**
   component and set **Tile Map** = `TM_Level02`.
4. Move the gems and the exit onto free cells of the new layout.

The ortho width: the map is 32 × 16 = 512 units wide and 18 × 16 = 288 high. 288 × 16/9 = 512, so
both sides fit exactly at 16:9, and with the margin the width is 512 × 1.1 = 563.2. The camera
sits at the midpoint of tiles (0, 0) and (31, 17): (248, 500, −136). Play `L_Level02` (the actor
name is illustrative):

```text
LogGridSnake: Board GSBoard_0: 32x18 cells of 16 units
```

Nothing in the C++ changed: the board reads everything from the map. Chapter 13 makes this the
second level of the game, with level data and level switching.

</details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-6--tile-maps-and-the-orthographic-camera)
