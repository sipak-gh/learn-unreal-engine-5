# Chapter 8 assets: Enhanced Input

Carried over: everything in `../ch07/ASSETS.md`.

| Path | Type | Parent class | Key settings |
|---|---|---|---|
| `/Game/GridSnake/Input/IA_Steer` | Input Action | - | **Value Type** = Axis2D (Vector2D) |
| `/Game/GridSnake/Input/IMC_Snake` | Input Mapping Context | - | Mappings below |
| `/Game/GridSnake/Blueprints/BP_GSPlayerController` | Blueprint Class | `GSPlayerController` | Input > Default Mapping Context = `IMC_Snake` |
| `BP_GSSnake` (changed) | Blueprint Class | `GSSnake` | Input > Steer Action = `IA_Steer` |
| `BP_GSGameMode` (changed) | Blueprint Class | `GSGameMode` | Classes > Player Controller Class = `BP_GSPlayerController` |

`IMC_Snake` mappings for `IA_Steer`. The game reads +Y as up and +X as right.

| Key | Modifiers (in order) |
|---|---|
| D, Right | none |
| A, Left | Negate |
| W, Up | Swizzle Input Axis Values (Order YXZ) |
| S, Down | Swizzle Input Axis Values (YXZ), then Negate |
| Gamepad Left Thumbstick 2D-Axis | Dead Zone (Radial, Lower Threshold 0.25) |
| Gamepad D-pad Right / Left / Up / Down | same as D / A / W / S |

## Editor steps

1. Create `Content/GridSnake/Input`.
2. Create the action:
   - Right-click > **Input > Input Action**, name it `IA_Steer`.
   - Set **Value Type** = **Axis2D (Vector2D)**.
3. Create the mapping context:
   - Right-click > **Input > Input Mapping Context**, name it `IMC_Snake`.
   - Add a mapping for `IA_Steer` and add the keys from the table.
   - Use the **+** next to each key to add its **Modifiers**.
4. Create `BP_GSPlayerController` from `GSPlayerController`.
   - Set **Input > Default Mapping Context** = `IMC_Snake`.
5. In `BP_GSSnake`, set **Input > Steer Action** = `IA_Steer`.
6. In `BP_GSGameMode`, set **Player Controller Class** = `BP_GSPlayerController`.
7. Play and press the arrow keys. In chapter 8 the snake only turns its head; it
   starts moving in chapter 9. Expected Output Log lines, one per accepted turn:
   `LogGridSnake: Steer: EGSDirection::Up`
