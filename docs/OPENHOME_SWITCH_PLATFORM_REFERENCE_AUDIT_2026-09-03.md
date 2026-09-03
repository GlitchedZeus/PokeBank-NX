# PokeBank NX — OpenHome / Switch Platform Reference Audit

Date: 2026-09-03  
Scope: research only; no application source changed by this audit.

This audit records additional projects that may reduce future PokeBank NX work around native Switch integration, save lifecycle safety, DS/3DS metadata, modern Switch game data, conversion rules, and offline HOME-style UX.

These projects do **not** change the current Session 2.6 critical path. The next application milestone remains: #23 mutation-UI safety, #24 defensive PLA crash handling, preserve #19 analog work and #13/#16 PokeBank NX shell, then package the exact second device-test `.nro`.

## Classification vocabulary

- **DIRECT REUSE** — code can plausibly be incorporated with normal attribution/license review.
- **ADAPTER / WRAPPER** — useful existing subsystem should be integrated behind a PokeBank NX interface rather than copied wholesale.
- **SELECTIVE PORT** — a bounded implementation/pattern may be ported after license and test review.
- **REFERENCE ONLY** — useful to understand behavior/data/architecture, but do not copy code into PokeBank NX.

---

## andrewbenington/OpenHome

Source: https://github.com/andrewbenington/OpenHome  
License: GPL-3.0  
Classification: **SELECTIVE PORT / REFERENCE**

OpenHome explicitly targets an open-source replacement for Pokémon HOME and advertises Generation 1–9 support, with modern Gen 8/9 support read-only. It intentionally avoids online Nintendo/HOME protocol communication and keeps original source saves on their original hardware.

The current project is desktop/web-oriented, and its documented Rust rewrite uses a separate architecture (`libpk_edit`, `libpk_convert`, `pk_filesys`) plus FlatBuffers/WGPU/iced. That makes it a poor direct dependency for our native Switch `.nro`, but a strong architecture and conversion-rule reference.

High-value lessons for PokeBank NX:

- separate Pokémon editing, conversion, and game-filesystem concerns;
- enforce destination game/species/form compatibility before insertion;
- preserve source/origin semantics rather than allowing arbitrary transformations;
- use generation-aware conversion rules rather than treating `.pk*` records as interchangeable;
- handle odd save-container sizes/revisions defensively;
- maintain offline-first behavior without private HOME/Nintendo protocol impersonation.

Use for #10 conversion/staging, #9 provenance/compatibility, #11 modern Switch read research, #31/#32 DS/3DS cross-checks, and later Vault conversion metadata.

Do not replace PKSM-Core/PKHeX with OpenHome.

---

## suloku/BW_tool

Source: https://github.com/suloku/BW_tool  
License: no explicit license was identified in the inspected repository root.  
Classification: **REFERENCE ONLY**

This is a specialized Pokémon Black/White Gen V save utility, not a general Pokémon entity/save library. It contains useful knowledge around Gen V save subsections and features such as Mystery Gift slots, trainer data, Dream World data, Entralink Forest data, Liberty Ticket/event state, C-Gear skin data, and battle-video handling.

Use for:

- #31 Gen V adapter cross-checking;
- future Mystery Gift/event work;
- Gen V save subsection/flag fixtures;
- independent verification of offsets/checksums where applicable.

PKMDS, PKSM-Core and PKHeX remain stronger primary Gen V references.

---

## semaj14/Multi-PokemonFramework

Source: https://github.com/semaj14/Multi-PokemonFramework  
License: no explicit license was identified in the inspected repository metadata/root.  
Classification: **REFERENCE ONLY**

This is a 3DS runtime/plugin framework rather than an offline save parser. Its project layout separates game-specific logic for titles such as Platinum, XY, ORAS and USUM and includes runtime tools/backup-style functionality.

Potential value:

- game/version-specific adapter separation ideas;
- 3DS title/version identification cross-checks;
- UI organization for multiple games;
- historical 3DS homebrew save-backup/runtime integration clues.

Do not use its runtime-memory cheat model as the PokeBank NX save architecture.

---

## kwsch/pk3DS

Source: https://github.com/kwsch/pk3DS  
License: no explicit license was identified in the inspected repository metadata/root.  
Classification: **REFERENCE ONLY**

pk3DS is primarily a Generation VI/VII 3DS **ROM/data editor**, not a save parser. It supports XY, ORAS, SM and USUM, with some older Gen V remnants.

Its value to PokeBank NX is static game data rather than save I/O:

- species/personal/form data;
- encounter tables and locations;
- move data and learnsets;
- evolutions;
- trainer/game-version data;
- cross-checking where a species/form can be encountered.

This can improve future #7 Pokédex, #9 Summary (`where found` / origin context), legality/compatibility research, and #32 3DS metadata. PKHeX remains the correctness oracle for Pokémon/save semantics.

---

## melsbacksfriend/Eevee

Source: https://github.com/melsbacksfriend/Eevee  
License: GPL-3.0 (`COPYING`)  
Classification: **ADAPTER / WRAPPER + SELECTIVE PORT CANDIDATE**

This is the strongest immediate find in this batch for the post-Session-2.6 engine work. Eevee describes itself as an offline Pokémon HOME alternative for Nintendo Switch and is a native C++ Switch application built around **PKSM-Core** and Borealis.

The inspected example application demonstrates:

