# Chapter 4 — Pixel Art with Paper2D

GridSnake is a 2D game, and in Unreal 2D means **Paper2D**: the plugin you enabled in chapter 1.
This chapter introduces its asset types (textures, sprites and flipbooks), the import settings that
keep pixel art crisp, and the engine settings that make a 2D project cheap and clean to render. It
also writes three small C++ classes that use sprites and flipbooks from code. GridSnake gains all of
its art: eleven textures, their sprites and nine flipbooks. A small Python script in the project
generates the art, so there is nothing to download.

Previous: [Chapter 3](03-objects-and-reflection.md). Next: [Chapter 5](05-actors-and-components.md).

## Paper2D in one page

Paper2D draws flat images in Unreal's 3D world. It does not replace the 3D renderer; it adds asset
types and components that produce camera-facing quads, plus editors for them. Everything else
(levels, actors, cameras, collision, Blueprints) works as it does for 3D.

| Asset | Class | What it is | Chapter |
|---|---|---|---|
| Texture | `UTexture2D` | An imported image, such as a PNG. Not Paper2D-specific | 4 |
| Sprite | `UPaperSprite` | A rectangle of a texture, with a pivot, a scale and a material | 4 |
| Flipbook | `UPaperFlipbook` | A sequence of sprites with timing: a frame animation | 4 |
| Tile Set | `UPaperTileSet` | A texture cut into equal tiles, with per-tile collision | 6 |
| Tile Map | `UPaperTileMap` | A grid of tiles from tile sets, in layers | 6 |

Assets are data. To show one in a level you need a component that renders it:

| Component | Renders | Used in GridSnake for |
|---|---|---|
| `UPaperSpriteComponent` | One sprite | Simple static pictures (this chapter's example) |
| `UPaperFlipbookComponent` | A flipbook, animated | The snake's head, gems, the exit (chapter 5) |
| `UPaperGroupedSpriteComponent` | Many sprites in one draw call | The snake's body (chapter 9) |
| `UPaperTileMapComponent` | A tile map | The board (chapter 6) |

When you drag a sprite or flipbook asset into a level, the editor creates a ready-made actor with
the right component: a `PaperSpriteActor` or a `PaperFlipbookActor`. Your own actors (chapter 5)
add the components in C++ instead.

### The X-Z plane

Unreal is a 3D, Z-up engine. Paper2D draws sprites in the **X-Z plane**: **+X is right** and **+Z
is up** in the picture, and **Y is depth**. A sprite has no thickness; seen from above it would be
a line. So a 2D game in Unreal is a flat scene standing upright, filmed by a camera that looks along
the Y axis.

GridSnake is a *top-down* game, but "top-down" describes the art, not the axes. The board still
lies in the X-Z plane, and "up the screen" is +Z. The Y axis is only used to put things in front of
or behind each other. Chapter 6 sets up the camera; until then, you look at sprites in an
orthographic editor view.

### Pixels and units

One Unreal unit is one centimetre. Paper2D converts pixels to units with a **Pixels per unit**
value per sprite. GridSnake uses 1, so a 16x16-pixel tile is 16 units wide, and every position on
the board is a whole number. That makes grid math in chapter 6 simple, and it means a value such as
`X = 24` in the Details panel is exactly one and a half tiles from the origin.

## Generating the art

The ch04 snapshot adds `Tools/make_placeholder_art.py`. It is a standard-library Python script (no
packages to install) that draws GridSnake's pixel art and synthesises its sound effects. Run it from
the project folder, the one with `GridSnake.uproject`:

```console
$ python3 Tools/make_placeholder_art.py
wrote Textures/T_Tiles.png (64x32)
wrote Textures/T_SnakeHead_Idle.png (32x16)
wrote Textures/T_SnakeHead_Move.png (32x16)
wrote Textures/T_SnakeHead_Eat.png (64x16)
wrote Textures/T_SnakeHead_Dead.png (48x16)
wrote Textures/T_SnakeBody.png (16x16)
wrote Textures/T_Gem_Idle.png (96x16)
wrote Textures/T_Gem_Pop.png (80x16)
wrote Textures/T_Exit_Closed.png (16x16)
wrote Textures/T_Exit_Opening.png (64x16)
wrote Textures/T_Exit_Open.png (64x16)
wrote Audio/SW_Eat.wav (0.12s)
wrote Audio/SW_Die.wav (0.60s)
wrote Audio/SW_ExitOpen.wav (0.36s)
wrote Audio/SW_LevelComplete.wav (0.84s)
wrote Audio/SW_UIClick.wav (0.04s)
wrote Audio/SW_Music.wav (8.00s)
```

This is real output, from Python 3.10.12. On Windows, type `python` or `py` instead of `python3`.
The script is deterministic (its random numbers use a fixed seed), so you get the same pixels every
time.

- Every texture except `T_Tiles` is a **strip**: frames of 16x16 pixels side by side, left to
  right. `T_Gem_Idle` is 96 pixels wide, so it has six frames. This is the simplest kind of
  *sprite sheet*.
- `T_Tiles` is a 4x2 grid of tiles: the top row is floor, the bottom row is wall. Chapter 6 turns it
  into a tile set.
- The snake's head is drawn facing right (+X). The game rotates it for the other directions.
- The `.wav` files wait until chapter 17.

The files go to `RawAssets/`, next to `Content/`, not inside it. `Content/` is for imported assets
(`.uasset` files), and the editor may notice new image files in it and offer to import them. Source
files outside `Content/` are imported only when you decide to. The course repository's `.gitignore`
ignores `RawAssets/`, because the script can always recreate it. In your own projects, keep your
artists' source files (`.png`, `.aseprite`, `.psd`) in a folder like this and commit them: the
editor remembers each asset's source path, so **Reimport** can pick up a changed file later.

If you have your own 16x16 art, use the same layout: one row of frames per strip, each frame 16x16,
transparent background. The rest of the chapter works unchanged.

## Importing textures for pixel art

Unreal's texture defaults are made for 3D: photos and painted surfaces seen at every distance and
angle. Pixel art needs the opposite treatment, so the order of these steps matters.

### Paper2D import defaults

Open **Edit > Project Settings > Editor > Paper2D - Import** (the class behind the page is
`UPaperImporterSettings`). Set:

- **Default Pixels per Unreal Unit** = `1`. New sprites get this scale.
- **Default Sprite Texture Group** = **2D Pixels (unfiltered)**. Texture groups decide filtering
  and size limits for whole sets of textures; this one means "no smoothing".
- The default material for new sprites = **MaskedUnlitSpriteMaterial** (the next section explains
  sprite materials). Depending on the setting for picking the best material automatically, the
  editor may choose masked or translucent by looking at each texture's alpha. Masked is what
  GridSnake wants everywhere.

These are editor-side defaults. The editor saves them to `Config/DefaultEditor.ini` as soon as you
change them, in a section of its own (`[/Script/Paper2DEditor.PaperImporterSettings]`). The course
never edits that file by hand, and the snapshot's copy only notes that the section exists. The
settings affect sprites and textures created *after* you set them, so do this first. The exact
labels can differ slightly between engine versions; the page is short, so you will find each
setting.

### Import the PNGs

1. In the **Content Browser**, create the folders `GridSnake/Art/Textures`, `Art/Sprites`,
   `Art/Flipbooks` and `Art/Tiles` (right-click > **New Folder**).
2. Open `Art/Textures` and drag the eleven PNGs from `RawAssets/Textures/` onto it, or click
   **Import** and select them.

Each PNG becomes a `Texture2D` asset with the same name, `T_Gem_Idle` and so on. Double-click one:
it looks blurry. That is the 3D defaults at work.

### Apply the Paper2D texture settings

3. Select all eleven textures, right-click, and choose **Sprite Actions > Apply Paper2D Texture
   Settings**. (The documentation calls this entry **Apply Paper 2D Import Settings**; it is the
   same action.)

It changes each texture's settings as follows:

| Setting (Texture editor) | 3D default | After the Paper2D action | Why |
|---|---|---|---|
| **Filter** | from the texture group (bilinear-like) | **Nearest** | Each screen pixel takes the colour of the nearest texel: hard edges |
| **Mip Gen Settings** | from the texture group | **NoMipmaps** | No smaller, pre-blurred copies |
| **Compression Settings** | Default (DXT/BC) | **UserInterface2D (RGBA)** | Uncompressed colours: block compression smears 16x16 art |
| **Texture Group** | World | the Paper2D default (2D Pixels (unfiltered)) | Group-level filtering stays off |

Open `T_Gem_Idle` again: the pixels are sharp. Check the four settings in the Texture editor's
Details panel, under **Compression**, **Level Of Detail** and **Texture**. A screenshot of the
before and after would show the difference best; try it on one texture by undoing (Ctrl+Z) and
redoing.

Why is the 3D default blurry? Two features, both right for 3D and wrong for pixel art:

- **Bilinear filtering** blends the four nearest texels whenever a screen pixel falls between them.
  On a 16x16 sprite drawn 64 pixels wide, every edge becomes a smooth ramp.
- **Mipmaps** are half-size, quarter-size, ... copies used when the texture is drawn small. They
  are made by averaging pixels, which destroys single-pixel details such as an eye.

Compression adds a third problem: GPU block compression stores 4x4 blocks with a limited palette,
which visibly shifts colours in tiny, high-contrast art.

## Sprites

A **sprite** (`UPaperSprite`) is a rectangle cut from a texture, plus everything Paper2D needs to
draw it: where its pivot is, how many pixels make a unit, which material to draw it with, and
optional collision. The texture holds the pixels; many sprites can share one texture.

### Making one sprite

The quickest way to get a sprite is from a whole texture:

1. Right-click `T_SnakeBody` in the Content Browser.
2. Choose **Sprite Actions > Create Sprite**.

The editor creates `T_SnakeBody_Sprite` next to the texture. Rename it to `S_SnakeBody` (select it
and press F2) and move it to `Art/Sprites` (drag it onto the folder, then choose **Move Here**).
GridSnake uses this sprite for every body segment in chapter 9.

A note on naming: the course uses `T_` for textures, `S_` for sprites and `FB_` for flipbooks.
Epic's own content often uses `SPR_` or no prefix for sprites; any scheme works if it is
consistent. Later chapters' editor steps refer to these exact names (for example
`FB_Gem_Idle` in chapter 5 and `S_SnakeBody` in chapter 9), so keep them.

