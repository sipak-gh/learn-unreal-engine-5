# Chapter 9 assets: Ticks, timers and grid movement

Carried over: everything in `../ch08/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `BP_GSSnake` (changed) | Blueprint Class | `GSSnake` | Snake > **Body Sprite** = `S_SnakeBody`; Initial Length 3; Max Queued Turns 2 |
| `BP_GSGameMode` (changed) | Blueprint Class | `GSGameMode` | Rules > Countdown Seconds = 3 (the default) |

## Editor steps

1. Open `TM_Level01`, select the `Floor` layer and make sure **Layer Collides**
   is **off** (only `Walls` collides). `AGSBoard::IsWallTile` treats every tile
   on a colliding layer as a wall, so a colliding `Floor` kills the snake on its
   first step.
2. Build. Open `BP_GSSnake`, **Class Defaults > Snake**:
   - Set **Body Sprite** = `S_SnakeBody`.
   - Leave Initial Length 3 and Max Queued Turns 2.
3. Play. Expected Output Log lines:

   ```
   LogGridSnake: Countdown: 3
   LogGridSnake: Countdown: 2
   LogGridSnake: Countdown: 1
   LogGridSnake: Go!
   ```

   After "Go!" the snake moves one cell every 0.15 s. That is **Project Settings >
   Game > GridSnake > Default Step Interval**.
4. Steering into a wall tile, or into the snake's own body, logs a warning such
   as `LogGridSnake: Warning: Snake hit a wall at cell X=5 Y=1 (length 3)` and the
   snake stops. Walls come from the tile data (`IsWallTile`); chapter 10 replaces
   that test with a collision query. The snake still drives over gems and the
   exit. Stop PIE with **Esc**.
