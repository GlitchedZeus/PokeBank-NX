# PokeBank NX — Project Map

Last updated: 2026-09-02

Short navigation/index for verified state, active prompts, hardware reports, architecture, and GitHub work.

## Read first

1. [`../PROJECT_STATUS.md`](../PROJECT_STATUS.md) — authoritative verified state
2. [`NEXT_SESSION_PLAN.md`](NEXT_SESSION_PLAN.md) — current execution order
3. [`BUILD_RECORD.md`](BUILD_RECORD.md) — source/artifact/device-test bookkeeping
4. [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md) — first physical Switch result
5. [`DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md) — extended torture-test findings
6. [`PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`](PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md) — **HIGH**, current blocker session
7. [`PROMPT_SESSION3_PKSM_CORE.md`](PROMPT_SESSION3_PKSM_CORE.md) — **MAX**, PKSM-Core after second hardware pass
8. [`SESSION_RUNBOOK.md`](SESSION_RUNBOOK.md) — implementation/test/build/push discipline
9. [`TRANSFER_MODEL.md`](TRANSFER_MODEL.md) — Copy / Move / Clone / conversion / active-location contract
10. [`NRO_QUALITY_ROADMAP.md`](NRO_QUALITY_ROADMAP.md) — diagnostics/reliability/performance/QoL backlog
11. [`ARCHITECTURE.md`](ARCHITECTURE.md) — module boundaries

---

## Physical hardware milestone

First exact tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Original short pass established many working paths:

```text
Boot
D-pad
A Action Sheet
B/Cancel
+ / -
OLED Black
Dark
Light
Theme persistence
Party
Boxes
Storage
```

The longer pass on the same artifact clarified new failures/risks:

```text
Left Stick single tap / hold / diagonal   FAIL — no input/action
one old Legends Arceus save               REPRODUCIBLE CRASH
inherited Release/Create/Move/Edit UI      PHYSICALLY REACHABLE
app/legacy Storage persistence             PHYSICALLY OBSERVED
live installed save write                  NOT PROVEN
```

Issue #8 remains closed because the first test milestone itself is complete.

---

## Current application checkpoint

Session 2.5 application source:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed.

This source contains the reported visible PokeBank NX shell and real-libnx analog input fix, but it has **not** been physically tested.

The extended first-device findings mean this checkpoint is no longer automatically the final second-device artifact source.

If #23/#24 require application changes, create a new application-source checkpoint and build/hash from that exact new SHA.

---

## Completed milestones

```text
#2  Controller-first Pokemon Action Sheet              CLOSED
#8  First exact physical PokeBank NX .nro test        CLOSED
```

---

## Current blockers / near-term issues

### #23 — inherited mutation UI safety audit — CURRENT BLOCKER

Trace user-reachable:

```text
Release
Create Pokemon
Move / Multi
Edit / apply / save
unsaved-changes flow
legacy/app Storage move/import
reachable Save / Commit / Restore / Inject paths
```

Classify final persistence targets and block unsafe installed-source mutations while the current alpha live-write policy remains disabled.

### #24 — old / malformed Legends Arceus crash — CURRENT BLOCKER

One older PLA save reproducibly crashes on physical hardware. Valid supported saves should still open; old/unsupported/malformed saves must fail gracefully rather than crash or auto-repair/write.

### #19 — Left Stick navigation

Old device-tested build receives no Left Stick navigation input at all. `361c6f55...` contains the reported source fix; keep issue open until physical Switch retest passes single-tap, held repeat, and diagonal stability.

### #13 — visible PokeBank NX shell

Session 2.5 source is published. Preserve it while fixing blockers. Keep issue open until the exact replacement artifact passes physical visual acceptance.

### #16 — branding/startup/NRO metadata

Visible identity/NRO naming has progressed; final splash/loading/icon/NACP work remains later.

### #25 — Pokémon Summary visual/model support — LATER

Add a proper Pokémon visual to View/Summary. Optional Right Stick model rotation is only a nice-to-have if 3D rendering is practical.

### #26 — normalize controller semantics — LATER EXCEPT SAFETY

Extended hardware testing exposed context differences such as L/R account switching, inactive ZL/ZR, X Dex-sort, and Y Menu/Move/Multi. Normalize after current blockers, except unsafe mutation shortcuts which belong to #23 now.

### #27 — legacy Storage vs Master Vault — CLASSIFY NOW / REDESIGN LATER

Legacy app Storage is already writable/persistent. Determine exact storage/persistence semantics and preserve user data where possible when Master Vault v1 eventually arrives.

### #4 — PKSM-Core Gen III spike — NEXT MAX AFTER SECOND DEVICE GATE

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

```text
COPY  = source remains active
MOVE  = destination becomes active; verified source is removed/retired
CLONE = deliberate duplicate with clone provenance
```

Current installed saves do not have approved live-write capability. True Move requires individually validated write adapters and is intentionally blocked until much later.

### #21 — NRO reliability / diagnostics / performance / QoL

Cross-cutting backlog. Implement incrementally without derailing core engine work.

---

## Supporting issues

```text
#15 persistent device-test .nro artifacts
#17 reproducible Pokemon/save golden corpus
#21 diagnostics/reliability/performance/QoL
#25 Summary visuals/model idea
#26 controller normalization
#27 legacy Storage migration/clarification
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
- [`DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md)
- [`BUILD_RECORD.md`](BUILD_RECORD.md)
- [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md)
- [`.github/workflows/host-tests.yml`](../.github/workflows/host-tests.yml)

---

## Execution dependency sketch

```text
first physical build                              DONE
        |
        +--> Session 2.5 visible shell + analog source      PUBLISHED
                 |
                 +--> extended first-build torture test      DONE
                          |
                          +--> #23 mutation safety audit      NOW
                          +--> #24 PLA crash hardening        NOW
                          +--> preserve #19/#13/#16 work
                                   |
                                   +--> NEW exact app source if changed
                                            |
                                            +--> exact replacement .nro
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

Live installed-game save writing is not an approved current capability until an explicitly named adapter passes `SAVE_SAFETY.md`.

The extended first-device test proved that inherited mutation UI still exists above the lower-level lock. Trace and block those paths rather than assuming the low-level guard is enough.

Do not weaken the safety model for UI, transfer, true-Move, editor, or integration demos.