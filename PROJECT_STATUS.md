# PokeBank NX Project Status

Last updated: **2026-09-29**

For exact active head and CI status, read `CURRENT_STATUS.md` first.

## Project identity

- Product: **PokeBank NX**
- Repository: `GlitchedZeus/PokeBank-NX`
- Platform: Nintendo Switch homebrew
- Current state: active alpha
- Active MAIN implementation lane: **PR #92**
- PR #100 Product UI polish: **MERGED INTO PR #92**
- PR #92 remains **OPEN / DRAFT / NOT MERGED**

GitHub is authoritative. Checkpoint SHAs document evidence; they are never instructions to move a branch backward.

## Headline

PokeBank NX has passed its original Gen I–III foundation stage.

Today the project has:

- hardware-accepted Gen I–III shared editor workflows;
- a real staged Gen IV Party/Box editor with Create and field parity work;
- provider-aware Gen IV source discovery/assignment;
- the modern Product Home integrated into the MAIN Gen IV lane;
- real Party sprite presentation;
- real Gen I–IV Pokédex progress plumbing;
- Gen IV trainer-name propagation;
- game-appropriate trainer portraits driven by proven game/gender data;
- Classic Game Sources;
- cursor-memory infrastructure;
- Backpack/Items quick navigation through the existing safe open flow;
- compact quick Items/Settings controls;
- a two-pane Settings screen with remembered cursor state.

The next milestone is **one combined Gen I–IV + Product UI NRO** for real Switch testing.

## Active MAIN lane

PR #92  
Branch: `feature/gen4-full-editor-20260928`

Last verified head:
`58a56f8d3b8350283f34fcc4d8dafc495b6515f2`

At the time of this update, exact-head Host Tests and Product UI Native were running and the Gen IV Candidate Gate was queued.

## Generation status

### Gen I — accepted

Red / Blue / Yellow read, inventory and shared Pokémon editor workflows are hardware accepted.

### Gen II — accepted

Gold / Silver / Crystal read, inventory and shared Pokémon editor workflows are hardware accepted.

### Gen III — accepted

Ruby / Sapphire / Emerald / FireRed / LeafGreen read, inventory and shared Pokémon editor workflows are hardware accepted.

### Gen IV — active

Diamond / Pearl / Platinum / HeartGold / SoulSilver have a working staged editor foundation.

The first Party/Box View/Edit milestone is already physically accepted. G4-04 extends that foundation with Create, additional native fields, forms, species mutation, move handling, action parity, and current Product UI integration.

The full G4-04 + Product UI combination is **hardware pending**.

## Product experience

The intended top-level UX is now stable:

1. **Product Home** for the selected game, trainer identity, Party, Pokédex progress, Open/Launch and high-level features.
2. **Classic Game Sources** for users who prefer the familiar direct game grid.
3. Existing proven Party / Boxes / Trainer / Inventory / editor flows underneath.
4. Compact access to Backups, Search, future features, Items and Settings.
5. Two-pane Settings and contextual help.
6. Remembered navigation state across rebuilt menus.

Master Vault and global Pokédex/collection systems may be previewed in the UI but remain non-persistent until their backends are ready.

## Current technical priorities

Before the next hardware candidate:

- finish exact-head CI cleanup;
- verify cursor-memory behavior across the intended navigation surfaces;
- verify Backpack/Items quick entry;
- verify DraStic/melonDS direct launch handoff;
- keep explicit Link Game File as the fail-closed fallback;
- run full host/sanitizer/native regression gates;
- ship one exact Actions-built NRO for owner hardware testing.

After hardware acceptance:

- full app-wide touch control parity;
- then Master Vault / Banks and later-generation expansion according to the roadmap.

## Safety / scope locks

- original source saves stay immutable;
- external emulator saves remain read-only;
- installed-game live writes remain disabled;
- staged editing remains app-owned;
- Launch never grants write permission;
- ambiguous source/content matching fails closed;
- cross-game True Move remains locked;
- Gen V is not started;
- Master Vault persistence is not started;
- future Inject Save work remains separate.

## Hardware acceptance rule

A feature is not device accepted because CI is green or because a neighboring SHA passed previously.

Acceptance requires the **exact Actions-built NRO** for the exact application commit to be physically tested on a real Switch.

That rule remains in force for the upcoming integrated Gen IV + Product UI candidate.
