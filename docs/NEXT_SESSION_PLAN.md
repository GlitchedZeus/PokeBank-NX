# PokeBank NX — Next Session Plan

Last updated: 2026-09-02  
Status: **ACTIVE EXECUTION PLAN**

`PROJECT_STATUS.md` is authoritative for verified state.  
`docs/V1_ROADMAP.md` / issue #29 describe the full road to v1.0.  
This file describes only what to do **next**.

---

# Current position

Completed milestones:

```text
#2  Controller-first Pokémon Action Sheet
#8  First exact physical PokeBank NX .nro test
```

First physically tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Session 2.5 later published:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

That source contains the reported PokeBank NX visual shell and real libnx analog-input fix. GitHub host CI passed, but it is **NOT DEVICE TESTED**.

The extended hardware pass on the old `3be4de6b...` binary added these facts:

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

Permanent report:

```text
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
```

Because of those findings, the next session is no longer a packaging-only session.

---

# Block A — HIGH — Session 2.6 safety/crash finish

Use exactly:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Primary issues:

```text
#23 audit/block inherited mutation UI before second device build
#24 old/malformed Legends Arceus crash
#19 preserve/retest full Left Stick navigation fix
#13 preserve visible PokeBank NX shell
#16 preserve visible branding/NRO identity work
```

Later/supporting findings:

```text
#25 Pokémon Summary visuals/model concept
#26 controller normalization
#27 legacy Storage vs future Master Vault/Banks
```

## Required source-level safety audit

Trace:

```text
Release
Create Pokémon
Move / Multi
Edit / apply / save
unsaved-changes prompt
legacy/app Storage move/import
any reachable Save / Commit / Restore / Inject path
```

Classify each final persistence target:

```text
LIVE INSTALLED SAVE WRITE
BACKUP/STAGED SAVE WRITE
APP-OWNED STORAGE WRITE
IN-MEMORY ONLY
DISABLED / UNREACHABLE
UNKNOWN — NEEDS BLOCKING
```

Do not rely on UI wording.

While live installed-save writing remains disabled:

- installed-game browsing stays read-only;
- Release/Create must not be reachable on installed-game sources;
- unsafe Move/Multi must not persistently mutate an installed source;
- Edit must not commit into an installed source;
- controller shortcuts must not bypass the read-only contract;
- app-owned Storage may remain writable only if its separation/persistence semantics are proven and clearly labeled.

## Required PLA crash behavior

```text
valid supported PLA save           opens
old/unsupported/malformed save     useful read-only error / safe return
parser failure                     no crash
failing save                       never auto-written/repaired
```

If the exact old save is unavailable to the coding runtime, make source-grounded defensive fixes/tests and keep #24 open for physical retest.

## Preserve Session 2.5 work

Do not redo from scratch:

```text
real libnx Left Stick handling
deadzone/hysteresis
single-tap + held repeat path
diagonal stability
PokeBank NX header/chrome/cards
Options / Help styling
Action Sheet styling
NRO/window identity
OLED / Dark / Light themes
```

## Application-source rule

If #23/#24 changes application code:

```text
implement
→ host test
→ sanitize
→ diff check
→ native build
→ COMMIT APPLICATION SOURCE
→ push/verify remote SHA
→ clean rebuild from exact new SHA
→ hash/preserve .nro
```

Never change source and still label the binary `361c6f55`.

---

# Block B — second physical Switch test

The replacement exact artifact should verify at minimum:

```text
BOOT                                      PASS/FAIL
VISIBLY POKEBANK NX                       PASS/FAIL
OBVIOUS PKSE BRANDING REMOVED             PASS/FAIL
D-PAD                                     PASS/FAIL
LEFT STICK SINGLE TAP                     PASS/FAIL
LEFT STICK HELD REPEAT                    PASS/FAIL
LEFT STICK DIAGONAL                       PASS/FAIL
A/B ACTION SHEET                          PASS/FAIL
NO RELEASE ON INSTALLED SOURCE            PASS/FAIL
NO CREATE ON INSTALLED SOURCE             PASS/FAIL
NO UNSAFE MOVE/MULTI ON INSTALLED SOURCE  PASS/FAIL
EDIT CANNOT WRITE INSTALLED SOURCE        PASS/FAIL
OLD PLA SAVE                              OPEN OR GRACEFUL ERROR / NO CRASH
OLED BLACK / DARK / LIGHT                 PASS/FAIL
THEME PERSISTENCE                         PASS/FAIL
PARTY / BOXES / STORAGE                   PASS/FAIL
NO NEW CRASHES                            PASS/FAIL
```

Only behavior physically exercised on the exact replacement artifact becomes device evidence.

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

Additional integration references discovered 2026-09-02:

```text
FlagBrew/PKSM
Universal-Team/pkmn-chest
```

Both should be inspected alongside PKSM-Core to see how mature Nintendo homebrew applications wrap the core. See:

```text
docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md
```

Issue #17 supports reproducible fixtures. No live writes.

---

# Product order after PKSM-Core

Detailed ordering lives in `docs/V1_ROADMAP.md`. Near-term sequence:

```text
Gen III production read adapter
        ↓
#3 Master Vault v1 + named Banks
        ↓
#9 Summary/provenance + #25 Pokémon visuals
        ↓
#17 golden corpus + #5 PKHeX Oracle
        ↓
#6 Gen I/II + RetroArch
#11 modern Switch read validation
        ↓
#10 conversion/staged transfer
        ↓
#7 Pokédex/Living Dex
        ↓
legality/edit/generation/events
        ↓
staged-save write engine
        ↓
per-game approved live-write adapters
        ↓
#20 true Move
        ↓
release hardening / v1.0 (#29)
```

Future Transfer Workspace UX may use the PHBank/Pokémon Chest pattern for one/multi/whole-box Game↔Vault operations, but do not implement that during the current blocker session.

---

# NRO quality work

Detailed backlog:

```text
docs/NRO_QUALITY_ROADMAP.md
GitHub issue #21
```

High-value items:

```text
Diagnostics + export
Applet/constrained-memory warning
privacy-safe error logs
real startup stages
READ ONLY / Vault / staged / active-location badges
Vault recovery + storage health
search/filter
Quick Jump
Favorites / recent items
text sizing / Reduced Motion / non-color-only focus
optional original sounds/rumble
virtualized large grids
bounded artwork caches
intentional missing-resource fallback
```

Pull these in incrementally when their dependent subsystem exists.

Recommended balance after the second-device milestone:

```text
70% core functionality
20% hardware testing / regression fixes
10% polish / infrastructure
```

---

# Do not do prematurely in Session 2.6

```text
PKSM-Core
Master Vault
full Pokédex
full legality engine
true Move/live writes
Pokémon 3D/model work
all generations at once
another large UI redesign
```

Finish the safety/crash blocker source, produce an exact replacement artifact, and physically test it first.