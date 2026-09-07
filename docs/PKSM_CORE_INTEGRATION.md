# PokeBank NX — PKSM-Core Integration Plan

Status: GEN III READ-ONLY HOST SPIKE IMPLEMENTED
Last updated: 2026-09-07

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

## Session 3A implementation result

Decision: **ADAPTER-WRAPPER**.

The pinned repository is now a recursive Git submodule at `vendor/PKSM-Core`. PokeBank code sees it
only through:

```text
include/Integration/Gen3/PKSMGen3Adapter.h
src/Integration/Gen3/PKSMGen3Adapter.cpp
```

No PKSM-Core type appears in the public adapter header or UI. The implementation uses `Sav3` /
`SavFRLG` to enumerate the party and boxes and `PK3` for Pokémon semantics and byte round trips.
The wrapper adds validation the pinned `Sav3::isValid()` does not provide: sector signatures,
per-sector checksums, consistent counters and wrap-aware active-slot selection. When a newer slot
fails strict validation but an older one is sound, the wrapper masks only the rejected slot in the
private Core copy so Core cannot reselect it by counter. Caller/source bytes are never changed.

### Deterministic fixture

The host test generates the smallest useful legal-structure fixture in memory; no personal save is
stored in the repository.

```text
Type:             synthetic deterministic FireRed/LeafGreen-family GBA save
Size:             131072 bytes (0x20000)
SHA-256:          b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
Older slot:       counter 7, unrotated, distinct Bulbasaur PID
Active slot:      counter 9, rotation 5, Bulbasaur
Party:            one 100-byte PK3
Boxes:            one aligned PK3 and one PK3 split across the 0xF80 sector boundary
```

Expected assertions cover species, PID, TID, SID, EXP, normalized and raw Gen III held item,
moves, PP, IVs, EVs, nickname and OT. Copies generate truncated, bad-checksum, bad-signature,
invalid-ID, duplicate/missing-ID, counter-mismatch, unsupported-game and malformed-PK3 cases.

### Independent check and round trip

The existing PokeBank/PKSE `Encryption3FRLG` implementation independently decrypts the extracted
fixture PK3, agrees on the canonical fields, and re-encrypts to the exact source bytes. Separately,
PKSM-Core `PK3` decrypt -> clone -> encrypt is **BYTE IDENTICAL** for both 80-byte box and 100-byte
party records. No save resign or write API is called.

### Build/dependency findings

- C++20 compiles on host.
- Required nested dependencies are pinned Core submodules `memecrypto` and `pcg-cpp`.
- Configuration also requires `_PKSMCORE_PERSONAL_FOLDER` for this pinned tree in addition to the
  README-documented `_PKSMCORE_LANG_FOLDER`.
- `memecrypto.c` must be compiled as C or with C++ alternative operator names disabled because its
  helper is named `xor`; the host integration uses `-fno-operator-names`.
- The public PokeBank boundary avoids name/type collisions.
- Full host Core relocatable object: about 2.7 MiB optimized; final Gen III host test after section
  garbage collection: about 740 KiB (toolchain-dependent development measurements).
- The native application does not compile this adapter yet, so current `.nro` impact is 0 bytes.
  A bounded full-Core devkitA64 probe failed under the application's required `-fno-exceptions`:
  `source/personal/personal.cpp` throws on a personal-data size mismatch, and unrelated Gen VIII
  `source/utils/crypto_swsh.cpp` also throws. A Gen III-only, exception-free static slice is the next
  task; linking all generations would unnecessarily pull those dependencies into the Switch build.
- GPLv3 and the additional 7.b/7.c notices remain in the pinned submodule; the wrapper marks itself
  as derived integration work and preserves attribution.

---

# Success criteria

The PKSM-Core Gen III audit is successful when we have:

- [x] reproducible pinned source revision
- [x] license/notice inventory
- [x] PK3 host compile spike
- [x] normalized field comparison tests
- [x] Sav3 read-only fixture test
- [x] dependency/build-size measurement
- [x] untouched PK3 round-trip result documented
- [x] independent existing-parser comparison result
- [x] explicit integration decision: ADAPTER-WRAPPER
- [x] no live game saves modified
- [ ] devkitA64 Gen III Core slice compiled and measured
- [ ] real redistributable FRLG fixture added when one is available