### Extracting a strip

The other ten textures (apart from `T_Tiles`) are strips of frames. **Extract Sprites** cuts a
texture into many sprites at once:

1. Right-click `T_Gem_Idle` and choose **Sprite Actions > Extract Sprites**. A dialog opens with
   a preview of the texture and the rectangles it is going to cut.
2. Set **Sprite Extract Mode** to **Grid**. (**Auto** looks for islands of non-transparent pixels.
   For art like ours, where frames touch or differ in size, it gives uneven rectangles.)
3. Set **Cell Width** and **Cell Height** to `16`. **Margin X/Y** and **Spacing X/Y** stay at `0`:
   our strips have no border and no gaps between frames. **Num Cells X/Y** follow from the size;
   for `T_Gem_Idle` that is 6 x 1.
4. Set **Naming Template** to `S_Gem_Idle_{0}` and **Naming Start Index** to `0`. The `{0}` is
   replaced by the frame number. The default template is `Sprite_{0}`.
5. Click **Extract**.

You get `S_Gem_Idle_0` to `S_Gem_Idle_5` in the texture's folder. Move them to `Art/Sprites`.
Sprites are numbered left to right, then top to bottom, so `_0` is the leftmost frame.

Repeat for every strip. Here is the full list, with the template for each one:

| Texture | Naming Template | Sprites |
|---|---|---|
| `T_SnakeHead_Idle` | `S_SnakeHead_Idle_{0}` | 2 |
| `T_SnakeHead_Move` | `S_SnakeHead_Move_{0}` | 2 |
| `T_SnakeHead_Eat` | `S_SnakeHead_Eat_{0}` | 4 |
| `T_SnakeHead_Dead` | `S_SnakeHead_Dead_{0}` | 3 |
| `T_Gem_Idle` | `S_Gem_Idle_{0}` | 6 |
| `T_Gem_Pop` | `S_Gem_Pop_{0}` | 5 |
| `T_Exit_Closed` | `S_Exit_Closed_{0}` | 1 |
| `T_Exit_Opening` | `S_Exit_Opening_{0}` | 4 |
| `T_Exit_Open` | `S_Exit_Open_{0}` | 4 |

That is 31 sprites, plus `S_SnakeBody`. Leave `T_Tiles` alone; chapter 6 makes a tile set from it,
and tile sets do not use sprites.

If your engine version's dialog has no naming template, the sprites come out as
`<Texture>_Sprite_<n>` or similar. Rename them by hand. **Asset Actions > Bulk Edit via Property
Matrix** does not rename, but a few F2 presses per strip is quick enough.

### The Sprite Editor

Double-click `S_Gem_Idle_0` to open the **Sprite Editor**. The toolbar switches between four modes:

