# Chapter 20 assets: Shipping GridSnake, then making your own game

Carried over: everything in `../ch19/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `W_MainMenu` (changed) | Widget Blueprint | `GSMainMenuWidget` | New optional **VersionText** (Text), anchored bottom-right, font size 14 |

## Editor steps

1. **Project Settings > Project > Description > Project Version** = `1.0.0`. Also
   fill in **Project Displayed Title** if you like.
2. **Project Settings > Project > Packaging**:
   - **Build Configuration**: *Development* for test builds (keeps the console,
     logs and cheats), *Shipping* for the build you hand out.
   - **List of maps to include in a packaged build** (advanced): `L_MainMenu`,
     `L_Level01`, `L_Level02` and `L_Level03`.
   - **Additional Asset Directories to Cook**: `/Game/GridSnake/Data`,
     `/Game/GridSnake/Audio` and `/Game/GridSnake/UI`.
     Assets reached only through config soft references would otherwise be left
     out.
   - **Use Pak File** on, **Use Io Store** on.
   - Compare the `[/Script/UnrealEd.ProjectPackagingSettings]` section of
     `DefaultGame.ini` with this snapshot.
3. Optional: **Project Settings > Platforms > Windows > Game Icon** and **Splash**.
4. Package:
   - Main toolbar **Platforms** menu > *your platform* > **Package Project**.
   - Pick an output folder outside the project.
   - Windows packaging needs the Windows SDK; Linux needs the clang toolchain.
     See the chapter.
5. Run the packaged game:
   - It starts in the main menu, shows `v1.0.0` in the corner, plays all three
     levels, and keeps progress between runs.
   - In a Shipping build the console and the `GS*` cheats are gone.
