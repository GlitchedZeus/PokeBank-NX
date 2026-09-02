# PokeBank NX — Upstream Reuse Audit

Last reviewed: 2026-09-02

This is the permanent research map for external projects that can accelerate PokeBank NX. Before writing a major Pokémon-format, save-parser, legality, conversion, Pokédex, generated-Pokémon, bank, or transfer subsystem from scratch, check this file and the companion bank-project audit first.

Companion audit added 2026-09-02:

```text
docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md
```

The pins below are **research pins**, not permanent dependency versions. Any future pin change must record the new revision and rerun relevant comparison/regression tests.

## Classification

- **DIRECT REUSE** — source may be integrated when licenses/attribution requirements are compatible and preserved.
- **ADAPTER / SELECTIVE PORT** — reuse or translate a bounded subsystem behind PokeBank NX interfaces after review.
- **REFERENCE ONLY** — study behavior, algorithms, formats, UX, tests or data organization; independently implement the PokeBank NX version.

---

# Current PokeBank NX context

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Version: 0.1.0-alpha
Live installed-game writing: HARD DISABLED / not approved
Stable release/platform identities: 23
First exact physical .nro test: COMPLETE / PARTIAL PASS
Current useful UI/analog source: 361c6f55...
Second device artifact: PENDING safety/crash finish
```

First exact device-tested source:

```text
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256 df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Do not use upstream capability as proof that PokeBank NX meets its read-only/provenance/write-safety contract. The extended hardware test proved inherited mutation UI remains reachable and is now explicitly audited under #23.

---

# 1. PKSE

Repository: `https://github.com/kiasta/PKSE`  
Pinned research commit: `1133aedffeeffb5862f35162597e249233903597` (PKSE 1.1.3)  
Language: C++  
License: **AGPLv3**

## Role

Original upstream foundation already inherited into PokeBank NX:

- native Nintendo Switch application framework;
- save selection/access;
- Party/Box browsing;
- editor/creation/conversion/bank foundations;
- legality-related code/data;
- modern Switch handlers present in the inherited version.

## Classification

**DIRECT REUSE — already inherited into this repository.**

## Policy

- `origin` is PokeBank NX.
- PKSE is **upstream only**.
- Never push PokeBank NX changes to PKSE.
- Preserve upstream history/notices/AGPLv3 obligations.
- Existing PKSE mutation UI must not bypass current PokeBank NX read-only policy.

---

# 2. PKSM-Core

Repository: `https://github.com/FlagBrew/PKSM-Core`  
Pinned research commit: `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`  
Language: C++  
License: **GPLv3**

## Role

Highest-priority native historical Pokémon/save engine candidate.

High-value areas:

```text
include/pkx/PKX.hpp
include/pkx/PK1.hpp
include/pkx/PK2.hpp
include/pkx/PK3.hpp
source/pkx/PK3.cpp
include/sav/
source/sav/
include/sav/Sav3.hpp
source/sav/Sav3.cpp
include/utils/genToPkx.hpp
personals/
strings/
```

## Classification

**DIRECT REUSE / ADAPTER CANDIDATE**, subject to focused compatibility/build/license review.

## Required integration audit

1. Determine the smallest PKSM-Core set that compiles cleanly under the PokeBank NX/devkitPro environment.
2. Hide PKSM-Core objects behind PokeBank NX source adapters.
3. Do not expose PKSM-Core save-write capabilities while the PokeBank NX adapter is read-only.
4. Compare parsed Pokémon/save data with PKHeX/golden fixtures.
5. Establish untouched-save round-trip expectations.
6. Record exact source provenance/attribution for reused code.

First target: **Gen III / PK3 / Sav3 / FireRed-LeafGreen GBA** under #4.

---

# 3. FlagBrew/PKSM — full application

Repository: `https://github.com/FlagBrew/PKSM`  
Research pin inspected: `47d060ad80bcd55ed5c154d40ded0462d2448d21`  
Language: C++ / Nintendo 3DS  
License: **GPLv3 with additional 7.b / 7.c attribution/origin terms**

## Role

Full application reference around PKSM-Core. High value beyond the Core library itself:

- mature offline Banks;
- BankFile versioning/migration;
- corruption handling and typed parser errors;
- backup-before-save and backup rotation;
- SHA-256 change detection;
- bank box naming;
- historical Wondercard/event behavior;
- search/sort/editor flows;
- long-lived constrained-Nintendo-platform integration patterns.

Current PKSM BankFile work separates pure format parsing from GUI/I/O and has host tests for invalid box counts, short/truncated files, migrations, bad magic and newer versions.

## Classification

