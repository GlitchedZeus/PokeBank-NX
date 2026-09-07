# RetroArch source grouping, naming and identity

This document records the product semantics for file-based RetroArch/legacy sources so discovery, deduplication and future save-state support match the intended PokeBank NX UX.

## Top-level UX rule

**One top-level Game Source card per game identity/source family, not one top-level card per save instance.**

Examples:

```text
Game Sources
  Pokémon FireRed
    Game Boy Advance · RetroArch

  Pokémon LeafGreen
    Game Boy Advance · RetroArch
```

Opening a RetroArch game card should show the available save instances for that game:

```text
Pokémon FireRed — RetroArch

  WILL — Main Save
  WILL — Save State 1
  WILL — Save State 2
  WILL — Save State 3
  WILL — Backup 2026-09-07
```

Only after selecting one of those save instances does the user enter the trainer/source view and Party/Boxes flow.

This means the current duplicate top-level FireRed/LeafGreen cards are not the desired final UX even when multiple legitimate saves exist. Legitimate separate saves belong **inside the one game card**.

## Source-instance identity rule

Within a game card, deduplicate aliases of the same underlying source, not merely matching game IDs, trainer names or matching bytes.

Two separately stored save files are allowed to remain separate child entries even when their contents are currently identical. A single file reached through overlapping RetroArch roots, path aliases or equivalent traversal routes should appear only once in the child list.

Never deduplicate child saves merely by:

```text
game ID
trainer name
raw file hash alone
```

A game card groups by game identity/source family. A child entry identifies a distinct save instance.

## Source kinds

PokeBank NX should conceptually distinguish at least:

```text
BATTERY_SAVE        normal .sav / .srm game save
SAVE_STATE          RetroArch/core state snapshot such as content-name.state#
BACKUP              separately stored backup/copy when intentionally supported
MANUAL_IMPORT       user-selected external file
```

Current FRLG production support is **BATTERY_SAVE only** (`.sav` / `.srm`). Save-state support is future work and must not be added merely to fix the current duplicate-card bug.

RetroArch save states are emulator/core snapshots rather than ordinary Pokémon save files, so they require audited extraction/validation before PokeBank NX may expose their Pokémon data.

## Desired child labels

When metadata is available, each child entry inside the game card should clearly identify what it is.

Examples:

```text
WILL — Main Save
RetroArch Battery Save

WILL — Save State 1
RetroArch Save State

WILL — Save State 2
RetroArch Save State

WILL — Save State 3
RetroArch Save State

WILL — Backup
RetroArch Backup
```

Useful child metadata:

```text
trainer/character name when reliably readable
source kind
save-state slot/number when applicable
backup discriminator/timestamp when applicable
path/file hint when useful for disambiguation
last-modified time when trustworthy/useful
```

The parent game card already carries game/platform/source-family identity, for example:

```text
Pokémon FireRed
Game Boy Advance · RetroArch
```

Do not invent a trainer name when it cannot be reliably extracted.

## Selection flow

Desired flow:

```text
Game Sources
  -> Pokémon FireRed — Game Boy Advance · RetroArch
  -> Save Instances
       -> WILL — Main Save
       -> WILL — Save State 1
       -> WILL — Save State 2
       -> WILL — Save State 3
  -> choose one
  -> trainer/source view
  -> Party / Boxes
```

For a game with only one save instance, it is still acceptable to open the game card and show a one-entry child list for consistency. A future UX optimization may optionally auto-open a sole child only if that remains clear and does not undermine the hierarchy.

## Savestate handling boundary

A RetroArch save state is not automatically equivalent to a `.sav` / `.srm` file. Before PokeBank NX exposes Pokémon from a state snapshot, a future implementation must:

1. identify the state format/core/version reliably;
2. recover the emulated cartridge save memory or another trustworthy game-save image;
3. pass the same strict generation/game structural validation used by normal file sources;
4. keep the original state file immutable;
5. clearly label the child source as a save state and preserve the slot number/metadata;
6. refuse unsupported/ambiguous state formats rather than guessing.

Do not parse arbitrary core-state memory as a Pokémon save without an audited adapter.

## Current FRLG blocker semantics

For the immediate FRLG profile/deduplication fix:

- keep scanning `.sav` / `.srm` only;
- make legacy file sources app-global rather than Nintendo-user scoped;
- expose **one FireRed RetroArch parent card** and **one LeafGreen RetroArch parent card** when those identities are present;
- place distinct validated `.sav` / `.srm` save instances beneath the matching parent card;
- collapse only aliases/duplicate discovery of the same underlying file inside the child list;
- preserve separately stored save files as separate children even if their bytes happen to match;
- do not dedupe child saves by trainer name or file hash alone;
- design the child-source descriptor so future `SAVE_STATE` entries with slot numbers and `BACKUP` entries can be added without changing the parent-card model;
- do not add actual `.state#` parsing during the immediate duplicate/profile blocker fix.
