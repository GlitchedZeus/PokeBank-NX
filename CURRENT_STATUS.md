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

Implemented:

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

Implemented user path:

```text
validated session catalog
        -> existing Game Sources screen
        -> FireRed / LeafGreen GBA card
        -> exact validated catalog entry
        -> strict read-only trainer bridge
        -> Party / Boxes / View Pokémon
```

Cards carry exact `firered_gba` / `leafgreen_gba` identities and `Game Boy Advance` / `RETROARCH` labeling. They remain distinct from Switch FireRed/LeafGreen. View is permitted; Edit, Clone, Transfer, Move, Save and writeback remain blocked.

Verification before hardware testing:

- 12 host suites PASS;
- ASan/UBSan PASS;
- `git diff --check` PASS;
- GitHub Actions run #184 PASS;
- clean native devkitA64 `-fno-exceptions` build PASS.

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

## Physical Switch result — PARTIAL PASS / TWO BLOCKERS

The exact artifact above was physically tested.

What works:

- RetroArch FireRed/LeafGreen GBA saves are found;
- cards open successfully;
- read-only Party/Boxes/Pokémon viewing generally works;
- no reported crash/corruption in the tested path.

FRLG is **NOT DEVICE-ACCEPTED YET** because two integration bugs were observed.

### Blocker 1 — RetroArch incorrectly behaves like a Switch user/profile context

Observed behavior: the user must switch to a RetroArch-looking account/profile in order to see the legacy saves.

Required behavior:

```text
RetroArch / legacy file sources = app-global
Nintendo installed-title saves  = user/account scoped where appropriate
```

RetroArch must not be modeled as or hidden behind a Nintendo user profile. Valid legacy sources should be visible regardless of the currently selected Switch account.

### Blocker 2 — duplicate FRLG source cards

Observed on device:

```text
2 FireRed entries representing the same save
3 LeafGreen entries representing the same save
```

Likely classes of cause to inspect include overlapping configured/conventional roots, path aliases, duplicate traversal roots, or the same underlying file discovered through more than one route. Do not assume the exact cause without inspecting runtime discovery.

Required behavior:

- the same underlying logical source must appear once;
- genuinely distinct save files must still be allowed to coexist even when they are the same game/version;
- dedupe should occur only after strict validation;
- prefer a robust canonical-source identity using normalized/canonical path/file identity and, where useful, content hashing to collapse aliases/duplicate discovery without collapsing intentionally separate files incorrectly.

Issue #6 records this hardware report.

## Immediate next milestone

Fix only these two FRLG device blockers:

```text
make RetroArch legacy sources app-global
        +
deduplicate identical FRLG discoveries/cards
        ↓
keep strict validation + read-only safety
        ↓
run host tests/sanitizers/native build
        ↓
package new full-asset exact NRO
        ↓
STOP for physical retest
```

Do not begin Ruby/Sapphire/Emerald before this retest passes.

## Parked RSE work

A previous interrupted session produced local RSE work at:

```text
1a921515
```

It remains intentionally parked/local. Preserve it if present, but do not resume, merge or reimplement RSE until FRLG passes the physical retest.

## Next major order

```text
fix FRLG device blockers
-> physical FRLG retest / acceptance
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

Both remain post-v1/later.

## Session launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```
