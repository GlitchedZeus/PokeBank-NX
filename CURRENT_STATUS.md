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
- Preserve any local/uncommitted work before syncing or changing refs.

## Accepted UI / physical state

The current broad UI is intentionally frozen until the app is much closer to completion.

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
- old/problem Legends: Arceus save no longer crashes and returns a graceful missing-main-file error;
- installed-source read-only safety remains in place.

Closed hardware/safety/UI blockers include #13, #19, #23 and #24. Final branding/startup/NRO polish remains later under #16.

## Gen III host oracle / adapter

Verified host implementation:

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

The public boundary is PokeBank-owned and does not expose PKSM-Core types. Host-side PKSM-Core remains the correctness oracle/reference for the already-proven Gen III semantics.

Deterministic generated FRLG fixture:

```text
size    131072 bytes
SHA-256 b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

Proven behavior includes rotating save slots, all 14 sectors, signatures/counters/checksums, wrap-aware newest-slot selection, safe older-slot fallback, Party, all 14 Boxes, sector-boundary PK3 extraction, malformed/truncated rejection, source immutability and byte-identical untouched PK3 round trips.

## Exception-free Switch-native Gen III backend

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

## RetroArch FRLG runtime discovery — IMPLEMENTED / PUSHED

Current branch engineering head:

```text
54cb86892d290ae1c80f447af427ff17192681f9
gen3: wire RetroArch FRLG read-only sources
```

This checkpoint recovered and completed the interrupted Session 3B work.

Implemented behavior:

- reads configured RetroArch `savefile_directory`;
- considers only `.sav` / `.srm` candidates;
- bounded traversal: depth <= 2 and 256 candidates by default;
- strict full FRLG structure validation before trusting filename/path identity hints;
- ambiguous structurally valid FRLG-family saves remain unclassified instead of guessed;
- `firered_gba` / `leafgreen_gba` remain distinct from `firered_switch` / `leafgreen_switch`;
- Party/Boxes use the existing native Gen III read-only model;
- application startup/lifecycle owns the discovered FRLG catalog for the app session;
- the runtime genuinely invokes/retains the provider rather than relying on dead/unreferenced code;
- source files remain read-only and unchanged;
- no installed Switch title writes were enabled.

The current honest boundary is important:

```text
DONE:
RetroArch path discovery
-> bounded candidate scan
-> strict FRLG validation
-> reliable identity assignment when possible
-> native Party/Boxes parse model
-> application-lifecycle ownership

NOT YET DONE:
validated FRLG records exposed as normal selectable Game Source cards/entries in the existing user-facing source browser
```

Do not call the user-facing RetroArch FRLG browsing milestone device-complete until those records are actually selectable through the normal app UI and physically tested.

## Immediate next milestone

Expose the already-discovered validated RetroArch FireRed/LeafGreen GBA records through the **existing PokeBank Game Sources/source browser lifecycle**.

Target:

```text
validated legacyFRLGSources entry
        -> existing source/card descriptor
        -> FireRed GBA / LeafGreen GBA card with clear RetroArch/GBA labeling
        -> select card
        -> existing Party / Boxes browsing path
```

Rules:

- no second browser/debug UI;
- no duplicate FRLG parser/discovery implementation;
- no broad UI redesign;
- invalid/ambiguous sources must not masquerade as valid FireRed/LeafGreen cards;
- preserve `firered_gba` / `leafgreen_gba` identity separation from the Switch releases;
- keep all legacy-source operations read-only;
- preserve the live installed-save write hard lock.

Stop after a coherent pushed user-facing FRLG source-card/browser checkpoint. Do not immediately wander into RSE, Gen I/II, Vault/Banks or unrelated UI work in the same coding session.

## Next major order after the FRLG source-card milestone

Preferred implementation order remains:

```text
user-facing RetroArch FRLG source cards / browse path
-> broader Gen III production reads (Ruby/Sapphire/Emerald)
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

Two newer ideas are intentionally saved as post-v1/later work and must not derail the current critical path:

```text
#46  Gift, Event and Mystery Gift Library + EventDex
#47  PokeBank NX Link for real GB/GBC/GBA hardware transfers
```

Issue #46 covers the visual historical Gift/Event/Mystery Gift browser, EventDex/GiftDex, archived distribution reconstruction, legality-vs-provenance honesty and HOME/server provenance limits.

Issue #47 covers a future real-hardware link path:

```text
real GB/GBC/GBA
-> real link cable
-> MCU/USB PokeBank Link bridge
-> Nintendo Switch
-> PokeBank NX
-> Master Vault / compatible trade workflow
```

Both are explicitly parked until most heavy lifting/core architecture is complete.

## Session launcher

For the next coding session use:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```
