# PokeBank NX — Project Map

Last updated: 2026-09-02

Short navigation/index for verified state, active prompts, hardware reports, architecture, and GitHub work.

## Read first

1. [`../PROJECT_STATUS.md`](../PROJECT_STATUS.md) — authoritative verified state
2. [`NEXT_SESSION_PLAN.md`](NEXT_SESSION_PLAN.md) — current execution order
3. [`BUILD_RECORD.md`](BUILD_RECORD.md) — source/artifact/device-test bookkeeping
4. [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md) — first physical Switch result
5. [`PROMPT_SESSION2_5_FINISH.md`](PROMPT_SESSION2_5_FINISH.md) — **HIGH**, finish/package exact `361c6f55` replacement build
6. [`PROMPT_SESSION3_PKSM_CORE.md`](PROMPT_SESSION3_PKSM_CORE.md) — **MAX**, PKSM-Core after second hardware pass
7. [`SESSION_RUNBOOK.md`](SESSION_RUNBOOK.md) — implementation/test/build/push discipline
8. [`TRANSFER_MODEL.md`](TRANSFER_MODEL.md) — Copy / Move / Clone / conversion / active-location contract
9. [`NRO_QUALITY_ROADMAP.md`](NRO_QUALITY_ROADMAP.md) — diagnostics/reliability/performance/QoL backlog
10. [`ARCHITECTURE.md`](ARCHITECTURE.md) — module boundaries

---

## Physical hardware milestone

First exact tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Passed:

```text
Boot
D-pad
A Action Sheet
B/Cancel
L/R
ZL/ZR
+
-
OLED Black
Dark
Light
Theme persistence
Party
Boxes
Storage
No crashes
```

Failed/incomplete:

```text
Held Left Stick repeat      FAIL
Visible PokeBank NX shell   FAIL / INCOMPLETE
```

Issue #8 is closed because the test milestone itself is complete.

---

## Current application checkpoint

Session 2.5 application source:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed.

The session ended before clean exact-source artifact packaging. Therefore:

```text
SOURCE PUBLISHED
REPLACEMENT .NRO PENDING
SECOND DEVICE TEST PENDING
```

Use `PROMPT_SESSION2_5_FINISH.md`; do not redo the implementation unless a regression is found.

---

## Completed milestones

```text
#2  Controller-first Pokemon Action Sheet              CLOSED
#8  First exact physical PokeBank NX .nro test        CLOSED
```

---

## Active / near-term issues

### #13 — visible PokeBank NX shell

Application source `361c6f55` is published. Keep issue open until the exact replacement artifact passes physical visual acceptance.

### #19 — held Left Stick repeat

Source fix is published in `361c6f55`. Keep issue open until physical Switch retest passes.

### #16 — branding/startup/NRO metadata

Visible identity/NRO naming has progressed in Session 2.5; final splash/loading/icon/NACP work remains later.

### #4 — PKSM-Core Gen III spike — NEXT MAX

After the second UI/input hardware pass:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parse
party/box extraction
active save slot/sectors
round-trip strategy
integration decision
```

### #3 — Master Vault v1 + named Banks

Immutable raw entities, stable Vault IDs, SHA-256, provenance, active/current location, transaction journals, recovery, logical Bank references.

### #9 — professional Summary + provenance

Generation-aware Summary/Origin/Legality/Provenance/active-location presentation.

### #6 — RetroArch + read-only Gen I-III adapters

Depends heavily on PKSM-Core decision.

### #11 — modern Switch adapter validation

Read-only/source-specific validation before any write gates.

### #5 — host-side PKHeX Oracle

Correctness/legality/conversion comparison utility.

### #7 — Vault-driven Pokedex / Living Dex

Vault remains authoritative for collection ownership.

### #10 — conversion/transfer without live writes

Derived/staged/exported destination representations first.

### #20 — true Move semantics — LATER

End-state semantics:

```text
COPY  = source remains active
MOVE  = destination becomes active; verified source is removed/retired
CLONE = deliberate duplicate with clone provenance
```

Current installed saves remain read-only. True Move requires individually validated write adapters and is intentionally blocked until much later.

### #21 — NRO reliability / diagnostics / performance / QoL

Cross-cutting backlog. Implement incrementally without derailing core engine work.

---

## Supporting issues

```text
#15 persistent device-test .nro artifacts
#17 reproducible Pokemon/save golden corpus
#21 diagnostics/reliability/performance/QoL
```

---

## Core architecture / UX docs

- [`ARCHITECTURE.md`](ARCHITECTURE.md)
- [`CONTROLS.md`](CONTROLS.md)
- [`UI_FLOW.md`](UI_FLOW.md)
- [`UI_STYLE_GUIDE.md`](UI_STYLE_GUIDE.md)
- [`MASTER_VAULT_SPEC.md`](MASTER_VAULT_SPEC.md)
- [`SAVE_SAFETY.md`](SAVE_SAFETY.md)
- [`TRANSFER_MODEL.md`](TRANSFER_MODEL.md)
- [`POKEDEX_SPEC.md`](POKEDEX_SPEC.md)
- [`PKHEX_ORACLE.md`](PKHEX_ORACLE.md)
- [`NRO_QUALITY_ROADMAP.md`](NRO_QUALITY_ROADMAP.md)

## Integration / research

- [`PKSM_CORE_INTEGRATION.md`](PKSM_CORE_INTEGRATION.md)
- [`PKHOUSE_REFERENCE.md`](PKHOUSE_REFERENCE.md)
- [`GAME_SUPPORT_MATRIX.md`](GAME_SUPPORT_MATRIX.md)
- [`UPSTREAM_AUDIT.md`](UPSTREAM_AUDIT.md)

## Testing / release

- [`DEVICE_TEST_CHECKLIST.md`](DEVICE_TEST_CHECKLIST.md)
- [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md)
- [`BUILD_RECORD.md`](BUILD_RECORD.md)
- [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md)
- [`.github/workflows/host-tests.yml`](../.github/workflows/host-tests.yml)

---

## Execution dependency sketch

```text
first physical build                    DONE
        |
        +--> Session 2.5 visible shell + analog source    PUBLISHED
                 |
                 +--> exact artifact packaging             NOW
                         |
                         +--> second physical test
                                  |
                                  +--> PKSM-Core PK3/Sav3
                                           |
                                           +--> Master Vault + Banks
                                           |       |
                                           |       +--> Summary/provenance
                                           |       +--> Pokedex/Living Dex
                                           |
                                           +--> retro adapters
                                           +--> modern adapter validation
                                           +--> PKHeX Oracle
                                                   |
                                                   +--> conversion/staging
                                                            |
                                                            +--> safe per-game writes
                                                                     |
                                                                     +--> true Move (#20)
```

Quality issue #21 can be pulled into relevant milestones incrementally:

```text
diagnostics / logs / memory warning      early
Vault health / recovery / search          with Vault
virtualization / caches                    as datasets grow
accessibility / sounds / polish            later
```

---

## Permanent safety reminder

Live installed-game save writing remains hard disabled until an explicitly named adapter passes `SAVE_SAFETY.md`.

Do not weaken the lock for UI, transfer, true-Move, editor, or integration demos.