# Chapter 3 assets: Objects, reflection and settings

Carried over: everything in `../ch02/ASSETS.md`. This chapter adds **no new assets**.
It only changes settings:

1. Build and restart the editor. Two new C++ classes appear: `GSSettings` and
   `GSGameInstance`.
2. **Project Settings > Project > Maps & Modes > Game Instance Class** = `GSGameInstance`.
3. **Project Settings > Game > GridSnake** now shows **Rules**: Gems Per Level,
   Default Step Interval and Points Per Gem.
   - Change any value and the editor writes the `[/Script/GridSnake.GSSettings]`
     section of `Config/DefaultGame.ini`.
4. Play, open the console (the `` ` `` key) and run `GSDumpClass GSGameMode`.
   - Each reflected property and function is printed in the Output Log.
