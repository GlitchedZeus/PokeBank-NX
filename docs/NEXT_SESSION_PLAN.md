# PokeBank NX — Next Session Plan

Last updated: 2026-09-07
Status: **RETROARCH FRLG RUNTIME CATALOG COMPLETE / READ-ONLY BROWSER ROUTING NEXT**

This file is intentionally short. The detailed verified engineering handoff is `CURRENT_STATUS.md`; the complete next coding instructions are `docs/NEXT_CODEX_PROMPT.md`.

## Verified remote checkpoints

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter

283073a5215a471ef0ad07619b4856409658cfdc
ci: checkout pinned PKSM-Core submodules

43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice

54cb86892d290ae1c80f447af427ff17192681f9
gen3: wire RetroArch FRLG read-only sources
```

GitHub Actions on `43f3a9f9...`: **PASS** (PokeBank NX Host Tests run #158). CI for `54cb8689...` is recorded separately once complete.

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
- recovered bounded RetroArch catalog is now published;
- `.sav`/`.srm`, configured roots, depth/candidate bounds, ambiguity, malformed input and source immutability are tested;
- UIManager owns/invokes the catalog and native parser/discovery symbols are retained in the NRO.

## Immediate task

Complete the first user-visible legacy browse route:

```text
validated FRLG catalog
-> existing Game Sources cards
-> selected legacy source
-> read-only Party / Boxes presentation
```

Reuse the existing browser and adapter. No second UI, no permissive legacy reparse, no editing, no save writes, and no guessed identity.

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
