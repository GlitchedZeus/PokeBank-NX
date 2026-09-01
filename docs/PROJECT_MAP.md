# PokeBank NX — Project Map

Last updated: 2026-09-01

This is the short navigation page for the project's permanent documentation and active GitHub work items.

## Read first

1. [`../PROJECT_STATUS.md`](../PROJECT_STATUS.md) — current verified state and active task
2. [`SESSION_RUNBOOK.md`](SESSION_RUNBOOK.md) — how every coding session starts/ends
3. [`UPSTREAM_AUDIT.md`](UPSTREAM_AUDIT.md) — pinned PKSE/PKSM-Core/PKHeX/AutoMod/pkHouse/pkDex/PKForge research map

## Core architecture specs

- [`MASTER_VAULT_SPEC.md`](MASTER_VAULT_SPEC.md) — immutable Vault, provenance, banks, transactions
- [`SAVE_SAFETY.md`](SAVE_SAFETY.md) — hard-lock policy and future staged live-write gate
- [`TRANSFER_MODEL.md`](TRANSFER_MODEL.md) — copy/move/clone/convert/HOME Bridge model
- [`POKEDEX_SPEC.md`](POKEDEX_SPEC.md) — Vault-driven National/Regional/Living Dex design
- [`PKHEX_ORACLE.md`](PKHEX_ORACLE.md) — host-side PKHeX/AutoMod correctness tool

## Integration / research notes

- [`PKSM_CORE_INTEGRATION.md`](PKSM_CORE_INTEGRATION.md) — concrete PKSM-Core PK3/Sav3 integration spike and decision gate
- [`PKHOUSE_REFERENCE.md`](PKHOUSE_REFERENCE.md) — concrete modern Switch save behavior/reference notes; reimplementation only

## Testing / artifacts

- [`DEVICE_TEST_CHECKLIST.md`](DEVICE_TEST_CHECKLIST.md) — physical Switch test procedure
- [`BUILD_RECORD.md`](BUILD_RECORD.md) — recovery build SHA distinction and future artifact records
- [`.github/workflows/host-tests.yml`](../.github/workflows/host-tests.yml) — automatic host tests, ASan/UBSan, and whitespace checks on GitHub pushes/PRs

## Current GitHub issue order

The first active implementation issue is the action sheet. Subsequent issues are not required to be completed strictly numerically; follow dependencies and `PROJECT_STATUS.md`.

1. [#2 — Implement controller-first Pokémon action sheet](https://github.com/GlitchedZeus/PokeBank-NX/issues/2)
2. [#4 — Audit and spike PKSM-Core Gen III integration](https://github.com/GlitchedZeus/PokeBank-NX/issues/4)
3. [#3 — Implement Master Vault v1 and named banks](https://github.com/GlitchedZeus/PokeBank-NX/issues/3)
4. [#8 — Produce and physically test first recovery-era `.nro`](https://github.com/GlitchedZeus/PokeBank-NX/issues/8) — produce the test artifact as soon as a useful milestone is ready; physical testing can happen while deeper architecture work continues
5. [#9 — Build professional Pokémon summary and provenance view](https://github.com/GlitchedZeus/PokeBank-NX/issues/9)
6. [#6 — Rebuild RetroArch discovery and read-only Gen I–III adapters](https://github.com/GlitchedZeus/PokeBank-NX/issues/6)
7. [#11 — Audit modern Switch save adapters against pkHouse and PKHeX](https://github.com/GlitchedZeus/PokeBank-NX/issues/11)
8. [#5 — Build host-side PKHeX Oracle](https://github.com/GlitchedZeus/PokeBank-NX/issues/5)
9. [#7 — Implement Vault-driven Pokédex and Living Dex v1](https://github.com/GlitchedZeus/PokeBank-NX/issues/7)
10. [#10 — Implement conversion/transfer engine without live writes](https://github.com/GlitchedZeus/PokeBank-NX/issues/10)

## Architectural dependency sketch

```text
Current native PKSE/PokeBank foundation
        |
        +--> Action sheet
        |
        +--> PKSM-Core audit
        |       |
        |       +--> read-only Gen I-III adapters
        |
        +--> modern Switch adapter audit
        |       |
        |       +--> read-only ZA/SV/SwSh/PLA/BDSP/LGPE/FRLG validation
        |
        +--> Master Vault v1
        |       |
        |       +--> Named Banks
        |       +--> Provenance Summary
        |       +--> Vault-driven Pokédex
        |
        +--> PKHeX Oracle
                |
                +--> golden format tests
                +--> legality/conversion comparison
                +--> future encounter-driven generation

Vault + adapters + Oracle
        |
        +--> conversion/transfer engine without live writes
                |
                +--> staged-save validation
                        |
                        +--> per-adapter live-write testing much later
```

## Near-term device-test loop

```text
finish coherent playable milestone
        ↓
host tests + sanitizers
        ↓
build fresh .nro
        ↓
record source SHA + binary SHA-256
        ↓
preserve/publish artifact
        ↓
physical Switch test using DEVICE_TEST_CHECKLIST.md
        ↓
record bugs / promote only what actually passed
```

## Permanent safety reminder

Live installed-game save writing remains hard disabled until the gates in [`SAVE_SAFETY.md`](SAVE_SAFETY.md) are satisfied for an explicitly named adapter.

Do not remove that lock just to make a feature appear complete.
