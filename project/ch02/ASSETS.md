# Chapter 2 assets: Projects, modules and the build

Carried over: everything in `../ch01/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `Source/GridSnake/...` | C++ module | **Tools > New C++ Class...** | Creates the module, targets and `.Build.cs` (replaced by this snapshot's files) |
| `/Game/GridSnake/Blueprints/BP_GSGameMode` | Blueprint Class | `GSGameMode` (C++) | No changes yet |

## Editor steps

1. **Tools > New C++ Class... > Game Mode Base**.
   - Name it `GSGameMode`.
   - Set the path to `Source/GridSnake/Core/`.
   - The editor generates the module, both `Target.cs` files and the project
     files, then compiles.
2. Close the editor. Replace the generated sources with this snapshot's `Source/`
   folder. Then regenerate project files:
   - Windows: right-click `GridSnake.uproject` > **Generate Visual Studio project files**.
   - Other platforms: see the chapter.
3. Build the **GridSnakeEditor** target (Development Editor) from your IDE and start
   the editor from there.
4. In the Content Browser, create `Content/GridSnake/Blueprints`.
   - Right-click > **Blueprint Class**, expand **All Classes**, pick `GSGameMode`.
   - Name it `BP_GSGameMode`.
5. **Project Settings > Project > Maps & Modes > Default GameMode** = `BP_GSGameMode`.
6. Play. Expected Output Log line (filter by `LogGridSnake`):
   `LogGridSnake: GridSnake game mode started on map L_Level01`
   PIE may print the map name with a `UEDPIE_0_` prefix.
