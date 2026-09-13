# PokeBank NX — Research Reference Index

Last updated: 2026-09-13

This is the **canonical entry point for external research and reference material used by PokeBank NX**.

The project has accumulated a large amount of Pokémon/homebrew research across GitHub repositories, issue notes, code audits, hardware investigations, and design sessions. This index exists so future work does not depend on remembering which chat or issue originally contained a useful reference.

Before implementing a major parser, save writer, bank/Vault subsystem, conversion engine, legality feature, emulator resolver, Switch save adapter, Pokédex engine, or transfer path, check this index and the linked audit documents first.

> This file is an index, not a claim that every historical link ever submitted has already been recovered one-for-one. When an older useful repository/reference is rediscovered, add it to the appropriate audit and link it here.

---

## Core upstream / project reuse map

### `docs/UPSTREAM_AUDIT.md`

The primary permanent map for projects that may accelerate PokeBank NX. It classifies sources as:

- DIRECT REUSE
- ADAPTER / SELECTIVE PORT
- REFERENCE ONLY

It includes the core reference stack around PKSE, PKSM/PKSM-Core, PKHeX, Pokémon Chest, PHBank/PHBankGBC, pkHouse, pkDex, PKForge and other relevant Pokémon tooling.

Use this first when asking: **“Has somebody already solved this?”**

---

## Bank / Vault / storage research

### `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`

Dedicated audit of Pokémon bank/storage projects and the lessons useful for PokeBank NX Master Vault, Banks, migration and failure handling.

Topics include:

- bank file/version behavior;
- migration/failure cases;
- storage UX;
- backup/recovery ideas;
- PKSM/PHBank/Pokémon Chest-style reference architectures.

Related issues: #3, #17, #27.

### `docs/SAVE_STORAGE_DISCOVERY_RESEARCH_2026-09-13.md`

Newest high-value storage/platform research pass. This is the canonical detailed note for:

- exact PKSE `PKSEBANK` v1 decoder contract;
- legacy pre-`PKSEBANK` flat-bank migration behavior;
- PKSE persisted group tags;
- modern Switch save revision detection;
- emulator resolver/config behavior;
- native Switch save-first enumeration;
- deterministic golden-corpus strategy;
- Master Vault transactional/recovery direction.

Research status from this audit:

```text
PKSE bank format:             IMPLEMENTATION-READY / TESTS NEEDED
Modern save revision model:   GAME-SPECIFIC DETECTORS DEFINED
Emulator discovery:           RESOLVER ARCHITECTURE DEFINED
Native save enumeration:      SAVE-FIRST DESIGN IDENTIFIED
Golden corpus:                GENERATOR STRATEGY IDENTIFIED
Vault recovery:               IMMUTABLE-GENERATION DESIGN REAFFIRMED
```

Related issues: #11, #17, #27, #56.

---

## Save engines / parser oracles

### `docs/SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md`

Independent save-engine audit used to reduce parser risk and provide cross-engine comparison oracles.

Useful when developing:

- Gen I/II/III parsing;
- malformed-save handling;
- serializer round trips;
- conversion/compatibility checks;
- golden fixtures.

The preferred test philosophy is **independent agreement, not majority vote**: if PokeBank NX disagrees with PKHeX/PKSM-Core/another independent implementation, investigate the discrepancy explicitly.

Related issues: #5, #10, #17.

---

## Native Switch / modern game platform research

### `docs/OPENHOME_SWITCH_PLATFORM_REFERENCE_AUDIT_2026-09-03.md`

Research around:

- native Switch architecture;
- modern Switch game data;
- save lifecycle/safety;
- OpenHome/Eevee/pkNX references;
- DS/3DS metadata directions;
- offline HOME-style UX.

Use this alongside `SAVE_STORAGE_DISCOVERY_RESEARCH_2026-09-13.md` when working on modern Switch support.

Related issues: #11, #30, #31, #32.

---

## Emulator/save discovery research

Primary current reference:

### `docs/SAVE_STORAGE_DISCOVERY_RESEARCH_2026-09-13.md`

The desired #56 architecture is resolver-based:

```text
RetroArchResolver
MGBAResolver
MelonDSResolver
DrasticResolver
AzaharResolver
TicoResolver
CustomFolderResolver
```

Configuration outranks guessed defaults. Standalone emulators and libretro cores are separate source types when their save behavior differs.

Known strong references recorded in the audit include:

- RetroArch Switch platform defaults/config
- standalone mGBA config/save path behavior
- standalone melonDS Switch config/save path behavior
- DraSticDS_nx Switch directory layout
- Tico mGBA/Azahar cores
- Azahar NX virtual filesystem layout

