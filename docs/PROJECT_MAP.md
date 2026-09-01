# PokeBank NX — Project Map

Last updated: 2026-09-01

This is the short navigation page for the project's permanent documentation, current verified state, and active GitHub work items.

---

## Read first

1. [`../PROJECT_STATUS.md`](../PROJECT_STATUS.md) — authoritative verified state and active task
2. [`NEXT_SESSION_PLAN.md`](NEXT_SESSION_PLAN.md) — exact next coding blocks after the interrupted Session 2 UI/theme work
3. [`SESSION_RUNBOOK.md`](SESSION_RUNBOOK.md) — how every coding session starts/ends
4. [`ARCHITECTURE.md`](ARCHITECTURE.md) — module boundaries and dependency direction
5. [`CONTROLS.md`](CONTROLS.md) — Pokémon HOME-like controller contract including `+` / `-`
6. [`UI_FLOW.md`](UI_FLOW.md) — safe controller-first navigation/action semantics
7. [`UI_STYLE_GUIDE.md`](UI_STYLE_GUIDE.md) — OLED Black/Dark/Light visual contract and reference-derived layouts
8. [`UPSTREAM_AUDIT.md`](UPSTREAM_AUDIT.md) — pinned PKSE/PKSM-Core/PKHeX/AutoMod/pkHouse/pkDex/PKForge research map

---

## Core architecture / UX specs

- [`ARCHITECTURE.md`](ARCHITECTURE.md) — UI/services/source adapters/engines/Vault/write-pipeline boundaries
- [`CONTROLS.md`](CONTROLS.md) — A/B/X/Y/L/R/ZL/ZR/D-pad/sticks/+/- conventions
- [`UI_FLOW.md`](UI_FLOW.md) — Action Sheet, Home/Game/Vault/Bank/Dex flows
- [`UI_STYLE_GUIDE.md`](UI_STYLE_GUIDE.md) — semantic themes, focus/cards/modal/hint bar, Select Game/Summary/Dex layout direction
- [`MASTER_VAULT_SPEC.md`](MASTER_VAULT_SPEC.md) — immutable Vault, provenance, Banks, transactions
- [`SAVE_SAFETY.md`](SAVE_SAFETY.md) — hard-lock policy and future staged live-write gates
- [`TRANSFER_MODEL.md`](TRANSFER_MODEL.md) — copy/move/clone/convert/HOME Bridge model
- [`POKEDEX_SPEC.md`](POKEDEX_SPEC.md) — Vault-driven National/Regional/Living Dex design
- [`PKHEX_ORACLE.md`](PKHEX_ORACLE.md) — host-side PKHeX/AutoMod correctness tool

---

## Integration / research notes

- [`PKSM_CORE_INTEGRATION.md`](PKSM_CORE_INTEGRATION.md) — concrete PKSM-Core `PK3` / `Sav3` integration spike
- [`PKHOUSE_REFERENCE.md`](PKHOUSE_REFERENCE.md) — modern Switch save-behavior reference notes; reimplementation only
- [`GAME_SUPPORT_MATRIX.md`](GAME_SUPPORT_MATRIX.md) — all 23 current identities and independent detect/read/round-trip/write/device gates

---

## Testing / artifacts / release