| Mode | What you edit |
|---|---|
| **View** | Nothing; shows the sprite and its stats |
| **Edit Source Region** | The rectangle of the texture that the sprite uses |
| **Edit Collision** | The sprite's collision shape (GridSnake does not use sprite collision) |
| **Edit RenderGeom** | The polygon actually drawn (it can be tighter than the rectangle to save overdraw) |

The Details panel shows the sprite's properties. These are the ones that matter for GridSnake:

| Property | Value here | Meaning |
|---|---|---|
| **Source Texture** | `T_Gem_Idle` | Where the pixels come from |
| **Source UV** | `(0, 0)` for frame 0, `(16, 0)` for frame 1, ... | Top-left corner of the region, in pixels |
| **Source Dimension** | `(16, 16)` | Size of the region, in pixels |
| **Pixels per unit** | `1` | From the import default you set; 1 pixel = 1 unit |
| **Pivot Mode** | **Center Center** | The point that sits at the actor's location |
| **Default Material** | `MaskedUnlitSpriteMaterial` | How the pixels are drawn (next section) |

The **pivot** decides where the sprite sits relative to its actor. With **Center Center**, an
actor at `(X=16, Z=-16)` draws the sprite from 8 to 24 in X and -24 to -8 in Z: exactly one tile.
That is what GridSnake needs, because the board in chapter 6 places everything at tile centres.
Other presets (**Bottom Center** for a platformer character standing on the ground, and so on) and
**Custom** exist for other games. Check that every sprite you extracted uses **Center Center**; it
is the default.

The toolbar also has a toggle to snap edited regions and pivots to the pixel grid. Leave it on.
A pivot at half a pixel is the classic cause of a sprite that looks shifted by one pixel.

### Sprite materials

Every sprite is drawn with a material. Paper2D ships several in its plugin content:

| Material | Lit | Alpha | Use it for |
|---|---|---|---|
| `DefaultSpriteMaterial` | No | See below | Paper2D's general-purpose unlit material |
| `DefaultLitSpriteMaterial` | Yes | See below | Its lit counterpart |
| `MaskedUnlitSpriteMaterial` | No | **Masked**: each pixel is fully opaque or fully transparent | **GridSnake: everything** |
| `MaskedLitSpriteMaterial` | Yes | Masked | Pixel art that reacts to lights |
| `OpaqueUnlitSpriteMaterial` | No | None: transparent areas turn black | Backgrounds |
| `OpaqueLitSpriteMaterial` | Yes | None | Lit backgrounds |
| `TranslucentUnlitSpriteMaterial` | No | **Translucent**: smooth alpha | Smoke, glows, fades |
| `TranslucentLitSpriteMaterial` | Yes | Translucent | Lit translucent effects |

The two `Default...` materials are general-purpose; the documentation describes them by their
lighting, not their blend mode, so open one in the Material Editor if you want to know how it treats
alpha. The other six say what they do in their names.

*Unlit* means the sprite shows its texture colours exactly, with no lights in the level. That is
the right choice for GridSnake: there are no lights to place, and a pixel artist chose every colour
on purpose.

*Masked* versus *translucent* is the bigger decision. Masked sprites write depth like solid
objects, so the renderer sorts them correctly whatever order they are drawn in, and they are cheap.
Translucent sprites blend smoothly with what is behind them, but the renderer must sort them back
to front by distance, which goes wrong when sprites overlap at the same depth. They also cost more.

There is one place where GridSnake's art has partial transparency: the snake's death frames
(`T_SnakeHead_Dead`) fade out with alpha values of 195 and 135. With a masked material, each pixel
is either drawn or not, cut at a threshold, so the fade becomes a hard "there or gone". That is
fine for this game. If the import picked a translucent material for those sprites (the import
default can choose one per texture by looking at the alpha), set them back to
`MaskedUnlitSpriteMaterial` for consistency, or keep translucent if you prefer the soft fade and
accept the sorting caveat. Check the **Default Material** of `S_SnakeHead_Dead_0` to see which one
you got.

To change the material of many sprites at once, select them and use **Asset Actions > Bulk Edit
via Property Matrix**: it shows one row per asset and lets you set a column for all rows.

## Flipbooks

A **flipbook** (`UPaperFlipbook`) is a list of **keyframes**, each a sprite plus a duration, and a
**Frames Per Second** value. A keyframe's duration is a whole number of frames (the **Frame Run**),
so a keyframe with Frame Run 2 at 8 fps shows for a quarter of a second. Flipbooks are the classic
flip-book animation: show one picture, then the next.

### Creating the flipbooks

1. Select `S_Gem_Idle_0` to `S_Gem_Idle_5` in the Content Browser.
2. Right-click and choose **Create Flipbook**. Paper2D adds the selected sprites as keyframes in
   alphabetical order, each with Frame Run 1.
3. Name the new asset `FB_Gem_Idle` and move it to `Art/Flipbooks`.
4. Hover over it in the Content Browser: the thumbnail plays the animation.
5. Double-click it to open the **Flipbook Editor**. Set **Frames Per Second** to `8` in the Details
   panel. The editor plays the flipbook and shows a timeline of keyframes at the bottom.

Alphabetical order works because the numbers are single digits. With ten or more frames,
`_10` sorts before `_2`; you would then fix the order in the timeline or name frames `_00`, `_01`.

Repeat for the other strips:

| Flipbook | Sprites | Frames Per Second | Length |
|---|---|---|---|
| `FB_SnakeHead_Idle` | `S_SnakeHead_Idle_0..1` | 4 | 0.50 s |
| `FB_SnakeHead_Move` | `S_SnakeHead_Move_0..1` | 8 | 0.25 s |
| `FB_SnakeHead_Eat` | `S_SnakeHead_Eat_0..3` | 16 | 0.25 s |
| `FB_SnakeHead_Dead` | `S_SnakeHead_Dead_0..2` | 8 | 0.375 s |
| `FB_Gem_Idle` | `S_Gem_Idle_0..5` | 8 | 0.75 s |
| `FB_Gem_Pop` | `S_Gem_Pop_0..4` | 16 | 0.3125 s |
| `FB_Exit_Closed` | `S_Exit_Closed_0` | any (one frame) | — |
| `FB_Exit_Opening` | `S_Exit_Opening_0..3` | 10 | 0.40 s |
| `FB_Exit_Open` | `S_Exit_Open_0..3` | 8 | 0.50 s |