- native Switch save-data mounting;
- PKSM-Core initialization on Switch;
- creation of a `Sav8SWSH` object from a mounted Sword/Shield save;
- an app-owned persistent `bank.bin`;
- switching between game storage and bank storage;
- holding/cloning/releasing Pokémon;
- moving a held Pokémon between storage objects;
- explicit `finishEditing()` style persistence calls.

Why this matters:

PokeBank NX was already planning a PKSM-Core integration spike. Eevee is a concrete proof-of-concept showing **PKSM-Core inside an actual Switch `.nro` with a bank-like UI**. It should be inspected alongside PKSM and Pokémon Chest before we invent our own integration layer.

Important safety boundary:

Eevee's direct storage move/edit behavior is **not** the PokeBank NX true-Move safety model. Do not copy a simple `write destination -> clear source` sequence into installed saves. Our eventual live Move still requires destination staging/validation, backup, write, readback verification, rollback/recovery state, and source retirement only after verified destination success.

Use for #4 PKSM-Core integration, #3 Vault/bank migration ideas, #11 native Switch adapter research, and #20 Transfer Workspace UX. It may substantially reduce the risk/cost of the first native PKSM-Core session.

---

## J-D-K/JKSV

Source: https://github.com/J-D-K/JKSV  
License: GPL-3.0-or-later  
Classification: **SELECTIVE PORT / PLATFORM REFERENCE**

JKSV is the strongest Switch filesystem/save-lifecycle reference in this batch. The current `rewrite` branch is an actively developed native Switch save manager.

Relevant patterns observed:

- scoped save mounts;
- save metadata validation before import;
- account/device/BCAT/cache save-type handling;
- save creation when importing metadata-backed backups;
- directory/ZIP backup import;
- explicit copy-and-commit operations;
- recursive backup copying with progress reporting and cancellation/abort state;
- constrained/Applet-mode warnings;
- clean separation between UI/task state and filesystem jobs.

Use JKSV to inform the **Switch platform layer**, not Pokémon format correctness.

High-value destinations:

- #21 diagnostics/reliability/Applet-mode behavior;
- #23 defense-in-depth around save mounting;
- #20 eventual staged/live-write transaction implementation;
- future backup/restore progress UI;
- error handling and scoped mount lifetime.

PokeBank NX will remain stricter than a generic save manager for live Pokémon writes: parser validation and exact readback comparison are still mandatory before any adapter is approved.

---

## WerWolv/EdiZon

Source: https://github.com/WerWolv/EdiZon  
License: repository contains a GPL license; GitHub classifies the project as GPL-2.0. Treat code reuse as **reference-only unless compatibility is reviewed explicitly**.  
Classification: **REFERENCE ONLY**

EdiZon is an older/archived native Switch save manager/editor. Its most useful architectural idea is a configuration/script-driven editor where individual games can describe their save layouts and editing behavior without recompiling the whole application.

Potential PokeBank NX lessons:

- adapter capability descriptors;
- data-driven per-title metadata;
- backup/export UX;
- separation between generic Switch save management and game-specific parsing/editing.

JKSV is the preferred modern Switch save-lifecycle reference. EdiZon is retained mainly for historical architecture comparison.

---

## kwsch/pkNX

Source: https://github.com/kwsch/pkNX  
License: no explicit license was identified in the inspected repository metadata/root.  
Classification: **REFERENCE ONLY**

pkNX is primarily a Nintendo Switch Pokémon **ROM/data editor**, not a save parser. It supports data for LGPE, Sword/Shield, Legends: Arceus and Scarlet/Violet.

Useful data areas include:

- encounters/locations;
- species/personal/form data;
- moves and learnsets;
- trainers;
- evolutions;
- game text/data tables;
- version-specific game structures.

Use it as a modern-Switch static-data oracle alongside PKHeX, especially for #7 Pokédex, #9 Summary/location context, #11 modern title research, and compatibility/legality data. It should not replace the save adapter implementation.

---

# Recommended priority from this batch

```text
Tier A — materially reduces core engineering risk
  Eevee       native Switch + PKSM-Core + offline bank integration
  JKSV        native Switch save lifecycle / mount / backup / import reliability
  OpenHome    multi-generation conversion + HOME-style compatibility architecture

Tier B — strong data/oracle references
  pkNX        modern Switch static game/encounter/form data
  pk3DS       3DS static game/encounter/form data
  BW_tool     specialized Gen V save/event structures

Tier C — supporting/historical architecture
  EdiZon      data-driven Switch save editor patterns
  Multi-PokemonFramework 3DS runtime/title-specific organization
```

# Architecture impact

This strengthens, rather than replaces, the current plan:

```text
PKHeX
  -> correctness / legality / save oracle

PKSM-Core
  -> primary native historical Pokémon/save engine candidate

Eevee + PKSM + Pokémon Chest
  -> three real applications showing how PKSM-Core/bank concepts are integrated

JKSV
  -> Switch save filesystem / backup / mount / task-lifecycle reference

OpenHome
  -> independent HOME-style conversion / compatibility / generation architecture

pk3DS + pkNX
  -> static encounter/location/personal-data references

PokeBank NX
  -> its own safety, Vault, provenance, UI and per-adapter validation layer
```

No project in this audit authorizes weakening `docs/SAVE_SAFETY.md`, enabling global live writes, or skipping device validation.
