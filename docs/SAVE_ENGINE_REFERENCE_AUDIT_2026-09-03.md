# PokeBank NX — Additional Save-Engine Reference Audit

Date: 2026-09-03
Status: research/reference only; no application-source change

This audit records additional projects supplied during the legacy save-engine hunt. It does not replace the existing PKSM-Core / PKSM / PKHeX reference stack. The goal is to identify independent implementations that can reduce reverse-engineering risk and provide cross-engine test oracles.

## Classification legend

- DIRECT REUSE CANDIDATE — licensing/architecture may permit integration after dedicated spike.
- ADAPTER / WRAPPER CANDIDATE — useful engine shape, but integration cost/dependencies require evaluation.
- SECONDARY ORACLE — valuable independent implementation for cross-checking our parser/output.
- REFERENCE ONLY — useful design/research source; do not copy code without a clear license/reuse decision.

---

## ncorgan/pksav

Repository: `ncorgan/pksav`
License: MIT
Language: pure C
Repository status: archived

README goals are unusually aligned with PokeBank NX: small, fast, portable Pokémon save parsing/manipulation with no external dependencies, suitable for PCs, smartphones and handheld consoles. The project deliberately presents itself as a low-level library intended to sit underneath a higher-level UI/library.

README component status (last updated 2018-03-25):

```text
Generation I     complete
Generation II    complete
Game Boy Advance complete
Generation IV    in progress
Generation V     in progress
Generation VI    in progress
Generation VII   in progress
```

The repository exposes separate public include trees for Gen I, Gen II, GBA, Gen IV and Gen V. It also carried multi-compiler CI, Unity unit tests and CppCheck.

Important limitation: the README says only American save files were supported at that time because regional layouts differ.

PokeBank NX classification:

```text
Gen I / II / GBA     SECONDARY ORACLE + ADAPTER/WRAPPER CANDIDATE
Gen IV / V           REFERENCE ONLY until completeness is measured
Gen VI / VII         REFERENCE ONLY until completeness is measured
```

Why it matters:
- independent implementation from PKSM-Core/PKHeX;
- pure C/no external dependencies is attractive for Switch homebrew;
- strong fit for golden-corpus differential testing;
- may reduce risk in Gen I/II/GBA offsets, checksums, strings and entity parsing.

Do not call it production-ready for all regions/generations without new tests.

---

## ncorgan/libpkmn

Repository: `ncorgan/libpkmn`
License: MIT
Language: C++ core with C/C#/Lua/Python/Ruby interfaces
Repository status: archived

LibPKMN aimed to provide a broad Pokémon save editing/analysis library. Its README reports support for:

```text
Red / Blue / Yellow
Gold / Silver / Crystal
Ruby / Sapphire / Emerald
FireRed / LeafGreen
Colosseum / XD
```

It used `pksav` underneath portions of the main-series save work and credits TuxSH/PkmGCTools for GameCube save editing. The project had extensive multi-language tests plus CppCheck and Clang-Tidy.

The full library is heavier than we want for Switch runtime because its build stack includes Boost and optional GUI/wrapper infrastructure. Its architecture and tests are still valuable.

Classification:

```text
FULL LIBPKMN       SECONDARY ORACLE / ARCHITECTURE REFERENCE
pksav subcomponent separate candidate above
PkmGCTools         existing GameCube reference path
```

Useful for validating the idea of one generation-neutral API over multiple concrete save backends.

---

## codemonkey85/PKMDS

Repository: `codemonkey85/PKMDS`
License: The Unlicense
Language: C/C++ library
Repository status: archived

Repository description: Pokémon save hacking library currently supporting Gen V and limited portions of Gen III / VI.

The public library include tree contains, among other files:

```text
pkmds_g5.h
pkmds_g5_sqlite.h
pkmds_g6.h
pkmds_gba.h
pokeprng.h
wondercard.h
```

This is much more valuable to PokeBank NX than the old desktop editor by itself because it exposes the actual reusable engine layer.

Classification:

```text
Gen V              SECONDARY ORACLE; high-value audit for #31
Gen VI             SECONDARY ORACLE / partial reference for #32
GBA                SECONDARY ORACLE for #4
Wondercards        SECONDARY ORACLE for future event work
Direct integration ADAPTER/WRAPPER CANDIDATE only after completeness/build audit
```

PKSM-Core remains the preferred first native integration candidate because it has broader known coverage and an established Nintendo-homebrew lineage, but PKMDS is an excellent independent implementation for differential tests.

---

## codemonkey85/PKMDS-Save-Editor + release 1.1.1

Repository: `codemonkey85/PKMDS-Save-Editor`
License: The Unlicense
Language: C# application using PKMDS
Repository status: archived

The supplied `1.1.1` release is from 2014 and is mainly historical application evidence. Its useful role is showing how a real client consumed PKMDS and exposing old UX/save workflows.

Classification: REFERENCE ONLY for UI/application integration. Prefer the PKMDS engine repository for actual parser research.

---

## 0xA2/Pokemon-Savefile-Editor

Repository: `0xA2/Pokemon-Savefile-Editor`
Language: C++
License: no repository license detected during this audit

This is a compact command-line Gen IV editor supporting:

```text
Diamond
Pearl
Platinum
HeartGold
SoulSilver
```

The implementation contains save-block checksum updates and Pokémon encryption/decryption/edit handling. It is useful because it is small enough to understand end-to-end.

Classification: REFERENCE ONLY because no explicit license was detected and the code is a monolithic editor rather than a reusable engine.

