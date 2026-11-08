# Chapter 4 assets: Pixel art with Paper2D

Carried over: everything in `../ch03/ASSETS.md`.

First run `python Tools/make_placeholder_art.py` from the project folder. It writes
`RawAssets/Textures/*.png` and `RawAssets/Audio/*.wav`. The `.wav` files are used in
chapter 17. If you have your own 16x16 art, use the same layout: one row of frames
per strip, each frame 16x16 px.

### Textures and sprites

| Path | Type | Source | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Art/Textures/T_Tiles` | Texture2D | `T_Tiles.png` (64x32: 4x2 tiles) | Paper2D texture settings (below) |
| `/Game/GridSnake/Art/Textures/T_SnakeHead_Idle` | Texture2D | 32x16 strip, 2 frames | " |
| `/Game/GridSnake/Art/Textures/T_SnakeHead_Move` | Texture2D | 32x16, 2 frames | " |
| `/Game/GridSnake/Art/Textures/T_SnakeHead_Eat` | Texture2D | 64x16, 4 frames | " |
| `/Game/GridSnake/Art/Textures/T_SnakeHead_Dead` | Texture2D | 48x16, 3 frames | " |
| `/Game/GridSnake/Art/Textures/T_SnakeBody` | Texture2D | 16x16, 1 frame | " |
| `/Game/GridSnake/Art/Textures/T_Gem_Idle` | Texture2D | 96x16, 6 frames | " |
| `/Game/GridSnake/Art/Textures/T_Gem_Pop` | Texture2D | 80x16, 5 frames | " |
| `/Game/GridSnake/Art/Textures/T_Exit_Closed` | Texture2D | 16x16, 1 frame | " |
| `/Game/GridSnake/Art/Textures/T_Exit_Opening` | Texture2D | 64x16, 4 frames | " |
| `/Game/GridSnake/Art/Textures/T_Exit_Open` | Texture2D | 64x16, 4 frames | " |
| `/Game/GridSnake/Art/Sprites/S_<Name>_<n>` | Paper Sprite | **Sprite Actions > Extract Sprites** on each strip except `T_Tiles` | Grid mode, cell 16x16; pivot **Center Center** |
| `/Game/GridSnake/Art/Flipbooks/FB_<Name>` | Paper Flipbook | **Create Flipbook** from the sprites of one strip | Frames Per Second: Idle 4, Move 8, Eat 16, Dead 8, Gem_Idle 8, Gem_Pop 16, Exit_Opening 10, Exit_Open 8 |

The flipbooks are `FB_SnakeHead_Idle`, `FB_SnakeHead_Move`, `FB_SnakeHead_Eat`,
`FB_SnakeHead_Dead`, `FB_Gem_Idle`, `FB_Gem_Pop`, `FB_Exit_Closed`,
`FB_Exit_Opening` and `FB_Exit_Open`. `FB_Exit_Closed` has one frame. `T_SnakeBody`
gives the single sprite `S_SnakeBody`; it needs no flipbook.

## Editor steps

1. Set the Paper2D project defaults:
   **Project Settings > Editor > Paper2D - Import**.
   - **Default Pixels per Unreal Unit** = `1`. One pixel is one world unit, so a
     tile is 16 units.
   - **Default Sprite Texture Group** = `2D Pixels (unfiltered)`.
   - **Default Material** for new sprites = `MaskedUnlitSpriteMaterial`. Masked
     unlit materials avoid sorting problems between translucent sprites.
2. Rendering: **Project Settings > Engine > Rendering**. Change the options this
   snapshot's `DefaultEngine.ini` lists under `ch04`:
   - Turn off Lumen GI and reflections, distance fields and virtual shadow maps.
   - Turn off auto exposure, bloom and motion blur.
   - Set **Anti-Aliasing Method** to **None**.

   Restart when asked.
3. Create `Content/GridSnake/Art/{Textures,Sprites,Flipbooks,Tiles}`. Drag the
   PNGs into `Art/Textures`.
4. Select all the textures, right-click > **Sprite Actions > Apply Paper2D Texture
   Settings**. This sets:
   - **Filter** Nearest.
   - **Compression** UserInterface2D (RGBA).
   - **Texture Group** 2D Pixels (unfiltered).
   - **No mipmaps**.
5. For each strip except `T_Tiles`:
   - Right-click > **Sprite Actions > Extract Sprites**.
   - **Sprite Extract Mode: Grid**, **Cell Width/Height** 16, margins 0.
   - Name the sprites `S_<TextureNameWithoutT_>_<n>`, for example `S_Gem_Idle_0`
     ... `S_Gem_Idle_5`. The editor proposes `<Texture>_Sprite_<n>` names; rename
     them, or use the naming template in the dialog if your version shows one.
   - Move the sprites to `Art/Sprites`.
   - Open one sprite and check **Pivot Mode** = **Center Center**. It is the default.
6. Select the sprites of one strip in order, right-click > **Create Flipbook**,
   name the result `FB_<Name>` and move it to `Art/Flipbooks`. Open it and set
   **Frames Per Second** as in the table.
7. Optional check: drag `FB_Gem_Idle` into `L_Level01` and look at it in the
   **Left** or **Right** orthographic viewport (in UE 5.8, Front and Back look along
   X at the Y-Z plane, where a sprite is edge-on). Pick the view in which the gizmo
   shows red (X) across and blue (Z) up. Delete it afterwards. Chapter 6 adds a
   proper camera.
