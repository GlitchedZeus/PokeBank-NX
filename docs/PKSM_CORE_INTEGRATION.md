# PokeBank NX — PKSM-Core Integration Plan

Status: RESEARCH / IMPLEMENTATION PLAN  
Last updated: 2026-09-01

Pinned reference:

```text
FlagBrew/PKSM-Core
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
GPLv3
```

PKSM-Core is the highest-priority candidate for avoiding unnecessary reimplementation of historical Pokémon/save infrastructure in native C++.

---

## Why this is promising

PKSM-Core's own README explicitly documents use from another project.

Required include directories:

```text
memecrypto
include
```

Required source directories:

```text
memecrypto
source and all subdirectories
```

Configuration may be supplied through `PKSMCORE_CONFIG.h` or compiler defines. The required define is:

```text
_PKSMCORE_LANG_FOLDER
```

The library is designed to be largely exception-free; its README calls out a Generation 8 decryption exception case as the notable exception.

The project credits PKHeX as the source from which much code was translated to C++, which makes PKHeX a natural independent comparison oracle for PokeBank NX tests.

---

## Confirmed Pokémon classes

The pinned `include/pkx/` tree includes generation-specific native C++ classes including:

```text
PK1
PK2
PK3
PK4
PK5
PK6
PK7
PK8
PB7
...
```

The generic abstraction is:

```text
include/pkx/PKX.hpp
source/pkx/PKX.cpp
```

For our first spike, focus on:

```text
include/pkx/PK3.hpp
source/pkx/PK3.cpp
```

At the pinned revision, `PK3` exposes:

- stored length 80 bytes
- party length 100 bytes
- encrypt/decrypt/checksum behavior
- PID/TID/SID
- species/items/experience
- ability handling
- IV/EV/moves/PP
- ribbons/contest fields
- shiny/PID-related behavior
- met/location/Ball/language/OT fields
- generation-aware conversion methods including Gen IV through Gen VIII

This overlaps heavily with the Gen III functionality PokeBank NX would otherwise need to recreate.

---

## Confirmed save classes

The pinned `include/sav/` tree includes:

```text
Sav
Sav1
Sav2
Sav3
Sav4
Sav5
Sav6
Sav7
Sav8
...
```

The first integration target is:

```text
include/sav/Sav3.hpp
source/sav/Sav3.cpp
```

`Sav3` at the pinned revision contains explicit handling for:

- two Gen III save slots
- 14 sectors/blocks per slot
- sector/block ordering
- active-save selection
- Gen III checksums
- trainer fields
- party access
- box access
- 14 boxes
- box names/wallpapers
- Pokédex seen/caught state
- item pouches
- Gen III encryption/decryption flow
- save resigning/checksum repair

It also documents the important Gen III edge case where an 80-byte stored Pokémon can cross a `0xF80` used-sector boundary and be split between sector payload regions. Do not recreate this boundary logic casually if PKSM-Core can safely own it.

---

# Proposed architecture

Do **not** let the PokeBank NX UI depend directly on PKSM-Core concrete classes everywhere.

Introduce a narrow adapter layer.

Concept:

```text
PokeBank NX UI / Vault
        |
        v
PokemonRecord / SaveSource interfaces
        |
        v
PKSMCoreAdapter
        |
        +--> pksm::PK1 / PK2 / PK3 ...
        +--> pksm::Sav1 / Sav2 / Sav3 ...
```

This lets PokeBank NX:

- preserve its own stable game identities
- preserve its Vault/provenance model
- keep read/write capabilities separate
- replace/update an engine without redesigning the UI
- compare multiple engines during tests

---

## Proposed PokeBank adapter capabilities

Initial adapter interface should be read-oriented:

```text
probeSave(bytes)
getGameIdentity()
getTrainerSummary()
getPartyCount()
getPartyPokemon(slot)
getBoxCount()
getSlotsPerBox()
getBoxName(box)
getBoxPokemon(box, slot)
exportRawPokemon(box, slot)
```

Do **not** expose generic live write methods in the first integration.

Later, staged/offline editing may add:

```text
cloneSaveToWorkingBuffer()
setWorkingPokemon(...)
serializeWorkingSave()
validateWorkingSave()
```

Live-title writing remains outside this adapter until `SAVE_SAFETY.md` gates are met.

