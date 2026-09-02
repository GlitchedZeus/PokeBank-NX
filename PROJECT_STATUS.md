# PokeBank NX Project Status

Last updated: 2026-09-02

This is the authoritative verified-state handoff for coding sessions. Source commits, CI/build evidence and physical-device evidence are deliberately tracked separately.

## Project identity

- Product: **PokeBank NX**
- Version: `0.1.0-alpha`
- Repository: `GlitchedZeus/PokeBank-NX`
- Writable remote: `origin`
- Upstream-only remote: `upstream` (`kiasta/PKSE`)
- Development branch: `feature/pokebank-playable`
- Live installed-game save writing policy: **HARD DISABLED / NOT AN APPROVED CURRENT FEATURE**
- Canonical v1.0 roadmap: `docs/V1_ROADMAP.md`
- Master v1.0 tracking issue: **#29**

## Important application-source checkpoints

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40   hard-lock live game save writes
82a0779a5143cca0690d0c7068946d84ebe9f107   controller Pokémon Action Sheet
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a   first physically tested controls/theme source
361c6f551496470db305948d702944c6ed9889c1   Session 2.5 visible PokeBank shell + physical stick source
```

The feature branch contains later documentation/research commits. **Branch HEAD is not automatically application-source identity.**

`361c6f55...` is the useful current UI/analog application checkpoint, but the extended first-device test exposed safety/crash blockers. If #23/#24 change application code, the second-device artifact must use a **new application-source checkpoint** rather than being mislabeled as `361c6f55...`.

## Verification vocabulary

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

`DEVICE TESTED` means a human physically ran the exact recorded binary/hash. A tested build may still be a partial pass or fail.

---

# First exact physical Switch milestone — COMPLETE

Exact tested artifact:

```text
Application source:
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a

Artifact:
PokeBank-NX-UI-Theme-3be4de6.nro

Size:
9,707,957 bytes

SHA-256:
df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a

Physical status:
DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Original short pass:

```text
BOOT                     PASS
D-PAD                    PASS
LEFT STICK               FAIL
A ACTION SHEET           PASS
B / CANCEL               PASS
L / R                    PASS in exercised context
ZL / ZR                  PASS in exercised context
+                        PASS
-                        PASS
OLED BLACK               PASS
DARK                     PASS
LIGHT                    PASS
THEME PERSISTENCE        PASS
PARTY                    PASS
BOXES                    PASS
STORAGE                  PASS
VISIBLE POKEBANK NX UI   FAIL / INCOMPLETE
```

Permanent reports:

```text
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
```

Issue #8 is complete because the physical-test milestone occurred. The failures remain tracked separately.

---

# Extended first-device hardware evidence

The longer torture pass was run on the **same exact `3be4de6b...` binary**.

## Stability

```text
5 minutes idle                 PASS
10 minutes normal browsing     PASS
5 repeated relaunches          PASS
Action Sheet repeated heavily  PASS / no lag / no stuck input
one older Legends Arceus save  REPRODUCIBLE CRASH
```

The PLA crash is tracked by #24.

## Left Stick failure clarified

The old build receives **no navigation input from Left Stick at all**:

```text
single tap Up/Down/Left/Right  FAIL — no input/action
hold Up/Down/Left/Right        FAIL — no input/action
diagonal                       FAIL — no input/action
```

Held D-pad directions pass and repeat predictably.

Issue #19 tracks full physical Left Stick navigation. `361c6f55...` contains the reported source fix but is not device-tested.

## User-reachable inherited mutation UI

Extended testing physically reached:

```text
X on Pokémon        Release path
Y in Boxes          Menu / Move / Multi
Y on Pokémon        grab/move behavior
A on empty slot     Create Pokémon path
Action Sheet Edit   inherited editable view
```

The tester changed an IV, applied it, reopened it, then restored the original value and applied again.

Inherited/app Storage is writable/persistent: a Pokémon moved into Storage remained present.

### Evidence boundary

The final external check of launching the actual original Pokémon game and proving whether its installed save changed was not completed.

Therefore current truth is:

```text
LIVE INSTALLED SAVE WRITE: NOT PROVEN
USER-REACHABLE MUTATION UI: PROVEN
APP STORAGE PERSISTENCE: PROVEN
```

Do **not** call this a confirmed live-save write regression.

However, user-facing mutation paths conflict with the current alpha read-only contract and must be traced/classified/blocked before the second device artifact. Issue #23 is a blocker.

Legacy Storage vs the future Master Vault is tracked by #27.

## Other controller observations