```text
FULL APP: REFERENCE / SELECTIVE ADAPTER SOURCE
PKSM BankFile: possible reuse/import candidate after license review
3DS-specific UI/archive code: REFERENCE ONLY for Switch architecture
```

Do **not** replace the Master Vault with PKSM's flat Bank format. Use its defensive parser/test/backup ideas and consider a future PKSMBANK importer after Vault v1.

Full notes: `BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`.

---

# 4. Universal-Team/pkmn-chest

Repository: `https://github.com/Universal-Team/pkmn-chest`  
Research pin inspected: `a847bb63a021a39375a5cb6673b2ed3f24195a2f`  
Language: C++ / Nintendo DS(i)  
License: **GPLv3**

## Role

Very high-value integration/UX reference because Pokémon Chest is a Gen III–V Bank and embeds PKSM-Core as a submodule.

Useful lessons:

- proof that PKSM-Core can be wrapped by another Nintendo-platform application;
- clear Game-save side vs Bank side architecture;
- bank-backed Pokémon selection through shared PKX interfaces;
- sorting/filtering and Summary flows;
- GBA/DS save access patterns as historical context;
- dedicated bank/game workspace concepts.

## Classification

**REFERENCE / ADAPTER PATTERN.**

During #4, compare both full PKSM and Pokémon Chest before inventing a PokeBank NX PKSM-Core wrapper from scratch.

Full notes: `BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`.

---

# 5. gocario/PHBank

Repository: `https://github.com/gocario/PHBank`  
Research pin inspected: `a956ab39060148c8dafdb1f472c96632b22dcc65`  
State: archived/historical  
Language: C/C++ / Nintendo 3DS  
License: **GPLv3**

## Role

Strong UX reference for the desired offline Game-PC ↔ Bank experience:

- single Pokémon movement;
- multi-Pokémon movement;
- whole-box PC↔Bank operations;
- explicit Bank vs game views;
- IV/EV Summary;
- save/exit/backup workflow.

## Classification

**REFERENCE / UX REFERENCE.**

Do not treat its old XY/ORAS save code as a modern correctness authority. The highest-value idea is a future dedicated **Transfer Workspace** for Game ↔ Vault/Bank operations, implemented through the stricter PokeBank NX transaction model.

Full notes: `BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`.

---

# 6. 0xb01u/PHBankGBC

Repository: `https://github.com/0xb01u/PHBankGBC`  
Research pin inspected: `9baf997c470b3eaf26c97a9867e35f6acecf527b`  
Default research branch inspected: `GEN2`  
Language: C / Nintendo 3DS

## Role

Secondary historical Gen I/II cross-check for:

- party/current-box/PC-box offsets;
- trainer/language differences;
- Pokédex/checksum locations;
- Gen I Pokémon extraction layout;
- bank concepts.

## Classification

**REFERENCE ONLY.**

Some historical constants/comments are uncertain and the inspected root did not provide a clear modern licensing signal suitable for direct reuse. Any useful fact must be cross-checked against PKSM-Core, PKHeX/public format research and actual fixtures before adoption.

Full notes: `BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`.

---

# 7. PKHeX

Repository: `https://github.com/kwsch/PKHeX`  
Pinned research commit: `e15d2467b32da7bc26ce7cc8e5c4ede32740e20a`  
Language: C#  
License: **GPLv3**

## Role

Primary technical correctness/reference implementation for:

- Pokémon structures;
- save structures;
- conversions;
- encounters/forms/locations/moves;
- Mystery Gifts;
- legality;
- generation-specific behavior.

## Classification

**REFERENCE / DATA-GENERATION / HOST ORACLE.**

Do not port the desktop application into the Switch runtime. Build `tools/pkhex-oracle/` for machine-readable host comparisons.

Planned Oracle operations:

```text
inspect
legality
convert
encounters
generate
```

Issue: #5.

---

# 8. Auto Legality / PKHeX-Plugins

Repository: `https://github.com/santacrab2/PKHeX-Plugins`  
Pinned research commit: `90410f2681a0a72680d12280a1e0f14715e67dff`  
Language: C#  
License at pinned repo: **MIT**

## Role

Encounter-driven generation/legalization reference for host tooling:

- encounter candidates;
- generation constraints;
- shiny locks;
- forms;
- trainer/encounter consistency;
- test vectors.

## Classification

**HOST TOOL / REFERENCE; possible direct host-tool reuse under reviewed terms.**

Never use it to forge HOME tracker/history or to promise guaranteed online acceptance.

---

# 9. pkHouse

Repository: `https://github.com/Insektaure/pkHouse`  
Pinned research commit: `4e288f4a66acf2e58822ad1bb372a6803d3c46dd`  
Language: C++ / Nintendo Switch homebrew  
License: **GPLv2**

