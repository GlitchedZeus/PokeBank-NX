# PokeBank NX — Coding Session Runbook

Last updated: 2026-09-01

Use this at the start and end of every coding-agent session.

Recovery is complete. Do not repeat the old forensic repository recovery workflow unless the verified GitHub project state is genuinely unavailable.

---

## Source of truth

Repository:

```text
GlitchedZeus/PokeBank-NX
```

Writable remote:

```text
origin
```

PKSE remote:

```text
upstream
```

**Never push PokeBank NX changes to PKSE upstream.**

Development branch:

```text
feature/pokebank-playable
```

Verified safety milestone:

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40
safety: hard-lock live game save writes
```

Completed Action Sheet application milestone:

```text
82a0779a5143cca0690d0c7068946d84ebe9f107
ui: add controller Pokemon action sheet
```

Always read `PROJECT_STATUS.md` for anything newer.

---

## Recovery status

Recovery is complete.

The old requested checkpoint `1932cf0` was not found in available refs/reflogs/stashes/unreachable history/workspace archives/GitHub during the completed recovery session.

The surviving 23-game identity work was recovered and published.

Do **not** restart that investigation just because a later feature session was interrupted.

---

# Critical rule: preserve interrupted-session work

A coding runtime can end before work is committed/pushed. This happened during the 2026-09-01 issue #13 UI/theme session.

If the previous session may contain useful local work, **do not immediately reset/clean/switch over it**.

First inspect:

```bash
pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git remote -v
git log --all --oneline --decorate --graph -30
git reflog -30
git stash list
git worktree list
```

Look for:

```text
uncommitted modifications
untracked project files
local branches not on origin
local commits not pushed
reflog-only commits
stashes
secondary worktrees
```

Do not use these commands over unknown work:

```text
git reset --hard
git clean -fd
force push
destructive rebase
checkout that overwrites changes
```

If local work matters but is not ready to merge, preserve it first with a branch/commit/stash/worktree as appropriate.

Only after local work is accounted for should the session synchronize with GitHub.

---

# Start-of-session procedure

## 1. Inspect first

Use the interrupted-session checks above.

## 2. Fetch safely

```bash
git fetch --all --prune
```

Fetching is fine; destructive reset is not.

## 3. Verify remote state

Confirm:

```text
origin/feature/pokebank-playable exists
upstream points to kiasta/PKSE
latest remote history matches PROJECT_STATUS.md or contains a clearly newer valid milestone
```

If GitHub is newer than the docs, update the docs after verifying what changed.

If local work is newer than GitHub, preserve/integrate it rather than discarding it.

## 4. Read documentation

Always read:

```text
PROJECT_STATUS.md
docs/NEXT_SESSION_PLAN.md
docs/SESSION_RUNBOOK.md
```

Then read the feature-specific contracts.

For UI/control work:

```text
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/ARCHITECTURE.md
docs/SAVE_SAFETY.md
```

For Pokémon/save-engine work:

```text
docs/UPSTREAM_AUDIT.md
docs/PKSM_CORE_INTEGRATION.md
docs/PKHOUSE_REFERENCE.md
docs/GAME_SUPPORT_MATRIX.md
docs/SAVE_SAFETY.md
```

For Vault/Dex/transfer work:

```text
docs/MASTER_VAULT_SPEC.md
docs/TRANSFER_MODEL.md
docs/POKEDEX_SPEC.md
docs/PKHEX_ORACLE.md
```

## 5. Run baseline host verification

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

If the baseline is broken before new work, diagnose/document it before piling changes on top.

## 6. Native integration build

Use the environment recorded in `PROJECT_STATUS.md` and run:

```bash
make -j1
```

When building a test artifact record:

```text
application source full SHA
filename
size
SHA-256
host-test result
sanitizer result
native-build result
device-tested yes/no
```

---

# Current development order

Follow `PROJECT_STATUS.md` if it contains a newer priority.

As of 2026-09-01:

```text
DONE
#2 Action Sheet

CURRENT
#13 HOME-style controls + OLED/Dark/Light UI shell
    - recover interrupted local Session 2 work first
    - finish tests/build/push/artifact

THEN
#8 physical Switch test of exact combined build

THEN — use MAX/deep reasoning
#4 PKSM-Core PK3/Sav3 Gen III integration spike

