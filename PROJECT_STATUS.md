# PokeBank NX Project Status

Last updated: 2026-09-02

This file is the authoritative verified-state handoff for coding sessions. GitHub source/build evidence and physical-device evidence are tracked separately; a build may be physically tested and still contain failures.

## Project identity

- Product: **PokeBank NX**
- Version: `0.1.0-alpha`
- Repository: `GlitchedZeus/PokeBank-NX`
- Writable remote: `origin`
- Upstream-only remote: `upstream` (`kiasta/PKSE`)
- Development branch: `feature/pokebank-playable`
- Live installed-game save writing policy: **HARD DISABLED**

## Important application/source checkpoints

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40   hard-lock live game save writes
82a0779a5143cca0690d0c7068946d84ebe9f107   controller Pokemon Action Sheet
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a   first physically tested controls/theme source
361c6f551496470db305948d702944c6ed9889c1   Session 2.5 visible PokeBank shell + physical stick source
```

The branch contains later documentation commits. Do not confuse branch HEAD with application-source identity.

`361c6f55...` remains the useful Session 2.5 shell/analog checkpoint, but the extended first-device test exposed new safety/crash blockers. If those blockers require source changes, the second-device artifact must use a **new application-source checkpoint** rather than being mislabeled as `361c6f55...`.

## Verification vocabulary

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

`DEVICE TESTED` means a human physically ran the exact recorded binary/hash. It does not imply every tested capability passed.

---

# Milestone 1 — first physical Switch hardware test complete

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

Original shorter physical pass:

```text
BOOT                     PASS
D-PAD                    PASS
LEFT STICK + HOLD        FAIL
A ACTION SHEET           PASS
B / CANCEL               PASS
L / R                    PASS
ZL / ZR                  PASS
+                        PASS
-                        PASS
OLED BLACK               PASS
DARK                     PASS
LIGHT                    PASS
THEME PERSISTENCE        PASS
PARTY                    PASS
BOXES                    PASS
STORAGE                  PASS
CRASHES                  NONE DURING SHORTER PASS
VISIBLE POKEBANK NX UI   FAIL / INCOMPLETE
```

Permanent first report:

```text
docs/DEVICE_TEST_REPORT_2026-09-01.md
```

Issue #8 is complete because the first exact physical-test milestone happened. The failures became follow-up issues rather than being hidden.

---

# Extended first-device hardware pass — new evidence

A longer hardware torture pass was performed on the same exact `3be4de6b...` binary.

Permanent extended report:

```text
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
```

## Stability

```text
5 minutes idle                 PASS
10 minutes normal browsing     PASS
5 repeated relaunches          PASS
Action Sheet repeated heavily  PASS / no lag / no stuck input
one older Legends Arceus save  REPRODUCIBLE CRASH
```

The old/malformed/unsupported/buggy PLA path is tracked by issue #24.

## Left Stick clarification

The earlier `LEFT STICK + HOLD: FAIL` description was incomplete. Hardware testing now shows the Left Stick provides **no navigation input at all** on `3be4de6b...`:

```text
single tap Up/Down/Left/Right  FAIL — no input/action
hold Up/Down/Left/Right        FAIL — no input/action
diagonal                       FAIL — no input/action
```

Held D-pad Up/Down/Left/Right all pass and repeat predictably.

Issue #19 now tracks full Left Stick navigation rather than only held-repeat behavior.

## Inherited mutation UI physically reachable

The extended test also proved that inherited PKSE mutation controls are reachable in current game-save views:

```text
X on Pokemon        can expose Release
Y in Boxes          Menu / Move / Multi
Y on Pokemon        can grab/move Pokemon
A on empty slot     can expose Create Pokemon
Action Sheet Edit   opens editable Pokemon view
```

The tester changed an IV, used the available apply/save flow, reopened it, then restored the original value and applied it again.

Inherited/app Storage is also writable/persistent: an Arbok moved from a Legends Z-A context into Storage remained present.

### Evidence boundary

The extended checklist did **not** complete the final external verification step of launching the original Pokémon game and proving whether the installed live title save itself changed.

Therefore current truth is:

```text
LIVE INSTALLED SAVE WRITE: NOT PROVEN
USER-REACHABLE MUTATION UI: PROVEN
APP STORAGE PERSISTENCE: PROVEN
```

Do not falsely describe this as a confirmed live-save write regression.

However, these paths conflict with the current alpha UX contract and must be traced/classified at source level before another device artifact is handed over.

Tracked by issue #23.

Legacy/app Storage versus future Master Vault/Banks is tracked by issue #27.

## Other extended controller observations

```text
X in Boxes        sorts by Dex
Y in Boxes        exposes inherited Menu/Move/Multi
L/R repeatedly    switched accounts in the tested context
ZL/ZR             no visible action in that tested context
Right Stick       no current action
```

Controller normalization is tracked by issue #26.

The user also requested a proper Pokémon visual in View/Summary and optional Right Stick model rotation if 3D rendering is practical. This is later work under issue #25.

---

# Current application source — Session 2.5

A later HIGH session implemented visible-shell and physical analog-input work and pushed:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on that exact commit.

Reported implementation includes:

```text
real libnx Left Stick position handling
analog deadzone + hysteresis adapter
single-tap + sustained held-stick navigation
analog navigation in Select Game / Backups / Party / Boxes / Storage
visible PokeBank NX header/app identity
PokeBank NX branded chrome/cards
shared Options / Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

