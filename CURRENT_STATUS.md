# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-07

This file is the short authoritative handoff for coding sessions. Historical reports, long-form research and earlier roadmaps remain in `docs/` and GitHub issues.

## Repository / branch

- Repository: `GlitchedZeus/PokeBank-NX`
- Development branch: `feature/pokebank-playable`
- Writable remote: `origin`
- Upstream-only remote: `kiasta/PKSE`
- Never push PokeBank NX custom code upstream.
- Live installed-game save writing remains **HARD DISABLED**.
- Preserve useful local/uncommitted work before syncing, resetting, cleaning, restoring or changing refs.

## Accepted UI / prior physical state

Accepted red PokeBank identity source:

```text
af2acf043a15dbf48b8195880a80cc5de562fced
ui: adopt red PokeBank identity accents
```

Accepted prior artifact:

```text
PokeBank-NX-Red-UI-af2acf04.nro
SHA-256 898df286cf34b895f1f71f4abc35f0818e4afa66725b67c2d020fc20c01bfac4
```

Previously device-verified behavior includes D-pad + Left Stick navigation, HD Pokémon artwork, accepted red identity, no artificial sprite bobbing, graceful handling of the known Legends: Arceus missing-main case, and installed-source read-only safety.

## Gen III engine foundation

Host/oracle adapter:

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter
```

Pinned PKSM-Core:

```text
FlagBrew/PKSM-Core
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
```

Native exception-free backend:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

Architecture remains:

```text
HOST / correctness oracle:
PKSMGen3Adapter -> pinned PKSM-Core PK3/Sav3/SavFRLG

SWITCH NATIVE:
same PokeBank adapter API -> PKSMGen3NativeAdapter.cpp
```

Do not redo this architecture unless a real regression requires it.

## RetroArch FRLG runtime discovery

Canonical implementation:

```text
54cb86892d290ae1c80f447af427ff17192681f9
gen3: wire RetroArch FRLG read-only sources
```

Implemented baseline:

```text
RetroArch savefile_directory
        -> bounded .sav/.srm discovery
        -> strict FRLG structural validation
        -> firered_gba / leafgreen_gba when reliable
        -> native Gen III read model
        -> Party / all 14 Boxes
        -> UIManager-owned session catalog
```

Bounds remain depth <= 2 and 256 candidates by default. Filename/path hints are considered only after strict FRLG validation. Ambiguous valid saves are not guessed. Sources remain read-only.

## Selectable RetroArch FRLG browser

Canonical application source:

```text
f6a3052daeffe7cd30d7acceba81a5dfda7615ee
gen3: expose RetroArch FRLG game sources
```

Implemented baseline path:

```text
validated session catalog
        -> existing Game Sources screen
        -> FireRed / LeafGreen GBA card
        -> exact validated catalog entry
        -> strict read-only trainer bridge
        -> Party / Boxes / View Pokémon
```

Cards carry exact `firered_gba` / `leafgreen_gba` identities and `Game Boy Advance` / `RETROARCH` labeling. They remain distinct from Switch FireRed/LeafGreen. View is permitted; Edit, Clone, Transfer, Move, Save and writeback remain blocked.

Verification before hardware testing: 12 host suites PASS; ASan/UBSan PASS; `git diff --check` PASS; GitHub Actions run #184 PASS; clean native devkitA64 `-fno-exceptions` build PASS.

## Exact FRLG hardware-test artifact

Exact tested artifact:

```text
Application source: f6a3052daeffe7cd30d7acceba81a5dfda7615ee
Embedded version:   f6a3052d
NRO size:           155174825 bytes
NRO SHA-256:        809c94c842a3c385d23907c61e5ecfa201b24f08e8ac935e87a4add60770282d
ZIP size:           148435779 bytes
ZIP SHA-256:        66b1e16eb5443fd6ce682f2485aacf0cb50a2b00cca0846b8d170cc89727b701
```

Asset verification:

```text
HD Pokémon renders:         3260
Base species coverage:      1025 / 1025
Embedded RomFS comparison:  3281 / 3281 files byte-identical
Native Switch build:        PASS
Asset preflight:            PASS
```

## Physical Switch result — PARTIAL PASS / SEVEN BLOCKERS OR REQUIREMENTS

What works:

- RetroArch FireRed/LeafGreen GBA saves are discovered and open;
- strict read-only Party/Boxes/Pokémon viewing generally works;
- no reported source corruption or crash in the tested route;
- gender happened to display correctly in the Trainer panel.

FRLG is **NOT DEVICE-ACCEPTED** until the following are fixed and physically retested.

### 1 — RetroArch is incorrectly Nintendo-user/profile scoped

Observed: the user must select a RetroArch-looking profile/account to see the legacy saves.

Required:

```text
RetroArch / legacy file sources = app-global
Nintendo installed-title saves  = user/account scoped where appropriate
```

### 2 — wrong game/save hierarchy and duplicate top-level cards

Observed: 2 FireRed and 3 LeafGreen top-level cards.

Required product model:

```text
Game Sources
  -> Pokémon FireRed
       Game Boy Advance · RetroArch
       -> Save Instances
            -> WILL — Main Save
            -> WILL — Save State 0   [future]
            -> WILL — Save State 1   [future]
            -> WILL — Save State 2   [future]
            -> WILL — Save State 3   [future]
            -> WILL — Backup         [future]
       -> choose one
       -> Trainer / Items / Party / Boxes
