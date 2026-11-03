# PokeBank NX — Additional Bank Project Reference Audit

Date: 2026-09-02

Purpose: record four additional Pokémon storage/save-manager projects that may accelerate PokeBank NX without replacing the existing upstream-first plan.

This audit supplements `docs/UPSTREAM_AUDIT.md`.

Current rule remains:

```text
PKSM-Core = primary native historical format/save integration candidate
PKHeX     = correctness oracle/reference
PKSE      = inherited Switch foundation
pkHouse   = modern Switch behavior reference
PokeBank NX Master Vault = our own provenance/transaction architecture
```

The projects below are useful primarily for bank/storage design, transfer UX, defensive parsing, constrained-console integration, and legacy save behavior.

Do not let this research derail Session 2.6 safety/crash work.

---

## 1. FlagBrew/PKSM

Repository: https://github.com/FlagBrew/PKSM

Research pin inspected:

```text
47d060ad80bcd55ed5c154d40ded0462d2448d21
```

Language: C++ / Nintendo 3DS

License: GPLv3 with PKSM Additional Terms 7.b / 7.c requiring preservation of specified notices/attribution and avoiding origin misrepresentation.

### Why it matters

PKSM is the full application built around the same PKSM-Core family already targeted by PokeBank NX. Its full application layer adds mature patterns that PKSM-Core alone does not show:

- resizable offline Banks;
- bank backup behavior;
- bank migration/versioning;
- box naming;
- change detection via hashes;
- multi-generation Pokémon storage;
- save-manager UX;
- Wondercard/event database behavior;
- scripts;
- sorting/filtering and editor flows;
- a long-lived example of integrating PKSM-Core into a constrained Nintendo homebrew application.

### Very high-value current BankFile work

Current PKSM separates its raw bank format from UI/I/O through:

```text
common/include/BankFile.hpp
common/source/BankFile.cpp
tests/BankFileTests.cpp
```

Observed BankFile v3 design:

```text
magic: PKSMBANK
versioned format
30 slots per box
maximum 500 boxes
entry stores generation + up to 0x148 bytes
v1/v2 migration support
typed parse errors
newer-version rejection
invalid box-count rejection
truncated-file detection
missing tail slots filled as empty rather than read from uninitialized memory
host-side parser tests
```

`3ds/source/Bank.cpp` also demonstrates:

```text
automatic/optional backup-before-save
backup rotation (.bak -> .bak.old)
SHA-256 change detection
bank + box-name sidecar handling
legacy bank.bin conversion
per-generation PKX reconstruction
```

### PokeBank NX classification

```text
FULL PKSM APP: REFERENCE / SELECTIVE ADAPTER SOURCE
PKSM-CORE: existing DIRECT REUSE / ADAPTER CANDIDATE
BANKFILE: possible DIRECT REUSE or LEGACY IMPORT ADAPTER after license/notice review
3DS UI / archive I/O: REFERENCE ONLY for Switch architecture
```

### Important architectural decision

Do **not** replace the PokeBank NX Master Vault with PKSM's flat Bank format.

Our Vault needs stronger invariants:

```text
immutable original entities
stable Vault IDs
SHA-256 identity
origin + active-location separation
parent/derived provenance
transaction journal
crash recovery
rebuildable indexes
future true-Move history
```

PKSM BankFile is instead useful for:

1. bank parser hardening ideas;
2. migration/version tests;
3. optional future import of PKSM-compatible bank files;
4. backup/change-detection patterns;
5. validating our own corruption/recovery test strategy.

### Action items

- During Vault work, compare our parser/storage tests against `BankFileTests.cpp` failure classes.
- Consider an optional `PKSMBANK` legacy importer after Vault v1 exists.
- Preserve PKSM legal notices if any source is directly reused.
- Review full PKSM bank UI/search/sort behavior for UX ideas, but implement PokeBank NX-specific controller UI.

---

## 2. gocario/PHBank

Repository: https://github.com/gocario/PHBank

Research pin inspected:

```text
a956ab39060148c8dafdb1f472c96632b22dcc65
```

State: archived / historical

Language: C / C++ / Nintendo 3DS

License: GPLv3

### Why it matters

PHBank is extremely close to the original product interaction the PokeBank NX user wants: a free offline bank where a game PC and Bank are both navigable and Pokémon can be moved between them.

Its documented behavior includes:

