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

Finish the blockers discovered by the physical tests, preserve the Session 2.5 PokeBank NX UI/analog work, produce a new exact `.nro`, preserve/hash it, and stop at:

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

Later commits on `feature/pokebank-playable` are documentation/research/roadmap/tooling commits. Branch HEAD is not automatically application-source identity.

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
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
docs/NEXT_SESSION_PLAN.md
docs/SESSION_RUNBOOK.md
docs/SAVE_SAFETY.md
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/BUILD_RECORD.md
docs/DEVICE_BUILD_ASSET_GATE.md
docs/DEVICE_ARTIFACT_PACKAGING.md
docs/RELEASE_CHECKLIST.md
docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

`docs/V1_ROADMAP.md`, `docs/GAME_SUPPORT_MATRIX.md`, and later platform issues describe later work only. Read them for context if needed, but **do not implement them now**.

Inspect current blocker/support issues:

```text
#13  visible PokeBank NX shell
#16  branding/startup/NRO metadata
#19  Left Stick navigation
#23  inherited mutation UI safety audit — BLOCKER
#24  old/malformed Legends Arceus crash — BLOCKER
#25  Pokémon visual/model Summary — later, except existing asset packaging gate
#26  controller normalization — only safety-critical changes now
#27  legacy Storage vs Master Vault — classification already clarified
#37  require device-test RomFS visual assets
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
#35  full cry feature implementation
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

## Sep 3 clarified Arbok/Storage flow

This supersedes the older ambiguous wording that sounded like Arbok might have been removed from the installed Z-A title.

Exact tester-reported flow:

```text
installed Z-A source
    ↓ automatic backup
open Z-A BACKUP representation
    ↓
move Arbok from backup into inherited Storage
    ↓
return to main menu
    ↓
open another supported game's backup/session
    ↓
open Storage
    ↓
Arbok is still present
```

The tester reports the **original installed Z-A save remained unchanged** in the exercised flow.

Therefore current evidence is:

```text
LIVE INSTALLED SAVE WRITE OBSERVED:      NO
ORIGINAL INSTALLED SAVE CHANGED:         NO — tester-reported for exercised flow
USER-REACHABLE MUTATION UI:              PROVEN
BACKUP/IN-MEMORY MUTATION:               PROVEN
APP STORAGE PERSISTENCE:                 PROVEN
CROSS-GAME STORAGE VISIBILITY:           PROVEN
MASTER VAULT:                            NOT IMPLEMENTED
TRUE MOVE:                               NOT IMPLEMENTED
```

The inherited UI appears capable of placing the stored Pokémon into another game's loaded backup representation when compatible, but do not claim exhaustive cross-game persistence/compatibility verification from this test alone.

Current physical transfer shape is:

```text
INSTALLED GAME — read / backup
        ↓
MUTABLE BACKUP REPRESENTATION
        ↓
APP-OWNED LEGACY STORAGE (`PKSEBANK` / `bank.dat`)
        ↓