```text
X in Boxes       Dex-sort behavior
Y in Boxes       inherited Menu / Move / Multi
L/R repeated     account switching in tested context
ZL/ZR            no visible action in tested context
Right Stick      no current action
```

Issue #26 tracks controller normalization after safety-critical shortcuts are handled.

The tested Summary/View lacked a Pokémon visual; #25 tracks the future PokeBank NX visual Summary integration.

---

# Session 2.5 visible-shell / analog application source

Published application source:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on this exact source.

Reported implementation:

```text
real libnx Left Stick position handling
analog deadzone + hysteresis
single-tap + sustained held-stick navigation
analog navigation in Select Game / Backups / Party / Boxes / Storage
visible PokeBank NX header/app identity
PokeBank NX branded chrome/cards
shared Options / Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

Current status:

```text
IMPLEMENTED
GITHUB HOST CI PASS
NOT DEVICE TESTED
```

The interrupted coding session did not finish the final clean exact-source artifact packaging/preservation.

Because the extended test found #23/#24 blockers, the next session must preserve this work but perform the safety/crash fixes before handing over the second artifact.

---

# Current critical blockers — Session 2.6

Use:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

## #23 — source-level mutation safety audit

Trace:

```text
Release
Create Pokémon
Move / Multi
Edit / apply / save
unsaved-changes flow
legacy/app Storage move/import
reachable Save / Commit / Restore / Inject paths
```

Classify each final persistence target as:

```text
LIVE INSTALLED SAVE WRITE
BACKUP/STAGED SAVE WRITE
APP-OWNED STORAGE WRITE
IN-MEMORY ONLY
DISABLED / UNREACHABLE
UNKNOWN — NEEDS BLOCKING
```

While live installed-save writing remains disabled, Release/Create/unsafe Move/Edit/save behavior must not be reachable on installed-game sources.

## #24 — Legends Arceus defensive failure

Required behavior:

```text
valid supported save            opens
old/unsupported/malformed save  useful read-only error / safe return
parser failure                  never crashes app
failing save                    never auto-written/repaired
```

If the exact old save is unavailable in the coding runtime, add source-grounded defensive handling/tests and keep #24 open for physical retest.

## Preserve #19/#13/#16

Do not throw away the Session 2.5 analog/UI source while fixing safety/crash issues.

---

# Required second-device build pipeline

If #23/#24 changes application source:

```text
IMPLEMENT
→ HOST TEST
→ ASan/UBSan
→ git diff --check
→ NATIVE BUILD
→ COMMIT APPLICATION SOURCE
→ PUSH origin/feature/pokebank-playable
→ VERIFY REMOTE SHA
→ CLEAN REBUILD FROM EXACT NEW SHA
→ RECORD/HASH/PRESERVE .NRO
→ UPDATE DOCS/ISSUES
→ HANDOFF AS READY FOR SECOND DEVICE TEST
```

Do not change source and still label the artifact `361c6f55`.

Before physical handoff require:

```text
HOST TESTS PASS
ASan/UBSan PASS
git diff --check PASS
NRO BUILDS
live-write hard lock intact
mutation paths classified
unsafe installed-source mutation UI blocked
PLA failure path hardened as far as evidence allows
```

Then the user physically tests the exact replacement hash.

---

# Current verification table

| Area | State | Evidence / notes |
|---|---|---|
| Repository recovery | COMPLETE | recovered/published foundation |
| Stable game identity registry | HOST TESTED | 23 release/platform IDs |
| Low-level live installed-save hard lock | HOST TESTED / NRO BUILDS | remains mandatory |
| User-reachable inherited mutation UI | DEVICE TESTED — PRESENT / AUDIT REQUIRED | #23 |
| Legacy/app Storage persistence | DEVICE TESTED — PRESENT | #27 |
| Controller-first Action Sheet | HOST TESTED / NRO BUILDS / DEVICE TESTED | #2 |
| D-pad | DEVICE TESTED — PASS | held repeat works |
| Left Stick on first build | DEVICE TESTED — FAIL | no input at all |
| Left Stick source fix | IMPLEMENTED / HOST CI PASS / RETEST PENDING | `361c6f55`, #19 |
| `+` / `-` | DEVICE TESTED — PASS in exercised contexts | Options/Help |
| OLED / Dark / Light | DEVICE TESTED — PASS | all three |
| Theme persistence | DEVICE TESTED — PASS | restart confirmed |
| Party / Boxes / Storage browsing | DEVICE TESTED — PASS WITH CAVEATS | inherited mutation UI exposed |
| old PLA save handling | DEVICE TESTED — FAIL | reproducible crash, #24 |
| visible PokeBank NX shell | IMPLEMENTED / HOST CI PASS / DEVICE RETEST PENDING | #13 |
| final startup/icon/NACP | PARTIAL / PLANNED | #16 |
| Pokémon visual Summary | PLANNED | #25 |
| Master Vault v1 | SPECIFIED / NOT IMPLEMENTED | #3 |
| True Move | SPECIFIED / LATER | #20 |
| PKSM-Core Gen III | AUDITED / NEXT DEEP PHASE AFTER DEVICE GATE | #4 |
| Retro Gen I–III adapters | PLANNED | #6 |
| modern Switch adapters | PLANNED | #11 |
| PKHeX Oracle | SPECIFIED | #5 |
| Vault-driven Pokédex | SPECIFIED | #7 |
| NRO quality/reliability | BACKLOG / ROADMAP | #21 |
| artifact automation | PLANNED | #15 |
| golden corpus | PLANNED | #17 |
| v1.0 master roadmap | PUBLISHED | `docs/V1_ROADMAP.md`, #29 |

---

# Product transfer semantics

```text
COPY  = intentionally keep source active and create destination representation
MOVE  = relocate active Pokémon; source stops being active only after verified destination success
CLONE = deliberate duplicate with clone provenance
```

Desired end-state:

```text
Game A → Master Vault / Bank → Game B
```

For safety, immutable archival/provenance/rollback evidence may remain behind the scenes without being presented as another active playable copy.

**Current alpha does not authorize true Move or live installed-save writing.**

True Move is issue #20 and is gated per game adapter by `docs/SAVE_SAFETY.md`.

---

# Master Vault principles

```text
immutable raw entity bytes
stable Vault ID
SHA-256
origin/source provenance
active/current location separate from origin
parent/derived lineage
transaction journal + crash recovery
named Banks as references/organization
rebuildable metadata/search indexes
```

Archival history must never be confused with an active duplicate.

See `docs/MASTER_VAULT_SPEC.md`.

---

# External reference/reuse state

Primary audit: `docs/UPSTREAM_AUDIT.md`.

Additional bank/save-manager audit added 2026-09-02:

```text
docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md
```

New inspected references:

```text
FlagBrew/PKSM
Universal-Team/pkmn-chest
gocario/PHBank
0xb01u/PHBankGBC
```

High-value conclusions:

- full PKSM gives BankFile/versioning/corruption/backups/event/app integration references beyond PKSM-Core;
- Pokémon Chest is another Nintendo-platform PKSM-Core integration example and Game↔Bank UX reference;
- PHBank strongly informs a future Transfer Workspace and one/multi/whole-box operations;
- PHBankGBC is secondary Gen I/II reference only and must be independently verified.

These do **not** replace the PokeBank NX Master Vault/provenance design.

---

# Roadmap / task order

Canonical full v1.0 plan:

```text
docs/V1_ROADMAP.md
GitHub issue #29
```

Immediate order:

```text
#23 safety audit + #24 PLA hardening
+ preserve #19 analog + #13/#16 shell
        ↓