The length is the number of frames divided by the frame rate, when every Frame Run is 1. Nine
flipbooks in all. `FB_Exit_Closed` has a single frame, so it never changes; it is still a flipbook
because the exit actor (chapter 5, extended in chapter 12) switches between flipbooks on one
flipbook component, and a flipbook component cannot show a plain sprite.

You can also make a flipbook from nothing (**+Add > Animation > Paper Flipbook**) and add keyframes
in the Details panel's **Key Frames** array or by dragging sprites onto the timeline.

### Looping and one-shot animations

A flipbook asset has no "loop" setting; looping is a property of the *component* that plays it.
`UPaperFlipbookComponent` loops by default. Some of GridSnake's flipbooks are meant to loop (idle,
move, gem idle, exit open) and others to play once and stop on the last frame (eat, dead, pop, exit
opening). Chapter 12 calls `SetLooping(false)` for those and reacts when they finish.

## Rendering settings for 2D

A new UE 5.8 project turns on the renderer's expensive 3D features: **Lumen** for global
illumination and reflections, **mesh distance fields**, **virtual shadow maps**, plus
post-processing effects tuned for photo-real scenes. A flat, unlit, pixel-art game uses none of
them, and some actively hurt it. Turn them off in **Edit > Project Settings > Engine > Rendering**.

| Setting (Rendering page) | Section | Set to | Why |
|---|---|---|---|
| **Dynamic Global Illumination Method** | Global Illumination | **None** | Unlit sprites receive no light; Lumen GI only costs time |
| **Reflection Method** | Reflections | **None** | Nothing in GridSnake is shiny |
| **Generate Mesh Distance Fields** | Software Ray Tracing (or Lighting) | **Off** | Used by Lumen and distance field shadows; sprites have no meshes worth tracing |
| **Shadow Map Method** | Shadows | **Shadow Maps** (not Virtual Shadow Maps) | No shadows are cast at all, so the cheap option is enough |
| **Auto Exposure** | Default Settings | **Off** | Otherwise the image brightens or darkens as the view changes |
| **Bloom** | Default Settings | **Off** | Bright pixels would glow and bleed into their neighbours |
| **Motion Blur** | Default Settings | **Off** | Moving sprites would smear |
| **Anti-Aliasing Method** | Default Settings | **None** | Every anti-aliasing method softens edges, which are the point of pixel art |

The labels and sections above are from recent engine versions; Epic moves settings between
sections from time to time. Use the search box at the top of Project Settings: typing "bloom" finds
the checkbox wherever it lives. The editor asks you to restart after some of these changes. Do it.

The editor saves these settings to `Config/DefaultEngine.ini`. Compare the ch03 and ch04 snapshots
and you see exactly what changed:

```diff
 [/Script/Engine.RendererSettings]
-r.ReflectionMethod=1
-r.GenerateMeshDistanceFields=True
-r.DynamicGlobalIlluminationMethod=1
-r.Shadow.Virtual.Enable=1
+; GridSnake (ch04): Project Settings > Engine > Rendering, tuned for unlit pixel art.
+; Lumen, distance fields and virtual shadow maps do nothing for flat sprites.
+r.ReflectionMethod=0
+r.GenerateMeshDistanceFields=False
+r.DynamicGlobalIlluminationMethod=0
+r.Shadow.Virtual.Enable=0
 r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange=True
 r.AllowStaticLighting=False
+; Auto exposure would brighten/darken the sprites as the view changes.
+r.DefaultFeature.AutoExposure=False
+; Bloom and motion blur smear pixel edges.
+r.DefaultFeature.Bloom=False
+r.DefaultFeature.MotionBlur=False
+; Anti-aliasing blurs pixel art; 0 = None.
+r.AntiAliasingMethod=0
```

- Each line is a **console variable** (cvar) and its value. `r.` cvars belong to the renderer.
  The Rendering page is a friendly editor for this one ini section, which is why the section name is
  the settings class from chapter 3's config hierarchy: `/Script/Engine.RendererSettings`.
- The values are numbers where a setting is a choice. For `r.DynamicGlobalIlluminationMethod` and
  `r.ReflectionMethod`, 0 is None and 1 is Lumen. For `r.AntiAliasingMethod`, 0 is None; the
  others are FXAA, TAA, MSAA and TSR (the default for new projects, which is why the ch03 file had
no line for it).
- The `;` comments are the course's. The editor does not write comments, and when it saves this
  section again it may drop them. That is harmless.
- `ExtendDefaultLuminanceRange` and `AllowStaticLighting=False` were already there from the project
  template and stay.

You could type these lines into the ini file instead of clicking (with the editor closed). Using
the UI is safer, because it validates values and tells you when a restart is needed.

These are *project defaults*. A **Post Process Volume** in a level, or a camera's post-process
settings, can override bloom, exposure and motion blur locally. Chapter 6 gives the game camera its
own settings, which is where per-camera overrides belong.

### Colours and the tonemapper

Even with all of the above off, the renderer still runs the **tonemapper**, which maps the scene's
colours to the screen's. It can shift colours slightly, so a sprite's on-screen colour may not match
the PNG's value exactly. For GridSnake's placeholder art that is fine. To see what the tonemapper
does, type `ShowFlag.Tonemapper 0` in the console (the `~` key in a Play session, or the field at
the bottom of the Output Log) and compare; `ShowFlag.Tonemapper 1` restores it. Treat this console
command as a diagnostic: it can make the image noticeably different. Games that need exact palette
colours handle this in the material or the post-process settings, which is beyond this course.

## Looking at sprites in the level

You have sprites and flipbooks; now put one in a level and look at it the way the game will.

1. Open `L_Level01` (it has been the editor startup map since chapter 1).
2. Drag `FB_Gem_Idle` from the Content Browser into the viewport. The editor creates a
   `PaperFlipbookActor` with a `PaperFlipbookComponent` playing the flipbook.
3. In the Details panel set its **Location** to `X=16, Y=0, Z=-16` and its **Rotation** to
   `0, 0, 0`.
4. In the viewport's view menu (top-left, labelled **Perspective**), switch to the **Left** or
   **Right** orthographic view. In UE 5.8 these two look along the Y axis at the X-Z plane, where
   sprites lie. **Front** and **Back** look along X at the Y-Z plane, and a sprite seen from there
   is edge-on: an invisible line.
5. Select the actor and look at the transform gizmo. Pick the view (Left or Right) in which the
   **red** arrow (X) points **right** and the **blue** arrow (Z) points **up**. That is GridSnake's
   view: +X right, +Z up, exactly as the art was drawn.
6. Zoom in with the mouse wheel. The gem should be sharp at every zoom level, and animate (if it
   does not animate in the editor viewport, check the viewport's **Realtime** option, or press
   Play).

