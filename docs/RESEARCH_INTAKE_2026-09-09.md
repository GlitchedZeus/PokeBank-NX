# PokeBank NX — Research intake 2026-09-09

> Research/navigation only. This file does **not** expand the active milestone. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative.

This file preserves a high-value research pass across the future problem areas most likely to consume months if rediscovered from scratch. It is intentionally grouped by subsystem so Codex can open only the relevant section/resource when that subsystem becomes active.

Before copying any external code/data, recheck the exact revision, file-level license, attribution requirements and data/asset rights.

---

# 1. Switch filesystem / safe writing — P0 NOW

Primary references:

```text
switchbrew/libnx
znxDomain/DNS-MITM_Manager
J-D-K/JKSV
```

Dedicated PokeBank note:

```text
docs/SWITCH_FILESYSTEM_SAFE_REPLACE_RESEARCH_2026-09-09.md
```

Key architecture decision:

```text
SafeSdFileReplace
    -> config / binding DB
    -> Vault manifests/indexes
    -> Banks
    -> ordinary SD-card/emulator files

SwitchSaveTransaction
    -> mounted retail Switch title saves
    -> Horizon save FS commit/journal semantics
    -> destination-specific backup/readback
```

Do not collapse these into one generic file writer.

Immediate relevance: the physically observed LeafGreen binding failure occurs while persisting `sdmc:/PKSE/legacy_source_bindings.cfg`.

---

# 2. Modern Switch Pokémon saves — P0/P1

## current upstream `kiasta/PKSE`

High-value native Switch reference. Current upstream documents hardware-validated implementations for Switch FRLG, LGPE, SwSh, BDSP, PLA, SV and Z-A, plus a cross-game bank, creator, legality feedback and generated PKHeX-derived data tables.

License: AGPL-3.0.

Policy: **reference/diff against upstream first**. Never push PokeBank custom code upstream.

## `Insektaure/pkHouse`

Native C++ Switch implementation with current save families including:

```text
LGPE      BEEF blocks
SwSh      SCBlock
BDSP      flat binary + checksum
PLA       SCBlock
SV        SCBlock
Z-A       SCBlock
FRLG      GBA sector saves
```

Also useful for profile selection, backups, banks, checksums, round-trip crypto and mounted-save write behavior.

License: GPL-2.0.

## `Insektaure/pkBakery`

Narrower Z-A reference with SCBlock/SwishCrypto, account selection, automatic backup, in-place mounted-save writing and round-trip encryption verification.

License: GPL-2.0.

Recommended research order:

```text
PKHeX oracle
    ↓
current upstream PKSE
    ↓
pkHouse
    ↓
pkBakery / title-specific native tools
    ↓
PokeBank-owned independently tested adapter
```

---

# 3. Nintendo DS — Gen IV/V — P1

## `FlagBrew/PKSM-Core`

Strong reusable C++ save-engine/reference candidate for Gen IV/V. Relevant implementation areas include redundant-save selection, checksums, general/storage regions, party/box PK4/PK5 data, trainer/language fields and Mystery Gift structures.

License: GPL-3.0; inspect file-level attribution before reuse.

## `NaGaa95/DrasticDS_nx`

Current native Switch DraStic port documents normal cartridge saves separately from savestates:

```text
sdmc:/switch/drastic/user/backup/      normal cartridge saves
sdmc:/switch/drastic/user/savestates/ save states
```

This fits PokeBank's policy: discover real battery/cartridge saves, not savestates as a workaround.

For RetroArch melonDS, use the configured frontend Save directory and normal `.srm` battery saves rather than hardcoding one universal absolute path.

---

# 4. Nintendo 3DS — Gen VI/VII — P1

Primary behavior oracles:

```text
PKSM-Core
PKHeX
XYSAV / KeySAV family as independent historical checks
```

Citra/Azahar-style decrypted save roots commonly mirror the SD title tree and ultimately expose the game's `main` save under the title data directory. Do not trust filename alone: validate expected file size + structure + checksums.

Important test concern: emulator-produced malformed/wrong-size saves can still appear to work inside the emulator that created them. PokeBank should validate the actual format before accepting a source.

---

# 5. Cross-generation conversion — P1/P2 substantial

No single independent native project replaces PKHeX as the broad conversion oracle.

Primary references:

```text
PKHeX
current PKSE
Striaton-Lab-Team/Poke_Transporter_GB
GearsProgress/Pokemon-Community-Conversion-Standard
kwsch/pk2pk (historical)
```

Preferred architecture:

```text
PK3 -> canonical transfer model -> PK4
PK4 -> canonical transfer model -> PK5
PK5 -> canonical transfer model -> PK6
...
```

Every generation boundary should have independent test vectors and explicit loss/change reporting. Avoid one opaque mega-function that converts arbitrary source to arbitrary destination.

Gen III deserves special care because PID can encode nature, gender, shininess and ability slot; rebuilding a Gen III-compatible PID can preserve visible traits while producing a PID/IV relationship that does not correspond to a natural RNG frame.

---

# 6. Offline legality / encounter data — P1

## `projectpokemon/PoGoEncTool`

Very useful **architecture reference**: human-maintainable legality data in JSON -> generated compact PKHeX Legality Binary (`.pkl`).

PokeBank should consider the same pattern:

