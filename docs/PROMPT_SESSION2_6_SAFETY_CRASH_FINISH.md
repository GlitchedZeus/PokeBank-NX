# PokeBank NX — Session 2.6 Safety / Crash / Second-Device Build Prompt

Use **HIGH reasoning**.

Continue development of:

```text
GlitchedZeus/PokeBank-NX
```

Development branch:

```text
feature/pokebank-playable
```

PKSE is upstream only. **NEVER push PokeBank NX changes to upstream.**

---

# Mission

Finish the blockers discovered by the extended physical test, preserve the Session 2.5 PokeBank NX UI/analog work, produce a new exact `.nro`, preserve/hash it, and stop at:

```text
READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED
```

Do **not** begin the later engine/catalog roadmap in this session.

---

# Critical continuity

Useful Session 2.5 application source:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

This source was host/CI green and reported to build natively, but it is **NOT DEVICE TESTED**.

Later commits on `feature/pokebank-playable` are documentation/research/roadmap commits. Branch HEAD is not automatically application-source identity.

Exact old physically tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

---

# Read first

```text
PROJECT_STATUS.md
README.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/NEXT_SESSION_PLAN.md
docs/SESSION_RUNBOOK.md
docs/SAVE_SAFETY.md
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/BUILD_RECORD.md
docs/RELEASE_CHECKLIST.md
docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

`docs/V1_ROADMAP.md`, `docs/GAME_SUPPORT_MATRIX.md`, and issues #29–#34 describe later work only. Read them for context if needed, but **do not implement them now**.

Inspect current blocker issues:

```text
#13  visible PokeBank NX shell
#16  branding/startup/NRO metadata
#19  Left Stick navigation
#23  inherited mutation UI safety audit — BLOCKER
#24  old/malformed Legends Arceus crash — BLOCKER
#25  Pokémon visual/model Summary — later
#26  controller normalization — only safety-critical changes now
#27  legacy Storage vs Master Vault — classify only as needed now
```

Explicitly out of scope:

```text
#3   Master Vault
#4   PKSM-Core
#20  true Move
#29  v1 roadmap implementation
#30  DS/3DS identities
#31  Nintendo DS adapters
#32  Nintendo 3DS adapters
#33  Colosseum/XD
#34  Stadium 1/2
```

---

# Preserve local work before destructive operations

Run:

```bash
pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git remote -v
git log --all --oneline --decorate --graph -30
git reflog -20
git stash list
git worktree list
```

Do not reset/clean away legitimate interrupted work.

Fetch safely and verify `origin/feature/pokebank-playable`.

---

# Physical evidence to treat as source of truth

## Left Stick

On old exact `3be4de6b...`:

```text
single tap Up/Down/Left/Right  FAIL — no input/action
hold Up/Down/Left/Right        FAIL — no input/action
diagonal                       FAIL — no input/action
held D-pad                     PASS
```

Preserve the real libnx stick-position/deadzone/hysteresis work from `361c6f55...` unless source inspection proves it broken.

## Legends Arceus

```text
5-minute idle              PASS
10-minute browsing         PASS
5 relaunches               PASS
one older PLA save         REPRODUCIBLE CRASH
another PLA save           opens more successfully
```

Treat the old save as unsupported/old/malformed/valid-with-parser-bug until proven otherwise.

## Inherited mutation UI

Physically reachable on old build:

```text
X on Pokémon        Release path
Y in Boxes          Menu / Move / Multi
Y on Pokémon        grab/move behavior
A on empty slot     Create Pokémon path
Action Sheet Edit   editable Pokémon view
```

The tester changed an IV, applied it, reopened it, then restored the original value.

Legacy/app Storage was also observed to persist an Arbok moved into it.

Evidence boundary:

```text
LIVE INSTALLED SAVE WRITE: NOT PROVEN
USER-REACHABLE MUTATION UI: PROVEN
APP STORAGE PERSISTENCE: PROVEN
```

The original game was not externally re-opened to prove whether its live save changed. Do not falsely claim a confirmed live-save write regression.

---

# BLOCK A — issue #23 — source-level mutation safety audit

Trace the actual call paths for:

```text
Release
Create Pokémon
Move / Multi box manipulation
Action Sheet Edit
editable Summary apply/save
unsaved-changes prompt
legacy/app Storage move/import
Save / Commit / Restore / Inject reachable from these screens
```

Classify every final persistence target as exactly one of:

```text
LIVE INSTALLED SAVE WRITE
BACKUP/STAGED SAVE WRITE
APP-OWNED STORAGE WRITE
IN-MEMORY ONLY
DISABLED / UNREACHABLE
UNKNOWN — NEEDS BLOCKING
```

Follow the code to filesystem/save APIs. Do not classify from UI wording.

While live installed-save writing is disabled, installed-game sources must behave read-only:

- Release not reachable;
- Create Pokémon not reachable;
- unsafe Move/Multi not reachable;
- Edit cannot commit into installed source;
- if safe copy-editing is not implemented, Edit returns clear read-only/not-supported behavior;
- no Save Changes path can write installed source;
- no controller shortcut bypasses the safety model;
- Action Sheet navigation/B/Cancel remains zero-mutation.

If legacy Storage is proven separate/app-owned, it may remain writable, but do not silently remove an installed-source Pokémon while live writes are disabled.

Add host regression tests where practical.

**Do not remove the low-level hard lock.** UI blocking is defense-in-depth.

---

# BLOCK B — issue #24 — PLA crash hardening

Inspect for:

```text
file/container size assumptions
block lookup assumptions
SCBlock/block-map assumptions
null/optional accesses
out-of-range reads
version/revision handling
truncated/malformed input
unsupported save revisions
```

Required behavior:

```text
valid supported PLA save           opens
old/unsupported/malformed save     useful read-only error + safe return
parser failure                     never crashes app
failing save                       never auto-repaired/written
```

If the exact old user save is unavailable:

- do not claim the hardware crash is fixed;
- add source-grounded defensive checks/tests;
- use synthetic malformed/truncated fixtures where practical;
- keep #24 open pending physical retest.

Do not commit private user saves.

---

# BLOCK C — preserve Session 2.5 UI/input work

Preserve:

```text
real libnx Left Stick position handling
deadzone/hysteresis
single-tap + held-repeat analog navigation
diagonal stability
Select Game / Backups / Party / Boxes / Storage analog support
visible PokeBank NX header/app identity
PokeBank NX chrome/cards/focus language
shared Options/Help styling
matching Action Sheet styling
PokeBank NX NRO/window identity
OLED Black / Dark / Light + persistence
```

Do not redesign the UI from scratch.

Do not start Pokémon renders/models (#25) except to preserve existing behavior.

Do not do full controller redesign (#26) except where required to remove unsafe mutation shortcuts.

---

# Application-source checkpoint rule

If this session changes application code, create a **NEW application-source commit**.

Do not modify source and still label the artifact as `361c6f55`.

Required order:

```text
coherent source changes
        ↓
