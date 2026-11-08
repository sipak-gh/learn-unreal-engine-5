# Chapter 14 assets: Level transitions and persistence

Carried over: everything in `../ch13/ASSETS.md`. This chapter adds **no new assets**:

- The fade uses the player camera manager.
- The loading screen is a Slate widget built in C++ (`GSLevelFlowSubsystem.cpp`), so
  it works before any UMG asset could load.
- The run state lives in a game instance subsystem, created automatically.

## Editor steps

1. Build. `Build.cs` now depends on `MoviePlayer`, `Slate` and `SlateCore`. Close the
   editor and build from the IDE if Live Coding refuses the module change.
2. Play `L_Level01` in the viewport (PIE).
   - Finishing the level fades to black, opens the next map and fades back in.
   - The run score carries over: `Level L_Level02 started (index 1, run score 300)`.
3. To see the loading screen, run the game as a separate process:
   - **Play > Standalone Game** (in the Play button's drop-down).
   - Or launch with `UnrealEditor GridSnake.uproject -game`.
   - The MoviePlayer loading screen is not shown in PIE.
   - The maps are tiny, so it shows for its minimum display time (1 s in this
     project's code) and then disappears.
4. Expected Output Log lines when a level finishes:

   ```
   LogGridSnake: Level 0 complete: +300, run score 300
   LogGridSnake: Opening /Game/GridSnake/Maps/L_Level02.L_Level02
   LogGridSnake: Level L_Level02 started (index 1, run score 300)
   ```
