# PokeBank NX — Project Map

Last updated: 2026-09-02

Short navigation/index for verified state, active prompts, hardware reports, architecture, research and the full v1.0 plan.

## Read first

1. [`../PROJECT_STATUS.md`](../PROJECT_STATUS.md) — authoritative verified state
2. [`V1_ROADMAP.md`](V1_ROADMAP.md) — full milestone checklist through v1.0
3. [`NEXT_SESSION_PLAN.md`](NEXT_SESSION_PLAN.md) — current execution order
4. [`BUILD_RECORD.md`](BUILD_RECORD.md) — source/artifact/device-test bookkeeping
5. [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md) — first physical Switch result
6. [`DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md) — extended torture-test findings
7. [`PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`](PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md) — **HIGH**, current blocker session
8. [`PROMPT_SESSION3_PKSM_CORE.md`](PROMPT_SESSION3_PKSM_CORE.md) — **MAX**, PKSM-Core after second hardware gate
9. [`SESSION_RUNBOOK.md`](SESSION_RUNBOOK.md) — implementation/test/build/push discipline
10. [`TRANSFER_MODEL.md`](TRANSFER_MODEL.md) — Copy / Move / Clone / conversion / active-location contract
11. [`NRO_QUALITY_ROADMAP.md`](NRO_QUALITY_ROADMAP.md) — diagnostics/reliability/performance/QoL backlog
12. [`ARCHITECTURE.md`](ARCHITECTURE.md) — module boundaries
13. [`SESSION_LOG_2026-09-02.md`](SESSION_LOG_2026-09-02.md) — today's historical work log

Master v1.0 tracking issue: **#29**.

---

# Physical hardware milestone

First exact tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Passed during the first/extended passes include boot, D-pad, Action Sheet/B/Cancel, three themes + persistence, Party/Boxes/Storage browsing and repeated Action Sheet stability.

Known current physical failures/risks from that exact old build:

```text
Left Stick single tap / hold / diagonal   FAIL — no input/action
visible PokeBank NX shell                 FAIL / incomplete on old build
one old Legends Arceus save               REPRODUCIBLE CRASH
inherited Release/Create/Move/Edit UI      PHYSICALLY REACHABLE
app/legacy Storage persistence             PHYSICALLY OBSERVED
live installed save write                  NOT PROVEN
```

Issue #8 remains closed because the first exact physical-test milestone itself is complete.

---

# Current application checkpoint

Session 2.5 application source:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed.

This source contains the reported visible PokeBank NX shell + real-libnx analog input fix, but it is **NOT DEVICE TESTED**.

The extended first-device findings mean it is no longer automatically the final second-device artifact source. If #23/#24 require app changes, create a new exact application-source checkpoint and build/hash from that source.

---

# Completed milestones

```text
#2  Controller-first Pokémon Action Sheet             CLOSED
#8  First exact physical PokeBank NX .nro test       CLOSED
```

Foundation also includes stable 23-game identities, low-level live-write hard lock, themes/controller infrastructure and verified host/native build workflow.

---

# Current blockers / near-term issues

## #23 — inherited mutation UI safety audit — CURRENT BLOCKER

Trace Release/Create/Move/Multi/Edit/apply/save/Storage and all reachable persistence calls. Block unsafe installed-source mutation UI while live installed-save writing remains disabled.

## #24 — old / malformed Legends Arceus crash — CURRENT BLOCKER

Valid supported saves should open; old/unsupported/malformed saves must fail gracefully rather than crash or auto-repair/write.

## #19 — Left Stick navigation — PHYSICAL RETEST REQUIRED

Old build receives no Left Stick navigation input. `361c6f55...` contains the source fix candidate.

## #13 — visible PokeBank NX shell — PHYSICAL RETEST REQUIRED

Preserve Session 2.5 shell/chrome/Options/Help/Action Sheet styling and verify on the exact replacement binary.

## #16 — branding/startup/NRO metadata

Visible identity/NRO naming has progressed. Final splash/loading/icon/NACP remains later.

## #25 — Pokémon visual Summary — LATER

Proper sprite/artwork/render on Summary/View. Optional 3D model rotation only if practical.

## #26 — controller normalization — LATER EXCEPT SAFETY

Normalize L/R, ZL/ZR, X/Y and visible hints after current blocker work.

## #27 — legacy Storage vs Master Vault — CLASSIFY NOW / MIGRATE LATER

Storage is physically proven writable/persistent. Determine exact semantics and preserve user data where possible during future Vault migration.

---

# Next deep engineering after device gate #2

## #4 — PKSM-Core Gen III spike

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

New useful integration references:

```text
FlagBrew/PKSM
Universal-Team/pkmn-chest
```

See `BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`.

---

# Core v1.0 feature issues

```text
#3   Master Vault v1 + named Banks
#4   PKSM-Core Gen III integration
#5   host-side PKHeX Oracle
#6   RetroArch + read-only Gen I-III adapters
#7   Vault-driven Pokédex / Living Dex
#9   professional Summary + provenance
#10  conversion/transfer without live writes
#11  modern Switch adapter validation
#15  persistent device-test .nro artifacts
#16  final branding/startup/icon/NACP
#17  reproducible Pokémon/save golden corpus
#20  true Move after verified write adapters
#21  diagnostics/reliability/performance/QoL
#23  inherited mutation safety audit
#24  PLA crash hardening
#25  Pokémon visuals/model idea
#26  controller normalization
#27  legacy Storage migration/clarification
#29  v1.0 master roadmap/release gates
```

---

# Core architecture / UX docs

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
- [`V1_ROADMAP.md`](V1_ROADMAP.md)

## Integration / research

- [`PKSM_CORE_INTEGRATION.md`](PKSM_CORE_INTEGRATION.md)
- [`PKHOUSE_REFERENCE.md`](PKHOUSE_REFERENCE.md)
- [`GAME_SUPPORT_MATRIX.md`](GAME_SUPPORT_MATRIX.md)
- [`UPSTREAM_AUDIT.md`](UPSTREAM_AUDIT.md)
- [`BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`](BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md)

Key research stack:

```text
PKSE          = inherited native Switch foundation
PKSM-Core     = native historical format/save candidate
PKSM          = mature Bank/backups/events/full-app integration reference
Pokémon Chest = second Nintendo PKSM-Core integration + Game↔Bank UX
PHBank        = historical offline Game-PC ↔ Bank UX reference
PHBankGBC     = secondary Gen I/II save-layout reference only
PKHeX         = correctness oracle/reference
Auto Legality = host/reference generation/legality tooling
pkHouse       = modern Switch save-behavior reference
pkDex         = Pokédex UX/data-organization reference
PKForge       = Vault/provenance/transaction architecture reference
```

## Testing / release

- [`DEVICE_TEST_CHECKLIST.md`](DEVICE_TEST_CHECKLIST.md)
- [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md)
- [`DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md)
- [`BUILD_RECORD.md`](BUILD_RECORD.md)
- [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md)
- [`V1_ROADMAP.md`](V1_ROADMAP.md)
- [`.github/workflows/host-tests.yml`](../.github/workflows/host-tests.yml)

