# PokeBank NX — Session Log — 2026-09-02

Historical daily log. `PROJECT_STATUS.md` remains authoritative for current verified state.

## First physical hardware milestone

The exact Session 2 binary was physically run on Nintendo Switch hardware:

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
Held Left Stick repeat
Visible PokeBank NX product identity
```

Issue #8 was closed as the completed first physical-test milestone. #13 was reopened for visible UI acceptance and #19 created for the physical held-stick bug.

## Session 2.5 source checkpoint

A follow-up HIGH coding session identified the physical stick root cause: the app had been consuming button state without separately feeding real libnx analog stick position into the repeat model.

The session implemented/published:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

Reported work includes:

```text
real stick-position input
analog deadzone/hysteresis
sustained analog repeat
Select Game / Backups / Party / Boxes / Storage analog path
visible PokeBank NX identity/chrome/cards
shared Options/Help treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

GitHub host CI passed on the source commit.

The session ended before the exact-source clean rebuild/artifact hash/preservation, so `361c6f55` is a **source checkpoint**, not yet a second device-tested build.

Permanent continuation prompt created:

```text
docs/PROMPT_SESSION2_5_FINISH.md
```

## Product semantics clarified

The intended long-term transfer behavior was clarified:

```text
COPY  = intentional duplicate representation; source remains active
MOVE  = real relocation; source stops being active after destination success
CLONE = deliberate duplicate with clone provenance
```

Desired end-state:

```text
Game A -> Master Vault / Bank -> Game B
```

Current alpha remains read-only against installed saves and therefore uses safe Copy/import/staged behavior.

True Move is later work under issue #20 and requires independently validated per-game write adapters.

`docs/TRANSFER_MODEL.md` was rewritten to define destination-first true Move, active-location versus archival-history semantics, transfer journal states, and safety gates.

## NRO quality backlog formalized

Issue #21 and `docs/NRO_QUALITY_ROADMAP.md` were created to track:

```text
Diagnostics + privacy-safe export
Applet/constrained-memory handling
real startup stages
privacy-safe crash/error logs
READ ONLY / Vault / staged / active-location badges
Vault recovery / storage health
search/filter
Quick Jump
Favorites
Recent/Continue views
text sizing
Reduced Motion
optional original sounds / restrained rumble
virtualized large grids
bounded artwork caches
missing-resource fallbacks
rebuildable metadata/indexes
safe-operation progress/rollback UX
```

Recommended balance after the second UI/device milestone:

```text
70% core functionality
20% hardware validation / bug fixes
10% polish / infrastructure
```

## Issue/documentation updates

Created:

```text
#20 true Move semantics
#21 NRO reliability/diagnostics/performance/QoL
docs/NRO_QUALITY_ROADMAP.md
docs/PROMPT_SESSION2_5_FINISH.md
docs/SESSION_LOG_2026-09-02.md
```

Updated:

```text
README.md
PROJECT_STATUS.md
docs/BUILD_RECORD.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
docs/TRANSFER_MODEL.md
#3 Master Vault
#10 conversion/transfer
#13 UI shell
#16 branding/startup
#19 physical stick repeat
```

## Branch policy

`main` remains at the last physically tested source-history milestone.

`feature/pokebank-playable` contains:

```text
361c6f55 application source
+ later documentation-only commits
```

Do not confuse a later docs head with the application source used for the second device artifact.

## Next action

Use:

```text
docs/PROMPT_SESSION2_5_FINISH.md
```

Build/package exact source `361c6f55`, preserve/hash the actual `.nro`, and perform the second physical Switch test before beginning PKSM-Core.