# RetroArch source naming and identity

This document records the product semantics for file-based RetroArch/legacy sources so deduplication does not accidentally hide legitimate saves.

## Core rule

**Deduplicate aliases of the same underlying source, not merely matching game IDs, trainer names, or matching bytes.**

Two separately stored save files are allowed to appear as separate PokeBank NX sources even when their contents are currently identical. A single file reached through overlapping RetroArch roots, path aliases or equivalent traversal routes should appear only once.

## Source kinds

PokeBank NX should conceptually distinguish at least:

```text
BATTERY_SAVE        normal .sav / .srm game save
SAVE_STATE          RetroArch/core state snapshot such as content-name.state#
BACKUP              separately stored backup/copy when intentionally supported
MANUAL_IMPORT       user-selected external file
```

Current FRLG production support is **BATTERY_SAVE only** (`.sav` / `.srm`). Save-state support is future work and must not be added merely to fix the current duplicate-card bug.

RetroArch documentation distinguishes frontend save files such as `.srm` from state files such as `content-name.state#`. Savestates are emulator/core snapshots and may require core/version-specific handling to recover a trustworthy Pokémon save image.

## Desired user-facing labels

When metadata is available, source cards should make distinct sources obvious. Example concepts:

```text
Pokémon FireRed — WILL
Main Save — RetroArch

Pokémon FireRed — WILL
Save State 1 — RetroArch

Pokémon FireRed — WILL
Save State 2 — RetroArch

Pokémon FireRed — WILL
Backup — RetroArch
```

Equivalent layouts matching the existing card UI are fine. The important information is:

```text
game
trainer/character name when reliably readable
source kind
save-state slot/number when applicable
backup discriminator/timestamp when applicable
platform/origin (Game Boy Advance / RetroArch)
```

Do not invent a trainer name when it cannot be reliably extracted.

## Savestate handling boundary

A RetroArch save state is not automatically equivalent to a `.sav` / `.srm` file. Before PokeBank NX exposes Pokémon from a state snapshot, a future implementation must:

1. identify the state format/core/version reliably;
2. recover the emulated cartridge save memory or another trustworthy game-save image;
3. pass the same strict generation/game structural validation used by normal file sources;
4. keep the original state file immutable;
5. clearly label the source as a save state and preserve the slot number/metadata;
6. refuse unsupported/ambiguous state formats rather than guessing.

Do not parse arbitrary core-state memory as a Pokémon save without an audited adapter.

## Current FRLG blocker semantics

For the immediate FRLG profile/deduplication fix:

- keep scanning `.sav` / `.srm` only;
- make legacy file sources app-global rather than Nintendo-user scoped;
- collapse only aliases/duplicate discovery of the same underlying file;
- preserve separately stored save files even if the bytes happen to match;
- do not dedupe by `firered_gba` / `leafgreen_gba`, trainer name or file hash alone;
- design the source descriptor so a future `SAVE_STATE` kind + slot label can be added without changing the identity model.