Use it to cross-check Gen IV block selection, offsets, encryption and CRC behavior against PKSM-Core/PKHeX/other sources. Do not use it as sole authority.

---

## LostMyPlaintext — Reverse Engineering Pokemon NDS save files

Article: `Reverse Engineering Pokemon NDS save files` (2021-06-20)
Focus: primarily Pokémon Platinum / Gen IV; same-generation games share concepts but offsets differ.

The article walks through:

```text
small/big save blocks
alternating recent/previous save blocks
CRC-16-CCITT save-block validation
236-byte party Pokémon structures
Pokémon XOR encryption
PRNG-based encryption/decryption
four-block data shuffling
Pokémon-local checksum repair
```

This is a very useful explanation for engineers learning the format and for creating synthetic corruption/checksum tests. It is not a production parser and should be verified against authoritative implementations; hand-written research articles can contain mistakes or incomplete regional/version handling.

Classification: REFERENCE ONLY / TEST-DESIGN INPUT.

---

## pkmn-savedata crate

Crate: `pkmn-savedata` 0.2.0
Repository host: Codeberg (`ayashibox/pkmn-savedata` according to docs.rs)
Language: Rust
Current implemented scope in published docs: GBA / Generation III

Its explicit engineering goals are exceptionally relevant to PokeBank NX:

```text
no unsafe code
no_std compatible
endianness safe
limit ways callers can break saves
never panic on malformed save input
never panic from valid caller interaction
support all official international releases of supported games
```

The CLI can extract all PC Pokémon as `.pkm3` files and decode them to JSON. The published TODO lists future GB, DS Gen IV/V, Stadium, GameCube/Box and Wii support, with 3DS/Switch as longer-term possibilities.

Current limitation for PokeBank NX runtime: C/C++ bindings are explicitly still a TODO in the published docs, so direct native integration would require extra Rust/toolchain/FFI work.

License was not established from the accessible pages in this audit; verify before any direct code reuse.

Classification:

```text
Gen III host testing     HIGH-VALUE SECONDARY ORACLE
parser-safety design     DIRECT DESIGN INSPIRATION
Switch runtime library   NOT YET SELECTED; FFI/toolchain cost
```

This project should strongly influence malformed-save requirements: a parser should return a typed failure instead of crashing regardless of hostile/truncated input.

---

## gocario/PKBrew

Repository: `gocario/PKBrew`
License: GPLv3 repository
Purpose: RomFS/data resource repository for Pokémon homebrew, not a save parser.

README states that personal/text data are sourced from PKHeX/veekun and much of the Pokémon imagery comes from external Pokémon sources; some UI graphics were made by the project.

Classification: RESOURCE REFERENCE ONLY for PokeBank NX unless individual data/assets and their redistribution terms are separately verified.

It does not solve the high-priority save-engine problem.

---

## gocario/PHBank

Already covered in `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`.

No new classification change from this duplicate link. PHBank remains a valuable historical Game-PC ↔ offline-bank UX reference, not our authoritative save engine.

---

# Revised reference priority

For the next deep engine phases:

```text
PRIMARY NATIVE CANDIDATE
PKSM-Core

PRIMARY CORRECTNESS ORACLE
PKHeX

HIGH-VALUE INDEPENDENT ORACLES
pksav             Gen I / II / GBA
PKMDS              Gen V + partial III/VI + Wondercards
pkmn-savedata      modern defensive Gen III implementation
libpkmn            multi-generation architecture + GC cross-check

FOCUSED REFERENCES
0xA2 Gen IV editor
LostMyPlaintext Gen IV reverse-engineering article
PkmGCTools / LibPkmGC GameCube work
PHBank / Pokémon Chest transfer UX
```

# Concrete roadmap impact

## #4 Gen III / PKSM-Core spike

Add `pksav`, PKMDS GBA, and `pkmn-savedata` as independent comparison implementations. Do not replace PKSM-Core before measuring all four against the same golden saves.

## #6 Gen I / II legacy

Audit `pksav` early because it reports Gen I/II complete, is pure C/MIT, and has no external dependencies. Cross-check every accepted output against PKSM-Core and PKHeX.

## #31 Nintendo DS Gen IV / V

Use:
- PKSM-Core/PKSM as primary native candidate;
- PKMDS as strong Gen V independent oracle;
- 0xA2 editor + LostMyPlaintext as Gen IV focused references;
- pksav Gen IV/V only after measuring incomplete areas.

## #32 Nintendo 3DS Gen VI / VII

PKMDS Gen VI and pksav Gen VI/VII are secondary/partial references only. PKSM-Core and PKHeX remain stronger starting points.

## #33 Colosseum / XD

Use PKHeX + PkmGCTools as primary references and libpkmn as an independent integration/oracle example.

## #17 golden corpus

Whenever practical, run the same legal fixtures through multiple independent parsers:

```text
PokeBank NX adapter
PKSM-Core
PKHeX Oracle
pksav / PKMDS / pkmn-savedata where generation coverage overlaps
```

A disagreement should become an explicit investigation, not a majority vote.

# Safety principle learned from this audit

Future save parsers should target this contract:

```text
arbitrary/truncated/malformed input
        ↓
validate size/version/container
        ↓
no unchecked offset assumptions
        ↓
typed error / unsupported result
        ↓
safe return to UI

NEVER parser crash
NEVER automatic repair/write on parse failure
```

That principle directly supports issue #24 and the broader `SAVE_SAFETY.md` model.