The interrupted session reported host tests, ASan/UBSan, and a native integration build passing before the application-source checkpoint.

Originally the next step was only an exact-source packaging pass. The extended device test changed that priority.

Current truth is now:

```text
SESSION 2.5 SHELL/ANALOG SOURCE: PUBLISHED
GITHUB HOST CI: PASS
SOURCE SAFETY AUDIT #23: REQUIRED BEFORE DEVICE HANDOFF
PLA CRASH HARDENING #24: REQUIRED BEFORE DEVICE HANDOFF
FINAL SECOND-DEVICE APPLICATION SOURCE: NOT YET FROZEN
SECOND DEVICE TEST: NOT DONE
```

Use next:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Do not use the old packaging-only prompt as the primary execution plan.

---

## Current verification table

| Area | State | Evidence / notes |
|---|---|---|
| Repository recovery | COMPLETE | recovered/published foundation |
| Stable game identity registry | HOST TESTED | 23 unique release/platform IDs |
| Low-level live installed-save hard-lock policy | HOST TESTED / NRO BUILDS | remains mandatory; extended UI mutation audit #23 now required |
| User-reachable inherited mutation UI | DEVICE TESTED — PRESENT / SAFETY AUDIT REQUIRED | Release/Create/Move/Edit reached on `3be4de6b` |
| Legacy/app Storage persistence | DEVICE TESTED — PRESENT | writable Arbok storage observed; issue #27 |
| Controller-first Action Sheet | HOST TESTED / NRO BUILDS / DEVICE TESTED | opens safely; B/Cancel stable |
| D-pad | DEVICE TESTED — PASS | held directions repeat predictably |
| Left Stick | DEVICE TESTED — FAIL on `3be4de6b`; SOURCE FIX PUBLISHED / RETEST PENDING | no input at all on old build; `361c6f55`; issue #19 |
| L/R + ZL/ZR | DEVICE TESTED — CONTEXT-DEPENDENT / NEEDS NORMALIZATION | issue #26 |
| `+` / `-` | DEVICE TESTED — PASS in exercised contexts | Settings/Help behavior |
| OLED Black / Dark / Light | DEVICE TESTED — PASS | all three exercised in first pass |
| Theme persistence | DEVICE TESTED — PASS | restart confirmed |
| Party / Boxes / Storage browsing | DEVICE TESTED — PASS WITH CAVEATS | stable generally; mutation UI exposed |
| PLA old-save handling | DEVICE TESTED — FAIL | reproducible crash; issue #24 |
| Visible PokeBank NX shell | SOURCE PUBLISHED / SECOND DEVICE TEST PENDING | `361c6f55`; issue #13 |
| Final branding/startup/icon/NACP | PARTIAL / PLANNED | issue #16 |
| Pokémon visual Summary/View | PLANNED | issue #25 |
| Master Vault v1 | SPECIFIED / NOT IMPLEMENTED | issue #3 |
| True Move Game <-> Vault <-> Game | SPECIFIED / LATER | issue #20; blocked on safe write adapters |
| PKSM-Core Gen III | AUDITED / NEXT DEEP PHASE AFTER SECOND DEVICE GATE | issue #4 |
| RetroArch Gen I-III adapters | PLANNED | issue #6 |
| Modern Switch adapter validation | PLANNED | issue #11 |
| PKHeX Oracle | SPECIFIED | issue #5 |
| Vault-driven Pokédex | SPECIFIED | issue #7 |
| NRO diagnostics/reliability/QoL | BACKLOG | issue #21 + `docs/NRO_QUALITY_ROADMAP.md` |
| Artifact automation | PLANNED | issue #15 |
| Golden test corpus | PLANNED | issue #17 |

---

## Product transfer semantics

The long-term product behavior is explicit:

```text
COPY = intentionally keep source and create destination representation
MOVE = relocate the active Pokemon; source stops being active only after destination verification
CLONE = intentional duplicate with clone provenance
```

Desired end-state:

```text
Game A -> Master Vault / Bank -> Game B
```

A true Move should feel like Pokémon Bank/HOME-style relocation: after success, the Pokémon is active in the destination and gone from the source location.

For safety, PokeBank NX may retain immutable archival/provenance records and rollback evidence behind the scenes. Those records are not another active playable copy.

