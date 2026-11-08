# Chapter 5 assets: Actors and components

Carried over: everything in `../ch04/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Blueprints/BP_GSGem` | Blueprint Class | `GSGem` | Component **Flipbook** > Source Flipbook = `FB_Gem_Idle` |
| `/Game/GridSnake/Blueprints/BP_GSExit` | Blueprint Class | `GSExit` | Class Defaults > Exit: Closed Flipbook = `FB_Exit_Closed`, Open Flipbook = `FB_Exit_Open` |

## Editor steps

1. Build and restart the editor (or use Live Coding). The C++ classes `GSGem` and
   `GSExit` are in **C++ Classes/GridSnake/Gameplay**.
2. Right-click `GSGem` > **Create Blueprint class based on GSGem**.
   - Name it `BP_GSGem`, in `GridSnake/Blueprints`.
   - In the Blueprint editor's **Components** panel, select **Flipbook**.
   - In Details > Sprite, set **Source Flipbook** = `FB_Gem_Idle`. Compile and save.
3. Create `BP_GSExit` from `GSExit` the same way.
   - In **Class Defaults > Exit**, set **Closed Flipbook** = `FB_Exit_Closed` and
     **Open Flipbook** = `FB_Exit_Open`.
4. Drag three `BP_GSGem` and one `BP_GSExit` into `L_Level01`.
   - Put them at Y = 0, with X and Z multiples of 16 (`(16, 0, -16)`, `(48, 0, -32)`, ...).
   - Play: the gems bob. Change **Bob Height** or **Bob Speed** on one gem to see the difference.
   - Select the exit and toggle **Is Open** in Details: the construction script
     swaps the flipbook in the editor.
