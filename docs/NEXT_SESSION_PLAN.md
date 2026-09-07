# PokeBank NX — Next Session Plan

Last updated: 2026-09-07
Status: **GEN III NATIVE BACKEND COMPLETE / RETROARCH FRLG RUNTIME WIRING NEXT**

This file is intentionally short. The detailed verified engineering handoff is `CURRENT_STATUS.md`; the complete next coding instructions are `docs/NEXT_CODEX_PROMPT.md`.

## Verified remote checkpoints

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter

283073a5215a471ef0ad07619b4856409658cfdc
ci: checkout pinned PKSM-Core submodules

43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

GitHub Actions on `43f3a9f9...`: **PASS** (PokeBank NX Host Tests run #158).

## What is complete

- UI accepted/frozen for now; return to final polish near release.
- Left Stick hardware fix accepted.
- old/problem PLA source now fails gracefully instead of crashing.
- installed-source live-write lock remains hard disabled.
- pinned PKSM-Core host oracle and PokeBank-owned FRLG adapter are implemented.
- deterministic 128 KiB FRLG fixture and corruption coverage exist.
- Party + all 14 boxes parse.
- PK3 boundary-crossing extraction and byte-identical untouched round trips are proven.
- native Switch Gen III backend now compiles without exceptions behind the same public adapter API.

## Interrupted local work to recover

A coding session after `43f3a9f9...` reported local RetroArch FRLG work but timed out before a remote checkpoint. Treat it as **unverified until recovered and pushed**.

Reported design/work:

```text
RetroArch configured savefile_directory
.sav / .srm only
max depth 2
max 256 candidates
strict FRLG validation before filename/path hints
ambiguous valid FRLG remains unclassified
Party / Boxes via existing Gen III adapter
```

Remaining reported task: connect the catalog to the real application source-discovery lifecycle so the runtime genuinely invokes it and the linker retains it.

## Immediate task

Recover that local work first, then complete:

```text
RetroArch save root
-> bounded source catalog
-> FRLG structural validation
-> reliable firered_gba / leafgreen_gba identity
-> native Gen III adapter
-> Party / Boxes
-> existing PokeBank source/browser lifecycle
```

No UI redesign. No save writes. No broad SD-card crawl. No guessing FireRed vs LeafGreen when ambiguous.

## After FRLG runtime is complete

Preferred order:

```text
Gen III production reads
-> Master Vault + Banks
-> Colosseum / XD
-> Gen I / II + RetroArch
-> Stadium stretch
-> DS
-> 3DS
-> modern Switch adapter validation
-> Summary / Oracle / conversion / Dex / legality / events
-> staged writes
-> individually approved live writes
-> true Move
-> RC / v1.0
```

## One-line launcher

Use this for the next Codex session:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```
