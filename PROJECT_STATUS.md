# PokeBank NX Project Status

Last updated: **2026-09-29**

For the exact current application head and CI result, read `CURRENT_STATUS.md` first.

## Project identity

- Product: **PokeBank NX**
- Platform: Nintendo Switch homebrew
- Repository: `GlitchedZeus/PokeBank-NX`
- State: active alpha
- Active MAIN implementation lane: **PR #92**
- PR #100 Product UI polish: **merged into PR #92**
- PR #92: **OPEN / DRAFT / NOT MERGED**

GitHub is authoritative. Historical SHAs document evidence; they are never instructions to move a branch backward.

## Where the project stands

PokeBank NX now has hardware-accepted Gen I–III shared editor workflows, a real staged Gen IV editor, provider-aware Gen IV source discovery/assignment, the modern Product Home integrated into MAIN, real Party sprites, real Gen I–IV Pokédex progress, trainer-name/portrait presentation, Classic Game Sources, cursor memory, Backpack/Items quick navigation, compact quick actions, two-pane Settings, and a completed full-repository forensic audit with remediation tracked separately.

The next user-facing milestone is **one combined Gen I–IV + Product UI NRO** for physical Switch testing.

## Generation status

### Gen I — hardware accepted
Red / Blue / Yellow read, inventory, Trainer/Party/Boxes, and shared Pokémon View/Create/Edit are hardware accepted.

### Gen II — hardware accepted
Gold / Silver / Crystal read, inventory, generation-native fields and shared View/Create/Edit are hardware accepted.

### Gen III — hardware accepted
Ruby / Sapphire / Emerald / FireRed / LeafGreen read, inventory and shared View/Create/Edit are hardware accepted.

### Gen IV — active full editor
Diamond / Pearl / Platinum / HeartGold / SoulSilver have a working staged editor foundation.

The first safe Party/Box View/Edit milestone is already hardware accepted. G4-04 extends it with Create, additional native fields, moves, forms, species mutation, trainer identity handling, action parity, strict reparse/rollback, and Product UI integration.

The combined G4-04 + Product UI build remains **hardware pending**.

## Product experience

The stable top-level UX direction is:

1. **Product Home** — selected game, trainer identity, Party, Pokédex progress, Open/Launch and major features.
2. **Classic Game Sources** — direct game/source grid for users who prefer the familiar workflow.
3. Existing proven Trainer / Party / Boxes / Inventory / Pokémon editor flows underneath.
4. Compact access to Banks, Backups, Search, More, Items and Settings.
5. Two-pane Settings and contextual help.
6. Remembered navigation state across reconstructed menus.

Master Vault, global Pokédex/collections, Search and other future modules remain non-persistent until their real backends are ready.

## Repository quality and audit

A full tracked-repository forensic audit completed on 2026-09-29 with 746/746 tracked paths accounted, 711/711 text files read, 35/35 non-text entries inspected, and 0 pending ledger entries.

The frozen evidence branch is `audit/full-repository-line-by-line-20260928`.

The separate remediation lane is `fix/full-audit-remediation-20260929`. Its matrix tracks 44 findings across repository integrity, parser/memory safety, save integrity, CI, launch behavior, UI/navigation and engineering documentation.

This detail stays in engineering docs rather than the public README.

## Current technical priorities

Before the next hardware candidate:

- finish exact-head CI;
- verify cursor memory;
- verify Backpack/Items quick entry;
- verify direct DraStic/melonDS launch handoff;
- retain explicit Link Game File as fail-closed fallback;
- integrate high-priority audit remediation forward;
- run full host/sanitizer/Gen-IV/native regression gates;
- ship one exact Actions-built NRO for physical testing.

After hardware acceptance:

- full app-wide touch parity;
- Master Vault / Banks persistence;
- broader provider and later-generation expansion according to the roadmap.

## Safety / scope locks

- original source saves remain immutable;
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