```text
research/source datasets
        ↓
host-side deterministic compiler/generator
        ↓
versioned compact PokeBank legality pack
        ↓
indexed runtime tables in romfs
```

This keeps heavy data maintenance and generation off the Switch.

## `Admiral-Fish/PokeFinder`

Useful C++ reference for Gen III/IV encounter/RNG generation and validation. Treat as an independent research oracle alongside PKHeX rather than assuming static legality tables are sufficient for every old-generation case.

Generic Pokédex metadata and legality provenance should remain separate datasets.

---

# 7. Mystery Gifts / event preservation — P1 research

Primary references:

```text
projectpokemon/EventsGallery
PKHeX
PKSM
Manu098vm/Switch-Gift-Data-Manager
Eiskasten/wc-beacon
```

EventsGallery remains an excellent technical/provenance source, but redistribution rights must be treated separately from technical availability. Do not wholesale bundle payloads unless the rights/license question is resolved.

PokeBank should build its own generated metadata/index layer and retain the permanent rule:

```text
LEGAL EVENT DATA != WITNESSED OFFICIAL SERVER REDEMPTION
```

Never fabricate HOME trackers, Nintendo-account history or server receipts.

---

# 8. Master Vault crash safety — P1

## `prodeveloper0/uNSS`

Useful transaction-model reference: uploads are revisions with pending/completed state, and only completed revisions can become the latest visible good revision. A pending/incomplete revision does not hide the previous known-good one.

Map the idea to Vault generations:

```text
generation 104 = COMMITTED
generation 105 = PENDING

crash

startup:
105 incomplete -> recover/discard
104 remains current
```

Do not assume SQLite WAL is automatically ideal on Switch. If SQLite is used, deliberately validate the chosen journal/VFS mode on libnx/SD storage.

A strong alternative fits PokeBank's immutable-Vault philosophy:

```text
vault/
    objects/<hash>.pkv        immutable objects
    manifests/vault-*.idx    immutable/replaceable generations
    CURRENT
    CURRENT.bak
```

A failed index commit should not threaten thousands of immutable Pokémon payloads.

---

# 9. Deterministic save fixture corpus — P1

PKHeX already provides test data and can generate language-specific blank saves across many generations.

PokeBank should build a deterministic host-side fixture generator rather than depending only on donated saves.

Examples:

```text
Gen4_Diamond_EN_blank
Gen4_Diamond_JA_blank
Gen4_HGSS_EN_full_boxes
Gen5_B2W2_KO_edge_text
Gen6_X_JA_full_party
Gen7_USUM_DE_events
Gen9_SV_all_forms
```

Mechanically derive negative/edge fixtures:

```text
corrupt one checksum
truncate one byte
duplicate save counters
0 / 1 / 6 party members
empty/full boxes
max inventory
language/Unicode edge cases
shiny/form/gender edge cases
```

Each fixture should have an independent expected-result manifest including game/language/trainer/party/checksum state and expected Pokémon hashes.

This corpus could become one of the highest-value assets in the repository because every future adapter can run against the same regression framework.

---

# 10. GameCube Colosseum/XD — P2, but jackpot references

Dedicated audit:

```text
docs/GAMECUBE_REFERENCE_AUDIT_2026-09-09.md
```

Primary stack:

```text
PKHeX
logdog2325/PokéBridge
TuxSH/PkmGCTools / LibPkmGC
Dolphin memory-card implementation
```

PokéBridge is a modern GameCube C/homebrew implementation with GBA/XD/Colosseum parsing, crypto/checksums, physical memory-card support and a valuable reparse-before-write safety lesson.

PkmGCTools is older but unusually useful as an independent implementation. Its `LibPkmGC` backend is LGPLv3 while the editor is GPLv3; it supports Colosseum/XD, GBA Pokémon and GC↔GBA conversion. Because it is old, Boost-dependent and uses C++ exceptions, default to historical/reference/host-oracle use rather than direct Switch-runtime integration.

---

# Current priority table

| Resource | Value | Priority |
|---|---|---|
| libnx `fs_dev.c` / `fs.h` | definitive filesystem/commit semantics | **P0 NOW** |
| DNS-MITM_Manager | concrete `tmp -> bak -> target` Switch SD replacement pattern | **P0 NOW** |
| JKSV | mature Switch backup/save safety reference | **P0 NOW** |
| current upstream PKSE | modern native Pokémon save/bank/conversion implementation | **P0** |
| pkHouse | native modern save-family implementation | **P0** |
| PKSM-Core | Gen IV-VII C++ save engine/reference | **P1** |
| DrasticDS_nx | exact Switch DS save discovery reference | **P1** |
| PoGoEncTool | generated offline legality-pack architecture | **P1** |
| PKHeX fixture/blank-save machinery | deterministic cross-generation test corpus | **P1** |
| uNSS | pending/committed transaction model | **P1** |
| EventsGallery | event provenance/payload oracle; rights unresolved | **P1 research** |
| PokéBridge | Colosseum/XD C/homebrew save implementation | **P2 / jackpot** |
| PkmGCTools | independent historical Colosseum/XD/GBA oracle | **P2** |

## Rule for Codex

Do not read this entire file during every session.

Use `docs/PROJECT_RESOURCE_INDEX.md` to pick the active subsystem, then read only the relevant section/reference files. Research does not become implementation scope unless `docs/NEXT_CODEX_PROMPT.md` explicitly promotes it.