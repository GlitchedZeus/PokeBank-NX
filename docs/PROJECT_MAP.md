# PokeBank NX — Project Map

Last updated: 2026-09-04

Current gate: **READY FOR SECOND DEVICE TEST / NOT DEVICE TESTED**, application `0ea98cc1`.
See `BUILD_RECORD.md` for the exact binary and
[`SESSION2_6_SAFETY_IMPLEMENTATION.md`](SESSION2_6_SAFETY_IMPLEMENTATION.md) for implemented
source guards, persistence classification, PLA hardening and verification boundaries.

Short navigation/index for verified state, active prompts, hardware reports, architecture, research and the full v1.0 plan.

## Read first

1. [`../PROJECT_STATUS.md`](../PROJECT_STATUS.md) — authoritative verified state
2. [`V1_ROADMAP.md`](V1_ROADMAP.md) — full milestone checklist through v1.0
3. [`GAME_SUPPORT_MATRIX.md`](GAME_SUPPORT_MATRIX.md) — current/planned game/source capability matrix
4. [`NEXT_SESSION_PLAN.md`](NEXT_SESSION_PLAN.md) — current execution order
5. [`BUILD_RECORD.md`](BUILD_RECORD.md) — source/artifact/device-test bookkeeping
6. [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md) — first physical Switch result
7. [`DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md) — extended torture-test findings
8. [`DEVICE_TEST_FOLLOWUP_2026-09-03.md`](DEVICE_TEST_FOLLOWUP_2026-09-03.md) — latest lifecycle/theme/Summary/safety clarification
9. [`MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md`](MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md) — source-level mutation persistence trace
10. [`DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md`](DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md) — prepared second-device acceptance sheet
11. [`PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`](PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md) — **HIGH**, current blocker session
12. [`PROMPT_SESSION3_PKSM_CORE.md`](PROMPT_SESSION3_PKSM_CORE.md) — **MAX**, PKSM-Core after second hardware gate
13. [`SESSION_RUNBOOK.md`](SESSION_RUNBOOK.md) — implementation/test/build/push discipline
14. [`TRANSFER_MODEL.md`](TRANSFER_MODEL.md) — Copy / Move / Clone / conversion / active-location contract
15. [`NRO_QUALITY_ROADMAP.md`](NRO_QUALITY_ROADMAP.md) — diagnostics/reliability/performance/QoL backlog
16. [`ARCHITECTURE.md`](ARCHITECTURE.md) — module boundaries

Master v1.0 tracking issue: **#29**.

---

# Current hardware/application state

First exact device-tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Current useful untested UI/analog application checkpoint:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

Latest first-build evidence:

```text
D-pad                           PASS
Left Stick                      FAIL — no input at all on old artifact
Action Sheet                    PASS / ~100 repeated opens stable
HOME/sleep/controller reconnect PASS
handheld                        PASS
docked                          NOT TESTED
one old PLA save                REPRODUCIBLE CRASH
installed-source mutation UI    reachable / must be clarified/blocked
legacy Storage persistence      PASS
legacy Storage cross-game view  PASS
original installed save         tester-reported unchanged in exercised flow
live installed write observed   NO
View data                       PASS/PARTIAL on sampled Pokémon
View Pokémon visual             MISSING in first artifact
```

Current blockers:

```text
#23 inherited mutation UI safety/source-state contract
#24 old/malformed Legends Arceus crash
#19 physical Left Stick acceptance
#13/#16 physical PokeBank NX shell/identity acceptance
#37 device visual asset preflight
```

The second `.nro` must not be handed off until the Session 2.6 safety/crash gate is complete enough and the exact artifact is built/hashed/preserved.

---

# Clarified Storage behavior

The Arbok test physically demonstrated:

```text
installed Z-A
   ↓ automatic backup
mutable Z-A backup
   ↓
legacy app Storage
   ↓ switch game context
Storage still contains Arbok
   ↓
potential compatible destination backup
```

The installed Z-A save was reported unchanged.

Therefore:

```text
legacy Storage = app-owned persistent compatibility bank
Master Vault   = future authoritative provenance storage
true Move      = future verified active-location relocation
```

Do not conflate those concepts. See #27 and `DEVICE_TEST_FOLLOWUP_2026-09-03.md`.

---

# Completed milestones

```text
#2  Controller-first Pokémon Action Sheet             CLOSED
#8  First exact physical PokeBank NX .nro test       CLOSED
```

Foundation also includes the current 23 stable source identities, low-level live-write hard lock, theme/controller infrastructure, build tooling, sprite asset preflight tooling and host/native build workflow.

---

# Expanded target catalog

Current source registry:

```text
23 host-tested identities
```

Planned additions:

```text
#30  +9 Nintendo DS +8 Nintendo 3DS identities
#33  +2 Nintendo GameCube identities: Colosseum / XD
#34  +2 Nintendo 64 identities: Stadium / Stadium 2
```

Total target:

```text
44 release/source identities
```

Classification:

```text
DS / 3DS                 = core v1 target
Colosseum / XD           = strong v1 legacy target
Stadium 1 / Stadium 2    = v1 stretch; may slip post-v1 if cost is disproportionate
```

See `GAME_SUPPORT_MATRIX.md`.

---

# Current blockers / near-term issues

```text
#23  inherited mutation UI safety contract      CURRENT BLOCKER
#24  old/malformed PLA crash                    CURRENT BLOCKER
#19  Left Stick navigation                      PHYSICAL RETEST REQUIRED
#13  visible PokeBank NX shell                  PHYSICAL RETEST REQUIRED
#16  branding/startup/NRO metadata              PARTIAL / LATER POLISH
#37  device visual asset gate                   CURRENT BUILD GATE
#25  Pokémon Summary visual/render              LATER
#26  controller normalization                   LATER EXCEPT SAFETY/READABILITY
#27  legacy Storage vs Master Vault             PHYSICALLY CLARIFIED / MIGRATE LATER
#35  Pokémon cry support                        LATER
```

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

Compare:

```text
PKSM-Core
FlagBrew/PKSM
Universal-Team/pkmn-chest
PKHeX
```

---

# Core v1.0 / legacy feature issues

```text
#3   Master Vault v1 + named Banks
#4   PKSM-Core Gen III integration
#5   host-side PKHeX Oracle
#6   RetroArch + read-only Gen I-III adapters
#7   Vault-driven Pokédex / Living Dex
#9   professional Summary + provenance
#10  conversion/transfer without live writes
#11  modern Switch adapter validation
#13  visible PokeBank NX shell
#15  persistent device-test .nro artifacts
#16  final branding/startup/icon/NACP
#17  reproducible Pokémon/save golden corpus
#19  Left Stick hardware navigation
#20  true Move after verified write adapters
#21  diagnostics/reliability/performance/QoL
#23  inherited mutation safety/UI contract
#24  PLA crash hardening
#25  Pokémon visuals/model idea
#26  controller normalization
#27  legacy Storage migration/clarification
#29  v1.0 master roadmap/release gates
#30  DS/3DS identities + source discovery
#31  Nintendo DS Gen IV/V read-only adapters
#32  Nintendo 3DS Gen VI/VII read-only adapters
#33  Colosseum/XD GameCube read-only support
#34  Stadium 1/2 N64 read-only support — stretch
#35  Pokémon cry/audio support
#37  required visual assets in device builds
```

---

# Platform/source roadmap

```text
GB/GBC
  R/B/Y + G/S/C
       ↓
GBA
  R/S/E + FR/LG
       ↓
GameCube side branch
  Colosseum + XD
       ↓
Nintendo DS
  D/P/Pt/HG/SS + B/W/B2/W2
       ↓
Nintendo 3DS
  X/Y/ORAS + S/M/US/UM
       ↓
Nintendo Switch
  FR/LG + LGPE + SwSh + BDSP + PLA + SV + Z-A
```

N64 Stadium is an archival branch off Gen I/II:

```text
Gen I / II engine
       ↓
Stadium 1 / Stadium 2 save containers
       ↓
Master Vault historical entity
```

Any Stadium Gen I/II → Gen III+ forward conversion must record the historical transfer discontinuity. Do not present it as an official uninterrupted HOME path.

---

# Research stack

```text
PKSE          = inherited native Switch foundation
PKSM-Core     = native historical format/save candidate
PKSM          = mature Bank/backups/events/full-app integration reference
Pokémon Chest = second Nintendo PKSM-Core integration + Game↔Bank UX
PHBank        = historical offline Game-PC ↔ Bank UX reference
PHBankGBC     = secondary Gen I/II save-layout reference only
PKHeX         = primary correctness oracle/reference, including GC/Stadium handlers
Auto Legality = host/reference generation/legality tooling
pkHouse       = modern Switch save-behavior reference
pkDex         = Pokédex UX/data-organization reference
PKForge       = Vault/provenance/transaction architecture reference
PokeAPI       = supplementary species/form/media organization reference
PokéSprite    = compact box/item/form metadata reference
```

PKHeX specifically gives strong reference coverage for:

```text
SAV3Colosseum
SAV3XD
SAV3GCMemoryCard
SAV1Stadium
SAV2Stadium
```

Visual/audio research:

```text
docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md
docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md
#25 Pokémon visual support
#35 Pokémon cries
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
- [`GAME_SUPPORT_MATRIX.md`](GAME_SUPPORT_MATRIX.md)

## Integration / research

- [`PKSM_CORE_INTEGRATION.md`](PKSM_CORE_INTEGRATION.md)
- [`PKHOUSE_REFERENCE.md`](PKHOUSE_REFERENCE.md)
- [`UPSTREAM_AUDIT.md`](UPSTREAM_AUDIT.md)
- [`BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`](BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md)
- [`POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md`](POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md)
- [`PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md`](PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md)

## Testing / release

- [`DEVICE_TEST_CHECKLIST.md`](DEVICE_TEST_CHECKLIST.md)
- [`DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md`](DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md)
- [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md)
- [`DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md)
- [`DEVICE_TEST_FOLLOWUP_2026-09-03.md`](DEVICE_TEST_FOLLOWUP_2026-09-03.md)
- [`MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md`](MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md)
- [`DEVICE_BUILD_ASSET_GATE.md`](DEVICE_BUILD_ASSET_GATE.md)
- [`DEVICE_ARTIFACT_PACKAGING.md`](DEVICE_ARTIFACT_PACKAGING.md)
- [`BUILD_RECORD.md`](BUILD_RECORD.md)
- [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md)
- [`.github/workflows/host-tests.yml`](../.github/workflows/host-tests.yml)

---

# Execution dependency sketch

```text
first exact physical build                     DONE
        |
Session 2.5 shell + analog source               PUBLISHED
        |
extended/follow-up first-build testing          DONE
        |
static mutation persistence audit               DONE / VERIFY + IMPLEMENT UI FIX NEXT
        |
#23 safety UI + #24 PLA + #37 asset gate        NOW
        |
replacement exact .nro
        |
physical device test #2
        |
#4 PKSM-Core PK3/Sav3
        |
Gen III production reads
        |
Master Vault + Banks
        |
+--> Colosseum/XD (#33)
+--> Gen I/II (#6) --> Stadium (#34 stretch)
+--> Nintendo DS (#31)
+--> Nintendo 3DS (#32)
        |
Summary / Oracle / modern Switch reads
        |
conversion / Dex / legality / events
        |
staged writes
        |
per-game approved writes
        |
true Move
        |
RC hardening / v1.0
```

Quality issue #21 is pulled into milestones incrementally rather than blocking all core development.

---

# Permanent safety reminder

Live installed-game save writing is not an approved current capability until an explicitly named adapter passes `SAVE_SAFETY.md`.

The first-build Arbok test supports that the installed title remained untouched while backup/app-Storage copies could mutate and persist. That is useful, but it does not authorize live writes.

Legacy file sources such as GameCube, Stadium, DS and 3DS imports should begin read-only even when the reference parser can write them.

Do not weaken the safety model for UI, transfer, true Move, editor, side-game support or integration demos.