replacement exact .nro
        ↓
physical Switch test #2
        ↓
#4 PKSM-Core Gen III spike
        ↓
Gen III read adapter
        ↓
#3 Master Vault + Banks
        ↓
Summary / Oracle / retro + modern reads
        ↓
conversion / Dex / legality / events
        ↓
staged writes
        ↓
per-game approved live writes
        ↓
true Move
        ↓
release hardening
        ↓
v1.0
```

Do not skip the current physical safety/input/UI gate to start PKSM-Core.

---

# Documentation map

Read first:

```text
README.md
PROJECT_STATUS.md
docs/V1_ROADMAP.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
docs/BUILD_RECORD.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
docs/SESSION_LOG_2026-09-02.md
```

Core contracts:

- `docs/SESSION_RUNBOOK.md`
- `docs/CONTROLS.md`
- `docs/UI_FLOW.md`
- `docs/UI_STYLE_GUIDE.md`
- `docs/ARCHITECTURE.md`
- `docs/SAVE_SAFETY.md`
- `docs/MASTER_VAULT_SPEC.md`
- `docs/TRANSFER_MODEL.md`
- `docs/POKEDEX_SPEC.md`
- `docs/NRO_QUALITY_ROADMAP.md`
- `docs/UPSTREAM_AUDIT.md`
- `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`
- `docs/PKSM_CORE_INTEGRATION.md`

---

# Next exact instruction

Use HIGH reasoning:

```text
Open GlitchedZeus/PokeBank-NX.

Read PROJECT_STATUS.md and
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md.

Then execute:

docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md

exactly.

Start working immediately.
```

Do not begin PKSM-Core until the replacement device-test gate is complete/stable enough.