Which of the two views shows +X to the right depends on the view's direction; the other one shows a
mirror image, with the gem drawn as if from behind. The gizmo check settles it in a second. The view
names in the menu have changed between engine versions, so trust the gizmo rather than the name.

The editor's **grid snapping** (the grid icon and value at the top-right of the viewport) moves
actors in steps. GridSnake's tiles are 16 units wide, and chapter 6 puts the board so that tile
centres sit on multiples of 16 (0, 16, 32, ... in X; 0, -16, -32, ... in Z, because rows count
downwards). A snap size of **16** puts actors on tile centres; **8** reaches the tile edges too.
The default snap sizes (1, 5, 10, 50, ...) include neither; turn on **Use Power of Two Snap Size** in **Editor Preferences** (search for "power of two") and
the menu offers 1, 2, 4, 8, 16, and so on. (Chapter 10 adds a function that snaps actors to
cells from code.)

Delete the gem actor before you continue: chapter 5 gives gems an actor class of their own, and
chapter 6 adds a real camera.

### Depth: what Y is for

All sprites drawn in the same X-Z plane at the same Y overlap like cards on a table, and the
renderer has to decide which is in front. GridSnake avoids ties by giving each kind of actor its own
small Y offset from chapter 5 on: the floor at Y = 0, gems and the exit at 5, the snake's head
at 10. The game camera (chapter 6) sits on the +Y side, so a larger Y is nearer to it. Because
the materials are masked and write depth, the result is stable. With translucent
materials, the renderer would sort by distance to the camera instead, and two sprites at the same Y
could flicker in front of each other.

## Sprites and flipbooks from C++

Everything so far happened in the editor. The game itself shows sprites through components that C++
creates, starting in chapter 5. The three examples below are a preview: one actor that shows a
sprite, one that plays a flipbook, and a Blueprint function library with a small pixel-grid helper.

### Setting up for the examples

The examples need the art you just made, so the easiest place for them this time is GridSnake
itself:

1. Close the editor.
2. Copy the six files from `examples/ch04/` into `Source/GridSnake/Examples/` (create the folder).
3. Add `"Paper2D"` to `PublicDependencyModuleNames` in `Source/GridSnake/GridSnake.Build.cs`:

   ```csharp
   PublicDependencyModuleNames.AddRange(new string[] {
   	"Core",
   	"CoreUObject",
   	"Engine",
   	"InputCore",
   	"EnhancedInput",
   	"DeveloperSettings",
   	"Paper2D"
   });
   ```

4. Build and start the editor.

The *plugin* (enabled in chapter 1) makes Paper2D's assets and editors available. The *module
dependency* lets your C++ include its headers and link against it. Without it, the build fails with
an error that `PaperSprite.h` cannot be found, or with unresolved externals for
`UPaperSpriteComponent`. Chapter 5 adds the same line to GridSnake for good, so you can keep it.
Delete the `Examples` folder before you start chapter 5, so your project matches the snapshots.

If you prefer the `Sandbox` project from chapter 2: enable the Paper2D plugin there (**Edit >
Plugins**, search "Paper2D", restart), add `"Paper2D"` to `Sandbox.Build.cs` as above, and import
`T_Gem_Idle.png` with the steps from this chapter. Replace `GridSnake` with `Sandbox` in what
follows.

### Example: a tinted sprite

`examples/ch04/ExSpriteActor.h`

```cpp
// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSpriteActor.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

/**
 * Shows one sprite with a tint. Pick the sprite and the tint in the Details panel;
 * OnConstruction applies them, so the change is visible in the editor at once.
 */
UCLASS()
class AExSpriteActor : public AActor
{
	GENERATED_BODY()

public:
	AExSpriteActor();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Sprite")
	TObjectPtr<UPaperSpriteComponent> SpriteComponent;

	UPROPERTY(EditAnywhere, Category = "Sprite")
	TObjectPtr<UPaperSprite> Sprite;

	/** Multiplied with the sprite's pixels. White leaves them unchanged. */
	UPROPERTY(EditAnywhere, Category = "Sprite")
	FLinearColor Tint = FLinearColor::White;
};
```

`examples/ch04/ExSpriteActor.cpp`

```cpp
// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExSpriteActor.h"
#include "Engine/CollisionProfile.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogExSprite, Log, All);

AExSpriteActor::AExSpriteActor()
{
	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
	SpriteComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetRootComponent(SpriteComponent);
}

void AExSpriteActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs in the editor whenever the actor is placed, moved or edited.
	SpriteComponent->SetSprite(Sprite);
	SpriteComponent->SetSpriteColor(Tint);
}

void AExSpriteActor::BeginPlay()
{
	Super::BeginPlay();

	if (!Sprite)
	{
		UE_LOG(LogExSprite, Warning, TEXT("%s has no sprite; pick one in the Details panel"), *GetName());
		return;
	}

	// The size of the sprite's source region in the texture, in pixels.
	const FVector2D Size = Sprite->GetSourceSize();
	UE_LOG(LogExSprite, Log, TEXT("%s shows %s: %.0f x %.0f pixels, tint %s"),
		*GetName(), *Sprite->GetName(), Size.X, Size.Y, *Tint.ToString());
}
```

Try it:

1. In the Content Browser, open **C++ Classes > GridSnake > Examples** (if you do not see C++
   classes, enable **Settings > Show C++ Classes** in the Content Browser) and drag
   `ExSpriteActor` into the level.
2. In the Details panel, under **Sprite**, set **Sprite** to `S_Gem_Idle_0` and **Tint** to pure
   red: R=1, G=0, B=0, A=1.
3. Look at it in the Left or Right orthographic view, as before. The gem changes as soon as you edit
   either property.
4. Press **Play** and filter the Output Log.

Expected Output Log (filter: `LogExSprite`), in PIE on `L_Level01`:

```text
LogExSprite: ExSpriteActor_0 shows S_Gem_Idle_0: 16 x 16 pixels, tint (R=1.000000,G=0.000000,B=0.000000,A=1.000000)
```

The actor's name (`ExSpriteActor_0`) depends on how many you have placed, and the line may carry a
timestamp prefix, depending on your Output Log settings.

- **`OnConstruction`** runs in the editor every time the actor is placed, moved or has a property
  changed, and once more when the game spawns it. Applying properties to components there gives
  instant feedback in the viewport. It is the C++ side of a Blueprint's Construction Script.