host tests
        ↓
ASan/UBSan
        ↓
git diff --check
        ↓
native build
        ↓
COMMIT APPLICATION SOURCE
        ↓
push origin/feature/pokebank-playable
        ↓
verify remote application-source SHA
        ↓
clean rebuild from that exact commit
        ↓
hash/preserve replacement .nro
```

If inspection proves no source changes are needed, explain exactly why before reusing `361c6f55`.

---

# Required verification

Run at minimum:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make clean
make -j1
```

Require:

```text
HOST TESTS PASS
ASan/UBSan PASS
git diff --check PASS
NATIVE .NRO BUILDS
LIVE-WRITE HARD LOCK INTACT
MUTATION PATHS CLASSIFIED
UNSAFE INSTALLED-SOURCE UI MUTATIONS BLOCKED
PLA FAILURE PATH HARDENED AS FAR AS AVAILABLE EVIDENCE ALLOWS
```

---

# Replacement artifact

Provide the actual `.nro`.

Record:

```text
Application source full SHA:
Application source commit message:
Embedded abbreviated SHA/version:
Artifact filename:
Artifact size:
Artifact SHA-256:
Host tests:
Sanitizers:
git diff --check:
Native build:
Device tested: NO
```

Use a source-addressed filename, for example:

```text
PokeBank-NX-Second-Device-<shortsha>.nro
```

If direct `.nro` delivery is unreliable, also provide a ZIP containing the `.nro` and a tiny manifest/SHA record.

Do not leave the only artifact in a temporary runtime.

---

# Populate second-device checklist

Before handoff update the identity block in:

```text
docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md
```

with the real:

```text
application source SHA
commit message
embedded short SHA/version
artifact filename
artifact size
artifact SHA-256
host tests
sanitizers
native build
```

Do not pre-fill PASS/FAIL hardware results. Those remain for the user.

---

# Update project state

Update as appropriate:

```text
PROJECT_STATUS.md
README.md
docs/BUILD_RECORD.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
docs/V1_ROADMAP.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md
```

Update issues accurately:

```text
#13 keep OPEN until visual hardware acceptance
#16 keep OPEN if startup/icon/NACP polish remains
#19 keep OPEN until physical Left Stick acceptance
#23 keep OPEN until required safety verification is complete
#24 keep OPEN until exact old PLA save physically retests successfully/gracefully
#26/#27 record findings without expanding scope
```

Do not rewrite historical evidence for `3be4de6b...`.

---

# Second physical acceptance target

Prepare the replacement artifact for:

```text
BOOT                                      PASS
VISIBLY POKEBANK NX                       PASS
NO OBVIOUS PKSE TOP-LEVEL BRANDING        PASS
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
NO NEW CRASHES                            PASS
```

Only the user's physical run can promote any item to `DEVICE TESTED`.

---

# DO NOT START

```text
PKSM-Core
Master Vault
DS/3DS identities or adapters
Colosseum/XD
Stadium 1/2
true Move/live writes
Pokémon 3D/model work
full controller redesign
new large UI redesign
```

---

# Stop condition

Stop only after:

```text
#23 audit complete enough for current alpha
unsafe installed-source mutation UI blocked
#24 defensive crash hardening done as far as evidence allows
Session 2.5 shell/analog work preserved
host tests PASS
sanitizers PASS
git diff check PASS
native build PASS
new app source committed/pushed/verified if changed
exact replacement .nro built from exact source
artifact size + SHA-256 recorded
artifact preserved/provided
second-device checklist populated with exact identity
project docs/issues updated
```

Then report exactly:

```text
READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED
```

Do not claim Left Stick, PLA crash, or live-save safety behavior is device-fixed until the user runs that exact replacement binary.