OTHER GAME'S LOADED BACKUP REPRESENTATION
```

Do **not** call this a true Move and do **not** call legacy Storage the Master Vault.

---

# BLOCK A — issue #23 — source-level mutation safety audit + UI contract

`docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md` already did a first source trace. Use it as a starting point, then verify against current source.

Known current classification from that audit:

```text
Release/Create/Move/Edit   mutate in-memory/backup-loaded representation
normal Save Changes        targets Working Backup / New Backup
legacy Storage             app-owned persistent PKSEBANK/bank.dat
low-level live restore     hard-blocked before mounting save:/ while policy disabled
```

Do not merely redo that audit from scratch. Confirm it, fill any gaps, then implement the required user-facing safety behavior.

Trace/verify actual call paths for:

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

### Required alpha UX

The underlying backup/storage behavior can be useful, but the UI must make the safety boundary unambiguous.

For an installed-game source / initial game browsing context:

- Release not reachable;
- Create Pokémon not reachable;
- unsafe Move/Multi not reachable;
- Edit cannot commit into installed source;
- if safe copy-editing is not implemented, Edit returns clear read-only/not-supported behavior;
- no Save Changes path can imply it writes the installed source;
- no controller shortcut bypasses the read-only contract;
- Action Sheet navigation/B/Cancel remains zero-mutation.

If the user deliberately opens a mutable **backup/staged copy**, UI may preserve useful backup-editing behavior only if it is clearly labeled as backup/staged and cannot be confused with the installed title.

If legacy Storage remains writable for this alpha, label/classify it as app-owned legacy/compatibility Storage. Depositing from a backup must not be presented as removing the Pokémon from the installed game.

Add host regression tests where practical.

**Do not remove the low-level hard lock.** UI restrictions and clear source-state labeling are defense-in-depth.

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

Hardware polish feedback from Sep 3:

- bottom controller/button-hint graphics are too low-contrast in OLED Black and Dark;
- Dark could be slightly darker (~10% was the tester's suggestion);
- Light color treatment is liked as-is;
- OLED Black Action Sheet appearance is currently preferred.

Treat this as small polish only if trivial while touching the same UI code. Do not derail the blocker session.

Do not start full Pokémon render/model work (#25). However, the existing inherited sprite pipeline must not be accidentally omitted from the replacement artifact.

---

# BLOCK D — device visual asset gate (#37)

The first device artifact's `View Pokémon` path showed no Pokémon visual even though inherited source contains visual rendering support. The inherited build pipeline uses generated gitignored RomFS assets.

Before the final native build, inspect/read:

```text
docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md
docs/DEVICE_BUILD_ASSET_GATE.md
```

Use the existing asset-generation/preflight workflow rather than inventing a new art system in this session.

At minimum run the repository's device-asset preflight and ensure the required generated sprite resources exist before packaging.

This is a packaging/release correctness task, not authorization to begin #25's full visual-resolver/3D roadmap.

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
asset preflight
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

Also run the repository device asset preflight required by `docs/DEVICE_BUILD_ASSET_GATE.md`.

Require:

```text
HOST TESTS PASS
ASan/UBSan PASS
git diff --check PASS
NATIVE .NRO BUILDS
LIVE-WRITE HARD LOCK INTACT
MUTATION PATHS CLASSIFIED
UNSAFE/AMBIGUOUS INSTALLED-SOURCE UI MUTATIONS BLOCKED
BACKUP/STAGED VS INSTALLED SOURCE CLEARLY DISTINGUISHED
PLA FAILURE PATH HARDENED AS FAR AS AVAILABLE EVIDENCE ALLOWS
DEVICE ASSET PREFLIGHT PASS
```

---

# Replacement artifact

Provide the actual `.nro`.

Use the repository packaging helper documented in:

```text
docs/DEVICE_ARTIFACT_PACKAGING.md
```

Preferred packaging command after the exact application-source commit is clean and built:

```bash
python tools/package_device_build.py PokeBankNX.nro --label Second-Device --zip
```

The helper must not replace independent verification; inspect its manifest/output.

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
Device asset preflight:
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
asset preflight
```

Do not pre-fill PASS/FAIL hardware results. Those remain for the user.

Make the checklist wording explicit about source state:

```text
INSTALLED SOURCE
BACKUP/STAGED COPY
LEGACY STORAGE
```

Do not use ambiguous `save` wording when the test needs to distinguish them.

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
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md
```

Update issues accurately:

```text
#13 keep OPEN until visual hardware acceptance
#16 keep OPEN if startup/icon/NACP polish remains
#19 keep OPEN until physical Left Stick acceptance
#23 keep OPEN until required safety/UI verification is complete
#24 keep OPEN until exact old PLA save physically retests successfully/gracefully
#26 record controller findings without broadening scope
#27 preserve exact backup -> legacy Storage -> other backup evidence
#37 close only if build asset gating is actually integrated/verified as scoped
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
POKÉMON VISUAL PRESENT                    PASS / or explicit packaged-resource diagnosis
NO RELEASE ON INSTALLED SOURCE            PASS
NO CREATE ON INSTALLED SOURCE             PASS
NO UNSAFE MOVE/MULTI ON INSTALLED SOURCE  PASS
EDIT CANNOT WRITE INSTALLED SOURCE        PASS
BACKUP/STAGED STATE CLEARLY LABELED        PASS
LEGACY STORAGE CLEARLY APP-OWNED           PASS
OLD PLA SAVE                              OPEN OR GRACEFUL ERROR / NO CRASH
OLED BLACK / DARK / LIGHT                 PASS
BOTTOM BUTTON HINTS READABLE               PASS
THEME PERSISTENCE                         PASS
PARTY / BOXES / STORAGE                   PASS
HOME / SLEEP / RESUME                     PASS
CONTROLLER RECONNECT                      PASS
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
full cry implementation
Pokémon realtime 3D/model work
full controller redesign
new large UI redesign
```

---

# Stop condition

Stop only after:

```text
#23 audit/UI contract complete enough for current alpha
unsafe/ambiguous installed-source mutation UI blocked
backup/staged vs installed source made unambiguous
#24 defensive crash hardening done as far as evidence allows
Session 2.5 shell/analog work preserved
host tests PASS
sanitizers PASS
git diff check PASS
native build PASS
device asset preflight PASS
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

Do not claim Left Stick, PLA crash, Summary visual, or installed-save safety behavior is device-fixed until the user runs that exact replacement binary.