- **`SetSprite`** and **`SetSpriteColor`** are the two calls GridSnake uses most on sprite
  components. The component draws the sprite's pixels *multiplied* by the colour. White leaves them
  unchanged; red keeps only the red channel. The gem is cyan, a colour with little red, so it
  turns a dark red. Chapter 12 uses the same multiply to shade the snake's body from head to tail.
- **`GetSourceSize`** returns the size of the sprite's source region, in pixels. It does not depend
  on **Pixels per unit**: with a value of 2, the sprite would still report 16 x 16 but be drawn
  8 units wide.
- `SetCollisionProfileName(NoCollision)` turns off collision for the component. Sprites can carry
  collision, but GridSnake gives its actors separate, simple collision shapes (chapter 10), so
  every purely visual component gets this line.
- The `Sprite` property is a hard reference (`TObjectPtr`): when the level loads, the sprite and its
  texture load with it. For a few small assets that is the right choice; chapter 13 shows soft
  references for when it is not.

### Example: reading a flipbook

`examples/ch04/ExFlipbookInfo.h`

```cpp
// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExFlipbookInfo.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;

/** Plays a flipbook and logs its frame count, frame rate and length on BeginPlay. */
UCLASS()
class AExFlipbookInfo : public AActor
{
	GENERATED_BODY()

public:
	AExFlipbookInfo();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Flipbook")
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> Flipbook;

	/** 1 = the flipbook's own speed, 2 = twice as fast. */
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "0"))
	float PlayRate = 1.0f;
};
```

`examples/ch04/ExFlipbookInfo.cpp`

```cpp
// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExFlipbookInfo.h"
#include "Engine/CollisionProfile.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogExFlipbook, Log, All);

AExFlipbookInfo::AExFlipbookInfo()
{
	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComponent"));
	FlipbookComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetRootComponent(FlipbookComponent);
}

void AExFlipbookInfo::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	FlipbookComponent->SetFlipbook(Flipbook);
	FlipbookComponent->SetPlayRate(PlayRate);
}

void AExFlipbookInfo::BeginPlay()
{
	Super::BeginPlay();

	if (!Flipbook)
	{
		UE_LOG(LogExFlipbook, Warning, TEXT("%s has no flipbook; pick one in the Details panel"), *GetName());
		return;
	}

	// These read the flipbook asset; the play rate does not change them.
	UE_LOG(LogExFlipbook, Log, TEXT("%s: %d frames at %.1f fps = %.2f s (play rate %.1f)"),
		*Flipbook->GetName(),
		FlipbookComponent->GetFlipbookLengthInFrames(),
		FlipbookComponent->GetFlipbookFramerate(),
		FlipbookComponent->GetFlipbookLength(),
		FlipbookComponent->GetPlayRate());
}
```

Try it: drag `ExFlipbookInfo` into the level, set **Flipbook** to `FB_Gem_Idle` and leave **Play
Rate** at 1. The gem animates in the viewport. Press **Play**.

Expected Output Log (filter: `LogExFlipbook`), in PIE on `L_Level01`:

```text
LogExFlipbook: FB_Gem_Idle: 6 frames at 8.0 fps = 0.75 s (play rate 1.0)
```

The numbers come from your asset: 6 keyframes with a Frame Run of 1 each, at the 8 frames per
second you set. If you see different values, open `FB_Gem_Idle` and check its keyframes and **Frames
Per Second**. Now set **Play Rate** to 2 and play again: the gem spins twice as fast, but the log
still says `8.0 fps` and `0.75 s`, with `play rate 2.0` at the end.

- **`SetFlipbook`** chooses what the component plays. GridSnake's snake calls it whenever its state
  changes: idle, moving, eating, dead (chapter 12).
- **`GetFlipbookLengthInFrames`**, **`GetFlipbookFramerate`** and **`GetFlipbookLength`** report the
  *asset's* timing: keyframes times frame runs, frames per second, and their ratio in seconds. They
  ignore the play rate. To get the real time on screen, divide the length by the play rate.
- **`SetPlayRate`** scales the speed of one component without touching the asset. Every actor using
  `FB_Gem_Idle` would change if you edited the asset's frames per second; with the play rate, you
  can speed up a single gem. A play rate of 0 freezes the animation, and negative rates play it
  backwards, which is why the property clamps at 0 here.
- `UPaperFlipbookComponent` starts playing on its own and loops. The calls that change that
  (`SetLooping`, `Play`, `Stop`, `PlayFromStart`, and the `OnFinishedPlaying` event) are
  chapter 12's topic.

### Example: snapping to the pixel grid

Pixel art looks right only when every sprite pixel lands on a whole number of units. A sprite at
`X = 23.4` is drawn between pixel columns; without filtering it still looks sharp, but it moves in
uneven steps as it slides. This Blueprint function library rounds a location to the grid. It does
not include any Paper2D header, so it works in a module without the `Paper2D` dependency.

`examples/ch04/ExPixelSnap.h`

```cpp
// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ExPixelSnap.generated.h"

/**
 * A Blueprint function library: static functions that show up in every
 * Blueprint's node menu under "Ex|Pixel Art". No instance is ever placed.
 */
UCLASS()
class UExPixelSnapLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Rounds X and Z (the Paper2D sprite plane) to the nearest multiple of Unit.
	 * Y is depth and stays as it is. Unit <= 0 returns the location unchanged.
	 */
	UFUNCTION(BlueprintPure, Category = "Ex|Pixel Art")
	static FVector SnapToPixelGrid(FVector Location, float Unit = 1.0f);

	/** Logs a few test calls. Call it from a Level Blueprint's BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Ex|Pixel Art")
	static void LogSnapExamples();
};
```

`examples/ch04/ExPixelSnap.cpp`

```cpp
// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExPixelSnap.h"

DEFINE_LOG_CATEGORY_STATIC(LogExPixel, Log, All);

FVector UExPixelSnapLibrary::SnapToPixelGrid(FVector Location, float Unit)
{
	if (Unit <= 0.0f)
	{
		return Location;
	}

	// FVector holds doubles in UE5, so snap in double precision.
	const double Grid = Unit;
	return FVector(FMath::GridSnap(Location.X, Grid), Location.Y, FMath::GridSnap(Location.Z, Grid));
}

void UExPixelSnapLibrary::LogSnapExamples()
{
	const FVector Samples[] = { FVector(23.4, 5.0, -7.6), FVector(25.0, 0.0, -25.0) };
	const float Units[] = { 1.0f, 16.0f };

	for (const FVector& Sample : Samples)
	{
		for (const float Unit : Units)
		{
			UE_LOG(LogExPixel, Log, TEXT("Snap(%s, %.0f) = %s"),
				*Sample.ToString(), Unit, *SnapToPixelGrid(Sample, Unit).ToString());
		}
	}
}
```

