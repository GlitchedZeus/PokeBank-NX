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

## Interrupted Session 3B work — NOT YET REMOTE

After `43f3a9f9...`, a later coding session reported local/uncommitted RetroArch FRLG work before timing out. Do **not** call this implemented until recovered, verified and pushed.

Reported local design/work:

- bounded read-only RetroArch source catalog;
- reads RetroArch configured `savefile_directory`;
- accepts only `.sav` / `.srm`;
- scans at most two directory levels and 256 candidates by default;
- validates full FRLG structure before trusting filename/path hints;
- exposes Party/Boxes through the existing Gen III adapter model;
- ambiguous structurally valid FRLG saves remain unclassified instead of being guessed.

The remaining reported task was to connect the catalog to the real application source-discovery lifecycle so the native linker retains it because the runtime genuinely invokes it.

## Immediate next milestone

Recover any uncommitted RetroArch FRLG work from the coding workspace and finish the end-to-end read-only runtime path:

```text
RetroArch savefile_directory
        -> bounded .sav/.srm catalog
        -> structural FRLG validation
        -> firered_gba / leafgreen_gba identity when reliable
        -> native Gen III adapter
        -> Party / Boxes read model
        -> existing PokeBank source/browser lifecycle
```

Do not create a second browser or debug UI. Do not guess FireRed vs LeafGreen when the evidence is ambiguous. Do not modify source saves.

## Next major order after FRLG runtime read path

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