## Role

High-value modern Switch save/bank behavior reference:

- title IDs/save filenames;
- current Pokémon formats;
- modern block/container behavior;
- box/slot sizing;
- Switch FireRed/LeafGreen identities;
- Z-A/SV/SwSh/BDSP/PLA/LGPE behavior;
- Pokédex/handling-trainer/Wondercard-related behavior.

The author encouraged reimplementation/reference rather than copy-paste and offered technical help, but that is not treated as a broad relicensing grant.

## Classification

**REFERENCE ONLY.**

Reimplement required behavior inside PokeBank NX interfaces and safety rules. Cross-check important facts with PKHeX/PKSM-Core/device tests.

Issue: #11.

---

# 10. pkDex

Repository: `https://github.com/Insektaure/pkDex`  
Pinned research commit: `c7a1f8debf4e70643464cf939d06f8e167839a15`  
Language: C++ / Switch homebrew  
License: **GPLv2**

## Role

Pokédex UX/regional organization/display-data reference:

- normal/shiny collection views;
- Alpha/Shiny Alpha ideas;
- multi-select/bulk UX;
- evolution/location/version information;
- regional/DLC organization.

## Classification

**REFERENCE ONLY.**

PokeBank NX difference: the **Master Vault is authoritative**. Pokédex state must be derived from Vault entities rather than a separate manual ownership database.

Issue: #7.

---

# 11. PKForge

Repository: `https://github.com/sofianeelhor/PKForge`  
Pinned research commit: `ded13d9cfb75f94df7c2d88b4621866cfd55e499`  
Language: C# / .NET MAUI  
License: **GPLv3**

## Role

Architecture reference for safe local Pokémon storage and save editing:

- immutable raw entities;
- stable entity IDs;
- SHA-256 identity;
- display metadata as rebuildable cache;
- provenance history;
- logical boxes/views;
- schema migrations;
- backup/stage/validate/atomic-replace ideas;
- engine adapter boundaries.

## Classification

**ARCHITECTURE REFERENCE / SELECTIVE PORT OF IDEAS.**

Adapt invariants to native C++, Switch memory limits, SD storage and controller UI rather than copying MAUI architecture wholesale.

---

# Recommended division of responsibility

```text
PKSE
  = inherited native Switch shell/save foundation

PKSM-Core
  = first native C++ candidate for historical Pokémon/save formats

PKSM + Pokémon Chest
  = mature integration/bank/defensive parser examples around PKSM-Core

PHBank
  = Game↔Bank transfer UX reference

PHBankGBC
  = secondary Gen I/II historical cross-check only

PKHeX
  = primary correctness oracle/reference

Auto Legality
  = encounter-driven generation/legalization reference for host tooling

pkHouse
  = modern Switch save behavior reference

pkDex
  = Pokédex UX/organization reference

PKForge
  = Vault/provenance/transaction architecture reference

PokeBank NX
  = safe integration layer + Master Vault + provenance + Banks + controller UX
```

---

# Mandatory comparison rule

For each format integrated/substantially changed, compare the same fixture among as many relevant engines as practical:

```text
PokeBank NX
PKHeX
PKSM-Core
pkHouse where relevant
```

Compare at minimum:

- species/form;
- PID/encryption constant where applicable;
- IVs/EVs;
- nature/ability/gender/shiny state;
- moves/Ball/item;
- OT/TID/SID;
- met/origin data;
- checksum/encryption validity;
- serialized size/format;
- conversion output.

For saves:

```text
load → no modification → serialize → reparse → compare
```

Where a format permits true byte identity, untouched saves should remain byte-identical. Any normalization must be explicit and tested.

---

# Next research/integration tasks

- [ ] Execute #23/#24 and complete the second physical device gate first.
- [ ] PKSM-Core `PK3` + `Sav3` spike (#4).
- [ ] During #4, compare PKSM and Pokémon Chest integration patterns.
- [ ] Build golden fixtures (#17).
- [ ] Build PKHeX Oracle (#5).
- [ ] Use PKSM BankFile corruption/migration failure classes when Vault parser/storage tests are built.
- [ ] Consider optional PKSMBANK importer after Vault v1.
- [ ] Translate PHBank/Chest dual-source ideas into a PokeBank NX Transfer Workspace after Vault exists.
- [ ] Use PHBankGBC only as a secondary Gen I/II cross-check.
- [ ] Keep all direct-reuse license/attribution decisions explicit in commits/docs.

Full product sequencing: `docs/V1_ROADMAP.md` / issue #29.