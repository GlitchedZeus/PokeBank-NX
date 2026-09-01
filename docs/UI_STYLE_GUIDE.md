# PokeBank NX — Visual UI Style Guide

Status: TARGET VISUAL CONTRACT  
Last updated: 2026-09-01

This document translates the current visual references into a native PokeBank NX design language. The goal is **not** to reproduce another application's UI pixel-for-pixel. Use the references for layout hierarchy, readability, controller ergonomics, and the friendly Pokémon-console feel while keeping PokeBank NX visually distinct.

The reference set supplied for this design pass contains three useful patterns:

1. a dark controller-first **game-selection grid** with large box-art cards and a strong focus border;
2. a bright **Pokémon summary** layout with grouped information, a large Pokémon render, compact tabs, and clear bottom button hints;
3. a bright **Pokédex grid** with many small species sprites on the left and a large selected-species preview/details panel on the right.

Reference screenshots are design-only material. Do not bundle those screenshots into the application or release artifacts.

---

# Design goals

PokeBank NX should feel like a polished first-party-style Switch utility rather than a desktop save editor squeezed onto a console.

Priorities:

- controller-first from every screen;
- large readable focus states;
- clear hierarchy at handheld distance;
- friendly Pokémon-world presentation rather than generic cyber/neon UI;
- no dense desktop tables as the default experience;
- no accidental mutation from navigation;
- the same information architecture in OLED Black, Dark, and Light themes;
- fast rendering and bounded memory usage on Switch hardware.

---

# Theme system

Implement one semantic theme layer. Screens must use theme tokens rather than hard-coded per-screen colors.

Required themes:

```text
OLED Black
Dark
Light
```

## OLED Black

Target:

- true/near-black main background;
- slightly raised charcoal cards/panels;
- bright readable text;
- restrained accent colors;
- selected/focused element gets the strongest border/highlight;
- avoid large glowing gradients that defeat OLED-black intent.

## Dark

Target:

- very dark blue/charcoal background rather than pure black;
- raised medium-dark panels;
- slightly softer contrast than OLED Black;
- same card/layout geometry as Light.

## Light

Target:

- warm or cool near-white base;
- subtle pastel tint/gradient regions where inexpensive to render;
- light neutral cards;
- dark text;
- colored section headers and focus accents;
- keep text/background contrast strong enough for handheld use.

## Semantic token concept

Names may change during implementation, but the renderer should expose concepts equivalent to:

```text
background
surface
surfaceRaised
surfaceSelected
textPrimary
textSecondary
textMuted
accentPrimary
accentSecondary
focusBorder
divider
success
warning
error
info
shadow/subtleDepth
```

Do not make screen code ask "is dark mode?" for every individual color. Screens request semantic tokens; the active theme provides values.

Theme preference is persisted in settings.

---

# Overall shell

Use a consistent Switch-style shell:

```text
[ optional top title/header ]

[ main content area ]

[ bottom controller hint bar ]
```

The bottom hint bar should remain compact and context-aware, for example:

```text
A Select    B Back    X Filter    Y Sort    + Settings
```

Only show buttons that currently perform something.

No fake buttons that exist only for decoration.

---

# Focus / selection language

The focused item must be obvious without relying on tiny text changes.

Preferred combination:

- clear border or outline;
- small scale/elevation change if cheap and stable;
- higher-contrast label;
- optional accent underline/header change;
- never rely on color alone for critical state.

For OLED/Dark themes, a bright accent outline around the selected card is preferred.

For Light theme, use a saturated but clean accent outline/header and subtle raised surface.

Animations should be short and optional. Never block input waiting for a cosmetic transition.

---

# Home / Select Game

The supplied game-selection reference is the primary visual direction for installed-game/source browsing.

Target layout:

```text
                   Select Game

       [ cover ] [ cover ] [ cover ] [ cover ]
       Game Name  Game Name  Game Name  Game Name
       Platform   Platform   Platform   Platform

             [ cover ] [ cover ] ...

A Select    B Back    X Details/Filter    + Settings
```

## Card requirements

Each card can show:

- game cover/art or a safe fallback icon;
- title;
- platform line;
- source badge/line where useful (`Installed Save`, `RetroArch`, `Imported Save`);
- status indicator only when actionable (`Read Only`, warning, unsupported version).