```text
XY / ORAS game boxes + Bank boxes
single-Pokémon movement
multi-Pokémon movement
whole-box PC <-> Bank swap
summary with IVs/EVs
Pokédex update on game import
save / exit / backup prompt
explicit game savedata import/export
SD-card bank storage
```

Controller concepts include:

```text
D-pad / Circle Pad navigation
L/R box change
ZL/ZR PC <-> Bank switch
selection modes: Single / Quick / Multiple
```

### PokeBank NX classification

```text
REFERENCE / UX REFERENCE
SELECTIVE ALGORITHM PORT only after comparison with modern PKHeX/PKSM-Core
```

The save-format code is old and specific to XY/ORAS-era 3DS behavior, so it should not become a correctness authority.

### Highest-value idea for PokeBank NX

PHBank gives us a very good reference for a future dedicated **Transfer Workspace**:

```text
SOURCE GAME                 DESTINATION
[Box grid]        <->       [Bank/Vault grid]

pick one / multi-select / entire-box operation
preview change
commit through safe transaction pipeline
```

PokeBank NX should keep the normal HOME-style browsing shell, but a dedicated Game <-> Vault/Bank transfer mode may be more efficient than forcing every move through nested menus.

### Relation to true Move

PHBank's UX is useful as a *behavioral reference* for issue #20, but PokeBank NX must use its much stricter safety pipeline:

```text
destination first
backup source
stage mutation
validate
write through approved adapter
read back
verify
then retire/remove source active location
```

Do not copy PHBank's direct-write assumptions into the current alpha.

---

## 3. Universal-Team/pkmn-chest

Repository: https://github.com/Universal-Team/pkmn-chest

Research pin inspected:

```text
a847bb63a021a39375a5cb6673b2ed3f24195a2f
```

Language: C++ / Nintendo DS(i)

License: GPLv3

Current PKSM-Core submodule observed at:

```text
6cba517305261f88fff0d27ede46e568e0367555
```

### Why it matters

Pokémon Chest is a Pokémon Bank for Generation III through V games on DS(i). It is especially valuable because it demonstrates PKSM-Core being adapted into a second constrained Nintendo homebrew environment rather than only PKSM itself.

The project credits PKSM / PKSM-Core for save and bank management and PKHeX as a major Gen III reference.

Observed useful areas include:

```text
arm9/core                       PKSM-Core submodule
arm9/include/utils/bank/        Bank abstraction
arm9/source/utils/bank/         Bank persistence / management
arm9/source/manager.cpp         bank-vs-save UI state and Pokémon rendering
arm9/source/menus/sort.cpp      bank sorting behavior
arm9/source/menus/aMenu.cpp     Pokémon action/edit paths
arm9/source/menus/xMenu.cpp     save/discard/backup UX
arm9/source/saves/              cartridge/save source handling
```

### Strong UX reference

The manager explicitly treats Bank and game save as two separate sides:

```text
top side    -> Bank
bottom side -> game save / party
```

and resolves the focused Pokémon from the appropriate source.

That is a strong precedent for PokeBank NX's future Transfer Workspace, adapted to a single 1280x720 Switch screen as two panels or a source/destination split view.

### Bank implementation lesson

Pokémon Chest carries a PKSM-derived Bank implementation with:

```text
PKSMBANK v3-style entries
30 slots per box
bank names
hash-based change detection
backup behavior
multi-generation PKX payloads
```

This reinforces that the PKSM bank abstraction has already been portable across more than one Nintendo homebrew application.

### Robustness lesson

The project is also useful as a source of edge-case thinking: current history includes out-of-bounds hardening, and its code has had to operate under tight DS memory/storage constraints.

For PokeBank NX, study its defensive changes and failure modes rather than assuming old code is automatically safe.

### PokeBank NX classification

```text
PKSM-CORE USE: confirms our existing integration direction
BANK/UI/CONSTRAINED-CONSOLE PATTERNS: REFERENCE / SELECTIVE PORT
DS cartridge hardware I/O: REFERENCE ONLY / mostly not applicable to Switch
```

### Action items

- During issue #4, compare how Pokémon Chest wraps PKSM-Core versus how PKSM itself does it.
- During issue #3, compare its bank adapter/persistence boundaries with our Vault interfaces.
- During future Transfer Workspace design, borrow the two-source mental model without copying DS screen layout.
- Review recent OOB fixes for test cases worth adding to the PokeBank NX golden/malformed corpus.

---

## 4. 0xb01u/PHBankGBC

Repository: https://github.com/0xb01u/PHBankGBC

Branch inspected:

```text
GEN2
```

