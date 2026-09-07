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

Proven behavior:

- two rotating save slots / 14-sector structure;
- sector IDs, signatures, counters and checksums;
- wrap-aware newest-slot selection;
- safe fallback to the older valid slot;
- party and all 14 boxes;
- an 80-byte PK3 crossing a PC-sector boundary;
- malformed/truncated/invalid structure rejection;
- source bytes remain unchanged;
- PK3 fields: species, PID, TID, SID, EXP, held item, moves, PP, IVs, EVs, nickname and OT;
- inherited PKSE crypto independently agrees;
- untouched 80-byte boxed and 100-byte party PK3 round trips are byte-identical.

## Session 3B checkpoint A — native exception-free Gen III backend

Current verified engineering checkpoint:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

GitHub Actions:

```text
PokeBank NX Host Tests — run #158 — PASS
```

Full pinned PKSM-Core cannot be linked directly into the Switch app under the normal `-fno-exceptions` build because unrelated/cross-generation PKSM-Core dependencies reach throwing code. The chosen solution keeps the same PokeBank adapter API and uses:

```text
HOST / correctness oracle:
PKSMGen3Adapter -> pinned PKSM-Core PK3/Sav3/SavFRLG

SWITCH NATIVE:
same PokeBank adapter API -> PKSMGen3NativeAdapter.cpp
```

The native backend selectively implements the already-proven Gen III read semantics without enabling exceptions globally or pulling unrelated later-generation Core code into the Switch app.

`src/Integration/Gen3` is now part of the normal native `SOURCES` list.

## Session 3B checkpoint B — RetroArch FRLG runtime catalog

Canonical implementation source:

```text
54cb86892d290ae1c80f447af427ff17192681f9
gen3: wire RetroArch FRLG read-only sources
```

The interrupted source was recovered on local ref `recovery/session3b-retroarch-wip-20260907`, verified, and published as one canonical remote commit. The runtime path is now:

```text
RetroArch savefile_directory
        -> bounded read-only .sav/.srm catalog
        -> structural FRLG validation
        -> firered_gba / leafgreen_gba identity when reliable
        -> native Gen III adapter
        -> Party / Boxes read model
        -> UIManager-owned application-session catalog
```

Only configured/conventional RetroArch roots are visited; the app never crawls the full SD card. Traversal defaults to depth 2 and 256 `.sav`/`.srm` candidates. A filename/path hint is used only after strict FRLG-family validation. Structurally valid but ambiguous saves remain unclassified. Invalid unrelated saves are suppressed, while named FRLG candidates retain typed diagnostics. All file opens are read-only and tests prove the source bytes do not change.

The normal Switch runtime owns and invokes the catalog at startup. Native symbols for discovery and the exception-free Gen III parser are retained in the final ELF; the NRO grows by 28,672 bytes from 62,637,553 to 62,666,225 bytes.

Verification: eleven host suites PASS, ASan/UBSan PASS, `git diff --check` PASS, and native devkitA64 `-fno-exceptions` build PASS. This path is **IMPLEMENTED / HOST TESTED / NRO BUILDS**, not DEVICE TESTED.

## Session 3C — selectable RetroArch FRLG browsing

Canonical application source:

```text
f6a3052daeffe7cd30d7acceba81a5dfda7615ee
gen3: expose RetroArch FRLG game sources
```

Recovered local checkpoint with the same application tree:

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

Only strict `Ready` sources become selectable. Cards carry `firered_gba` or
`leafgreen_gba`, show `Game Boy Advance` / `RETROARCH`, and remain distinct from the
Switch release identities. Selection resolves back through the session-owned catalog;
the file is not reparsed through permissive `Trainer3FRLG` logic.

`FRLGReadOnlyTrainer` is populated only from validated adapter records. It exposes Party
and all 14 Boxes to the existing renderer, has no source serializer, returns no blank
Pokémon for insertion, and has inert update virtuals. `RetroArchLegacy` permits View
only; Edit, Clone, Transfer, Direct Move and Save Changes remain blocked. Tests prove
the fixture bytes remain unchanged across card creation, resolution and view-model
construction.

Verification: twelve host suites PASS; ASan/UBSan PASS; `git diff --check` PASS;
GitHub Actions run #184 PASS; clean native devkitA64 `-fno-exceptions` build PASS.
The NRO is 62,694,897 bytes, +28,672 bytes over the catalog-only build and +57,344
bytes over the pre-runtime baseline.

State: **IMPLEMENTED / HOST TESTED / NRO BUILDS / NOT DEVICE TESTED**.

## Immediate next milestone

Physically verify the exact FRLG Game Sources -> Party/Boxes route on Switch. After that
acceptance, extend the same strict read-only Gen III production pipeline to Ruby,
Sapphire and Emerald without enabling mutation or live writes.

## Next major order after FRLG runtime catalog

Current preferred implementation order remains:

```text
FRLG RetroArch read path
-> Gen III production reads
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

## Session launcher

For the next coding session, do not paste a wall of text. Use:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```
