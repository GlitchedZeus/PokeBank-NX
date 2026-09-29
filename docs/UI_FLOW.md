# PokeBank NX — Controller / UI Flow Contract

Last updated: **2026-09-29**

PokeBank NX is a Nintendo Switch application first. Navigation must be predictable, readable at handheld distance, and non-destructive.

## Global conventions

```text
D-pad / Left Stick  Navigate
A                   Select / open
B                   Back / cancel
L / R               Contextual previous/next
ZL / ZR             Contextual secondary action / Launch where labeled
+                   Settings
-                   Help / controls
```

On-screen hints must match actual behavior. Navigation alone never mutates save data.

## Product Home

Product Home is the application root.

The selected game is the visual anchor and may show game art, trainer identity/portrait, provider/source, real per-save Pokédex progress, Party sprites, Open and Launch.

Bottom/quick navigation provides Games, Banks, Backups, Search, More, Items/Backpack and Settings.

## Games → Classic Game Sources

Selecting Games opens the familiar game/source grid.

```text
Game Sources
  ↓
validated save instance/source chooser when required
  ↓
Trainer / Party / Boxes / Items / Pokémon actions
```

The classic view reuses the same authoritative source model. It is not a second scanner or unsafe legacy path.

## Backpack / Items

Backpack is navigation only. It may enter the selected game's existing Items workflow after normal source/backup selection. It never bypasses validation, staging or write locks.

## Settings

Settings uses a two-pane categories/options layout and restores the previous valid category/option when returning.

## Cursor memory

Preserve previous profile/game/focus/scroll/menu positions where the underlying item still exists. If it disappeared, clamp to the nearest valid index rather than resetting blindly to row 0.

## Pokémon action safety

Focusing a Pokémon never mutates data. Destructive or source-affecting actions require explicit deliberate transaction/confirmation.

## Touch

After the integrated Product UI candidate is physically accepted, full touch parity becomes the next frontend milestone.