Try it from the level's Blueprint:

1. In the toolbar, open the Blueprint menu and choose **Open Level Blueprint**.
2. Right-click in the graph, type "Log Snap Examples" and add the node. It is listed under **Ex >
   Pixel Art**, as the `Category` says.
3. Connect **Event BeginPlay**'s execution pin to it (add the event node if the graph has none).
4. **Compile**, close the Blueprint editor and press **Play**.

Expected Output Log (filter: `LogExPixel`), in PIE on `L_Level01`:

```text
LogExPixel: Snap(X=23.400 Y=5.000 Z=-7.600, 1) = X=23.000 Y=5.000 Z=-8.000
LogExPixel: Snap(X=23.400 Y=5.000 Z=-7.600, 16) = X=16.000 Y=5.000 Z=0.000
LogExPixel: Snap(X=25.000 Y=0.000 Z=-25.000, 1) = X=25.000 Y=0.000 Z=-25.000
LogExPixel: Snap(X=25.000 Y=0.000 Z=-25.000, 16) = X=32.000 Y=0.000 Z=-32.000
```

You can check these by hand. `FMath::GridSnap(Value, Grid)` rounds `Value` to the nearest multiple
of `Grid`: -7.6 is nearer to -8 than to -7, and 25 is nearer to 32 than to 16. Exactly halfway
(say 24 with a grid of 16) rounds up, to 32. Y, the depth, is never touched.

Remove the node from the Level Blueprint when you are done (or delete the Level Blueprint's
contents): the level is part of the project, and the snapshots have an empty Level Blueprint.

- A **Blueprint function library** is a `UBlueprintFunctionLibrary` subclass with only `static`
  `UFUNCTION`s. Each one becomes a node available in every Blueprint, with no object to create.
  Unreal's own `UKismetMathLibrary` and `UKismetSystemLibrary` are built this way.
- **`BlueprintPure`** makes a node with no execution pins: it just computes a value, like `+`.
  **`BlueprintCallable`** makes a node with execution pins, for functions with side effects such as
  logging. Chapter 3 introduced both.
- The default argument `Unit = 1.0f` becomes the default value of the node's input pin.
- `FVector` stores `double`s in UE5 (Large World Coordinates), so the function snaps in `double`
  and only the parameter, which Blueprints edit, is a `float`.
- GridSnake does not need this helper: the board (chapter 6) computes every position from whole
  tile coordinates, so nothing ever lands between pixels. Keeping game state in integers and
  converting to world positions only for drawing is the general lesson.

## Project: chapter 4 milestone

This chapter changes almost no code. GridSnake gains its art and a 2D-friendly renderer.

| File | Change |
|---|---|
| `Tools/make_placeholder_art.py` | New. Generates `RawAssets/Textures/*.png` and `RawAssets/Audio/*.wav` |
| `Config/DefaultEngine.ini` | Rendering settings for unlit pixel art (the diff above) |
| `Config/DefaultEditor.ini` | The editor saves the Paper2D import defaults here. The snapshot's copy only has a comment about it: set the values in the editor, not by hand |
| `Content/GridSnake/Art/...` | 11 textures, 32 sprites, 9 flipbooks (binary assets; not in the snapshot) |
| `ASSETS.md` | The editor steps of this chapter, as a checklist |

The `.uasset` files are not in the course repository: Unreal assets are binary and tied to an
engine version, so the snapshots describe them in `ASSETS.md` instead. If you skipped ahead, follow
`project/ch04/ASSETS.md` to recreate them; it is the same as this chapter's steps, in order:

1. Run `Tools/make_placeholder_art.py`.
2. Set the **Paper2D - Import** defaults.
3. Set the **Rendering** options and restart.
4. Create the four `Art` folders and import the PNGs into `Art/Textures`.
5. **Apply Paper2D Texture Settings** on all textures.
6. **Extract Sprites** from every strip except `T_Tiles`, plus `S_SnakeBody`; move them to
   `Art/Sprites`.
7. **Create Flipbook** for each strip; set the frame rates; move them to `Art/Flipbooks`.
8. Optionally, check a flipbook in the Left or Right orthographic view, then delete it.

How to check the milestone:

- The project builds and starts as before. Press **Play** and filter the Output Log for
  `LogGridSnake`: you see the same game instance and game mode lines as at the end of chapter 3.
  This chapter adds no game code and no new log lines.
- In the Content Browser, `Art/Textures` has 11 textures, `Art/Sprites` 32 sprites and
  `Art/Flipbooks` 9 flipbooks. `Art/Tiles` is still empty (chapter 6).
- Hovering over each flipbook plays its animation in the thumbnail. Open `FB_SnakeHead_Eat`: the
  snake's mouth opens and closes, 16 frames per second.
- Double-click any texture: the pixels are hard-edged at every zoom, and **Filter** shows
  **Nearest**.
- In **Project Settings > Engine > Rendering**, **Anti-Aliasing Method** shows **None**.

### When it goes wrong

| Symptom | Likely cause | Fix |
|---|---|---|
| Sprites look blurry | The texture still has 3D settings | **Apply Paper2D Texture Settings** on it, or set **Filter** to **Nearest** and **Mip Gen Settings** to **NoMipmaps** by hand |
| Colours look blotchy or shifted | Block compression | **Compression Settings** = **UserInterface2D (RGBA)** |
| Thin dark or coloured lines at sprite edges | Neighbouring frames bleed in, usually from mipmaps or filtering | Same as blurry; also check the sprite's **Source UV** is a multiple of 16 |
| A sprite shows half of one frame and half of the next | Wrong cell size or margin in Extract Sprites | Delete the sprites and extract again with 16x16 cells, margins 0 |
| Flipbook frames out of order | The sprites were added alphabetically and your names do not sort | Reorder in the Flipbook Editor's timeline, or rename with zero-padding |
| Nothing visible in the viewport | The view looks at the sprite edge-on | Switch to Left or Right (not Front, Back or Top) and use the gizmo check |
| The sprite is mirrored | The view looks at it from behind | Switch to the other of Left and Right |
| The image gets brighter or darker as you move the camera | Auto exposure still on, or a Post Process Volume overrides it | Rendering settings, then look for a volume in the level |
| Translucent sprites flicker over each other | Translucency sorting | Use `MaskedUnlitSpriteMaterial`, or separate the sprites in Y |

## Summary

- **Paper2D** adds 2D asset types to Unreal's 3D world: textures become **sprites** (rectangles of
  a texture with a pivot and material), sprites become **flipbooks** (keyframes plus a frame rate),
  and chapter 6 adds tile sets and tile maps. Components (`UPaperSpriteComponent`,
  `UPaperFlipbookComponent`) draw them in a level.