Research head inspected:

```text
9baf997c470b3eaf26c97a9867e35f6acecf527b
```

Language: C / Nintendo 3DS

Project lineage: fork of JoycieC/PHBankGB, itself a fork of gocario/PHBankGB.

### License warning

No explicit LICENSE file was found at the inspected PHBankGBC `GEN2` root, and the inspected upstream PHBankGB root also did not expose a LICENSE file.

Therefore PokeBank NX policy is:

```text
REFERENCE ONLY unless provenance/license is separately clarified
```

Do not copy source directly.

### Why it may still be useful

`source/save.c` contains explicit legacy Game Boy / Virtual Console save knowledge, including:

```text
Gen I party offsets
current-box offsets
box group offsets
item storage offsets
Pokédex owned/seen offsets
checksums
language/title-ID handling
PK1 extraction logic
Japanese vs international naming/layout considerations
```

The code also shows direct formulas for reconstructing Gen I stats/IV-derived HP and extracting OT/nickname data from the save layout.

### Reliability warning

Some inspected title-ID entries are visibly placeholders/uncertain (`??`) and the Crystal entry is incomplete-looking. This is old experimental code and must **not** be treated as authoritative save-format documentation.

### PokeBank NX classification

```text
REFERENCE ONLY
```

Use it as an independent historical cross-check for issue #6, alongside much stronger authorities:

```text
PKSM-Core PK1/PK2 + Sav1/Sav2
PKHeX
known public save-format documentation
golden fixtures
```

If PHBankGBC disagrees with those sources, do not prefer PHBankGBC automatically.

---

# Practical ranking for PokeBank NX

```text
1. PKSM full app
   HIGH value
   bank parser, migration, backup, Bank UX, events/scripts, PKSM-Core integration patterns

2. Pokémon Chest
   HIGH value
   second PKSM-Core integration example, Gen III-V bank UX, constrained-console patterns

3. PHBank
   MEDIUM-HIGH value
   excellent direct Game <-> Bank transfer UX reference and multi/box move ideas

4. PHBankGBC
   MEDIUM reference value
   useful independent Gen I/II save-layout clues, but old and licensing/data-confidence concerns
```

---

# New PokeBank NX ideas derived from this audit

## Transfer Workspace

Later, after Vault v1 and safe transfer staging exist, add a dedicated controller-first workspace:

```text
┌──────────────────── GAME / SOURCE ────────────────────┐
│ Box 12                                                │
│ [pokemon grid]                                        │
└────────────────────────────────────────────────────────┘

                 move/copy/selection state

┌──────────────────── VAULT / BANK ─────────────────────┐
│ Living Dex / Bank 4                                   │
│ [pokemon grid]                                        │
└────────────────────────────────────────────────────────┘
```

On Switch this can be left/right panels or a focused source/destination view rather than literal DS top/bottom screens.

Possible later operations:

```text
Move selected
Copy selected
multi-select
Move entire box
Copy entire box
Find first free slots
Preview transaction
Cancel transaction
```

True Move remains blocked on issue #20 safe-write gates.

## Legacy bank import

After Master Vault v1 exists, consider read-only importers for:

```text
PKSMBANK
possibly Pokémon Chest-compatible PKSM-derived bank files
PHBank-style bank files only if format/licensing/value justify it
```

Import should create Vault entities with provenance such as:

```text
source_type: legacy_bank_import
source_project: PKSM
source_file_hash: <SHA-256>
imported_at: ...
```

Never make a foreign bank format the authoritative Vault database.

## Corruption test classes

Add these to issue #17 / Vault storage tests:

```text
bad magic
zero box count
huge box count
truncated header
truncated body
newer unsupported version
old supported version migration
extra trailing bytes
unknown generation value
corrupt sidecar/box-name JSON
backup failure
interrupted save
```

PKSM's current BankFile tests are an especially useful reference for this list.

---

# Immediate roadmap impact

No change to current order:

```text
Session 2.6 safety/crash/device build
    -> second physical test
    -> issue #4 PKSM-Core Gen III spike
    -> issue #3 Master Vault v1
```

But issue #4 should now inspect **both PKSM and Pokémon Chest as integration examples**, and issue #3 should explicitly review PKSM BankFile/Bank patterns before finalizing Vault persistence and legacy-import boundaries.

PHBank's Game <-> Bank UX should be revisited when transfer staging / true Move UI is designed.

PHBankGBC should be revisited during Gen I/II read-only adapter work, strictly as a secondary cross-check.
