# PokeBank NX — pkHouse Reference Notes

Status: REFERENCE / REIMPLEMENTATION PLAN  
Last updated: 2026-09-01

Pinned reference:

```text
Insektaure/pkHouse
4e288f4a66acf2e58822ad1bb372a6803d3c46dd
GPLv2
```

The pkHouse author, Insektaure, encouraged PokeBank NX to use pkHouse as a reference and specifically recommended reimplementing the behavior we need rather than pure copy/paste. The author also offered to answer technical questions when something is unclear.

Accordingly, pkHouse is **REFERENCE ONLY** for PokeBank NX. This document records concrete files/facts to inspect so future coding sessions do not have to rediscover them.

---

## 1. Game identity / title metadata

Reference file:

```text
include/game_type.h
```

At the pinned revision this file contains a `GameType` table for:

```text
Legends Z-A
Scarlet
Violet
Sword
Shield
Brilliant Diamond
Shining Pearl
Legends Arceus
Let's Go Pikachu
Let's Go Eevee
FireRed / LeafGreen regional Switch releases
```

The associated `GameInfo` table tracks useful implementation facts such as:

```text
title ID
save filename
Pokémon extension
party/stored sizes
box count
slots per box
save-slot size/gap
Wondercard capability
Alpha capability
```

Examples observed at the pinned commit:

```text
Z-A
  title ID: 0x0100F43008C44000
  save file: main
  Pokémon: pa9
  boxes: 32 x 30

Scarlet/Violet
  Pokémon: pk9
  boxes: 32 x 30

Sword/Shield
  Pokémon: pk8
  boxes: 32 x 30

BDSP
  save file: SaveData.bin
  Pokémon: pb8
  boxes: 40 x 30

Legends Arceus
  Pokémon: pa8
  boxes: 32 x 30

LGPE
  save file: savedata.bin
  Pokémon: pb7
  boxes: 40 x 25

Switch FireRed/LeafGreen
  language-specific .sav filenames
  Pokémon: pk3
  boxes: 14 x 30
```

### PokeBank NX use

Cross-check PokeBank's canonical `GameIdentity` table against these facts, but do not blindly replace our identity model. PokeBank NX intentionally keeps release/platform identity separate from display metadata and treats GBA FR/LG and Switch FR/LG as distinct sources.

---

## 2. Save-family dispatcher

Reference files:

```text
include/save_file.h
source/save_file.cpp
```

At the pinned revision, pkHouse dispatches save handling roughly as:

```text
FRLG     -> GBA sector-based path
BDSP     -> flat binary path
LGPE     -> flat binary + block checksum path
otherwise -> SCBlock path
```

The class exposes game-specific box counts/slot sizes, box access, trainer info, raw-data access for flat saves, SCBlock lookup for block saves, a small box cache, and a round-trip verification helper.

### PokeBank NX use

Use this as a checklist for our adapter boundaries. We should not build one giant save parser whose behavior is selected by scattered `if` statements throughout UI code. Game-family behavior belongs behind source/save adapters with explicit capabilities.

---

## 3. Switch FireRed / LeafGreen outer container research

Reference files:

```text
include/game_type.h
include/save_file.h
source/save_file.cpp
```

Observed pkHouse assumptions/behavior at the pinned revision:

```text
save size: 0x20000 (128 KiB)
sector size: 0x1000
used bytes per sector: 0xF80
14 sectors per save slot
2 save slots
storage sectors: IDs 5 through 13
14 boxes
30 slots per box
80-byte stored PK3
```

The read path:

1. requires the expected 128 KiB file size
2. scans each of the two slots
3. verifies all 14 sector IDs are present
4. reads the save counter from sector ID 0
5. selects the valid/newer slot
6. concatenates used payload from storage sector IDs 5–13
7. skips the first four bytes (current box index)
8. exposes the remaining storage as 14 x 30 x 80-byte PK3 slots

The write path in pkHouse writes storage back into sectors and recalculates sector checksums. **PokeBank NX must not copy or enable this write behavior yet.** Use it only as a research hypothesis to validate independently against PKSM-Core/PKHeX/test saves.

### Key question

The Switch FR/LG titles appear to expose a Gen III-style 128 KiB `.sav` file inside the title save environment at this pkHouse revision. We still need device/test evidence that the platform-specific outer storage behavior is correctly understood for every supported regional release.

Do not collapse `firered_switch` into `firered_gba` merely because the internal `.sav` resembles Gen III.

---

## 4. Modern SCBlock-family research

Reference files:

```text
include/save_file.h
source/save_file.cpp
swish_crypto.*
sc_block.*
```

pkHouse uses SCBlock-based handling for several modern titles and locates box/layout/status blocks by key.

Observed reference concepts:

- encrypted file → decoded SCBlock collection
- find box block by key
- find box-layout block separately
- retain original file bytes for round-trip comparison
- encrypt blocks back to a same-size representation
- use family-specific box block keys where needed (for example Legends Arceus differs from the common key)

### PokeBank NX use

For Z-A/SV/SwSh/PLA adapters:

1. identify exact block keys and expected lengths from independent references
2. read only first
3. compare parsed boxes/trainer data against PKHeX
4. create untouched round-trip tests
5. only later implement staged mutation

Do not expose live title write capability while this research is being integrated.

---

## 5. BDSP reference

Reference:

```text
source/save_file.cpp
```

