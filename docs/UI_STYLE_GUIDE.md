# PokeBank NX — Visual UI Style Guide

Last updated: **2026-09-29**

PokeBank NX should feel like a polished Pokémon-focused Switch application, not a desktop editor or developer diagnostics screen.

## Product identity

Use a consistent shell:

- compact PokeBank NX header;
- selected game/Pokémon content as the visual focus;
- clear profile/trainer context;
- compact bottom/quick navigation;
- contextual controller hints;
- rounded panels and restrained depth;
- Pokémon sprites/art where real data/assets exist.

Avoid giant diagnostic dashboards, engineering-status badges, excessive dead space, or identical generic feature cards.

## Interaction color

- focus/selection: **teal/cyan**
- selected text: bright/high contrast
- success/compatible: green
- warning: amber/orange
- destructive/error/illegal: red

Red may remain as restrained brand identity, but normal actions such as Launch must not look destructive.

## Themes

All themes share the same geometry and information hierarchy. Palette changes must not change navigation/layout.

Light mode must retain strong text/focus contrast.

## Product Home

```text
header: logo                         profile

selected game / trainer / Dex / Party     Master Vault
Open / Launch                             Pokédex

Games  Banks  Backups  Search  More       Items  Settings
controller hints
```

Use real Party sprites and trainer identity when known. Never invent trainer customization or Pokémon data.

Master Vault and Pokédex must have distinct visual identities.

## Classic Game Sources

Preserve the familiar game-cover grid as an alternate workflow. It should look like part of PokeBank NX, not a separate legacy app.

## Settings

Use the compact gear affordance and two-pane categories/options presentation. Developer diagnostics belong inside Settings, not on Product Home.

## Assets

Prefer existing reviewed PokeBank sprite/icon/art pipelines. Missing art must degrade gracefully; never substitute an incorrect Pokémon or trainer.

## Touch

Design touch-safe geometry now, but full touch behavior starts only after the integrated UI is physically accepted.
