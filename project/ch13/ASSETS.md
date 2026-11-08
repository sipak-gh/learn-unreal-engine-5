# Chapter 13 assets: Data-driven levels

Carried over: everything in `../ch12/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Data/DA_LevelList` | Data Asset | `GSLevelList` | Levels[0..2], see below |
| `/Game/GridSnake/Data/C_SpeedUp` | Curve Float | - | Keys (3, 1.0) and (40, 0.5): the step interval halves as the snake grows to 40 segments |
| `/Game/GridSnake/Art/Tiles/TM_Level02`, `TM_Level03` | Paper Tile Map | duplicates of `TM_Level01` | Repainted walls; bigger or tighter layouts |
| `/Game/GridSnake/Maps/L_Level02`, `L_Level03` | Level | duplicates of `L_Level01` | Their `GSBoard` uses `TM_Level02` / `TM_Level03`; 3 gems, 1 exit, 1 Player Start each |

`DA_LevelList` > **Levels**:

| # | Map | Display Name | Step Interval | Points Per Gem | Speed Curve |
|---|---|---|---|---|---|
| 0 | `L_Level01` | First Steps | 0.18 | 100 | none |
| 1 | `L_Level02` | The Maze | 0.15 | 150 | none |
| 2 | `L_Level03` | Long Snake | 0.15 | 200 | `C_SpeedUp` |

## Editor steps

1. Build. Create `Content/GridSnake/Data`.
2. Create the level list:
   - Right-click > **Miscellaneous > Data Asset**, pick `GSLevelList`, name it
     `DA_LevelList`.
   - Add three entries to **Levels** as in the table. **Map** is a level picker:
     choose the `L_Level0N` asset.
3. Create the speed curve:
   - Right-click > **Miscellaneous > Curve**, pick **CurveFloat**, name it
     `C_SpeedUp`.
   - In the curve editor, right-click to add keys at (time 3, value 1.0) and
     (time 40, value 0.5).
   - Time is the snake's length; value multiplies the step interval.
4. Make the levels:
   - Duplicate `TM_Level01` twice (Ctrl+D) as `TM_Level02` and `TM_Level03`, and
     repaint their `Walls` layer.
   - Duplicate `L_Level01` as `L_Level02` and `L_Level03`.
   - In each, select the board and point its **Tile Map** at the new tile map.
   - Move the gems, exit and Player Start onto free cells.
5. **Project Settings > Game > GridSnake > Levels > Level List** = `DA_LevelList`.
   This writes `LevelList=` in `DefaultGame.ini`.
6. Play `L_Level01` and finish it: the game opens `L_Level02`. After `L_Level03`
   it wraps to `L_Level01`. Expected Output Log lines:

   ```
   LogGridSnake: Level L_Level01: list index 0, 100 points per gem
   ...
   LogGridSnake: Level complete with 300 points
   LogGridSnake: Opening level 1: /Game/GridSnake/Maps/L_Level02.L_Level02
   ```
