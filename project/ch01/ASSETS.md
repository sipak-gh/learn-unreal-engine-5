# Chapter 1 assets: Hello, Unreal

Unreal assets (`.uasset`, `.umap`) are binary, so this snapshot ships only the text
files. Create the assets below in the editor. Use exactly these names and folders:
later chapters and the config files refer to them by path. Menu names are for
Unreal Engine 5.8. If a label reads slightly differently on your machine, look
for the closest match.

| Path | Type | Created from | Key settings |
|---|---|---|---|
| `GridSnake.uproject` | Project | Project Browser > **Games** > **Blank** | **Blueprint**, Target Platform **Desktop**, Quality **Maximum**, **Starter Content off**, Raytracing off |
| `/Game/GridSnake/Maps/L_Level01` | Level | **File > New Level > Empty Level** | Saved as `L_Level01` |

## Editor steps

1. In the Epic Games Launcher, open **Unreal Engine > Library** and launch **5.8**.
2. In the Project Browser choose **Games > Blank**.
   - Set the project type to **Blueprint**. Chapter 2 adds C++.
   - Turn **Starter Content** off.
   - Name the project `GridSnake` and click **Create**.
3. **Edit > Plugins**: search for `Paper2D` and make sure it is **enabled**. It is
   enabled by default; this writes the `Paper2D` entry into `GridSnake.uproject`.
4. In the **Content Browser**, create the folder `Content/GridSnake/Maps`.
5. **File > New Level > Empty Level**. Then **File > Save Current Level As...** into
   `GridSnake/Maps` as `L_Level01`.
6. **Edit > Project Settings > Project > Maps & Modes**: set both **Editor Startup
   Map** and **Game Default Map** to `L_Level01`. The editor writes these into
   `Config/DefaultEngine.ini`; compare yours with this snapshot.
7. Click **Play**. The viewport is black: an empty level has nothing in it. That is
   expected.
