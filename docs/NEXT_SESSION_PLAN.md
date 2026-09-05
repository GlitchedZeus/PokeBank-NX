# PokeBank NX — Next Session Plan

Last updated: 2026-09-04
Status: **WAITING FOR SECOND DEVICE TEST**

Session 2.6 is built and packaged from application source
`0ea98cc1a9f9dfc2b17abc33e944caa4aa9de915`. Use the exact artifact identity in
`BUILD_RECORD.md` and fill the blank hardware fields in
`DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md` after Will runs that binary.

Do not repeat recovery, sprite generation, or implementation merely because a later docs commit is
branch HEAD. Do not start later engines or merge main before physical acceptance. #24 remains open:
the old failing PLA save must open or return gracefully on the device. The application remains
**NOT DEVICE TESTED** until that exact binary is physically run.

The earlier planning context below is retained for continuity; the physical gate above takes priority.

`PROJECT_STATUS.md` is authoritative for verified state.  
`docs/V1_ROADMAP.md` / issue #29 describe the full road to v1.0.  
`docs/GAME_SUPPORT_MATRIX.md` tracks the current registry and expanded target catalog.  
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

Latest first-build hardware evidence:

```text
LEFT STICK SINGLE TAP      FAIL — no input/action
LEFT STICK HOLD            FAIL — no input/action
LEFT STICK DIAGONAL        FAIL — no input/action
HELD D-PAD                 PASS
OLD PLA SAVE               REPRODUCIBLE CRASH
INHERITED MUTATION UI      PHYSICALLY REACHABLE
APP STORAGE PERSISTENCE    PHYSICALLY PROVEN
CROSS-GAME STORAGE VIEW    PHYSICALLY PROVEN
ORIGINAL INSTALLED SAVE    TESTER-REPORTED UNCHANGED IN EXERCISED FLOW
LIVE INSTALLED SAVE WRITE  NOT OBSERVED
HOME/SLEEP/RECONNECT       PASS
HANDHELD                   PASS
DOCKED                     NOT TESTED
```

Permanent reports:

```text
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
```

Static safety starting point:

```text
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
```

---

# Clarified legacy Storage transfer behavior

The physically observed Arbok sequence was:

```text
installed Z-A source
    ↓ automatic backup
open Z-A backup representation
    ↓
move Arbok into inherited app Storage
    ↓
return to main menu / open another game
    ↓
Storage still contains Arbok
```

Current interpretation:

```text
Installed source        READ ONLY / unchanged in exercised check
Backup representation   mutable inherited copy
Legacy Storage          app-owned persistent PKSEBANK
Other game backup       potential compatible destination
Master Vault            NOT IMPLEMENTED
True Move               NOT IMPLEMENTED
```

This is useful proof-of-concept cross-game Bank UX but not product-level Move.

Issue #27 now records the exact sequence.

---

# Block A — HIGH — Session 2.6 safety/crash finish

Execute exactly:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Required reading now includes:

```text
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
docs/DEVICE_BUILD_ASSET_GATE.md
docs/DEVICE_ARTIFACT_PACKAGING.md
```

Primary issues:

```text
#23 mutation-path safety/UI contract
#24 old/malformed PLA crash
#19 preserve/retest Left Stick source fix
#13 preserve/retest visible PokeBank NX shell
#16 preserve visible identity/NRO work
#37 require generated visual assets in device build
```

Supporting classifications:

```text
#26 controller normalization — later except safety/readability
#27 legacy Storage vs Master Vault — now physically clarified
```

Do not begin later roadmap work during this session.

Explicit later/out-of-scope items include:

```text
#3 Master Vault
#4 PKSM-Core
#20 true Move
#25 full Pokémon visual/model redesign
#30 DS/3DS identities
#31 Nintendo DS adapters
#32 Nintendo 3DS adapters
#33 Colosseum/XD GameCube support
#34 Stadium 1/2 N64 support
#35 full Pokémon cry feature
```

## Session 2.6 success condition

```text
existing static safety audit verified/completed
unsafe/ambiguous installed-source mutation UI blocked
installed source vs backup/staged vs legacy Storage clearly labeled
low-level live-write hard lock preserved
PLA defensive failure path hardened
Session 2.5 UI/analog work preserved
small dark-theme hint readability polish only if trivial
required Pokémon visual assets generated/preflighted
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
Pokémon visual actually present if asset preflight passed
no Release/Create/unsafe Move/Edit on INSTALLED SOURCE
backup/staged state clearly distinguished
legacy Storage clearly app-owned
old PLA save opens or fails gracefully / no crash
OLED Black / Dark / Light
bottom button hints readable
Theme persistence
Party / Boxes / Storage
HOME / sleep / resume
controller reconnect
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
Summary/provenance + Pokémon visuals + cries
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

Future Transfer Workspace UX may use PHBank/Pokémon Chest patterns and the physically observed legacy Storage flow for one/multi/whole-box Game↔Vault operations, while using the safer Master Vault/transaction architecture underneath.

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

Do not call planned identities implemented until their source work and host tests exist.

---

# NRO quality work

Detailed backlog:

```text
docs/NRO_QUALITY_ROADMAP.md
GitHub issue #21
```

Build-specific helpers now include:

```text
tools/check_device_assets.py
tools/package_device_build.py
docs/DEVICE_BUILD_ASSET_GATE.md
docs/DEVICE_ARTIFACT_PACKAGING.md
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
full realtime Pokémon 3D work
another large UI redesign
```

Finish the safety/crash blocker source, preserve the newer shell/stick work, ensure required device assets are packaged, produce the exact replacement artifact, and physically test it first.
# Immediate device-feedback correction — static Pokémon artwork

The exact `0ea98cc1...` binary proved that HD HOME artwork is packaged, visible and good quality,
but the inherited `drawSpriteIdle(...)` bob/squash/stretch presentation failed visual acceptance.
The current source replaces active Summary/View and selected-preview call sites with a static,
aspect-ratio-preserving renderer while preserving the resolver/cache/fallback pipeline. Package and
physically retest the new exact-source artifact before treating this correction as accepted.

See `docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md`.
