# PokeBank NX — Next Session Plan

Last updated: 2026-09-02  
Status: **ACTIVE EXECUTION PLAN**

`PROJECT_STATUS.md` is authoritative for verified state.  
`docs/V1_ROADMAP.md` / issue #29 describe the full road to v1.0.  
`docs/GAME_SUPPORT_MATRIX.md` tracks the 23 current identities and the expanded 44-target catalog.  
This file describes what to do **next**, not every future feature.

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

Session 2.5 useful app-source checkpoint:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

That source is **NOT DEVICE TESTED**.

Extended old-build hardware findings:

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

---

# Block A — HIGH — Session 2.6 safety/crash finish

Execute exactly:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Primary issues:

```text
#23 mutation-path safety audit
#24 old/malformed PLA crash
#19 preserve/retest Left Stick source fix
#13 preserve/retest visible PokeBank NX shell
#16 preserve visible identity/NRO work
```

Do not begin later roadmap work during this session.

Explicit later/out-of-scope items now include:

```text
#30 DS/3DS identities
#31 Nintendo DS Gen IV/V adapters
#32 Nintendo 3DS Gen VI/VII adapters
#33 Colosseum/XD GameCube support
#34 Stadium 1/2 N64 support
```

## Session 2.6 success condition

```text
mutation paths classified
unsafe installed-source mutation UI blocked
PLA defensive failure path hardened
Session 2.5 UI/analog work preserved
host tests PASS
ASan/UBSan PASS
git diff --check PASS
native .nro BUILDS
new exact application source frozen if code changed
replacement .nro filename/size/SHA-256 recorded
artifact preserved/provided
second-device checklist populated with exact artifact identity
```

Then stop at:

```text
READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED
```

---

# Block B — second physical Switch test

Use:

```text
docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md
```

Critical acceptance areas:

```text
BOOT
visible PokeBank NX identity
no obvious PKSE top-level branding
D-pad regression
Left Stick single tap / hold / diagonal
Action Sheet regression
no installed-source Release/Create/unsafe Move/Edit commit
old PLA save opens or fails gracefully / no crash
OLED Black / Dark / Light
Theme persistence
Party / Boxes / Storage
no new crashes
```

Only the exact artifact physically run by the user receives device evidence.

---

# Block C — MAX — PKSM-Core Gen III spike

Start only after device gate #2 is stable enough.

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
active save slot/sectors
PK3 encrypt/decrypt/checksum behavior
adapter/dependency decision
```

Compare:

```text
PKSM-Core
FlagBrew/PKSM
Universal-Team/pkmn-chest
PKHeX
```

No live writes.

---

# Platform/source order after the Gen III foundation

Detailed dependencies live in `docs/V1_ROADMAP.md`. Current preferred source expansion order:

```text
Gen III GBA production reads
        ↓
Master Vault + Banks foundation
        ↓
Pokémon Colosseum / XD GameCube (#33)
        ↓
Gen I / II + RetroArch (#6)
        ↓
Pokémon Stadium 1 / 2 (#34, stretch if cheap enough)
        ↓
Nintendo DS Gen IV / V (#31)
        ↓
Nintendo 3DS Gen VI / VII (#32)
        ↓
modern Switch family validation (#11)
```

Catalog/source identities for DS/3DS are tracked by #30.

Why GameCube relatively early: Colosseum/XD are Gen III sources and can reuse the Gen III entity/Vault foundation. PKHeX already provides strong correctness references for their save and GameCube memory-card containers.

Why Stadium later: it becomes much cheaper conceptually after the Gen I/II entity engine exists. It is useful but may slip post-v1 rather than block the core release.

---

# Product sequence after broad read support

```text
Summary/provenance + Pokémon visuals
        ↓
golden corpus + PKHeX Oracle
        ↓
conversion/staged transfer
        ↓
Vault-driven Pokédex / Living Dex
        ↓
legality-aware edit / generation / events
        ↓
generic staged-save write framework
        ↓
first individually approved live-write adapter
        ↓
additional per-game adapters
        ↓
true Move
        ↓
release hardening / RC / v1.0
```

Future Transfer Workspace UX may use PHBank/Pokémon Chest patterns for one/multi/whole-box Game↔Vault operations.

---

# Expanded target catalog summary

```text
CURRENT IDENTITY TESTED: 23

PLANNED CORE:
Nintendo DS     +9
Nintendo 3DS    +8
GameCube        +2  Colosseum / XD

PLANNED STRETCH:
Nintendo 64     +2  Stadium / Stadium 2

TOTAL TARGET: 44
```

Do not call planned identities implemented until #30/#33/#34 source work and host tests exist.

---

# NRO quality work

Detailed backlog:

```text
docs/NRO_QUALITY_ROADMAP.md
GitHub issue #21
```

Pull diagnostics, logs, memory handling, search, Quick Jump, Favorites, recovery, virtualization, bounded caches and accessibility into dependent milestones incrementally.

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
DS/3DS adapters
GameCube adapter
Stadium adapter
full Pokédex
full legality engine
true Move/live writes
Pokémon 3D/model work
all generations at once
another large UI redesign
```

Finish the safety/crash blocker source, produce the exact replacement artifact, and physically test it first.