Related issue: #56.

---

## Modern save revision / DLC research

Primary current reference:

### `docs/SAVE_STORAGE_DISCOVERY_RESEARCH_2026-09-13.md`

Key rule:

> File size is an identification clue, not a universal revision authority.

Current game-specific direction:

- **SWSH:** historical update-path layouts + DLC block presence;
- **SV:** optional blocks / Kitakami / Blueberry block presence;
- **PLA:** Daybreak block `0x8184EFB4`;
- **BDSP:** explicit revision/size matrix;
- **Z-A:** explicit `KSaveRevision = 0x0926555A`;
- **LGPE:** structural/container validation;
- **Switch FR/LG:** Gen III semantics + Switch source/container handling.

Related issue: #11.

---

## Golden fixtures / fuzz / malformed-save research

Primary references:

- `docs/SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md`
- `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`
- `docs/SAVE_STORAGE_DISCOVERY_RESEARCH_2026-09-13.md`

Preferred corpus model:

```text
generated blank saves
+ synthetic Pokémon-filled saves
+ legally redistributable historical layouts
+ programmatically corrupted variants
```

Fixtures should be deterministic and carry size, SHA-256, expected game/revision/semantics and expected failure class.

Related issue: #17.

---

## Companion / map / guide / move-lab research

### `docs/COMPANION_MOVE_LAB_MAP_GUIDE_RESEARCH_2026-09-03.md`

Reference research for future companion-mode, guide/map, move-lab and related UX/data ideas. This is not part of the current critical path but is intentionally preserved for post-core work.

Related future roadmap issues include #39/#41 and other companion/map work.

---

## Classic save editor architecture

### `docs/CLASSIC_SAVE_EDITOR_ARCHITECTURE.md`

Internal architecture/reference for safe staged classic editing.

Use together with upstream/save-engine audits when implementing:

- Gen I/II/III staged Pokémon editors;
- inventory mutation;
- exact-generation semantic fields;
- staged export/checksum repair;
- source immutability.

---

## Important external references currently pinned by research

These are not runtime dependencies. They are research/oracle/reference projects and should be pinned to exact revisions during implementation audits.

### Core Pokémon engines / editors

- PKSE — inherited upstream foundation/reference
- PKHeX — save/Pokémon format and legality oracle
- PKSM / PKSM-Core — cross-generation/save/bank reference
- Pokémon Chest — DS-era bank/storage reference
- PHBank / PHBankGBC — legacy handheld bank references
- pkHouse — native Switch bank/reference
- pkDex — Pokédex/reference ideas
- PKForge / related tools — creation/editing reference where applicable

### Save/platform references highlighted in the 2026-09-13 audit

- PKHeX `SaveUtil.cs`
- PKHeX SWSH `Zukan8` revision/block logic
- RetroArch Switch platform source/config schema
- mGBA config implementation
- melonDS Switch fork
- DraSticDS_nx
- Tico mGBA / Tico Azahar cores
- Azahar NX experimental fork
- JKSV
- libnx
- pkHouse

See the detailed audit for URLs, constants and implementation notes.

---

## Research-to-code rule

Research is not automatically production truth.

When a reference becomes implementation input:

1. pin the exact upstream commit/revision;
2. confirm license/reuse classification;
3. extract the smallest useful behavior/specification;
4. implement behind PokeBank-owned interfaces;
5. add permanent host tests/fixtures;
6. compare against at least one independent oracle where practical;
7. fail closed on unknown layouts;
8. run sanitizers and native builds;
9. physically test before upgrading a safety-sensitive capability to DEVICE ACCEPTED.

Never enable live writes merely because an upstream project writes the same format.

---

## Current highest-value research-backed implementation opportunities

1. **Gen I full staged boxed-Pokémon editor** — current active classic-editor direction.
2. **PKSE bank decoder/importer core** — now implementation-ready as a read-only parser; Vault is the eventual destination.
3. **`SaveRevisionDetector`** — modern Switch revision safety foundation.
4. **Universal emulator resolver framework** — #56.
5. **Save-first native Switch discovery** — improve profile/orphan/device-save handling.
6. **Golden-corpus generator** — expand #17 across PKSE banks + modern revisions + malformed inputs.
7. **Master Vault/Banks** — use the bank audits and immutable-generation recovery model when this milestone starts.

The current active milestone should not be derailed solely because a later subsystem is now research-ready. Preserve these findings and pull them in when their roadmap phase begins.