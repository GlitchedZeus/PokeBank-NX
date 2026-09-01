# PokeBank NX — Upstream Reuse Audit

Last reviewed: 2026-09-01

This document is the permanent research map for external projects that can accelerate PokeBank NX. Before writing a major Pokémon-format, save-parser, legality, conversion, Pokédex, generated-Pokémon, or bank subsystem from scratch, check this file and the referenced project first.

The pins below are **research pins**, not permanent dependency versions. If a future session wants to update a pin, it must record the new commit and re-run the relevant comparison/regression tests.

## Classification

- **DIRECT REUSE** — source may be integrated when its license is compatible and attribution/notices are preserved.
- **PORT** — behavior/code can be translated into the PokeBank NX architecture with license obligations reviewed first.
- **REFERENCE ONLY** — study behavior, algorithms, formats, UX, tests, or data organization; independently implement the PokeBank NX version.

## Current PokeBank NX context

- Repository: `GlitchedZeus/PokeBank-NX`
- Development branch: `feature/pokebank-playable`
- Verified remote safety milestone: `c618bd5`
- Live game-save writing: hard disabled
- 23 release/platform identities: host tested
- Native `.nro`: builds
- Physical Switch execution of the recovery build: not yet device tested

---

## 1. PKSE

Repository: https://github.com/kiasta/PKSE  
Pinned research commit: `1133aedffeeffb5862f35162597e249233903597` (PKSE 1.1.3)  
Language: C++  
License: **AGPLv3**

### Role

PKSE is the original upstream foundation already imported into PokeBank NX. It provides the native Switch application framework, controller UI, save selection/access, party/box browsing, Pokémon editing/creation, conversion/bank foundations, legality-related code/data, and modern Switch game handlers present in PKSE 1.1.3.

### Classification

**DIRECT REUSE — already inherited into this repository.**

### Policy

- `origin` is PokeBank NX.
- PKSE is **UPSTREAM ONLY**.
- Never push PokeBank NX changes to the PKSE repository.
- Preserve upstream history, notices, and AGPLv3 obligations.

### Audit focus

Before replacing working PKSE code, compare behavior and add regression tests. Existing upstream capability is not automatically proof that it satisfies the PokeBank NX safety/provenance contract.

---

## 2. PKSM-Core

Repository: https://github.com/FlagBrew/PKSM-Core  
Pinned research commit: `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`  
Language: C++  
License: **GPLv3**

### Role

Highest-priority native C++ integration/reuse candidate for historical Pokémon formats and save logic. PKSM-Core is designed as reusable core save-editing code and contains generation-specific Pokémon classes, save classes, conversion paths, personal data, string/i18n support, and related utilities.

### Confirmed high-value files/areas

- `include/pkx/PKX.hpp` / `source/pkx/PKX.cpp` — common Pokémon abstraction
- `include/pkx/PK1.hpp` / `source/pkx/PK1.cpp`
- `include/pkx/PK2.hpp` / `source/pkx/PK2.cpp`
- `include/pkx/PK3.hpp` / `source/pkx/PK3.cpp`
- generation-specific `PK4`, `PK5`, `PK6`, `PK7`, etc.
- `include/sav/` / `source/sav/` — generation/game save implementations
- `include/sav/Sav3.hpp` / `source/sav/Sav3.cpp` — Gen III save/box handling
- `include/utils/genToPkx.hpp` — generation-to-Pokémon helpers/interfaces
- `personals/` — personal/species data
- `strings/` — supporting text resources

### Classification

**DIRECT REUSE / ADAPTER CANDIDATE**, subject to a focused compatibility and architecture audit.

### Required audit before integration

1. Determine which PKSM-Core files can compile cleanly in the PokeBank NX/devkitPro environment.
2. Map PKSM-Core `PKX`/`Sav` objects to PokeBank NX interfaces without exposing live-write paths.
3. Compare Gen I–VIII parsing/serialization with current PKSE/PokeBank behavior.
4. Establish untouched-save round-trip tests.
5. Establish Pokémon parse/serialize golden vectors against PKHeX.
6. Record attribution and GPLv3 source provenance for any directly reused code.

### First practical target

Audit **Gen III / PK3 / Sav3 first**, especially FireRed/LeafGreen, because it directly supports the planned GBA → Vault → Switch FR/LG workflow and gives an early test of the adapter strategy.

---

## 3. PKHeX

Repository: https://github.com/kwsch/PKHeX  
Pinned research commit: `e15d2467b32da7bc26ce7cc8e5c4ede32740e20a`  
Language: C#  
License: **GPLv3**

