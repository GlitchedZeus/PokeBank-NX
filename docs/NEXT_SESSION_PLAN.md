# PokeBank NX — Next Session Plan

Last updated: 2026-09-02
Status: ACTIVE EXECUTION PLAN

`PROJECT_STATUS.md` is the authoritative verified-state record. Session 2 is complete; the next step is physical testing of its exact combined binary.

Ready-to-run prompts are now stored permanently in the repository:

```text
docs/PROMPT_SESSION2_RECOVERY.md   ARCHIVE — completed issue #13 recovery instructions
docs/PROMPT_SESSION3_PKSM_CORE.md  MAX — hardware feedback + issue #4 PKSM-Core spike
```

Use those instead of reconstructing long prompts from chat history.

---

# Current position

Completed:

```text
Issue #2 — Controller-first Pokémon Action Sheet
Application source: 82a0779a5143cca0690d0c7068946d84ebe9f107
Host tests: PASS
ASan/UBSan: PASS
Native .nro: BUILDS
Issue state: CLOSED / COMPLETED

Issue #13 — HOME-style controls and OLED/Dark/Light UI shell
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Host tests: PASS (6 suites)
ASan/UBSan: PASS
Native .nro: BUILDS
Issue state: COMPLETED; awaiting physical test under issue #8
```

Device-test artifact already recorded:

```text
PokeBank-NX-ActionSheet-82a0779.nro
9,695,669 bytes
SHA-256:
6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b
```

That binary remains **NOT DEVICE TESTED**.

Current device-test artifact:

```text
PokeBank-NX-UI-Theme-3be4de6.nro
9,707,957 bytes
SHA-256 df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
NOT DEVICE TESTED
```

The older Action Sheet-only binary remains recorded, but the combined Session 2 binary is now the preferred hardware-test build.

---

# Block A — COMPLETED — Session 2 recovery/UI foundation

This section is retained as the historical acceptance checklist. Do not rerun it unless a regression is found.

Use:

```text
docs/PROMPT_SESSION2_RECOVERY.md
```

## Objective

Recover the interrupted local UI/control/theme work if it still exists, finish the smallest coherent issue #13 milestone, and produce a new combined device-test `.nro`.

Do **not** start PKSM-Core in this block.

## First action: preserve local work

Before reset/clean/branch switching, run:

```bash
pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git log --all --oneline --decorate --graph -40
git reflog -40
git stash list
git worktree list
```

The previous coding agent explicitly reported using a preserved Session 2 branch/worktree.

Look for:

- uncommitted Session 2 changes;
- local Session 2 branches/worktrees;
- local commits not pushed to origin;
- relevant reflog entries;
- stashes.

Do not destroy that work by immediately resetting to GitHub.

Only if the Session 2 work is genuinely absent should it be reimplemented from the docs.

## Read before changing code

```text
PROJECT_STATUS.md
docs/SESSION_RUNBOOK.md
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/ARCHITECTURE.md
docs/SAVE_SAFETY.md
docs/BUILD_RECORD.md
GitHub issue #13
```

Issue #2 is completed. Do not redo the Action Sheet unless fixing a regression.

## Reported Session 2 local work

The interrupted session reported:

```text
semantic three-theme foundation
Select Game semantic raised/focused cards
typed/context-aware bottom controller hints
held D-pad / Left-Stick navigation repeat
+ no longer globally exits
contextual + Options / Settings / compatibility / More
- read-only Help / Controls
persisted theme cycling
reusable focus/card/modal helpers
Action Sheet integration through shared primitives
```

Treat this as a recovery target, **not verified GitHub functionality**.

## Required control model

```text
D-pad       precise navigation
Left Stick  navigation / held scrolling
A           Select / Open; Pokémon -> Action Sheet
B           Back / Cancel
X           Filter / Search / context
Y           Sort / View / secondary action
L / R       previous/next box, Pokémon, or nearby tab
ZL / ZR     larger jumps / major navigation
+           contextual More / Options; never global Exit
-           Help / Controls / Screen Info
Right Stick optional fast scroll / secondary pane only
```

On Pokémon Summary, reserve/prefer:

```text
+ -> Compatible Games / Transfer Compatibility
```

## Required theme foundation

```text
OLED Black
Dark
Light
```

Use semantic tokens such as:

```text
background
surface
surfaceRaised
surfaceSelected
textPrimary
textSecondary
textMuted
accentPrimary
accentSecondary
focusBorder
divider
success
warning
error
info
```

Theme choice should persist. Invalid persisted values must fall back safely.

Light is not simply an inverted Dark theme.

## Required host regressions

Add practical tests where logic can be isolated for:

```text
exact theme enumeration
safe persisted-theme round trip
invalid persisted-theme fallback
required semantic palette roles
+ maps to contextual More/Options, not Exit
- maps to Help/Screen Info
held navigation repeat behavior
button-hint model exposes only active bindings
Action Sheet Session 1 behavior remains intact
Action Sheet open/close remains non-mutating
live-write hard lock remains enforced
```

Avoid brittle pixel-coordinate tests unless there is a strong reason.

## Verification gate

Run:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Fix regressions instead of deleting the new architecture merely to make the build pass.

## End of Block A

Required result:

```text
HOME-style controls/theme foundation: IMPLEMENTED
host tests: PASS
sanitizers: PASS
git diff --check: PASS
native .nro: BUILDS
PROJECT_STATUS updated
application-source commit pushed
remote SHA verified
new .nro preserved/provided
```

Record:

```text
filename
exact application source SHA
size
SHA-256
```

Do not say `DEVICE TESTED`.

Stop after the coherent UI/control milestone is saved.

---

# Between Block A and Block B — physical Switch test

Use `docs/DEVICE_TEST_CHECKLIST.md` and issue #8.

Test the **new combined build**, not an unidentified older binary.

Primary checks:

```text
launch through hbmenu
Select Game focus/navigation
D-pad navigation
Left Stick navigation + held repeat
A opens Action Sheet
B/Cancel safely close Action Sheet
Action Sheet still has correct order
unfinished Action Sheet actions stay safe
L/R behavior
ZL/ZR behavior where wired
X/Y contextual behavior where wired
+ opens More/Options and does NOT unexpectedly exit
- opens read-only Help / Controls
theme selection works
OLED Black readability
Dark readability
Light readability
theme preference persists after restart
bottom button hints match actual behavior
no source Pokémon moves/changes from browsing
no live-save write path becomes exposed
```

Record exact source SHA and `.nro` SHA-256 with the hardware report.

Only behaviors actually tested on that exact build may become `DEVICE TESTED`.

---

# Block B — HIGH if hardware fixes are needed

If the combined device build has obvious UI/input/device-only problems:

1. reproduce from the exact report;
2. fix the smallest coherent regression set;
3. rerun host tests/sanitizers;
4. native build;
5. update status;
6. commit/push/verify;
7. provide a replacement `.nro` if another physical pass is needed.

Do not ignore a crash or dangerous control problem in order to start deeper engine work.

If hardware testing is clean enough, skip straight to Block C.

---

# Block C — MAX — PKSM-Core Gen III spike

Use:

```text
docs/PROMPT_SESSION3_PKSM_CORE.md
```

Use **MAX/deep reasoning** for this block.

GitHub issue:

```text
#4 — Audit and spike PKSM-Core Gen III integration
```

Read:

```text
PROJECT_STATUS.md
docs/UPSTREAM_AUDIT.md
docs/PKSM_CORE_INTEGRATION.md
docs/ARCHITECTURE.md
docs/SAVE_SAFETY.md
docs/GAME_SUPPORT_MATRIX.md
GitHub issue #17
```

## Narrow first target

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
box/party extraction
active save-slot/sector behavior
PK3 encrypt/decrypt/checksum behavior
untouched round-trip strategy
adapter/dependency cost
```

Do not rewrite Gen III from scratch before testing PKSM-Core integration viability.

Possible outcome classifications:

```text
DIRECT REUSE
ADAPTER / WRAPPER
SELECTIVE PORT
REFERENCE ONLY
```

Base the choice on license compatibility, build/dependency cost, correctness, maintenance, testability, and overlap with existing PKSE code.

No live writes during this spike.

Issue #17 tracks the reproducible fixture corpus. Build only the smallest useful fixtures needed for the first spike; do not let fixture perfection block the integration decision.

---

# After PKSM-Core spike

Recommended product order:

```text
#3  Master Vault v1 + named Banks
#9  Professional Summary + provenance
#6  RetroArch discovery + read-only Gen I-III adapters
#11 Modern Switch adapter validation
#5  PKHeX Oracle
#7  Vault-driven Pokédex / Living Dex
#10 Conversion / transfer without live writes
```

Supporting issues:

```text
#15 persistent .nro artifact workflow
#16 final branding/startup/NRO metadata
#17 reproducible golden fixture corpus
```

These should be integrated when useful without derailing the current product milestone.

---

# Do not do prematurely

Avoid starting all of these at once:

```text
full Master Vault
full Pokédex
full legality engine
full event archive
all generations at once
live installed-save writing
large visual redesign before finishing the interrupted UI milestone
```

---

# Five-PM rule

At the next coding block, first process the physical report for the exact Session 2 artifact. If no blocking regression exists, use the Session 3 prompt.

Start with:

```text
docs/PROMPT_SESSION3_PKSM_CORE.md
```

and recover/finish issue #13 first.