The pinned code recognizes multiple BDSP save sizes and uses fixed offsets for box/layout data plus an MD5 field on save serialization.

### PokeBank NX use

Treat each accepted save-size/version as an explicit tested variant. Never assume one hard-coded offset works for all future versions merely because one fixture succeeds.

Before integrating:

- collect known-size fixtures where legally redistributable or synthetic
- compare trainer/box parsing with PKHeX
- test unsupported size rejection
- keep all writes staged/offline

---

## 6. LGPE reference

Reference files:

```text
include/save_file.h
source/save_file.cpp
```

Observed reference behavior:

```text
save size: 0x100000
flat Pokémon storage
40 boxes x 25 slots
party represented through pointers into the flat storage
21 checksum-described blocks
CRC16-based block checksum handling
```

pkHouse also compacts occupied storage before saving and updates party pointers.

### PokeBank NX use

Read-path research is immediately useful. Save compaction/write behavior is high risk and must be independently verified before any staged-write implementation.

Important tests:

- flat-index ↔ box/slot mapping
- party pointer mapping
- empty slots
- no-change round trip
- checksum validation
- compaction semantic equivalence

---

## 7. Box cache / performance idea

Reference:

```text
SaveFile::getCachedBox
```

pkHouse caches a small bounded number of decrypted boxes instead of repeatedly decrypting every Pokémon on every access.

### PokeBank NX use

This is a useful performance pattern, not a format rule. PokeBank NX should use bounded caches and lazy loading throughout:

```text
Home -> metadata only
open game -> source metadata
open box -> parse/cache that box
leave/evict -> bounded cache policy
```

Do not hold every save's every Pokémon object in RAM at startup.

---

## 8. Handling Trainer behavior

Reference files:

```text
include/handler_update.h
source/handler_update.cpp
```

At the pinned revision the documented behavior is:

- if Pokémon matches destination save trainer identity, mark OT as current handler
- otherwise populate the destination trainer as Handling Trainer, reset friendship to species base value, and clear HT memories as appropriate
- Gen III FR/LG is a no-op because Gen III has no Handling Trainer data

### PokeBank NX use

Do not blindly port the code. Use this as a checklist when implementing destination-game insertion semantics and compare against PKHeX's current `UpdateHandler` behavior for the exact target format.

Handling Trainer updates belong in destination conversion/insertion, not in generic Vault storage.

---

## 9. Pokédex registration behavior

Reference files:

```text
include/pokedex.h
source/pokedex.cpp
```

The pinned `pokedex.cpp` explicitly states that its implementations were ported from PKHeX.Core for multiple game families, including Z-A, Scarlet/Violet, Sword/Shield, BDSP, LGPE, and Gen III FRLG.

It contains game-specific logic for:

- caught/seen forms
- language flags
- gender seen/display behavior
- shiny seen flags
- Alpha-related state where applicable
- game-specific species/form presence
- special multi-form behavior

### PokeBank NX use

There are two separate Pokédex concepts in PokeBank NX:

1. **PokeBank's own collection Pokédex** — derived from Master Vault; specified in `POKEDEX_SPEC.md`.
2. **Game-save Pokédex registration** — a future destination-save side effect when inserting a Pokémon.

Do not mix them.

When game-save writing is eventually enabled, registration rules must be target-game specific and validated against current PKHeX/game behavior.

---

## 10. Wondercards

Reference:

```text
include/wondercard.h
```

pkHouse supports Wondercard-related behavior for its supported modern game families.

### PokeBank NX use

Event support should initially import/store event Pokémon/Wondercard provenance into the Vault, not immediately inject into live saves.

Future game-save Wondercard insertion requires its own adapter validation and should not be bundled into basic Vault event support.

---

# Reference-only workflow for Sol

When using pkHouse to implement a feature:

```text
1. Record pkHouse pinned commit + exact file(s)
2. Describe the observed behavior in neutral terms
3. Find an independent source/test where practical
4. Design the PokeBank NX interface
5. Implement independently
6. Add regression/golden tests
7. Record what was cross-checked
```

Do not copy comments/tables/functions wholesale merely because the author said "go for it". The preferred project path is independent reimplementation informed by the reference.

---

# Questions worth asking Insektaure later

Only ask when our own audit cannot resolve the issue. Good focused questions include:

- For Switch FR/LG, are there title-update/version-specific outer save-container differences beyond the 128 KiB Gen III `.sav` exposed by the current titles?
- Which Z-A SCBlock keys/structures were directly verified against current game data versus inferred/ported from PKHeX?
- Were any SCBlock serialization edge cases observed specifically on real Switch save mounts that do not appear when editing backup files?
- Which pkHouse save-family implementation does the author consider least trustworthy / most shortcut-heavy today?
- Are there known title-version offsets/sizes that have already changed since the pinned commit?

Do not ask broad "how does your app work?" questions when the source/tests can answer them.

---

# Immediate implementation priorities informed by pkHouse

```text
1. Keep current live-write hard lock
2. Use PKSM-Core for first historical Gen III engine spike
3. Use pkHouse to cross-check Switch FR/LG container expectations
4. Build read-only Z-A/SV/SwSh/PLA/BDSP/LGPE adapter tests
5. Compare modern format behavior against PKHeX
6. Add Handling Trainer/Pokédex registration only in destination-save staging later
```

This keeps pkHouse extremely valuable without importing its GPLv2 implementation into the AGPLv3 PokeBank NX codebase.