### Role

Primary technical correctness/reference implementation for Pokémon structures, save structures, conversion, encounters, forms, locations, moves, Mystery Gifts, legality, and generation-specific behavior.

PokeBank NX already contains Python generator tooling that uses PKHeX source/data as an upstream data source. Continue that pattern where it produces deterministic generated data suitable for the native Switch app.

### Classification

**REFERENCE / DATA-GENERATION / HOST-ORACLE.** Do not attempt to port the entire desktop application into the Switch runtime.

### Planned use

Create a host-side `tools/pkhex-oracle/` utility pinned to an exact PKHeX.Core revision. It should provide machine-readable answers and golden test vectors to validate native C++ behavior.

Useful oracle operations:

- `legality <pokemon-file>`
- `inspect <pokemon-file>`
- `convert <pokemon-file> <target-format>`
- `encounters <species> <game>`
- `generate <species> <game> [constraints]`

The Switch app remains native C++ and offline.

---

## 4. Auto Legality Mod / PKHeX-Plugins

Repository: https://github.com/santacrab2/PKHeX-Plugins  
Pinned research commit: `90410f2681a0a72680d12280a1e0f14715e67dff`  
Language: C#  
License: **MIT** in the pinned repository

### Role

Encounter-driven legal Pokémon generation/legalization reference. Useful for future generated collections, encounter selection, trainer/encounter consistency, shiny constraints, and legality-oriented test vectors.

### Classification

**HOST TOOL / REFERENCE; possible direct host-tool reuse under MIT terms.**

### Planned use

Pin AutoMod to a PKHeX.Core revision it supports and compile both into the host-side PKHeX Oracle. Keep this outside the Switch runtime.

High-value future outputs:

- encounter candidates
- generated legal Pokémon test vectors
- shiny-lock behavior
- form constraints
- origin/encounter constraints
- generation/conversion comparison vectors

Do not use the tool to forge Pokémon HOME tracker/history data or to make claims of guaranteed online acceptance.

---

## 5. pkHouse

Repository: https://github.com/Insektaure/pkHouse  
Pinned research commit: `4e288f4a66acf2e58822ad1bb372a6803d3c46dd`  
Language: C++ / Nintendo Switch homebrew  
License: **GPLv2**

### Role

High-value modern Nintendo Switch save/bank reference, especially where PokeBank NX needs behavior around current Switch titles rather than historical Pokémon formats.

### Author guidance

Insektaure has directly encouraged the project to use pkHouse as a reference and recommended **reimplementing needed behavior rather than pure copy/paste**. The author also offered to answer technical questions when implementation details are unclear.

This is useful project guidance, but it is **not recorded here as an explicit AGPLv3 relicensing grant**. Therefore the project policy remains reference/reimplementation.

### Confirmed high-value files/areas

- `include/game_type.h`
  - title IDs
  - save filenames
  - Pokémon file extensions
  - box/slot sizes
  - bank grouping
  - Z-A, SV, SwSh, BDSP, PLA, LGPE and regional Switch FR/LG identities
- `include/save_file.h` and related save implementation
  - family-specific save handling
  - modern block formats
  - box cache/round-trip behavior
- `include/pokemon.h` / Pokémon-format helpers
- `include/bank.h` / bank behavior
- `include/wondercard.h` / Wondercard behavior
- Pokédex registration/update logic
- handling-trainer update logic
- crypto/block helpers such as `poke_crypto`, `sc_block`, `swish_crypto`

At the pinned commit, `include/game_type.h` explicitly describes Z-A (`pa9`), Scarlet/Violet (`pk9`), Sword/Shield (`pk8`), BDSP (`pb8`), Legends Arceus (`pa8`), LGPE (`pb7`), and multiple regional Switch FireRed/LeafGreen title IDs using `pk3` data.

### Classification

**REFERENCE ONLY.**

### Policy

- Do not paste pkHouse GPLv2 source verbatim into PokeBank NX.
- Reimplement required behavior within PokeBank NX's interfaces and safety contract.
- Cross-check important format facts against PKHeX, PKSM-Core, game data, and device/round-trip tests where practical.
- Preserve notes about which pkHouse commit/file informed a reimplementation.
- When a difficult detail cannot be resolved confidently, prepare a concise technical question for Insektaure.

### Highest-value research order

1. Legends Z-A
2. Switch FireRed/LeafGreen outer save/container behavior
3. Scarlet/Violet SCBlock behavior
4. Legends Arceus
5. Sword/Shield
6. BDSP
7. LGPE
8. Pokédex registration
9. Handling Trainer updates
10. Wondercards