- Sprites stand in the **X-Z plane**: +X right, +Z up, Y is depth. Look at them in the **Left** or
  **Right** orthographic view, and confirm with the gizmo (red X right, blue Z up). Front and Back
  see them edge-on.
- With **Pixels per unit** = 1, one pixel is one unit, so GridSnake's 16-pixel tiles are 16 units
  and every position is a whole number.
- Pixel art needs **Nearest** filtering, **no mipmaps** and **uncompressed** (UserInterface2D)
  textures. **Apply Paper2D Texture Settings** sets all of them; the **Paper2D - Import** defaults
  make new sprites right from the start.
- **Extract Sprites** in **Grid** mode cuts a strip into frames, named with a template such as
  `S_Gem_Idle_{0}`. **Create Flipbook** turns selected sprites into an animation.
- **MaskedUnlitSpriteMaterial** draws every pixel fully opaque or fully transparent, ignores lights
  and sorts correctly. Translucent materials allow soft alpha at a cost in sorting and speed.
- A 2D game turns off Lumen, distance fields, virtual shadow maps, auto exposure, bloom, motion blur
  and anti-aliasing. The settings live in `[/Script/Engine.RendererSettings]` of
  `DefaultEngine.ini`.
- From C++, `SetSprite`, `SetSpriteColor`, `SetFlipbook` and `SetPlayRate` are the everyday calls,
  and `OnConstruction` applies Details-panel edits in the editor at once. Using Paper2D types needs
  `"Paper2D"` in your module's `Build.cs`.

Chapter 5 turns these assets into GridSnake's first actors, the gem and the exit, with components
created in C++ and Blueprint subclasses that pick the flipbooks.

## Exercises

1. **Your own tail.** Draw a 32x16 PNG with two 16x16 frames of a snake's tail tip (any image
   editor works; keep the background transparent). Import it as `T_SnakeTail`, apply the Paper2D
   texture settings, extract `S_SnakeTail_0` and `S_SnakeTail_1`, and make `FB_SnakeTail` at 4
   frames per second. Check it in the Content Browser preview. (GridSnake does not use it, so keep
   it out of `Art/` or delete it afterwards.)

<details>
<summary>Solution</summary>

1. Save the PNG to `RawAssets/Textures/T_SnakeTail.png`, outside `Content/`.
2. In `Art/Textures`, **Import** it, then right-click > **Sprite Actions > Apply Paper2D Texture
   Settings**.
3. Right-click > **Sprite Actions > Extract Sprites**: **Grid**, cell 16x16, margins and spacing 0,
   **Naming Template** `S_SnakeTail_{0}`, start index 0. Two sprites appear.
4. Select both, right-click > **Create Flipbook**, name it `FB_SnakeTail`, open it and set **Frames
   Per Second** to 4. The length is 2 / 4 = 0.5 s.

If the frames are blurry, the texture settings were not applied before you looked; if they show
parts of each other, the cell size was not 16.

</details>

2. **Why blurry?** Import `T_Gem_Idle.png` a second time, under a different name, and do *not*
   apply the Paper2D settings. Open it next to the original and zoom in. Explain which two texture
   settings cause the blur, and change only one of them at a time to see what each does.

<details>
<summary>Solution</summary>

The blur comes from **filtering** and **mipmaps**:

- With a bilinear (or trilinear) filter, each screen pixel blends the four nearest texels, so the
  hard edge between two pixels turns into a gradient when the texture is magnified. Setting
  **Filter** to **Nearest** alone fixes the zoomed-in view.
- Mipmaps are smaller, averaged copies of the texture used when it is drawn small. Zoom out and the
  gem's highlights and outline blur into the body colour even with Nearest filtering. Setting **Mip
  Gen Settings** to **NoMipmaps** fixes that.

A third, smaller effect is compression: with the default (block) compression, look closely at the
gem's highlight pixels for slightly shifted colours, which **UserInterface2D (RGBA)** removes.
Delete the duplicate texture when you are done.

</details>

3. **Faster gems.** Make the gems sparkle faster: 12 frames per second instead of 8. Do it twice:
   once for every gem, and once for a single `ExFlipbookInfo` actor only. What does the example's
   log line say each time?

<details>
<summary>Solution</summary>

- **Every gem:** open `FB_Gem_Idle` and set **Frames Per Second** to 12. Every component that plays
  the asset changes. The log line becomes
  `FB_Gem_Idle: 6 frames at 12.0 fps = 0.50 s (play rate 1.0)`.
- **One actor:** keep the asset at 8 and set the actor's **Play Rate** to 1.5 (8 x 1.5 = 12). Only
  this component speeds up. The log line stays `6 frames at 8.0 fps = 0.75 s` and ends with
  `(play rate 1.5)`, because the length functions describe the asset, not the component.

Set the asset back to 8 afterwards; later chapters assume the frame rates from the table.

</details>

4. **Tint from Blueprints.** Extend `AExSpriteActor` so a Blueprint can read and change the tint at
   runtime: make `Tint` `BlueprintReadOnly`, and add a `BlueprintCallable` function `SetTint` that
   stores the colour and updates the component. Test it from the Level Blueprint: on BeginPlay, set
   the tint of a placed `ExSpriteActor` to green.

<details>
<summary>Solution</summary>

In the header, change the property and add the function:

```cpp
public:
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetTint(FLinearColor NewTint);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	FLinearColor Tint = FLinearColor::White;
```

In the `.cpp`:

```cpp
void AExSpriteActor::SetTint(FLinearColor NewTint)
{
	Tint = NewTint;
	SpriteComponent->SetSpriteColor(Tint);
}
```

`BlueprintReadOnly` is enough because writes go through `SetTint`. With `BlueprintReadWrite`, a
Blueprint could set `Tint` directly, which changes the variable but not the component: the colour on
screen would not follow until the next `OnConstruction`. A setter function that keeps both in step
is the usual pattern. (The `BlueprintSetter` property specifier can even route Blueprint writes
of `Tint` through `SetTint`; look it up in the UPROPERTY documentation if you are curious.)

To test: select the actor in the level, open the Level Blueprint, right-click > **Create a
Reference to ExSpriteActor_0**, drag from the reference, add **Set Tint**, set the colour to
(0, 1, 0, 1) and connect it to **Event BeginPlay**. Press **Play**: the gem turns green. Clean up
the Level Blueprint afterwards.

</details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-4--pixel-art-with-paper2d)
