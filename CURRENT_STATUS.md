# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-07

This file is the short authoritative handoff for coding sessions. Historical device reports, long-form research and earlier roadmaps remain in `docs/` and GitHub issues; Git history preserves previous versions.

## Repository / branch

- Repository: `GlitchedZeus/PokeBank-NX`
- Development branch: `feature/pokebank-playable`
- Writable remote: `origin`
- Upstream-only remote: `kiasta/PKSE`
- Never push PokeBank NX custom code upstream.
- Live installed-game save writing remains **HARD DISABLED**.
- Documentation commits may sit above the latest engineering-source checkpoint; do not confuse branch HEAD with the source SHA being discussed.

## Accepted UI / physical state

The current UI is intentionally frozen until the app is much closer to completion.

Accepted application source:

```text
af2acf043a15dbf48b8195880a80cc5de562fced
ui: adopt red PokeBank identity accents
```

Accepted artifact:

```text
PokeBank-NX-Red-UI-af2acf04.nro
SHA-256 898df286cf34b895f1f71f4abc35f0818e4afa66725b67c2d020fc20c01bfac4
```

Physical acceptance includes:

- PokeBank NX red identity accepted for now;
- permanent left-side accent bar removed;
- Left Stick navigation works;
- D-pad navigation works;
- HD Pokémon artwork renders;
- artificial sprite breathing/bobbing removed;
- old/problem Legends: Arceus save no longer crashes and returns a graceful `main file is missing` error;
- installed-source read-only safety remains in place.

Closed hardware/safety/UI blockers include #13, #19, #23 and #24. Final branding/startup/NRO polish remains later under #16.

## Session 3A — Gen III host oracle / adapter

Implementation source:

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter
```

CI follow-up:

```text
283073a5215a471ef0ad07619b4856409658cfdc
ci: checkout pinned PKSM-Core submodules
```

Pinned PKSM-Core:

```text
FlagBrew/PKSM-Core
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
```

Issue #4 is complete. Integration decision: **ADAPTER-WRAPPER**.

The public boundary is PokeBank-owned and does not expose PKSM-Core types. Session 3A proves read-only FireRed/LeafGreen GBA parsing with stricter PokeBank validation in front of PKSM-Core.

Deterministic generated FRLG fixture:

```text
size    131072 bytes
SHA-256 b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

Proven behavior includes rotating save slots, all 14 sectors, signatures/counters/checksums, wrap-aware newest-slot selection, safe older-slot fallback, Party, all 14 Boxes, sector-boundary PK3 extraction, malformed/truncated rejection, source immutability and byte-identical untouched PK3 round trips.

## Session 3B checkpoint A — native exception-free Gen III backend

Verified engineering checkpoint:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

Full pinned PKSM-Core is not linked wholesale into the native Switch application. The native application remains `-fno-exceptions` and uses the same PokeBank-owned API through the selective native Gen III backend.

```text
HOST / correctness oracle:
PKSMGen3Adapter -> pinned PKSM-Core PK3/Sav3/SavFRLG

SWITCH NATIVE:
same PokeBank adapter API -> PKSMGen3NativeAdapter.cpp
```

Do not redo this architecture unless a real regression requires it.

## Session 3B checkpoint B — RetroArch FRLG runtime catalog

Canonical implementation source:

```text
54cb86892d290ae1c80f447af427ff17192681f9
gen3: wire RetroArch FRLG read-only sources
```

Implemented path:

```text
RetroArch savefile_directory
        -> bounded read-only .sav/.srm catalog
        -> structural FRLG validation
        -> firered_gba / leafgreen_gba identity when reliable
        -> native Gen III adapter
        -> Party / Boxes read model
        -> UIManager-owned application-session catalog
```

Only configured/conventional RetroArch roots are visited; the app never crawls the full SD card. Traversal defaults to depth 2 and 256 `.sav`/`.srm` candidates. A filename/path hint is used only after strict FRLG-family validation. Structurally valid but ambiguous saves remain unclassified. All file opens are read-only and tests prove the source bytes do not change.

State: **IMPLEMENTED / HOST TESTED / NRO BUILDS**.

## Session 3C — selectable RetroArch FRLG browsing

Canonical application source:

```text
f6a3052daeffe7cd30d7acceba81a5dfda7615ee
gen3: expose RetroArch FRLG game sources
```

Recovered local equivalent from the interrupted session:

```text
25fc12181bf1fffbe3f0a06b56dc9d676a0c29f0
gen3: expose RetroArch FRLG game sources
```

The first user-visible legacy route is complete:

```text
UIManager-owned validated catalog
        -> existing Game Sources screen
        -> FireRed / LeafGreen GBA card
        -> exact session catalog entry
        -> strict ReadOnlySave bridge
        -> existing Party / Boxes browser
```

Only strict `Ready` sources become selectable. Cards carry `firered_gba` or `leafgreen_gba`, show `Game Boy Advance` / `RETROARCH`, and remain distinct from the Switch release identities. Selection resolves back through the session-owned catalog rather than reparsing through permissive legacy Trainer logic.

`FRLGReadOnlyTrainer` is populated only from validated adapter records. It exposes Party and all 14 Boxes to the existing renderer and has no source serializer/write path. `RetroArchLegacy` permits View only; Edit, Clone, Transfer, Direct Move and Save Changes remain blocked. Tests prove fixture bytes remain unchanged across card creation, resolution and view-model construction.

Verification: twelve host suites PASS; ASan/UBSan PASS; `git diff --check` PASS; GitHub Actions run #184 PASS; clean native devkitA64 `-fno-exceptions` build PASS.

State: **IMPLEMENTED / HOST TESTED / NRO BUILDS / NOT DEVICE TESTED**.

## Exact FRLG browser device-test artifact — READY, NOT DEVICE TESTED

The complete pinned visual asset set has been restored and verified. This exact artifact is the only current FRLG browser device-test target:

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

Device status remains **NO** until the user physically runs the exact NRO whose SHA-256 is `809c94c842a3c385d23907c61e5ecfa201b24f08e8ac935e87a4add60770282d`.

Required physical flow:

```text
Game Sources
  -> FireRed GBA / LeafGreen GBA (RETROARCH)
  -> select source
  -> trainer/source view
  -> Party
  -> Boxes 1-14
  -> View Pokémon
```

Also confirm:

- GBA FireRed/LeafGreen are visibly distinct from the Switch releases;
- View works;
- Edit / Clone / Transfer / Move / Save / writeback remain blocked;
- no crash or missing-art regression.

Do not begin RSE until this exact artifact is physically accepted or a genuine blocker is reported and fixed with a newly hashed artifact.

## Parked interrupted RSE work

A later interrupted session produced local RSE work at:

```text
1a921515
```

It remains intentionally local/parked. Do not treat it as published or resume it before FRLG physical acceptance. After the device gate passes, inspect/preserve that local commit before reimplementing equivalent RSE work from scratch.

## Next milestone

Immediate action is **physical Switch testing of the exact FRLG browser artifact above**.

After physical acceptance, continue strict read-only Ruby/Sapphire/Emerald production support using the proven Gen III architecture and recover/reconcile the parked `1a921515` work if it still exists.

## Next major order

```text
physical FRLG browser acceptance
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

Both remain post-v1/later and must not derail the current critical path.

## Session launcher

Do not start another coding session until the exact FRLG artifact has a physical test result. After the user reports PASS or a blocker, use:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```
