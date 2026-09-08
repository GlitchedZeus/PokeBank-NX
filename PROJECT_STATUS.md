# PokeBank NX Project Status


## FRLG read-only browser completion — READY FOR PHYSICAL DEVICE TEST

```text
Application source SHA: 5d3e5e23f352dda4900ae42b4f396d9d4a4b8b8e
Application commit: gen3: complete FRLG read-only source browsing
Embedded version: 0.1.0-alpha (5d3e5e23)
Artifact: PokeBank-NX-FRLG-Complete-5d3e5e23.nro
Artifact size: 156249001 bytes
Artifact SHA-256: 8dd94277e609c96f37e82b0b0b47928bad50ce36fdb380520bd2521ab18ec78a
Device tested: NO
```

Implemented and verified in this source:

- bounded startup, parent-open, and manual X Refresh Saves discovery;
- configured RetroArch `savefile_directory` precedence, with the conventional root used only as fallback;
- exact `firered_gba` / `leafgreen_gba` parent cards and deduplicated child save instances;
- stale/deleted save invalidation via normalized path, file size, and modification time;
- immutable Party and all 14 Boxes browsing;
- Gen III trainer name, gender, TID16, SID16, ID32, security-key-decrypted money, and exact source identity;
- read-only Items, Key Items, Poke Balls, TM Case, Berry Pouch, and plaintext PC Items;
- malformed inventory rejection and unchanged source bytes;
- 12 host suites PASS, ASan/UBSan PASS, `git diff --check` PASS, native `-fno-exceptions` build PASS;
- pinned 3,260-render asset set and all 1,025 base species present;
- extracted embedded RomFS: 3,283 / 3,283 files byte-identical to source.

The parked RSE recovery checkpoint remains isolated and must not be merged, resumed, or pushed before this exact FRLG artifact is physically tested. Live installed-game and RetroArch source writes remain HARD DISABLED.

Last updated: 2026-09-08

This is the current verified project-state summary. For the shortest coding handoff, read `CURRENT_STATUS.md`. For the next Codex task, execute `docs/NEXT_CODEX_PROMPT.md`.

## Project identity

- Product: **PokeBank NX**
- Version: `0.1.0-alpha`
- Repository: `GlitchedZeus/PokeBank-NX`
- Development branch: `feature/pokebank-playable`
- Writable remote: `origin`
- Upstream-only remote: `upstream` (`kiasta/PKSE`)
- Live installed-game save writing: **HARD DISABLED**
- Canonical v1 tracker: issue #29 / `docs/V1_ROADMAP.md`

Verification vocabulary:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

`DEVICE TESTED` applies only to the exact recorded artifact/hash physically run on Switch.

---

## UI / hardware checkpoint — accepted and frozen for now

Accepted application source:

```text
af2acf043a15dbf48b8195880a80cc5de562fced
ui: adopt red PokeBank identity accents
```

Accepted artifact:

```text
PokeBank-NX-Red-UI-af2acf04.nro
size      155117481 bytes
SHA-256   898df286cf34b895f1f71f4abc35f0818e4afa66725b67c2d020fc20c01bfac4
```

Current physical acceptance:

```text
D-pad navigation                         PASS
Left Stick navigation                    PASS
PokeBank NX red identity                ACCEPTED FOR NOW
permanent left-side accent bar          REMOVED / PASS
HD Pokémon artwork                      PASS
artificial idle/breathing motion        REMOVED / ACCEPTED
old/problem Legends: Arceus source      GRACEFUL ERROR / NO CRASH
installed-source live-write path        HARD DISABLED
```

The user explicitly considers the current UI good enough for development and wants broad visual polish deferred until the application is close to completion.

Closed hardware/UI blockers:

```text
#13 visible PokeBank NX shell
#19 physical Left Stick navigation
#24 old/malformed PLA crash handling
```

Issue #16 remains open for final branding/startup/NRO polish near release.

---

## Save safety state

Installed game sources are read-only in the current product contract. Session 2.6 implemented source-state guards and PLA defensive parsing on application source:

```text
0ea98cc1a9f9dfc2b17abc33e944caa4aa9de915
safety: lock installed-source UI and harden PLA reads
```

The low-level hard lock preventing live installed-title writes remains in place. Explicit backup/staged workspaces and legacy app-owned Storage remain separate from live installed sources.

Legacy Storage is still inherited app-owned storage, not the future Master Vault.

---

## Session 3A — PKSM-Core Gen III read-only adapter

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

Integration decision: **ADAPTER-WRAPPER**.

PokeBank NX exposes a PokeBank-owned Gen III API; no PKSM-Core type leaks through the public boundary or UI.

Strict PokeBank validation is performed in front of pinned Core because `Sav3::isValid()` alone does not meet PokeBank NX's safety requirements.

Deterministic generated FRLG fixture:

```text
size      131072 bytes
SHA-256   b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

Proven read-only behavior:

- two rotating save slots;
- all 14 sector identities;
- signatures, counters and checksums;
- wrap-aware newest-slot selection;
- safe fallback to an older valid slot;
- Party enumeration;
- all 14 Boxes;
- 80-byte PK3 extraction across a PC-sector boundary;
- malformed/truncated/invalid structure rejection;
- source bytes remain unchanged;
- PK3 entity checksum validation;
- species, PID, TID, SID, EXP, held item, moves, PP, IVs, EVs, nickname and OT;
- inherited PKSE Gen III crypto cross-check agrees;
- untouched 80-byte boxed and 100-byte party PK3 round trips are byte-identical.

Session 3A host verification reached ten suites and ASan/UBSan PASS.

---

## Session 3B checkpoint A — native exception-free Gen III backend

Verified native checkpoint:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

GitHub Actions:

```text
PokeBank NX Host Tests
run #158
PASS
```

Full PKSM-Core is not linked directly into the Switch app because its cross-generation virtual/conversion closure reaches exception-throwing code while PokeBank NX intentionally builds native code with `-fno-exceptions`.

The selected architecture keeps the same public adapter API and separates host correctness-oracle behavior from native runtime behavior:

```text
HOST:
PokeBank Gen III API
    -> PKSM-Core PK3 / Sav3 / SavFRLG

