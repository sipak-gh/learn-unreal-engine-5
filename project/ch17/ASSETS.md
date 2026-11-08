# Chapter 17 assets: Sound and music

Carried over: everything in `../ch16/ASSETS.md`.

| Path | Type | Source / parent | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Audio/SW_Eat`, `SW_Die`, `SW_ExitOpen`, `SW_LevelComplete`, `SW_UIClick` | Sound Wave | `RawAssets/Audio/*.wav` | Sound > **Class** = `SC_SFX` |
| `/Game/GridSnake/Audio/SW_Music` | Sound Wave | `RawAssets/Audio/SW_Music.wav` | **Looping** on; played only through `MSS_Music`, so it needs no class |
| `/Game/GridSnake/Audio/SC_Music` | Sound Class | **Audio > Classes > Sound Class** | Volume 1 |
| `/Game/GridSnake/Audio/SC_SFX` | Sound Class | same | Volume 1 |
| `/Game/GridSnake/Audio/SMix_Volumes` | Sound Mix | **Audio > Mix > Sound Mix** | No adjusters: C++ adds class overrides at runtime |
| `/Game/GridSnake/Audio/MSS_Music` | MetaSound Source | **Audio > MetaSound Source** | Graph below; Sound > Class = `SC_Music` |
| `BP_GSGem` (changed) | Blueprint | `GSGem` | Gem > Pickup Sound = `SW_Eat` |
| `BP_GSExit` (changed) | Blueprint | `GSExit` | Exit > Open Sound = `SW_ExitOpen` |
| `BP_GSSnake` (changed) | Blueprint | `GSSnake` | Audio > Death Sound = `SW_Die` |
| `BP_GSGameMode` (changed) | Blueprint | `GSGameMode` | Audio > Level Complete Sound = `SW_LevelComplete` |
| `W_Options` (changed) | Widget Blueprint | `GSOptionsWidget` | Adds **MusicVolumeSlider** and **SfxVolumeSlider** (Slider, Min 0, Max 1, Step Size 0.05) |
| all menu buttons (changed) | Button style | - | Style > **Pressed Sound** = `SW_UIClick` |

`MSS_Music` graph. Node names as in the 5.8 MetaSound editor's node search; if a
name differs, search for the closest match.

```
Inputs:  Intensity (Float, default 0)

On Play ──► Wave Player (Mono)  Wave Asset = SW_Music, Loop = true
                 │ Out Mono
                 ▼
           One-Pole Low Pass Filter ──► Out Mono (the source's mono output)
                 ▲ Cutoff Frequency
Intensity ──► Map Range (In 0..1 → Out 800..18000)
```

At intensity 0 the music sounds muffled; it opens up as gems are collected. To make
the source mono, set its **Output Format** to **Mono** in the MetaSound's Details.
Wire the Wave Player's **On Finished** to the output **On Finished** only if you
turn looping off.

## Editor steps

1. Create `Content/GridSnake/Audio` and drag in the six `.wav` files from
   `RawAssets/Audio`.
2. Create the sound classes and the mix:
   - Right-click > **Audio > Classes > Sound Class**: `SC_Music` and `SC_SFX`.
   - Right-click > **Audio > Mix > Sound Mix**: `SMix_Volumes`.
3. Open each SFX wave and set **Sound > Class** = `SC_SFX`. With several selected,
   use right-click > **Asset Actions > Edit Selection in Property Matrix**.
4. Create **Audio > MetaSound Source** `MSS_Music`.
   - Add the float input `Intensity`: Members panel > Inputs > **+**.
   - Build the graph above.
   - In the source's Details set **Sound > Class** = `SC_Music`.
   - Press **Play** in the MetaSound editor to test, and drag the Intensity value
     while it plays.
5. **Project Settings > Game > GridSnake > Audio**:
   - Music = `MSS_Music`.
   - Volume Mix = `SMix_Volumes`.
   - Music Class = `SC_Music`.
   - Sfx Class = `SC_SFX`.
6. Assign the sounds in the four Blueprints listed above.
7. In `W_Options`, add a row per slider, named **MusicVolumeSlider** and
   **SfxVolumeSlider**, under the check boxes.
8. Give every menu button's **Style > Pressed Sound** = `SW_UIClick`.
9. Play from `L_MainMenu`.
   - Music starts, and keeps playing through level changes without restarting.
   - It brightens with each gem.
   - Gems, exit, death and level complete have sounds.
   - The sliders change the volumes live.