Keep stable game identity separate from displayed title.

Long names such as `Brilliant Diamond` and `Legends: Arceus` must fit without ugly truncation where practical. Prefer two-line titles before destructive abbreviations.

## Art loading

- lazy-load/decode art;
- cache only a bounded number of textures;
- use a placeholder immediately;
- one bad/missing image must never block the game list;
- art is presentation only and must never be required to identify a game.

---

# Game / Boxes browser

Use a familiar Pokémon storage rhythm rather than a file-manager layout.

Preferred structure:

```text
[ Game title + platform/source ]        [ box selector ]

[ Party tab ] [ Boxes tab ]

[ Pokémon grid / box ]                 [ selected preview ]
                                         sprite
                                         nickname/species
                                         level / gender / shiny
                                         compact origin/status

[ controller hint bar ]
```

The preview pane should make browsing feel alive while remaining read-only until the user deliberately opens an action.

A on an occupied slot opens the action sheet.

A on an empty slot must not fall through into an unrelated mutation.

L/R moves previous/next box. ZL/ZR may jump farther when useful.

---

# Pokémon Summary

The supplied summary reference is the main visual inspiration, adapted to PokeBank NX provenance/safety needs.

Target concept:

```text
[ Pokémon name ]       [ Level ]       [ shiny/gender/form markers ]
-------------------------------------------------------------------
[ tabs: Overview | Stats | Moves | Origin | Legality/Provenance ]

[ grouped data cards ]                     [ LARGE POKÉMON RENDER ]
[ stats / IV / EV ]
[ moves ]
[ nature / ability / item ]
[ OT / TID / SID ]
[ met/origin info ]

[ current location / Vault / hash / legality summary ]

B Back    L/R Tab
```

## Large render

Selected Pokémon gets a large sprite/render where available.

Fallback order can be:

1. preferred HD sprite/render;
2. normal sprite;
3. silhouette/placeholder.

Missing artwork must not prevent viewing data.

## Data grouping

Avoid one giant scrolling key/value table.

Use grouped cards/sections such as:

- Identity
- Battle / Stats
- Moves
- Trainer
- Encounter / Origin
- Current Storage
- Legality
- Provenance

Fields are generation-aware. Do not display meaningless fake fields for older generations.

## Stat visualization

A radar/hex chart is optional rather than required for first implementation.

If used:

- it must be readable in all themes;
- textual stat values remain available;
- do not spend disproportionate implementation time on the graph before summary data and navigation are stable.

---

# Pokédex / Collection Browser

The supplied Pokédex reference is the primary layout direction.

Target desktop/TV-width layout:

```text
[ National / Regional / Living / Shiny / Missing ]
[ Sort / Filters / Search ]

[ sprite grid..................... ]  [ large selected Pokémon ]
[ sprite grid..................... ]  [ No. 001 ]
[ sprite grid..................... ]  [ Bulbasaur ]
[ sprite grid..................... ]  [ ownership badges ]
[ sprite grid..................... ]  [ forms / shiny / source ]
```

For handheld readability, the right preview pane may consume roughly one third of the screen while the grid uses the rest.

## Grid state

Examples:

- owned normal: full-color sprite;
- missing: silhouette/dimmed treatment;
- shiny owned: shiny badge or alternate marker;
- form variants: explicit form marker rather than duplicate-looking mystery entries;
- selection: strong focus frame.

Do not use a manual caught toggle as authoritative ownership. Master Vault data drives owned/shiny/form state.

## Selected panel

Show, where known:

- National Dex number;
- species name;
- types;
- large sprite;
- normal owned count;
- shiny owned count;
- form coverage;
- game/source coverage;
- button to view matching Vault specimens.

---

# Pokémon Action Sheet

The action sheet should visually fit the selected Pokémon screen instead of feeling like a desktop context menu.

Preferred presentation:

- centered or right-side modal card;
- selected Pokémon name/sprite in header;
- large vertical action rows;
- disabled actions visibly disabled;
- `View Pokémon` first;
- `Cancel` last;
- clear warning treatment for actions requiring conversion or future writes.

Target actions remain:

```text
View Pokémon
Add to Master Vault
Add to Bank…
Transfer to Game…
Edit
Clone
Make Shiny
Legality & Provenance
Cancel
```

