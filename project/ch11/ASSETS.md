# Chapter 11 assets: Interfaces, delegates and the rules of the game

Carried over: everything in `../ch10/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `BP_GSGameMode` (changed) | Blueprint Class | `GSGameMode` | Rules > Growth Per Gem = 2, Level End Delay = 1.5 s (defaults) |
| `L_Level01` | Level | - | Exactly three `BP_GSGem` and one `BP_GSExit` (Is Open **off**) |

## Editor steps

1. Build. Nothing to set except checking the defaults above.
2. Make sure `L_Level01` has **exactly three** gems; the count comes from
   **Project Settings > Game > GridSnake > Gems Per Level**.
3. Play.
   - Eating a gem grows the snake by two segments.
   - After the third gem, the exit logs `BP_GSExit_C_... is now open`.
   - Entering the open exit finishes the level. The game state's phase becomes
     `Completed`, and in chapter 11 the level restarts after **Level End
     Delay**.
   - Dying restarts the level after the same delay.