---

# Execution dependency sketch

```text
first exact physical build                           DONE
        |
        +--> Session 2.5 visible shell + analog source       PUBLISHED
                 |
                 +--> extended first-build torture test       DONE
                          |
                          +--> #23 mutation safety audit       NOW
                          +--> #24 PLA crash hardening         NOW
                          +--> preserve #19/#13/#16 work
                                   |
                                   +--> new exact app source if changed
                                            |
                                            +--> replacement .nro
                                                     |
                                                     +--> physical test #2
                                                              |
                                                              +--> #4 PKSM-Core PK3/Sav3
                                                                       |
                                                                       +--> Gen III read adapter
                                                                                |
                                                                                +--> Master Vault + Banks
                                                                                         |
                                                                                         +--> Summary / Oracle / retro + modern reads
                                                                                                  |
                                                                                                  +--> conversion / Dex / legality / events
                                                                                                           |
                                                                                                           +--> staged writes
                                                                                                                    |
                                                                                                                    +--> per-game approved writes
                                                                                                                             |
                                                                                                                             +--> true Move
                                                                                                                                      |
                                                                                                                                      +--> RC hardening / v1.0
```

Quality issue #21 is pulled into milestones incrementally rather than blocking all core development at once.

---

# Permanent safety reminder

Live installed-game save writing is not an approved current capability until an explicitly named adapter passes `SAVE_SAFETY.md`.

The extended first-device test proved inherited mutation UI exists above the lower-level lock. Trace/block those paths rather than assuming the lower-level guard is enough.

Do not weaken the safety model for UI, transfer, true Move, editor or integration demos.