Opening the sheet performs zero mutation.

---

# Banks / Master Vault

Use the same visual vocabulary as game boxes so learning transfers across the application.

Master Vault should not look like a raw filesystem browser.

Suggested header distinction:

```text
MASTER VAULT
All Pokémon • immutable originals + derived records
```

Bank:

```text
BANK: Competitive
Box 4 / 20
```

Vault browsing can use the familiar sprite grid + selected-preview pattern.

Bank removal removes the reference, not the Master Vault record; dialogs must say that plainly.

---

# Provenance / Legality presentation

Do not turn provenance into a wall of hashes by default.

First-level summary:

```text
Legality        UNKNOWN / LEGAL / INVALID
Origin          Pokémon Emerald • Game Boy Advance
Current         Master Vault
Source          RetroArch save
Derived         No / Clone / Edited / Converted / Generated
```

Then an expandable/details page can expose:

- Vault ID;
- raw SHA-256;
- source save fingerprint;
- source box/slot;
- parent Vault ID;
- transform history;
- conversion history;
- warnings and unsupported checks.

`UNKNOWN` must never be styled as green/approved.

---

# Dialogs

Use large, readable cards rather than tiny desktop popups.

Good:

```text
Add Mimikyu to Master Vault?

The Pokémon in your Sword save will remain unchanged.

[ Add ]   [ Cancel ]
```

For normal safe-copy Vault insertion, a lightweight confirmation or immediate deliberate menu action may be used. Destructive/future live-write operations require stronger confirmation and preflight.

---

# Typography

- prioritize legibility over stylized fonts;
- one main UI family is enough;
- use weight/size for hierarchy;
- avoid tiny metadata text;
- no all-caps paragraph text;
- Pokémon/game names need enough width and must handle localization.

---

# Spacing and density

Aim for clean console spacing rather than desktop density.

Rules of thumb:

- cards should have clear internal padding;
- selected Pokémon sprites should not touch text blocks;
- keep consistent gaps between grid cells;
- reserve safe margins around TV edges;
- bottom controller bar should not cover content;
- important labels must still be comfortable at 1280×720.

Design at **1280×720 first** because that matches the Switch target, then verify the layout also scales cleanly for screenshots/promotional captures.

---

# Performance constraints

Visual polish must not compromise the application core.

- no unbounded texture cache;
- lazy-load game covers and Pokémon renders;
- precompute/reuse layout geometry when possible;
- avoid expensive blur effects;
- prefer simple panels, gradients, borders, sprites and cached text;
- UI should stay responsive while save parsing/indexing happens asynchronously or in staged work where architecture permits;
- missing/corrupt assets fall back gracefully.

---

# Theme implementation milestone

First theme milestone is considered complete when all of the following use semantic theme tokens and can switch without restarting unless the renderer requires it:

```text
Home / Select Game
Party / Boxes browser
Pokémon Action Sheet
Pokémon Summary
Master Vault browser
Bank browser
Pokédex browser
Settings
Dialogs
Bottom controller hint bar
```

Persist the selected theme.

Do not implement Light as an afterthought by simply inverting colors.

---

# Recommended implementation order

```text
1. theme token structure + persistence
2. reusable card/panel/focus/button-hint primitives
3. Select Game visual refresh
4. Pokémon Action Sheet
5. Party/Boxes grid + selected preview
6. Pokémon Summary
7. Master Vault/Bank visual shell
8. Pokédex grid + preview
9. animation/polish after device testing
```

The current functional priority is still the safe A-button action sheet. Visual work should support that milestone rather than delaying it indefinitely.

---

# Device-test visual questions

On physical Switch, check:

- Can every label be read comfortably in handheld mode?
- Is selection obvious in bright and dark rooms?
- Does OLED Black actually look black rather than gray?
- Are cover-art cards large enough without wasting most of the screen?
- Does the Pokémon summary feel organized instead of crowded?
- Is the large Pokémon render useful or does it steal too much data space?
- Can the Pokédex grid be navigated quickly with stick and D-pad?
- Are bottom button hints visible but unobtrusive?
- Does Light theme remain readable without washed-out text?
- Do Dark/OLED themes avoid crushed low-contrast secondary text?

Device testing decides final spacing and scale.
