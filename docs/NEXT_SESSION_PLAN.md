# PokeBank NX — Next Session Plan

Last updated: 2026-09-02  
Status: ACTIVE EXECUTION PLAN

`PROJECT_STATUS.md` is authoritative.

## Current position

Completed:

```text
#2  Controller-first Pokemon Action Sheet
#8  First exact physical PokeBank NX .nro test
```

First physically tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Initial short test exposed:

```text
LEFT STICK + HOLD          FAIL
VISIBLE POKEBANK NX UI    FAIL / INCOMPLETE
```

Session 2.5 then produced/pushed:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on that source.

A later extended hardware pass on the **old `3be4de6b` binary** clarified additional blockers.

Permanent report:

```text
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
```

New facts:

```text
LEFT STICK SINGLE TAP      FAIL — no input/action
LEFT STICK HOLD            FAIL — no input/action
LEFT STICK DIAGONAL        FAIL — no input/action
HELD D-PAD                 PASS
OLD PLA SAVE               REPRODUCIBLE CRASH
INHERITED MUTATION UI      PHYSICALLY REACHABLE
APP STORAGE PERSISTENCE    PHYSICALLY OBSERVED
LIVE INSTALLED SAVE WRITE  NOT PROVEN
```

Because of these findings, the old packaging-only continuation is no longer the correct primary next step.

---

# Block A — HIGH — safety/crash blocker session

Use:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Primary issues:

```text
#23  audit/block inherited mutation UI before second device build
#24  old/malformed Legends Arceus save crash
#19  preserve and retest full Left Stick navigation fix
#13  preserve visible PokeBank NX shell
#16  preserve current visible branding/NRO identity work
```

Supporting findings:

```text
#25  Pokémon visual/model Summary idea — later
#26  normalize controller semantics — later except safety-relevant shortcuts
#27  legacy Storage vs future Master Vault/Banks — classify now, redesign later
```

## Required safety audit

Trace these user-reachable paths to their final persistence targets:

```text
Release
Create Pokémon
Move / Multi
Edit/apply/save
unsaved-changes prompt
legacy/app Storage move/import
any reachable Save / Commit / Restore / Inject path
```

Classify each as:

```text
LIVE INSTALLED SAVE WRITE
BACKUP/STAGED SAVE WRITE
APP-OWNED STORAGE WRITE
IN-MEMORY ONLY
DISABLED / UNREACHABLE
UNKNOWN — NEEDS BLOCKING
```

Do not rely on UI wording.

Current alpha behavior for installed-game sources must remain read-only at the user-facing level. Unsafe Release/Create/Move/Edit/save paths must not remain reachable merely because a lower-level write lock probably rejects the final write.

## Required PLA crash behavior

```text
valid supported PLA save           opens
old/unsupported/malformed save     useful error / safe return
parser failure                     never crashes the app
failing save                       never auto-written/repaired
```

If the exact old save is not available in the coding runtime, make source-grounded defensive fixes/tests only and keep #24 OPEN for physical retest.

## Preserve Session 2.5 work

Do not redo the visible shell/analog work from scratch.

Preserve:

```text
real libnx Left Stick position handling
deadzone/hysteresis
single-tap + held repeat path
diagonal stability
PokeBank NX header/chrome/cards
Options / Help styling
Action Sheet styling
NRO/window PokeBank NX identity
OLED / Dark / Light themes
```

## Application source rule

Because #23/#24 may require source changes, `361c6f55...` is no longer automatically the final second-device artifact source.

If source changes:

```text
implement coherent blocker fix
→ host test
→ sanitize
→ diff check
→ native build
→ COMMIT APPLICATION SOURCE
→ push/verify remote SHA
→ clean rebuild from exact new SHA
→ hash/preserve .nro
```

Never modify source and still label the binary as `361c6f55`.

---

# Block B — second physical Switch test

The new replacement artifact should test:

```text
BOOT                                      PASS
D-PAD                                     PASS
LEFT STICK SINGLE TAP                     PASS
LEFT STICK HELD REPEAT                    PASS
LEFT STICK DIAGONAL                       PASS
A/B ACTION SHEET                          PASS
NO RELEASE ON INSTALLED SOURCE            PASS
NO CREATE ON INSTALLED SOURCE             PASS
NO UNSAFE MOVE/MULTI ON INSTALLED SOURCE  PASS
EDIT CANNOT WRITE INSTALLED SOURCE        PASS
OLD PLA SAVE                              OPEN OR GRACEFUL ERROR / NO CRASH
OLED BLACK / DARK / LIGHT                 PASS
THEME PERSISTENCE                         PASS
PARTY / BOXES / STORAGE                   PASS
VISIBLY POKEBANK NX                       PASS
OBVIOUS PKSE BRANDING REMOVED             PASS
NO NEW CRASHES                            PASS
```

Only physically exercised behavior on the exact replacement hash becomes device evidence.

If a device-only regression remains, do the smallest coherent HIGH fix/build/retest loop.

---

# Block C — MAX — PKSM-Core Gen III spike

Start only after the replacement safety/input/UI build is stable enough on hardware.

Use:

```text
docs/PROMPT_SESSION3_PKSM_CORE.md
```

Issue #4 target:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
party / box extraction
active save-slot / sector behavior
PK3 encrypt/decrypt/checksum behavior
untouched round-trip strategy
adapter/dependency decision
```

Issue #17 supports reproducible fixtures. Do not let fixture perfection block the first useful integration decision.

No live writes.

---

# Core product order after PKSM-Core

```text
#3  Master Vault v1 + named Banks
#9  Professional Summary + provenance
#6  RetroArch discovery + read-only Gen I-III adapters
#11 Modern Switch adapter validation
#5  PKHeX Oracle
#7  Vault-driven Pokedex / Living Dex
#10 Conversion / transfer without live writes
#20 True Move semantics after verified safe-write adapters
```

Supporting/later:

```text
#15 persistent .nro artifact automation
#16 final branding/startup/icon/NACP
#17 golden fixture corpus
#21 NRO reliability/diagnostics/performance/QoL
#25 Pokémon Summary visual/model support
#26 controller normalization
#27 legacy Storage migration/clarification
```

---

# Transfer semantics going forward

Product behavior remains:

```text
COPY  = source stays active; destination representation is created
MOVE  = destination becomes active; source stops being active after verification
CLONE = deliberate duplicate with clone provenance
```

Current alpha does not authorize true Move/live installed-save writes.

True Move is **later**, not now. It requires per-adapter backup, stage, checksum/container repair, reparse, write, readback, verification, rollback, and physical hardware gates.

See:

```text
docs/TRANSFER_MODEL.md
GitHub issue #20
```

---

# NRO quality / polish plan

Detailed backlog:

```text
docs/NRO_QUALITY_ROADMAP.md
GitHub issue #21
```

High-value items include:

```text
Diagnostics + diagnostic export
Applet/constrained-memory warning
privacy-safe error logs
real startup stages
READ ONLY / Vault / staged / active-location badges
Vault recovery + storage health
search/filter
Quick Jump
Favorites
recent items
text sizing
Reduced Motion
optional original sounds/rumble
virtualized large grids
bounded artwork caches
intentional missing-resource fallback
```

Recommended balance after the second-device milestone:

```text
70% core functionality
20% hardware testing / regression fixes
10% polish / infrastructure
```

---

# Do not do prematurely

Do not begin these during the current blocker session:

```text
PKSM-Core
Master Vault
full Pokedex
full legality engine
true Move/live writes
Pokemon 3D/model work
all generations at once
another large UI redesign
```

Finish the safety/crash blocker source, build an exact replacement artifact, and physically test it first.