---

## 6. pkDex

Repository: https://github.com/Insektaure/pkDex  
Pinned research commit: `c7a1f8debf4e70643464cf939d06f8e167839a15`  
Language: C++ / Borealis / Nintendo Switch homebrew  
License: **GPLv2**

### Role

Pokédex UX, regional organization, controller interaction, and display-data reference.

The pinned project supports region/game datasets including Kanto, FireRed/LeafGreen, Galar/DLC, Sinnoh/BDSP, Legends Arceus, Paldea/DLC, and Legends Z-A. It includes normal/shiny tracking, Alpha/Shiny Alpha where applicable, bulk actions, multi-select, evolution/location/version information, and normal/shiny imagery.

### Useful areas

- `resources/data/*.json` — region/DLC organization and display-data shape
- `app/src/tab/recycling_list_tab.cpp` — controller-oriented list, multi-select, and bulk-action concepts
- data-loading layer under `app/src/data/`
- detail views under `app/src/view/`
- localization structure under `resources/i18n/`

### Classification

**REFERENCE ONLY.**

### PokeBank NX difference

PokeBank NX should not maintain a separate manual tracker as the authority for collection ownership. The **Master Vault is authoritative**. Pokédex views should derive owned/shiny/form/Alpha/event completion from indexed Vault entities.

Use pkDex for UX/organization inspiration and as a secondary data cross-check, not as the technical legality/source-of-truth database.

---

## 7. PKForge

Repository: https://github.com/sofianeelhor/PKForge  
Pinned research commit: `ded13d9cfb75f94df7c2d88b4621866cfd55e499`  
Language: C# / .NET MAUI  
License: **GPLv3**

### Role

Architecture/reference for safe local Pokémon storage and save editing. Its application code is not Switch-native, but its separation of domain/engine/infrastructure/UI concerns is useful for PokeBank NX.

### High-value concepts

- immutable raw Pokémon entities
- stable internal entity IDs
- SHA-256 identity/fingerprints
- display metadata as a cache rather than authority
- provenance sidecars/history
- logical/unlimited boxes over persistent entities
- schema versions and migrations
- backup-before-write
- stage/validate/atomic replace
- engine adapter separating UI/domain code from PKHeX churn
- pinned PKHeX.Core + AutoMod integration

### Classification

**ARCHITECTURE REFERENCE / SELECTIVE PORT OF IDEAS.**

Do not copy the MAUI application structure wholesale. Adapt the invariants to native C++, bounded Switch memory, SD-card filesystems, controller UI, and PokeBank NX provenance requirements.

---

# Recommended division of responsibility

```text
PKSE
  = existing native Switch application foundation

PKSM-Core
  = first native C++ candidate for historical Pokémon/save formats

PKHeX
  = primary correctness oracle and generated-data source

Auto Legality Mod
  = encounter-driven generation/legalization reference for host tooling

pkHouse
  = modern Switch save behavior/research reference

pkDex
  = Pokédex UX/organization reference

PKForge
  = Vault/provenance/transaction architecture reference

PokeBank NX
  = safe integration layer + Vault + provenance + banks + controller UX
```

# Mandatory comparison rule

For every format integrated or substantially changed, build a corpus and compare the same sample among as many of these as applicable:

- PokeBank NX
- PKHeX
- PKSM-Core
- pkHouse (reference comparison where relevant)

Compare at minimum:

- species
- form
- PID
- encryption constant where applicable
- IVs / EVs
- nature
- ability
- gender
- shiny state
- moves
- Ball
- OT
- TID / SID
- met/origin data
- checksum/encryption validity
- serialized size/format
- conversion output

For saves, test:

```text
load → no modification → serialize → reparse → compare
```

Where the save format permits it, untouched saves should remain byte-identical. Any intentional normalization must be documented and tested rather than silently accepted.

# Next audit tasks

- [ ] Build a concrete PKSM-Core integration spike for `PK3` + `Sav3` without live writes.
- [ ] Map PKSM-Core dependencies and Switch build cost.
- [ ] Design/build the PKHeX Oracle.
- [ ] Build shared golden Pokémon vectors for PK3 and a modern Switch format.
- [ ] Document Switch FR/LG container findings from pkHouse and independent tests.
- [ ] Document Z-A save/block findings from pkHouse + PKHeX/reference data.
- [ ] Convert the useful pkDex UX ideas into the PokeBank NX Pokédex spec.
- [ ] Keep all direct-reuse attribution/license decisions explicit in commits and documentation.
