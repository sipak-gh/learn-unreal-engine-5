# Chapter 18 assets: Saving progress

Carried over: everything in `../ch17/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `W_MainMenu` (changed) | Widget Blueprint | `GSMainMenuWidget` | New **ContinueButton** "Continue", first button in `MainPanel`, above `PlayButton` |
| `W_Options` (changed) | Widget Blueprint | `GSOptionsWidget` | New **ResetProgressButton** "Reset Progress", above `BackButton` |
| `W_LevelButton` (changed) | Widget Blueprint | `GSLevelButtonWidget` | Add **DetailText** if you skipped it in chapter 16 |

## Editor steps

1. Build. The save object needs no asset: `UGSSaveGame` is created from C++.
2. Add the two buttons. The widget Blueprints will not compile until they exist,
   because they are `BindWidget`.
3. Play from `L_MainMenu`:
   - Only level 1 is unlocked in Select Level.
   - Finish level 1 and return to the menu. Continue appears, level 2 unlocks and
     level 1 shows `Best 300`.
4. The save file is in the project's `Saved/SaveGames/GridSnake.sav` (PIE and
   Standalone from the editor). Packaged games write to a per-user folder; on
   Windows, `%LOCALAPPDATA%/GridSnake/Saved/SaveGames/`.
5. **Options > Reset Progress** deletes the progress and relocks the levels.
   Volumes are kept.
