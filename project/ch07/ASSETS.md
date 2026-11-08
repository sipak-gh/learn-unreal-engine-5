# Chapter 7 assets: The Gameplay Framework

Carried over: everything in `../ch06/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Blueprints/BP_GSSnake` | Blueprint Class | `GSSnake` | Component **Head** > Source Flipbook = `FB_SnakeHead_Idle` |
| `BP_GSGameMode` (changed) | Blueprint Class | `GSGameMode` | Classes > **Default Pawn Class** = `BP_GSSnake`. Game State Class and Player Controller Class keep the C++ defaults, `GSGameState` and `GSPlayerController` |
| `L_Level01` > `Player Start` | Placed actor | **Place Actors > Basic > Player Start** | On a free cell, e.g. cell (12, 7) = location (192, 0, -112); rotation ignored |

## Editor steps

1. Build. Right-click the C++ class `GSSnake` > **Create Blueprint class based on
   GSSnake**. Name it `BP_GSSnake`.
   - In the Components panel, select **Head** and set **Source Flipbook** =
     `FB_SnakeHead_Idle`.
2. Open `BP_GSGameMode`. In Class Defaults > **Classes**:
   - Set **Default Pawn Class** = `BP_GSSnake`.
   - Check that **Game State Class** shows `GSGameState` and **Player Controller
     Class** shows `GSPlayerController`. They come from the C++ constructor.
3. Add a **Player Start** to `L_Level01`.
   - Put it on a free cell centre. Cell (x, y) is centred at (16x, 0, -16y).
   - Leave room behind it, to the left for a snake starting Right.
4. Select the placed `GSBoard` and set **Level > Start Direction** = Right.
5. Play.
   - The view is the board's camera, set by `GSPlayerController`.
   - The snake's head appears on the Player Start's cell.
   - Use the **Outliner** while playing to see the spawned
     `GSGameState`, `GSPlayerController` and `BP_GSSnake_C`.