THEN
#3 Master Vault v1 + Banks
#9 Summary + provenance
#6 RetroArch/read-only Gen I-III adapters
#11 modern Switch adapter validation
#5 PKHeX Oracle
#7 Vault-driven Pokédex
#10 conversion/transfer without live writes
```

Issue #15 tracks artifact automation and can be handled when it does not block the product path.

---

# Controller/action safety rule

The Action Sheet milestone is complete.

Pressing A on a focused Pokémon opens:

```text
View Pokémon
Add to Master Vault
Add to Bank…
Transfer to Game…
Edit
Clone
Make Shiny
Legality & Provenance
Cancel
```

A must never silently mutate from Pokémon focus.

B/Cancel/navigation must remain non-mutating.

Controller/UI refactors must preserve this behavior.

---

# Current UI/control contract

Use `docs/CONTROLS.md` as canonical.

High-level model:

```text
D-pad       precise navigation
Left Stick  navigation / repeat
A           Select/Open
B           Back/Cancel
X           Filter/Search/context
Y           Sort/View/secondary
L/R         nearby box/Pokémon/tab navigation
ZL/ZR       larger jumps/major navigation
+           More/Options; never global Exit
-           Help/Controls/Screen Info
Right Stick optional only
```

Target themes:

```text
OLED Black
Dark
Light
```

Use semantic theme tokens. Do not scatter unrelated `if dark` logic through each screen.

---

# Upstream-first rule

Before writing major Pokémon infrastructure from scratch, check:

```text
PKSE
PKSM-Core
PKHeX
Auto Legality Mod / PKHeX-Plugins
pkHouse
pkDex
PKForge
```

Use `docs/UPSTREAM_AUDIT.md`.

Especially:

- audit PKSM-Core before rebuilding historical Pokémon/save logic;
- use PKHeX as primary technical oracle/reference;
- keep pkHouse as reference/reimplementation, not pasted GPLv2 source;
- keep pkDex as UX/data reference, not Vault ownership authority;
- adapt PKForge architecture concepts rather than copying unrelated app architecture wholesale.

When adopting behavior record:

```text
project
exact revision
file/path
language/license
DIRECT REUSE / ADAPTER / PORT / REFERENCE ONLY
tests/golden vectors
required attribution
```

---

# pkHouse policy

Insektaure encouraged technical reference use and specifically recommended reimplementation rather than pure copy/paste.

Current PokeBank NX policy:

```text
REFERENCE ONLY
```

Do not paste pkHouse GPLv2 source into PokeBank NX.

If an important format behavior is unclear, prepare a focused technical question for the author after checking PKHeX/PKSM-Core/tests first.

---

# Current safety rule

**LIVE INSTALLED-GAME SAVE WRITING REMAINS HARD DISABLED.**

Do not remove/bypass the lock to make a demo look complete.

Safe development targets include:

```text
read-only live source
Master Vault
backup file
staged save copy
exported Pokémon representation
```

Read `docs/SAVE_SAFETY.md` before any mutation/write-path work.

---

# Milestone discipline

For every coherent milestone:

```text
IMPLEMENT
    ↓
HOST TEST
    ↓
SANITIZERS
    ↓
GIT DIFF CHECK
    ↓
BUILD .NRO
    ↓
UPDATE PROJECT_STATUS.md
    ↓
COMMIT APPLICATION SOURCE
    ↓
PUSH
    ↓
VERIFY REMOTE SHA
    ↓
PRESERVE/RECORD ARTIFACT
```

Do not accumulate large unpushed sessions if a smaller safe checkpoint is available.

If a later documentation/status commit is created after the application-source commit, distinguish the two clearly.

---

# Verification vocabulary

Use:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

Do not say `DEVICE TESTED` until the user physically runs the exact recorded binary/hash on Switch hardware.

A local session report is not a verification state.

---

# Build artifact discipline

Temporary coding runtimes disappear.

A testable `.nro` is not safely preserved merely because it existed under `/mnt/data`.

For every hardware-test build:

1. record application source SHA;
2. record filename;
3. record file size;
4. record SHA-256;
5. provide/preserve the actual binary before session end;
6. update `docs/BUILD_RECORD.md`;
7. eventually use issue #15's durable GitHub artifact/prerelease flow.

Do not commit large build outputs to normal source history unless the repository explicitly changes policy.

---

# End-of-session procedure

## 1. Stop at a coherent point

Do not begin a huge subsystem if there is not enough time to reach a safe checkpoint.

## 2. Verify

```bash
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

## 3. Update status

Record:

```text
what changed
what passed
what builds
what is device tested
application source SHA
later docs/status SHA if different
.nro filename/hash/size if built
known blockers
next task
```

## 4. Commit/push/verify

Use meaningful milestone commits.

Verify the exact remote SHA after publishing.

## 5. Preserve user-test artifact

If intended for physical testing, ensure the actual `.nro` is accessible before the workspace disappears.

## 6. Leave next action explicit

`PROJECT_STATUS.md` and `docs/NEXT_SESSION_PLAN.md` should make the next action obvious.

---

# Compact generic prompt

```text
POKEBANK NX — CONTINUE FROM VERIFIED STATE

Open/fetch GlitchedZeus/PokeBank-NX.
PKSE is upstream only; never push PokeBank NX changes there.

Recovery is complete.

Before reset/clean/checkout, inspect git status, local branches, reflog, stash and worktrees for unpushed interrupted-session work.
Preserve anything useful newer than GitHub.

Read PROJECT_STATUS.md, docs/NEXT_SESSION_PLAN.md, docs/SESSION_RUNBOOK.md,
and the feature-specific specs for the current task.

Run host tests/sanitizers and native .nro baseline when appropriate.
Keep live installed-save writing hard disabled.

Continue only the highest-priority unfinished task in PROJECT_STATUS.md.

For every stable milestone:
test -> sanitize -> diff check -> build .nro -> update status -> commit -> push -> verify remote SHA.

If producing a device build, preserve/provide the actual .nro and record source SHA, size and SHA-256.

Do not mark DEVICE TESTED without the user's physical report for that exact binary.
Start working immediately.
```