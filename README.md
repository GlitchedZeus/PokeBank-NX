<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, transfer, provenance and save-management project for Nintendo Switch homebrew.

The project began from the PKSE codebase, but the goal is broader: one controller-first Switch application for browsing supported Pokémon saves, building a permanent local Master Vault, organizing Pokémon into named Banks, tracking provenance, building Living Dex collections and eventually moving compatible Pokémon safely between games and generations.

> **Alpha safety warning:** live installed-game save writing is not an approved current feature. Installed game sources remain read-only until an individual adapter passes explicit staged-write, validation, readback, rollback and physical-device safety gates.

---

## Current status — September 7, 2026

Current engineering focus: **Gen III FireRed/LeafGreen GBA read support + RetroArch source integration**.

Short authoritative handoff:

- [`CURRENT_STATUS.md`](CURRENT_STATUS.md)
- [`docs/NEXT_SESSION_PLAN.md`](docs/NEXT_SESSION_PLAN.md)
- [`docs/NEXT_CODEX_PROMPT.md`](docs/NEXT_CODEX_PROMPT.md)

Development branch:

```text
feature/pokebank-playable
```

Writable repository:

```text
GlitchedZeus/PokeBank-NX
```

PKSE remains **upstream only**. PokeBank NX changes must never be pushed to the original PKSE repository.

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

Physically accepted for continued development:

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

Broad UI work is intentionally frozen until the app is much closer to completion. Final startup/icon/NACP/branding polish remains tracked under issue #16.

Completed hardware/UI blockers include #13, #19, #23 and #24.

---

## Gen III engine milestone

### Session 3A — PKSM-Core host oracle / adapter

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter
```

CI/submodule follow-up:

```text
283073a5215a471ef0ad07619b4856409658cfdc
ci: checkout pinned PKSM-Core submodules
```

Pinned Core:

```text
FlagBrew/PKSM-Core
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
```

Integration decision: **ADAPTER-WRAPPER**.

The adapter is read-only and exposes PokeBank-owned data rather than PKSM-Core types. PokeBank performs stricter Gen III sector validation before the Core host oracle sees a private copy of source bytes.

Deterministic generated FRLG fixture:

```text
131072 bytes
SHA-256 b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

Verified coverage includes:

- two rotating save slots;
- all 14 sectors;
- signatures, counters and checksums;
- wrap-aware newest-slot selection;
- safe fallback to an older valid slot;
- Party + all 14 Boxes;
- an 80-byte PK3 crossing a PC-sector boundary;
- malformed/truncated rejection;
- source immutability;
- PK3 fields including species, PID, TID, SID, EXP, held item, moves, PP, IVs, EVs, nickname and OT;
- byte-identical untouched boxed/party PK3 round trips;
- independent agreement with inherited PKSE Gen III crypto.

### Session 3B — exception-free Switch-native Gen III backend

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

Full PKSM-Core is intentionally not linked directly into the Switch app because its cross-generation dependency graph reaches exception-throwing code while PokeBank NX builds native code with `-fno-exceptions`.

The native architecture is now:

```text
PokeBank Gen III API
    +-- host correctness oracle -> pinned PKSM-Core
    +-- Switch runtime backend  -> exception-free selective Gen III implementation
```

The public PokeBank API stays the same regardless of backend.

---

## Immediate next task

A later interrupted coding session reported local/uncommitted RetroArch FRLG source work. It is not considered implemented until recovered, verified and pushed.

Reported design:

```text
RetroArch configured savefile_directory
.sav / .srm only
max directory depth 2
max 256 candidates by default
full FRLG structural validation before path/name hints
ambiguous valid FRLG remains unclassified
Party / Boxes through the existing Gen III adapter
```

The next milestone is to recover that work and wire the provider/catalog into the real application source-discovery lifecycle:

```text
RetroArch save root
-> bounded read-only catalog
-> validated FRLG source
-> firered_gba / leafgreen_gba when reliable
-> native Gen III backend
-> Party / Boxes
-> existing PokeBank source/browser lifecycle
```

Issue #6 tracks this work.

---

## Planned v1 source catalog

Current host-tested source registry: **23 identities**.

Planned expansion:

```text
+ 9 Nintendo DS
+ 8 Nintendo 3DS
+ 2 Nintendo GameCube: Colosseum / XD
+ 2 Nintendo 64: Stadium / Stadium 2 (stretch)
= 44 total target identities
```

See:

- [`docs/GAME_SUPPORT_MATRIX.md`](docs/GAME_SUPPORT_MATRIX.md)
- issue #29 — v1.0 master roadmap
- issue #30 — DS/3DS identities
- issue #31 — DS Gen IV/V adapters
- issue #32 — 3DS Gen VI/VII adapters
- issue #33 — Colosseum/XD
- issue #34 — Stadium stretch

---

## Major product milestones still ahead

```text
RetroArch FRLG runtime read path
-> broader Gen III production reads
-> Master Vault + named Banks
-> Colosseum / XD
-> Gen I / II + RetroArch
-> DS / 3DS
-> modern Switch validation
-> Summary / provenance / PKHeX Oracle
-> conversion / Pokédex / legality / events
-> staged writes
-> individually approved live-write adapters
-> true Move
-> release hardening / RC / v1.0
```

Master Vault is not implemented yet. Legacy inherited Storage remains separate app-owned compatibility storage.

---

## Safety principles

- Installed Switch game sources are read-only today.
- RetroArch/legacy sources are read-only during parser/import milestones.
- No parser auto-repairs malformed source saves.
- Live writes are approved one adapter at a time only after backup/staging/readback/rollback testing.
- `COPY`, `MOVE` and `CLONE` will remain distinct operations.
- PokeBank NX does not impersonate private Nintendo/HOME protocols or forge HOME tracker/history data.

---

## Fast Codex continuation

Future coding sessions no longer need a giant pasted prompt. Send:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.
