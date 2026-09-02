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

# Critical continuity state

The previous Session 2.5 visible-shell / analog source exists at:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

That source was host-tested/CI-green and reported to build natively, but it has **NOT been physically tested**.

Subsequent commits on `feature/pokebank-playable` include documentation/roadmap updates. Do not confuse branch HEAD with application-source identity.

A longer hardware torture test of the **older physically tested** build exposed new blockers that must be handled before the second `.nro` handoff.

Exact physically tested old build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
```

Read the new permanent report:

```text
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
```

---

# Read first

```text
PROJECT_STATUS.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/NEXT_SESSION_PLAN.md
docs/SESSION_RUNBOOK.md
docs/SAVE_SAFETY.md
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/BUILD_RECORD.md
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

Inspect GitHub issues:

```text
#13  visible PokeBank NX shell
#16  branding/startup/NRO metadata
#19  Left Stick navigation
#23  inherited mutation UI safety audit — BLOCKER
#24  old/malformed Legends Arceus save crash — BLOCKER
#25  Pokémon visual/model Summary idea — later, do not implement now
#26  controller normalization — record/handle only what is required for safety/current milestone
#27  legacy Storage vs Master Vault — classify now, redesign later
```

Issue #20 true Move is future work. **DO NOT implement true Move now.**

---

# Preserve interrupted work first

Before destructive operations:

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

Do not reset/clean away legitimate local work.

Fetch safely and verify the remote feature branch.

---

# New physical findings — treat as source-of-truth evidence

## 1. Left Stick failure clarified

On exact device-tested source `3be4de6b...`:

```text
single tap Up       FAIL — no input/action
single tap Down     FAIL — no input/action
single tap Left     FAIL — no input/action
single tap Right    FAIL — no input/action
hold Up             FAIL — no input/action
hold Down           FAIL — no input/action
hold Left           FAIL — no input/action
hold Right          FAIL — no input/action
diagonal             FAIL — no input/action
```

Held D-pad Up/Down/Left/Right all pass.

The `361c6f55...` source reportedly fixes this with real libnx stick-position handling. Preserve that fix unless inspection proves a regression.

## 2. Reproducible Legends Arceus crash

Hardware result:

```text
5-minute idle                 PASS
10-minute normal browsing     PASS
5 relaunches                  PASS
one older PLA save            REPRODUCIBLE CRASH
another PLA save              opens more successfully
```

Do not assume the old save is corrupt. Treat it as unsupported/old/malformed/valid-with-parser-bug until proven otherwise.

## 3. Inherited mutation controls are physically reachable

On the old tested build:

```text
X on Pokémon        can expose Release
Y in Boxes          Menu / Move / Multi
Y on Pokémon        can grab/move Pokémon
A on empty slot     can expose Create Pokémon
Action Sheet Edit   opens editable Pokémon view
```

The tester changed an IV, applied/saved it in the app flow, reopened it, and reverted it successfully.

The tester also moved an Arbok from a Legends Z-A context into inherited/app Storage and observed it persisted there.

IMPORTANT EVIDENCE BOUNDARY:

The tester did NOT complete the final external check of launching the original Pokémon game and proving whether the installed live title save itself changed.

Therefore:

```text
LIVE INSTALLED SAVE WRITE: NOT PROVEN
USER-REACHABLE MUTATION UI: PROVEN
APP STORAGE PERSISTENCE: PROVEN
```

Do not falsely report a confirmed live-save write regression, but do not hand over another device build without tracing these paths.

---

# BLOCK A — issue #23 — source-level mutation safety audit

This is the highest priority.

Trace the exact source code behind:

```text
Release
Create Pokémon
Move / Multi box manipulation
Action Sheet Edit
editable Summary save/apply
unsaved-changes prompts
legacy/inherited Storage move/import
any Save / Commit / Restore / Inject path reachable from these screens
```

For every path, trace it to its final persistence target and classify it as exactly one of:

```text
LIVE INSTALLED SAVE WRITE
BACKUP/STAGED SAVE WRITE
APP-OWNED STORAGE WRITE
IN-MEMORY ONLY
DISABLED / UNREACHABLE
UNKNOWN — NEEDS BLOCKING
```

Do not classify based only on UI text. Follow the call chain to filesystem/save APIs.

## Required current-alpha behavior

For Pokémon loaded from an installed game source while the hard live-write policy is active:

- browsing/focus stays read-only;
- Release is not reachable;
- Create Pokémon is not reachable;
- direct Move/Multi mutation is not reachable if it can affect persistent source state;
- Edit must NOT mutate the installed source;
- if Edit cannot safely operate on a separate app/Vault copy yet, return explicit `Not yet supported` for installed-game sources;
- there must be no user-facing Save Changes action that can write the installed title save;
- no controller shortcut may bypass the Action Sheet/safety model;
- Action Sheet open/navigation/B/Cancel remain zero-mutation.

If legacy/app Storage is proven isolated and app-owned, it may remain writable. Clearly distinguish it from installed game saves and future Master Vault. Do not silently remove a Pokémon from an installed game while live writes are disabled.

## Safety tests

Add/extend host regression coverage where practical for:

```text
installed-game source → Release blocked
installed-game source → Create blocked
installed-game source → direct box move mutation blocked where persistence-capable
installed-game source → Edit cannot commit live source
B/Cancel/navigation zero mutation
legacy Storage writes stay isolated from live installed saves
```

Do not remove the low-level hard lock. UI blocking is defense-in-depth, not a replacement.

---

