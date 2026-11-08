# Chapter 15 assets: A HUD with UMG

Carried over: everything in `../ch14/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `/Game/GridSnake/UI/W_HUD` | Widget Blueprint | `GSHUDWidget` | Layout below; HUD > Gem Icon Sprite = `S_Gem_Idle_0` |
| `BP_GSPlayerController` (changed) | Blueprint Class | `GSPlayerController` | UI > HUD Widget Class = `W_HUD` |

`W_HUD` hierarchy. Names in **bold** must match exactly: they are `BindWidget`
properties, and the Widget Blueprint will not compile without the required ones.

```
[Canvas Panel]
├─ [Horizontal Box]  anchored top-left, offset (24, 16), Size To Content
│  ├─ [Text] LevelText        "Level"       font size 24
│  ├─ [Spacer] 32 x 1
│  ├─ [Text] GemsText         "Gems 0/3"    font size 24
│  ├─ [Horizontal Box] GemIconBox  (optional; filled with icons by C++)
│  ├─ [Spacer] 32 x 1
│  └─ [Text] ScoreText        "Score 0"     font size 24
└─ [Text] BannerText          anchored centre, Alignment (0.5, 0.5), Size To Content,
                              font size 48, Justification Center, text empty
```

The text in quotes is design-time preview text; C++ overwrites it. Use the
default Roboto font, or import a pixel font: drag a `.ttf` into the Content
Browser, which creates a Font Face and a Font.

## Editor steps

1. Build. `Build.cs` now depends on `UMG`.
2. Create the widget:
   - Create `Content/GridSnake/UI`. Right-click > **User Interface > Widget
     Blueprint**.
   - In **Pick Parent Class**, expand **All Classes**, choose `GSHUDWidget` and
     name it `W_HUD`.
3. In the **Designer**, build the hierarchy above from the **Palette**.
   - Rename each widget in the Details panel's name field.
   - Tick **Is Variable** where needed. Bound widgets are variables automatically.
4. **Graph > Class Defaults > HUD**:
   - **Gem Icon Sprite** = `S_Gem_Idle_0`.
   - **Gem Icon Size** (32, 32).
   - **Missing Color** dark grey with alpha 0.6.
5. In `BP_GSPlayerController`, set **UI > HUD Widget Class** = `W_HUD`.
6. Play.
   - The HUD shows the level name, `Gems 0/3` and three dark gem icons that light
     up as you eat gems.
   - It shows `Score 0`, the countdown in the banner, then `Level complete!` or
     `Ouch!`.
7. **The Widget Reflector** (Tools > Debug > Widget Reflector) lets you pick a widget
   on screen and see its Slate hierarchy.