---

# First spike: PK3 parsing without Sav3

The smallest useful integration experiment is direct `PK3` parsing.

Goal:

```text
known 80-byte .pk3 sample
        ↓
PKSM-Core PK3
        ↓
normalized PokeBank fields
        ↓
compare with current PokeBank/PKSE
        ↓
compare with PKHeX Oracle later
```

Compare:

```text
species
PID
TID/SID
nickname
OT
language
shiny
gender
nature
ability
held item
moves
IVs
EVs
Ball
met level/location
checksum validity
```

This proves the build/dependency boundary before integrating an entire save engine.

---

# Second spike: Sav3 read-only FireRed/LeafGreen

Use a copied/test 128 KiB Gen III save fixture.

Goal:

```text
save bytes
   ↓
validate/detect
   ↓
Sav3
   ↓
active slot
   ↓
party + boxes
   ↓
PK3 entities
   ↓
PokeBank normalized view
```

Tests:

- valid save accepted
- truncated save rejected
- malformed/missing sectors rejected
- active slot chosen correctly
- party count matches reference
- first/middle/last box slots match reference
- Pokémon crossing a sector payload boundary parses correctly
- box names match
- trainer identity matches

No save mutation required for this milestone.

---

# Third spike: untouched round trip

Only after read parsing is stable:

```text
load save
   ↓
begin/finish edit with NO semantic changes
   ↓
serialize/resign
   ↓
reparse
```

Compare:

- save structural validity
- sector IDs/order
- checksums
- active save behavior
- party/boxes/trainer semantics
- raw bytes

If raw bytes differ, classify every difference. Do not accept unexplained normalization.

The live installed save is never touched during this test.

---

# Fourth spike: conversion

Use a known PK3 sample and test PKSM-Core conversion into one later-generation representation already understood by the engine.

Compare native output with PKHeX Oracle:

```text
PK3 source
  ├── PKSM-Core conversion
  └── PKHeX Oracle conversion
```

Classify mismatches rather than automatically choosing either result.

---

# Dependency/build questions Sol must answer

- Can the required PKSM-Core source compile cleanly under the current devkitA64 toolchain?
- Which C++ language standard/features are required?
- What additional libraries/submodules are actually needed for PK1-3/Sav1-3?
- Can we initially compile a reduced subset rather than every Gen VIII component?
- How large is the `.nro` size increase?
- What runtime files are required by `_PKSMCORE_LANG_FOLDER`?
- Can PokeBank NX reuse its existing generated data instead of shipping duplicate personal/string data, or would that create unnecessary adapter complexity?
- Are there name/type collisions with existing PKSE/PokeBank classes?
- What attribution/legal notices are required by the reused files, including the additional GPLv3 7.b/7.c notices present in PK3/Sav3 headers?

Record answers in the implementation commit/issue rather than leaving them only in chat.

---

# License/attribution note

PKSM-Core's top-level license is GPLv3. Individual files such as `PK3.hpp` and `Sav3.hpp` also contain additional GPLv3 section 7.b/7.c terms requiring preservation of specified notices/attributions and prohibiting misrepresentation of origin / requiring modified versions to be marked appropriately.

Any direct reuse must preserve the applicable notices and clearly credit PKSM-Core/PKSM contributors.

This document is an engineering plan, not legal advice.

---

# Decision gate

After the Gen III spike, classify PKSM-Core integration:

```text
A. DIRECT ENGINE INTEGRATION
   Best if build size/dependencies are reasonable and behavior/tests are strong.

B. SELECTIVE DIRECT REUSE
   Best if PK3/Sav3 are valuable but full engine integration is too heavy.

C. ADAPTER/REFERENCE ONLY
   Best if integration cost or architecture conflicts outweigh reuse value.
```

Do not start hand-writing a new Gen III engine until this gate is answered.

---

# Success criteria

The PKSM-Core Gen III audit is successful when we have:

- [ ] reproducible pinned source revision
- [ ] license/notice inventory
- [ ] PK3 host compile spike
- [ ] normalized field comparison tests
- [ ] Sav3 read-only fixture test
- [ ] dependency/build-size measurement
- [ ] untouched round-trip result documented
- [ ] PKHeX comparison plan/result
- [ ] explicit integration decision
- [ ] no live game saves modified