# BLOCK B — issue #24 — Legends Arceus crash hardening

Inspect the inherited PLA path for:

```text
file/container size assumptions
block lookup assumptions
SCBlock / block map assumptions
null/optional accesses
out-of-range reads
version/revision handling
truncated/malformed input handling
unsupported save revisions
```

Goal:

```text
valid supported PLA save → opens
old/unsupported/malformed save → useful error and safe return
never crash merely because a save cannot be parsed
never auto-repair/write the failing save
```

If the exact old user save is not available in the coding runtime:

- do NOT claim the hardware crash is fixed;
- make only source-grounded defensive improvements;
- add synthetic malformed/truncated/bounds tests where possible;
- keep issue #24 OPEN pending physical retest with the exact old save.

Do not ask for or commit private personal saves to the repository. If a future local-only fixture is needed, use the fixture/privacy rules from issue #17.

---

# BLOCK C — preserve Session 2.5 visible shell + analog fix

After safety/crash changes, preserve the useful work from `361c6f55...`:

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
OLED Black / Dark / Light themes + persistence
```

Do not redo the UI from scratch.

Do not start issue #25 Pokémon renders/models in this session.

Do not fully redesign issue #26 controller semantics unless needed to remove an unsafe shortcut. Record remaining normalization work for later.

---

# Application-source checkpoint rule

Because this session is expected to modify application source for #23 and/or #24, the second-device build should likely have a **NEW application source SHA**.

Do NOT label a modified build as `361c6f55`.

Required order:

```text
finish coherent source changes
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

If inspection proves no application-source changes are required, explain exactly why before reusing `361c6f55`. Do not assume this outcome.

---

# Verification

Run at minimum:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make clean
make -j1
```

Use the repository's documented equivalents if the environment requires them.

Required result before device handoff:

```text
HOST TESTS PASS
ASan/UBSan PASS
git diff --check PASS
NATIVE .NRO BUILDS
LIVE-WRITE HARD LOCK INTACT
INHERITED MUTATION PATHS CLASSIFIED
UNSAFE INSTALLED-SOURCE UI MUTATIONS BLOCKED
PLA MALFORMED/UNSUPPORTED PATH HARDENED AS FAR AS SOURCE/TESTS SUPPORT
```

---

# Replacement artifact

Provide the actual `.nro` for physical testing.

Record:

```text
Application source full SHA:
Application source commit message:
Embedded abbreviated SHA:
Artifact filename:
Artifact size:
Artifact SHA-256:
Host tests:
Sanitizers:
Native build:
Device tested: NO
```

Use a filename that identifies the new source, for example:

```text
PokeBank-NX-Second-Device-<shortsha>.nro
```

If direct `.nro` delivery is fragile, additionally provide a ZIP with the `.nro` plus a tiny manifest.

Do not leave the only artifact inside a temporary runtime.

---

# Update project state

Update as appropriate:

```text
PROJECT_STATUS.md
README.md
docs/BUILD_RECORD.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
```

Update issues:

```text
#13 keep OPEN until visual hardware acceptance
#16 keep OPEN for remaining startup/icon/NACP polish
#19 keep OPEN until physical Left Stick acceptance
#23 keep OPEN until safety path physical verification is complete
#24 keep OPEN until exact old PLA save physically retests successfully
#26/#27 record findings but do not derail scope
```

The README freshness rule from `docs/SESSION_RUNBOOK.md` applies.

Do not rewrite historical hardware evidence. The first exact device build remains `3be4de6b...`.

---

# Second physical test acceptance matrix

The replacement artifact should be prepared for the user to test:

```text
BOOT                                      PASS
D-PAD                                     PASS
LEFT STICK SINGLE TAP                     PASS
LEFT STICK HELD REPEAT                    PASS
LEFT STICK DIAGONAL STABILITY             PASS
A/B ACTION SHEET                          PASS
NO RELEASE ON INSTALLED SOURCE            PASS
NO CREATE ON INSTALLED SOURCE             PASS
NO UNSAFE MOVE/MULTI ON INSTALLED SOURCE  PASS
EDIT CANNOT WRITE INSTALLED SOURCE        PASS
OLD PLA SAVE                              NO CRASH / GRACEFUL ERROR OR OPEN
OLED BLACK / DARK / LIGHT                 PASS
THEME PERSISTENCE                         PASS
VISIBLE POKEBANK NX                       PASS
NO OBVIOUS PKSE TOP-LEVEL BRANDING        PASS
PARTY / BOXES / STORAGE                   PASS
NO NEW CRASHES                            PASS
```

Only the user's physical test can promote these to `DEVICE TESTED`.

---

# DO NOT START

Do not begin:

```text
PKSM-Core issue #4
Master Vault issue #3
true Move issue #20
live installed-game save writing
Pokémon 3D/model work issue #25
full controller redesign issue #26
```

This session is a **safety/crash/second-device-build blocker session**.

---

# Stop condition

Stop when:

```text
#23 source audit complete enough for current alpha
unsafe installed-source mutation UI blocked
#24 defensive crash hardening completed as far as evidence allows
Session 2.5 shell/analog work preserved
host tests PASS
sanitizers PASS
git diff check PASS
native build PASS
new application source committed/pushed/verified if changed
exact replacement .nro built from exact source
artifact size + SHA-256 recorded
project docs/issues updated
actual artifact provided
```

Then report:

```text
READY FOR SECOND DEVICE TEST
```

Do not claim the Left Stick, PLA crash, or live-save safety behavior is device-fixed until the user runs that exact replacement binary.
