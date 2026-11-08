# Chapter 10 assets: Collision in 2D

Carried over: everything in `../ch09/ASSETS.md`. No new assets; one project setting
and a check of the tile set:

1. **Project Settings > Engine > Collision > Trace Channels > New Trace Channel...**
   - Name `Grid`, **Default Response** = **Ignore**. Accept.
   - Ignore makes the channel opt-in: only components that explicitly block Grid
     (the board's tile map and the closed exit's `Blocker`, both set in C++) are walls.
   - Compare the `+DefaultChannelResponses` line in `Config/DefaultEngine.ini`.
   - It must be `ECC_GameTraceChannel1`, which `GridSnake.h` calls `ECC_GSGrid`.
     If you created other custom channels earlier, the number differs: fix the
     `#define`.
2. Check the wall tiles:
   - Open `TS_Dungeon` and confirm that tiles 4-7 have a collision box. In the
     tile map editor, **Show > Collision** draws it.
   - Open `TM_Level01`: the `Walls` layer has **Layer Collides** ticked.
   - Optionally raise **Collision Thickness** (Details > Collision) to 50 or more.
3. Play.
   - Driving into a wall or into the snake's own body logs
     `LogGridSnake: Warning: Snake hit a wall at cell X=5 Y=1 (length 3)`
     (or `bit itself`) and the snake stops. The cell is the head's cell, next to
     the wall.
   - Driving over a gem logs `Snake entered BP_GSGem_C_... at cell ...`.
   - The closed exit counts as a wall.
4. Visualise collision while playing: run the console command `show collision`.