SWITCH:
PokeBank Gen III API
    -> PKSMGen3NativeAdapter.cpp
```

The native backend selectively implements the already-proven read-only Gen III semantics without globally enabling exceptions or pulling unrelated later-generation PKSM-Core code into the app.

`src/Integration/Gen3` is now compiled by the normal native Makefile.

Current integration classification:

```text
ADAPTER-WRAPPER
+ SELECTIVE NATIVE BACKEND
```

This is based on measured devkitA64 / `-fno-exceptions` evidence, not preference.

---

## Session 3B checkpoint B — RetroArch FRLG runtime catalog

Canonical implementation source:

```text
54cb86892d290ae1c80f447af427ff17192681f9
gen3: wire RetroArch FRLG read-only sources
```

Implemented runtime path:

```text
RetroArch savefile_directory
        -> bounded read-only .sav/.srm catalog
        -> FRLG structural validation
        -> firered_gba / leafgreen_gba when reliable
        -> native Gen III backend
        -> Party / Boxes model
        -> UIManager-owned application-session catalog
```

Properties and verification:

- RetroArch `savefile_directory`, plus the conventional save root only when present;
- `.sav` / `.srm` only, default depth 2 and candidate limit 256;
- strict FRLG-family validation before identity hints;
- ambiguous valid FRLG remains unclassified rather than guessed;
- exact GBA identities remain separate from Switch FireRed/LeafGreen;
- Party and all Boxes remain available through the validated adapter result;
- source files remain unchanged; no repair, resign or writeback API exists;
- eleven host suites PASS;
- ASan/UBSan PASS;
- `git diff --check` PASS;
- native devkitA64 `-fno-exceptions` build PASS;
- native discovery/parser symbols retained in the ELF;
- NRO size: 62,666,225 bytes, an increase of 28,672 bytes.

Issue #6 tracks this legacy/RetroArch path.

State: **IMPLEMENTED / HOST TESTED / NRO BUILDS / NOT DEVICE TESTED**.

---

## Session 3C — RetroArch FRLG Game Sources and read-only browser

Canonical application source:

```text
f6a3052daeffe7cd30d7acceba81a5dfda7615ee
gen3: expose RetroArch FRLG game sources
```

Recovered local checkpoint with the same application tree:

```text
25fc12181bf1fffbe3f0a06b56dc9d676a0c29f0
```

Implemented:

- validated ready FireRed/LeafGreen entries become normal cards in the existing Game
  Sources browser;
- cards show `Game Boy Advance` and `RETROARCH`, with exact `firered_gba` /
  `leafgreen_gba` identities distinct from Switch releases;
- activation resolves the exact UIManager-owned catalog entry and never reparses the
  file through permissive legacy slot selection;
- `FRLGReadOnlyTrainer` maps strict adapter Party/Box records into the existing browser;
- `RetroArchLegacy` is View-only and every mutation action remains blocked;
- source bytes remain unchanged and there is no legacy save serializer/writeback path.

Verification:

```text
host suites                 12 PASS
ASan/UBSan                  PASS
git diff --check            PASS
native devkitA64 build      PASS
GitHub Actions run #184     PASS
NRO size                    62694897 bytes
device test                 NOT RUN
```

State: **IMPLEMENTED / HOST TESTED / NRO BUILDS / NOT DEVICE TESTED**.

---

## Immediate next milestone

Physically verify the exact FRLG source-card and Party/Boxes route. Once accepted, add
strict read-only Ruby/Sapphire/Emerald production sources through the same architecture.
Do not enable edits, conversion, writeback or live installed-title writes.

---

## Current roadmap order

After the FRLG runtime path:

```text
Gen III production reads
        -> Master Vault + Banks foundation
        -> Colosseum / XD
        -> Gen I / II + RetroArch
        -> Stadium 1/2 stretch
        -> DS Gen IV/V
        -> 3DS Gen VI/VII
        -> modern Switch adapter validation
        -> Summary / provenance / PKHeX Oracle
        -> conversion / Dex / legality / events
        -> staged writes
        -> individually approved live-write adapters
        -> true Move
        -> release hardening / RC / v1.0
```

Master Vault, true Move, live writes, Android/USB Link, Companion Mode, Move Lab and similar later features are not part of the current Session 3C work.

---

## Important historical source checkpoints

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40   hard-lock live game save writes
82a0779a5143cca0690d0c7068946d84ebe9f107   controller Pokémon Action Sheet
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a   first physically tested controller/theme source
361c6f551496470db305948d702944c6ed9889c1   visible PokeBank shell + physical stick source
0ea98cc1a9f9dfc2b17abc33e944caa4aa9de915   installed-source safety + PLA hardening
af2acf043a15dbf48b8195880a80cc5de562fced   accepted red UI identity
936e75d98daa7e61fcf8ea199bcda958b1b78d7a   PKSM-Core FRLG host adapter
283073a5215a471ef0ad07619b4856409658cfdc   recursive PKSM-Core CI checkout
43f3a9f90a3314725979d59afdd68f19ee159009   exception-free native Gen III backend
```

---

## Compact next-session launcher

Future coding sessions should not require a large pasted prompt. Use:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```