- [`DEVICE_TEST_CHECKLIST.md`](DEVICE_TEST_CHECKLIST.md) — physical Switch test procedure
- [`BUILD_RECORD.md`](BUILD_RECORD.md) — source SHA / `.nro` size / SHA-256 / device-test records
- [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md) — alpha/prerelease packaging discipline
- [`.github/workflows/host-tests.yml`](../.github/workflows/host-tests.yml) — portable host tests, ASan/UBSan, whitespace checks
- [Issue #15](https://github.com/GlitchedZeus/PokeBank-NX/issues/15) — automate/persist device-test `.nro` artifacts

---

## Historical session logs

- [`SESSION_LOG_2026-09-01.md`](SESSION_LOG_2026-09-01.md) — recovery/docs work, UI contracts, completed Action Sheet, interrupted Session 2, issue/branch updates, and end-of-day handoff

Historical logs explain how the project reached a state. They do not override `PROJECT_STATUS.md`.

---

## Completed milestone

### Issue #2 — Controller-first Pokémon Action Sheet — **CLOSED / COMPLETED**

Application source:

```text
82a0779a5143cca0690d0c7068946d84ebe9f107
```

Verification:

```text
HOST TESTED
NRO BUILDS
NOT DEVICE TESTED
```

The exact device-test artifact is recorded in `docs/BUILD_RECORD.md`.

Do not reopen/reimplement this milestone unless a real regression is found.

---

## Current active work

### 1. Issue #13 — HOME-style controls and OLED/Dark/Light UI shell

https://github.com/GlitchedZeus/PokeBank-NX/issues/13

**Current condition:** interrupted Session 2 reported substantial local-only implementation, but it ended before new-code tests/native build/commit/push.

Next coding session must first inspect local branches/worktree/reflog/stash before resetting anything.

Target reusable foundation:

```text
HOME-style controller conventions
held navigation repeat
contextual + More/Options
- Help / Controls
bottom hint bar
semantic theme tokens
OLED Black
Dark
Light
theme persistence
focus/card/modal primitives
```

### 2. Issue #8 — first recorded physical PokeBank NX `.nro` test

https://github.com/GlitchedZeus/PokeBank-NX/issues/8

The Action Sheet `.nro` already exists, but the user elected to finish issue #13 first so the next hardware pass tests the combined Action Sheet + controls/theme build.

### 3. Issue #4 — PKSM-Core Gen III integration spike

https://github.com/GlitchedZeus/PokeBank-NX/issues/4

Use **MAX/deep reasoning** after the combined UI build is tested or any blocking device regression is fixed.

Narrow target:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parse
box + party extraction
round-trip strategy
integration/dependency decision
```

### 4. Issue #3 — Master Vault v1 and named Banks

https://github.com/GlitchedZeus/PokeBank-NX/issues/3

Implement immutable raw entities, stable Vault IDs, SHA-256, provenance, transactions, and logical Bank references.

### 5. Issue #9 — Professional Pokémon Summary + provenance

https://github.com/GlitchedZeus/PokeBank-NX/issues/9

Generation-aware Summary/Origin/Legality/Provenance presentation.

### 6. Issue #6 — RetroArch discovery + read-only Gen I-III adapters

https://github.com/GlitchedZeus/PokeBank-NX/issues/6

Depends heavily on the PKSM-Core decision.

### 7. Issue #11 — Modern Switch adapter audit

https://github.com/GlitchedZeus/PokeBank-NX/issues/11

Validate inherited Switch handlers against PKHeX and pkHouse reference behavior.

### 8. Issue #5 — Host-side PKHeX Oracle

https://github.com/GlitchedZeus/PokeBank-NX/issues/5

Machine-readable legality/conversion/golden-vector oracle for native C++ testing.

### 9. Issue #7 — Vault-driven Pokédex / Living Dex

https://github.com/GlitchedZeus/PokeBank-NX/issues/7

Vault remains authoritative for ownership.

### 10. Issue #10 — Conversion/transfer engine without live writes

https://github.com/GlitchedZeus/PokeBank-NX/issues/10

Derived Vault entities + staged/exported destination representations first.

### Infrastructure: Issue #15 — persistent device artifacts

https://github.com/GlitchedZeus/PokeBank-NX/issues/15

Add GitHub Actions/prerelease artifact preservation without blocking product milestones.

---

## Architectural dependency sketch

```text
Recovered/native PKSE-PokeBank foundation
        |
        +--> Action Sheet                         DONE
        |
        +--> HOME controls + theme shell          CURRENT / INTERRUPTED LOCAL WORK
        |       |
        |       +--> combined device-test .nro
        |               |
        |               +--> physical Switch validation
        |
        +--> PKSM-Core Gen III spike              NEXT DEEP ENGINEERING
        |       |
        |       +--> read-only Gen I-III adapters
        |
        +--> Master Vault v1
        |       |
        |       +--> Named Banks
        |       +--> Provenance Summary
        |       +--> Vault-driven Pokédex
        |
        +--> modern Switch adapter audit
        |
        +--> PKHeX Oracle
                |
                +--> golden format tests
                +--> legality/conversion comparison
                +--> generated collection data

Vault + adapters + Oracle
        |
        +--> conversion/transfer engine without live writes
                |
                +--> staged-save validation
                        |
                        +--> adapter-specific live writes much later
```

---

## Near-term execution loop

```text
recover/finish issue #13
        ↓
host tests + sanitizers + diff check
        ↓
native .nro
        ↓
record application source SHA + binary SHA-256
        ↓
preserve/provide artifact
        ↓
physical Switch test
        ↓
fix device-only regression if needed
        ↓
MAX: PKSM-Core PK3/Sav3 spike
```

---

## Verification rule

A useful capability can have different levels of confidence:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

Do not collapse those into one `done` label.

Session reports about unpushed local work are not GitHub implementation evidence.

---

## Permanent safety reminder

Live installed-game save writing remains hard disabled until the gates in [`SAVE_SAFETY.md`](SAVE_SAFETY.md) are satisfied for an explicitly named adapter.

Do not remove the lock to make a demo, transfer, editor action, or UI shortcut appear complete.