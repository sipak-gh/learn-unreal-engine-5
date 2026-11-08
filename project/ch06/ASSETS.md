# Chapter 6 assets: Tile sets, tile maps and the orthographic camera

Carried over: everything in `../ch05/ASSETS.md`.

| Path | Type | Created from | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Art/Tiles/TS_Dungeon` | Paper Tile Set | `T_Tiles` > right-click > **Sprite Actions > Create Tile Set** | Tile Size 16x16, margins/spacing 0; collision box on tiles 4-7 |
| `/Game/GridSnake/Art/Tiles/TM_Level01` | Paper Tile Map | Content Browser > **Add > Paper2D > Tile Map** | Map 24x14 tiles, Tile 16x16, Projection **Orthogonal**, layers `Walls` (top) and `Floor` |
| `L_Level01` > `GSBoard` | Placed actor | C++ class `GSBoard` dragged into the level | Tile Map component > Tile Map = `TM_Level01`, location (0, 0, 0) |

## Editor steps

1. Create the tile set:
   - Right-click `T_Tiles` > **Sprite Actions > Create Tile Set**.
   - Name it `TS_Dungeon` and move it to `Art/Tiles`.
   - Open it and check **Tile Size** = 16 x 16. The editor shows 8 tiles:
     row 0 is floor (indices 0-3), row 1 is wall (indices 4-7).
2. Give the wall tiles collision:
   - Select tile 4.
   - In the **Single Tile Editor** toolbar, click **Add Box**. It fills the tile.
   - Repeat for tiles 5, 6 and 7. Save.
3. Create the tile map:
   - **Add > Paper2D > Tile Map**, name it `TM_Level01`, in `Art/Tiles`.
   - Open it. In Details > Setup set:
     - **Map Width** 24, **Map Height** 14.
     - **Tile Width/Height** 16.
     - **Projection Mode** Orthogonal.
   - Keep the default **Pixels Per Unreal Unit** (1) and **Separation Per Layer** (4).
4. Set up the layers:
   - In the **Layers** panel, rename the default layer `Floor`.
   - With `Floor` selected, **untick Layer Collides** in Details. New layers
     collide by default, and `AGSBoard::IsWallTile` treats every painted cell of a
     colliding layer as a wall, so a colliding `Floor` turns the whole map into wall.
   - Add a layer named `Walls`. **Add New Layer** inserts it above the selected
     layer; the layer at the top of the list is drawn in front.
   - With `Walls` selected, check that **Layer Collides** is ticked.
5. Paint:
   - Select `Floor`, pick floor tiles from `TS_Dungeon` in the **Tile Set** panel
     and fill the whole map (the **Fill** tool).
   - Select `Walls` and paint a border of wall tiles around the edge, plus a few
     inner walls.
   - Keep cell (12, 7) and the cells left of it free for the snake's start.
   - Keep three free cells for gems and one free cell on the border for the exit.
6. In `L_Level01`:
   - Delete the test sprites from chapter 4.
   - From **C++ Classes/GridSnake/Gameplay**, drag `GSBoard` into the level and set
     its location to (0, 0, 0).
   - Select its **TileMap** component and set **Tile Map** = `TM_Level01`.
   - The camera frames the map as soon as you do this, through `OnConstruction`.
     Check in the Details panel: **Camera** > Ortho Width.
7. Move the gems and the exit onto free floor cells, by hand: nothing snaps them
   yet (chapter 10 adds `SnapActorToCell`). Cell (x, y) is centred at
   (16x, 0, -16y), so with the viewport's grid snap at 16 units every snapped
   position is a cell centre.
8. Preview the framing:
   - Right-click the board in the Outliner > **Pilot 'GSBoard'**. While piloting,
     the viewport looks through the board's camera component.
   - Click **Eject** (the icon at the viewport's top left) to stop piloting.
   - Pressing Play still shows the default pawn's view. Chapter 7 makes the board
     the view target from the player controller.