**Current alpha does not authorize true Move or live installed-save writing.** True Move is later work under issue #20 and may be enabled only per game adapter after staged write, backup, rollback, checksum/container repair, reparse, readback, and physical-device safety gates pass.

See `docs/TRANSFER_MODEL.md`.

---

## Master Vault principles

The Vault remains the permanent, game-independent safety/provenance layer:

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

Archival history must not be confused with an active duplicate after a future true Move.

See `docs/MASTER_VAULT_SPEC.md`.

---

## Current safety posture

**LIVE INSTALLED-GAME SAVE WRITING REMAINS HARD DISABLED AS PROJECT POLICY.**

Known lower-level protections include:

1. safe/backup destination posture;
2. generic save API cannot request title injection;
3. low-level restore rejects live-title writes before mounting save data;
4. legacy `injectToGame=1` is disabled/rewritten.

Extended hardware testing now proves that inherited mutation UI still exists above those protections. Therefore the next session must trace every reachable Release/Create/Move/Edit/Save path to its actual persistence target.

Do not claim the low-level lock is sufficient evidence that all user-reachable save mutation behavior is safe.

Issue #23 is the current blocker.

---

## Current task order

### Priority 1 — HIGH — safety/crash blocker session

Use:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Primary issues:

```text
#23 source-level mutation safety audit / block unsafe installed-source UI
#24 old/malformed Legends Arceus crash hardening
#19 preserve Session 2.5 analog fix
#13/#16 preserve visible PokeBank NX shell/branding work
```

The session must preserve the useful `361c6f55...` UI/analog work while fixing blockers.

If application source changes, create a **new application-source checkpoint**, push it, verify it, then clean-build the second device-test `.nro` from that exact new commit.

Do not falsely reuse `361c6f55...` as the artifact source if code changed.

### Priority 2 — physical Switch test #2

Verify at minimum:

```text
visible PokeBank NX identity
Left Stick single taps
Left Stick held repeat
Left Stick diagonal stability
D-pad regression
Action Sheet/B/Cancel
no Release on installed source
no Create on installed source
no unsafe Move/Multi on installed source
Edit cannot write installed source
old PLA save no longer crashes OR fails gracefully
OLED/Dark/Light + persistence
Party / Boxes / Storage
no new crash
```

Only physically exercised behavior on the exact replacement artifact becomes device evidence.

### Priority 3 — MAX — PKSM-Core Gen III spike

Only after the replacement safety/input/UI build is stable enough on hardware.

Use `docs/PROMPT_SESSION3_PKSM_CORE.md`.

Target issue #4:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
party/box extraction
active save-slot/sector behavior
round-trip strategy
integration/dependency decision
```

### Product order after PKSM-Core

```text
#3  Master Vault v1 + Banks
#9  Professional Summary + provenance
#6  RetroArch / read-only Gen I-III adapters
#11 Modern Switch adapter validation
#5  PKHeX Oracle
#7  Vault-driven Pokédex / Living Dex
#10 Conversion / transfer without live writes
#20 True Move semantics after verified write adapters
```

Supporting/later work:

```text
#15 persistent .nro artifacts
#16 final branding/startup/icon/NACP
#17 golden fixtures
#21 diagnostics/reliability/performance/QoL
#25 Pokémon visuals / optional model rotation
#26 controller normalization
#27 legacy Storage migration/clarification
```

---

## NRO quality roadmap

`docs/NRO_QUALITY_ROADMAP.md` captures planned high-value improvements including:

```text
Diagnostics screen + privacy-safe export
Applet/constrained-memory detection
real startup stages
privacy-safe crash/error logs
clear READ ONLY / Vault / staged badges
Vault recovery + storage health
search/filter
Box Quick Jump
Continue / Recently Viewed / Recently Added
Favorites
Reduced Motion
optional original UI sounds / restrained rumble
text-size options
color-independent focus
virtualized large grids
bounded artwork/sprite caches
intentional missing-resource fallbacks
metadata/index rebuildability
safe-operation progress UX
```

Use an approximate implementation balance after the second UI/device milestone:

```text
70% core functionality
20% hardware validation / bugs
10% polish / infrastructure
```

---

## Documentation map

Start with:

```text
docs/PROJECT_MAP.md
docs/NEXT_SESSION_PLAN.md
docs/BUILD_RECORD.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
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
- `docs/NRO_QUALITY_ROADMAP.md`
- `docs/UPSTREAM_AUDIT.md`
- `docs/PKSM_CORE_INTEGRATION.md`
- `docs/PKHEX_ORACLE.md`
- `docs/BUILD_RECORD.md`

## Permanent rule

Implement -> host test -> sanitize -> native build -> record exact application SHA/artifact hash -> physical test -> record exact pass/fail result.

Never confuse a later documentation commit with the application source used to build a device artifact.