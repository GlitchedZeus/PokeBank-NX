# PokeBank NX Project Status

Last updated: **2026-10-01**

For exact active heads and CI evidence, read `CURRENT_STATUS.md` first.

## Project identity

- Product: **PokeBank NX**
- Repository: `GlitchedZeus/PokeBank-NX`
- Platform: Nintendo Switch homebrew
- State: active alpha
- MAIN development lane: **PR #92**
- Audit remediation lane: **PR #101**
- Both remain **OPEN / DRAFT / NOT MERGED**

GitHub is authoritative. Checkpoint SHAs are evidence, never instructions to move a branch backward.

## Headline

PokeBank NX now has a hardware-accepted Gen I–III foundation, a substantially complete staged Gen IV editor, a modern Switch-style Product UI, and a fully validated audit-remediation package ready for later integration.

The project is currently in **finishing-polish / integrated-hardware-candidate** territory.

## What is already solid

### ✅ Gen I–III

Red/Blue/Yellow, Gold/Silver/Crystal, and Ruby/Sapphire/Emerald/FireRed/LeafGreen have hardware-accepted browsing and staged shared-editor workflows.

### 🧪 Gen IV

Diamond/Pearl/Platinum/HeartGold/SoulSilver currently support the staged full-editor foundation:

- Party / Boxes;
- View / Create / Edit;
- native Gen IV fields;
- move compatibility and PP handling;
- Species mutation;
- supported Form editing;
- trainer/origin inspection;
- checksum/reparse/rollback behavior;
- immutable external source saves.

The earlier safe Party/Box milestone is hardware accepted. The full G4-04 + current Product UI combination still needs its exact physical acceptance pass.

### ✨ Product experience

Current MAIN includes:

- game-focused Product Home;
- region-aware hero presentation;
- grounded trainer portraits;
- larger Party presentation;
- stronger handheld typography and Party-name hierarchy;
- real Gen I–IV Pokédex progress;
- Open / Launch actions;
- Classic Game Sources;
- cursor-memory state;
- Backpack/Items quick entry;
- compact Settings access;
- two-pane Settings.

Current PR #92 head is automated-green under Host Tests, the Gen IV candidate gate, and native Product UI build/package validation.

## Audit / hardening

The forensic audit itself is frozen and complete.

The remediation lane currently records:

- **43 VERIFIED**
- **1 DEFERRED WITH JUSTIFICATION**
- **0 OPEN**

The validated remediation application head passes Host Tests, ASan/UBSan, native devkitA64 compile/link, Product UI packaging, and focused Gen I/II regression suites.

This work improves parser boundaries, memory safety, source-mutation policy, backup/custody behavior, regression coverage, generated-data checks, and native build confidence.

## Current milestone

The next milestone is:

> **One polished, automated-green Gen I–IV + Product UI NRO that passes physical Switch testing.**

Before that candidate is accepted:

- finish the remaining UI polish;
- preserve current Gen IV behavior;
- keep source writes locked;
- verify trainer/region/Party presentation;
- verify Classic Game Sources and Settings/Items flows;
- verify DraStic/melonDS direct launch handoff;
- retain explicit **Link Game File** fallback whenever content identity is ambiguous.

After that exact hardware pass:

1. integrate the completed remediation package into the MAIN development lane;
2. begin full app-wide touch-control parity;
3. proceed to Master Vault / named Banks;
4. expand providers and later generations according to the roadmap.

## Safety / scope locks

- original source saves stay immutable;
- external emulator saves remain read-only;
- installed-game live writes remain disabled;
- staged editing stays app-owned;
- Launch never grants write permission;
- ambiguous source/content matching fails closed;
- cross-game True Move remains locked;
- source injection remains locked;
- Gen V is not started;
- Master Vault persistence is not started.

## Hardware acceptance rule

CI green is necessary, but it is not hardware acceptance.

A feature becomes device accepted only when the **exact Actions-built NRO for the exact application commit** is physically tested on a real Switch.