```

Likewise one LeafGreen parent card. Current production support remains `.sav/.srm` battery saves only; actual `.state#` parsing is future work. Distinct physical saves stay distinct children, while aliases of the same underlying file collapse inside the child list. See `docs/RETROARCH_SOURCE_NAMING.md`.

### 3 — FireRed / LeafGreen GBA card artwork missing

Required: `firered_gba` and `leafgreen_gba` must resolve to their correct GBA parent-card artwork through the existing asset system and remain distinct from Switch FRLG identities.

### 4 — legacy catalog needs refresh/rescan semantics

Required:

- initial bounded scan at startup;
- refresh current `.sav/.srm` children when opening a RetroArch parent card;
- manual **Refresh Saves / Rescan Sources** action;
- changed files invalidate and rebuild cached strict read models;
- deleted files disappear safely;
- no hot-swap while already browsing a selected source;
- all refresh behavior read-only.

Future Save State support must recognize slots **0, 1, 2, 3** and detect slot replacement/removal after refresh, but `.state#` parsing is not part of this immediate fix.

### 5 — scanner can surface stale saves RetroArch is not actively using

Current code scans both configured `savefile_directory` and `sdmc:/retroarch/cores/savefiles` additively.

Required precedence:

```text
usable configured savefile_directory
        -> authoritative BATTERY_SAVE root

otherwise
        -> sdmc:/retroarch/cores/savefiles fallback
```

Do not add the conventional root merely because it exists. Retain selected physical path/root metadata for diagnostics.

### 6 — FRLG Trainer view is only a placeholder read model

Observed on device:

```text
Name: (none)
Money: 0
Gender: correct
Trainer ID: zero/missing
Full TID: 0
Full SID: 0
```

Code inspection confirms `FRLGReadOnlyTrainer` deliberately clears/zeros trainer name, money, ID32, TID, SID, TID16 and SID16 and then only populates Party/Boxes.

Required strict read-only trainer data:

```text
trainer name
gender
TID16
SID16
ID32 / existing Gen III combined-ID semantics
money
exact source game identity
```

Expand the PokeBank-owned Gen III read-only adapter boundary; do not reparse selected raw files through mutable/permissive `Trainer3FRLG` as a shortcut. Existing verified Trainer3FRLG offsets/logic may be used as a cross-check/oracle. Add fixture tests and preserve byte immutability.

### 7 — FRLG Items view says `Invalid category`

Observed: opening Items displays `Invalid category`.

Cause: the Items panel expects `trainer.items` pouches, while the current FRLG read-only bridge populates none.

Required strictly read-only FRLG inventory containers, in existing order:

```text
Items
Key Items
Poké Balls
TM Case
Berry Pouch
PC Items
```

Use the verified `Inventory3FRLG` pouch definitions. Bag counts that are keyed must decode with the low 16 bits of the FRLG security key; PC Items retain their plaintext/non-keyed behavior. Preserve exact Gen III item IDs, counts, empty-slot behavior and source bytes. No item writeback or serializer is allowed.

## Immediate next milestone

Fix all seven FRLG blockers/requirements as one coherent read-only completion checkpoint:

```text
app-global legacy sources
+ active RetroArch root precedence
+ one game parent -> child save instances
+ alias dedupe
+ refresh/rescan
+ FireRed/LeafGreen GBA artwork
+ real trainer metadata
+ six read-only FRLG inventory containers
        ↓
full host/sanitizer/native verification
        ↓
new 3260-render full-asset exact NRO
        ↓
STOP for physical retest
```

Do not begin Ruby/Sapphire/Emerald before this retest passes.

## Parked RSE work

A previous interrupted session produced useful local RSE work at:

```text
1a921515
```

It remains intentionally parked/local. Preserve it if present, but do not resume, merge, push or reimplement RSE until FRLG passes physical acceptance.

## Next major order

```text
complete + retest FRLG read-only source browsing
-> Ruby / Sapphire / Emerald strict production reads
-> Master Vault + Banks foundation
-> Colosseum / XD
-> Gen I / II + RetroArch
-> Stadium stretch
-> DS Gen IV/V
-> 3DS Gen VI/VII
-> modern Switch adapter validation
-> Summary / Oracle / conversion / Dex / legality
-> staged writes
-> individually approved live-write adapters
-> true Move
-> RC / v1.0
```

## Parked later roadmap

```text
#46  Gift, Event and Mystery Gift Library + EventDex
#47  PokeBank NX Link for real GB/GBC/GBA hardware transfers
```

## Session launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```
