# Chapter 19 assets: Debugging and profiling

Carried over: everything in `../ch18/ASSETS.md`. This chapter adds **no assets**.
Everything is C++ and editor tools.

## Editor steps

1. Build. Then play a level and open the console with the `` ` `` key. The cheat
   manager only exists in non-Shipping builds.

   | Command | What it does |
   |---|---|
   | `GS.Debug.Grid 1` | Draws every cell: red = blocked, green = free (`0` turns it off) |
   | `GSGrow 10` | Adds 10 segments |
   | `GSCollectAll` | Eats every gem; the exit opens |
   | `GSWin` | Finishes the level |
   | `GSUnlockAll` | Unlocks every level and saves |
   | `stat GridSnake` | Shows the `Snake Step` cycle counter |
   | `stat unit`, `stat fps` | Frame, game, draw and GPU times |
   | `Log LogGridSnake Verbose` | Shows the Verbose lines of the project's log category |
   | `show collision` | Draws collision shapes |

2. **Visual Logger**:
   - **Tools > Debug > Visual Logger**, then click **Start** (record).
   - Play, die, stop.
   - Scrub the timeline: every step left a yellow marker labelled with the
     direction, and the death is a red entry.
3. **Unreal Insights**:
   - Start a trace from the editor's status bar: **Trace > Start Trace**, with the
     **CPU** channel on. Or launch Standalone with `-trace=cpu,frame,log`.
   - Play for a few seconds, stop the trace.
   - Open it in **Tools > Unreal Insights**. In **Timing Insights**, search the
     timeline for `AGSSnake::Step`.
4. Expected death line with structured logging, as an example:
   `LogGridSnake: Warning: Snake hit a wall at cell (5, 1), length 3`
