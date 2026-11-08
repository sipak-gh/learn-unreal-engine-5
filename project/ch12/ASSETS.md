# Chapter 12 assets: Flipbook animation driven from C++

Carried over: everything in `../ch11/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Blueprints/BP_GSGemPop` | Blueprint Class | `GSFlipbookEffect` | Component **Flipbook** > Source Flipbook = `FB_Gem_Pop` |
| `BP_GSSnake` (changed) | Blueprint Class | `GSSnake` | Animation: Idle `FB_SnakeHead_Idle`, Move `FB_SnakeHead_Move`, Eat `FB_SnakeHead_Eat`, Dead `FB_SnakeHead_Dead`; Body Head Color white, Body Tail Color (0.35, 0.55, 0.35); Death Blinks 4 |
| `BP_GSGem` (changed) | Blueprint Class | `GSGem` | Gem > Pickup Effect Class = `BP_GSGemPop` |
| `BP_GSExit` (changed) | Blueprint Class | `GSExit` | Exit > Opening Flipbook = `FB_Exit_Opening` |

## Editor steps

1. Build.
2. Create `BP_GSGemPop` from `GSFlipbookEffect`.
   - Select **Flipbook** and set **Source Flipbook** = `FB_Gem_Pop`.
   - Looping is switched off in C++: the actor destroys itself when the flipbook
     finishes.
3. In `BP_GSSnake` > Class Defaults > **Animation**:
   - Assign the four head flipbooks.
   - Leave the body colours and Death Blinks at their defaults, or tune them.
4. In `BP_GSGem`, set **Pickup Effect Class** = `BP_GSGemPop`.
5. In `BP_GSExit`, set **Opening Flipbook** = `FB_Exit_Opening`.
6. Open `FB_SnakeHead_Eat` and `FB_SnakeHead_Dead`. The code plays them once, so
   their length is what you see. 4 frames at 16 fps is 0.25 s.
7. Play.
   - The head animates and turns with the snake.
   - The body fades from head to tail colour.
   - Gems pop.
   - The exit plays its opening animation, then loops the open one.
   - On death the body blinks.
