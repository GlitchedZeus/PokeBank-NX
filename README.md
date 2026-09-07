<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, transfer, provenance and save-management project for Nintendo Switch homebrew.

The project began from the PKSE codebase, but the goal is broader: one controller-first Switch application for safely browsing supported Pokémon saves, building a permanent local Master Vault, organizing Pokémon into named Banks, tracking provenance, building Living Dex collections, and eventually moving compatible Pokémon safely between games and generations.

> **Alpha safety warning:** live installed-game save writing is not an approved current feature. Installed-game sources remain read-only until an individual adapter passes explicit staged-write, validation, readback, rollback and physical-device safety gates.

---

## Current verified status — September 7, 2026

Active development is on:

```text
feature/pokebank-playable
```

The `main` branch intentionally does **not** receive every in-progress engine commit immediately. This README reflects the latest verified project state, while current implementation work lives on the development branch until an appropriate merge gate is reached.

Writable repository:

```text
GlitchedZeus/PokeBank-NX
```

PKSE remains **upstream only**. PokeBank NX changes must never be pushed to the original PKSE repository.

Authoritative live-development handoff files:

- [CURRENT_STATUS.md on feature/pokebank-playable](https://github.com/GlitchedZeus/PokeBank-NX/blob/feature/pokebank-playable/CURRENT_STATUS.md)
- [NEXT_SESSION_PLAN.md](https://github.com/GlitchedZeus/PokeBank-NX/blob/feature/pokebank-playable/docs/NEXT_SESSION_PLAN.md)
- [NEXT_CODEX_PROMPT.md](https://github.com/GlitchedZeus/PokeBank-NX/blob/feature/pokebank-playable/docs/NEXT_CODEX_PROMPT.md)
- [v1.0 roadmap](https://github.com/GlitchedZeus/PokeBank-NX/blob/feature/pokebank-playable/docs/V1_ROADMAP.md)
- [game support matrix](https://github.com/GlitchedZeus/PokeBank-NX/blob/feature/pokebank-playable/docs/GAME_SUPPORT_MATRIX.md)

Current engineering focus:

```text
Recover + finish RetroArch FireRed/LeafGreen GBA runtime read path
```

---

## Foundation / playable shell

Completed:

- Repository recovery + Git/upstream safety discipline.
- Stable 23-game current release/platform identity registry.
- Low-level live installed-save write hard lock.
- Controller-first Pokémon Action Sheet (#2).
- HOME-style controller/theme foundation.
- First exact physical Switch `.nro` milestone (#8).
- Extended device torture testing and follow-up reports.
- Installed-source mutation-path safety audit + UI lock (#23).
- Old/malformed Legends: Arceus crash hardening (#24).
- Real physical Left Stick navigation (#19).
- Visible PokeBank NX shell accepted for continued development (#13).
- HD Pokémon artwork packaged and rendered on hardware.
- Artificial inherited sprite breathing/bobbing removed.
- Red PokeBank NX identity accepted for now.
- Permanent left-side inherited accent bar removed.
- Required visual-asset preflight + exact-artifact packaging tooling.

Still later / not complete:

- Final startup/icon/NACP/branding polish near release (#16).
- Final controller semantics/hints normalization (#26).
- Persistent device-test artifact automation (#15).
- Final docked/handheld/release-candidate UI polish.

Broad UI redesign is intentionally **frozen for now** so development can focus on real Pokémon/save functionality.

---

## Current accepted Switch UI / hardware milestone

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

Physical acceptance for continued development:

| Area | Result |
|---|---|
| D-pad navigation | **PASS** |
| Left Stick navigation | **PASS** |
| Red PokeBank NX identity | **ACCEPTED FOR NOW** |
| Permanent left-side accent bar | **REMOVED** |
| HD Pokémon artwork | **PASS** |
| Artificial sprite breathing/bobbing | **REMOVED** |
| Old/problem Legends: Arceus source | **GRACEFUL ERROR / NO CRASH** |
| Installed-source live write | **HARD DISABLED** |

The UI can be revisited for final polish when the application is much closer to completion.

---

## Gen III engine — major milestone reached

PokeBank NX now has a real tested FireRed/LeafGreen GBA read engine on the development branch.

### Session 3A — PKSM-Core host oracle / adapter

Implementation source:

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter
```

CI/submodule follow-up:

```text
283073a5215a471ef0ad07619b4856409658cfdc
ci: checkout pinned PKSM-Core submodules
```

Pinned PKSM-Core:

```text
FlagBrew/PKSM-Core
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
```

Integration decision:

```text
ADAPTER-WRAPPER
```

The public boundary is PokeBank-owned and does not expose PKSM-Core types. PokeBank performs stricter Gen III validation before the Core host oracle sees a private copy of source bytes.

Deterministic generated FRLG fixture:

```text
size:    131072 bytes
SHA-256: b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

Verified behavior includes:

- two rotating save slots;
- all 14 Gen III sectors;
- sector IDs, signatures, counters and checksums;
- wrap-aware newest-slot selection;
- safe fallback to an older valid slot when a newer slot is corrupt;
- Party enumeration;
- all 14 Boxes;
- an 80-byte PK3 crossing a PC-sector boundary;
- malformed/truncated/invalid save rejection;
- source immutability;
- PK3 checksum validation;
- species, PID, TID, SID, EXP, held item, moves, PP, IVs, EVs, nickname and OT extraction;
- independent agreement with the inherited PKSE Gen III crypto implementation;
- byte-identical untouched 80-byte boxed and 100-byte party PK3 round trips.

Issue #4 is complete.

---

## Session 3B — exception-free Switch-native Gen III backend

Verified native engineering checkpoint:

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

Full PKSM-Core is intentionally not linked directly into the Switch app because its cross-generation dependency graph reaches exception-throwing code while PokeBank NX uses `-fno-exceptions` for native code.

The chosen architecture is:

```text
PokeBank Gen III API
    ├── host correctness oracle -> pinned PKSM-Core
    └── Switch runtime backend  -> exception-free selective Gen III implementation
```

The same public adapter API is preserved on both sides.

The normal Switch build now includes the Gen III integration source directory.

---

## Current in-progress work — RetroArch FireRed / LeafGreen GBA

A later interrupted coding session reported local RetroArch FRLG discovery/runtime work after the native Gen III checkpoint. That work is **not considered implemented until recovered, verified and pushed**.

Reported design/work already attempted:

```text
RetroArch configured savefile_directory
.sav / .srm candidates only
maximum directory depth: 2
maximum candidates: 256 by default
full FRLG structural validation before path/name hints
ambiguous structurally valid FRLG remains unclassified
Party / Boxes through the existing Gen III adapter model
```

The next development milestone is:

```text
RetroArch savefile_directory
        ↓
bounded read-only catalog
        ↓
validated FRLG source
        ↓
firered_gba / leafgreen_gba when evidence is reliable
        ↓
native exception-free Gen III backend
        ↓
Party / Boxes read model
        ↓
existing PokeBank source/browser lifecycle
```

The provider must be genuinely owned/invoked by the runtime rather than existing as dead code that the linker can discard.

Issue #6 tracks the wider RetroArch + Gen I–III read-only source work.

---

## Current game/source catalog

Current host-tested source registry:

```text
23 release/platform identities
```

Planned expansion:

```text
+ 9 Nintendo DS
+ 8 Nintendo 3DS
+ 2 Nintendo GameCube: Colosseum / XD
+ 2 Nintendo 64: Stadium / Stadium 2 (stretch)
= 44 total target identities
```

Important identity rule:

```text
firered_gba      != firered_switch
leafgreen_gba    != leafgreen_switch
```

GBA and Switch releases remain distinct sources/platforms throughout provenance and transfer logic.

Detailed matrix:

[docs/GAME_SUPPORT_MATRIX.md on the development branch](https://github.com/GlitchedZeus/PokeBank-NX/blob/feature/pokebank-playable/docs/GAME_SUPPORT_MATRIX.md)

---

## Master Vault / Banks — planned core product layer

Master Vault is **not implemented yet**.

The planned Vault is the permanent game-independent storage/provenance layer with concepts such as:

```text
immutable raw Pokémon payloads
stable Vault IDs
SHA-256 integrity
source/origin provenance
active/current location separate from origin
parent / derived lineage
transaction journal + crash recovery
named Banks as organization/reference layers
rebuildable search/metadata indexes
```

Legacy inherited Storage remains a separate app-owned compatibility area for now and is not automatically the future Master Vault.

Issue #3 tracks Master Vault + Banks.

---

## Long-term transfer semantics

PokeBank NX distinguishes:

```text
COPY
= keep the source active and intentionally create a destination representation

MOVE
= relocate the active Pokémon only after destination success is verified

CLONE
= intentionally create a duplicate with clone provenance
```

True Move is a later milestone (#20) and is **not implemented today**.

---

## Safety principles

- Installed Switch game sources are read-only today.
- RetroArch/legacy sources are read-only during parser/import milestones.
- No parser silently repairs or overwrites malformed source saves.
- Live writes are approved one adapter at a time only after backup/staging/readback/rollback testing.
- The low-level live installed-save write lock remains hard-disabled.
- `COPY`, `MOVE` and `CLONE` stay distinct user-facing operations.
- PokeBank NX does not impersonate private Nintendo/HOME protocols or forge HOME tracker/history data.

---

## Road to v1.0

Current preferred implementation order:

```text
RetroArch FRLG runtime read path
        ↓
broader Gen III production reads
        ↓
Master Vault + named Banks
        ↓
Colosseum / XD
        ↓
Gen I / II + RetroArch
        ↓
Stadium stretch
        ↓
Nintendo DS Gen IV / V
        ↓
Nintendo 3DS Gen VI / VII
        ↓
modern Switch adapter validation
        ↓
Summary / provenance / PKHeX Oracle
        ↓
conversion / Pokédex / legality / events
        ↓
staged writes
        ↓
individually approved live-write adapters
        ↓
true Move
        ↓
release hardening / RC / v1.0
```

Master roadmap:

[Issue #29 — Track PokeBank NX v1.0 roadmap and release gates](https://github.com/GlitchedZeus/PokeBank-NX/issues/29)

---

## Fast Codex continuation

Future coding sessions do not need a giant pasted prompt.

Send:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

The full current handoff and next task live on GitHub.

---

## Project status vocabulary

PokeBank NX deliberately separates these claims:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

`DEVICE TESTED` means a human physically ran the exact recorded `.nro` / SHA-256 on Switch hardware.

Documentation branch HEAD is not automatically the application-source identity of a device-tested artifact.

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.
