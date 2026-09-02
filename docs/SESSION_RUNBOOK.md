# PokeBank NX — Coding Session Runbook

Last updated: 2026-09-02

Use this at the start and end of every coding-agent session.

Recovery is complete. Do not repeat the old forensic repository recovery workflow unless verified project state is genuinely unavailable.

---

## Source of truth

```text
Repository: GlitchedZeus/PokeBank-NX
Writable remote: origin
PKSE remote: upstream
Development branch: feature/pokebank-playable
```

**Never push PokeBank NX changes to PKSE upstream.**

Always read `PROJECT_STATUS.md` for the newest verified state and exact application-source checkpoint.

The branch may contain documentation commits newer than the application source used to build a device artifact. Never confuse branch head with artifact source identity.

---

## Preserve interrupted-session work

Before reset/clean/branch switching over a workspace that may contain useful local work, inspect:

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
local commits/branches not pushed
reflog-only commits
stashes
secondary worktrees
```

Do not blindly run:

```text
git reset --hard
git clean -fd
force push
destructive rebase
checkout that overwrites unknown changes
```

Preserve legitimate newer work first.

---

# Start-of-session procedure

## 1. Inspect and fetch safely

```bash
git fetch --all --prune
```

Confirm `origin/feature/pokebank-playable` and `upstream` identities are correct.

## 2. Read current state

Always read:

```text
PROJECT_STATUS.md
docs/NEXT_SESSION_PLAN.md
docs/SESSION_RUNBOOK.md
docs/BUILD_RECORD.md
```

Then read the current task's specs/prompts.

### UI/control

```text
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/SAVE_SAFETY.md
```

### Save/format engine

```text
docs/UPSTREAM_AUDIT.md
docs/PKSM_CORE_INTEGRATION.md
docs/GAME_SUPPORT_MATRIX.md
docs/SAVE_SAFETY.md
```

### Vault/transfer/Dex

```text
docs/MASTER_VAULT_SPEC.md
docs/TRANSFER_MODEL.md
docs/POKEDEX_SPEC.md
docs/PKHEX_ORACLE.md
```

### Cross-cutting NRO quality

```text
docs/NRO_QUALITY_ROADMAP.md
```

## 3. Baseline verification

When appropriate:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
```

Native integration:

```bash
make -j1
```

Diagnose a broken baseline before piling unrelated changes on top.

---

# Current execution order

Follow `PROJECT_STATUS.md` if newer.

Current sequence:

```text
NOW
finish/package exact Session 2.5 application source 361c6f55
        ↓
second physical Switch test
        ↓
fix blocking device regression if needed
        ↓
MAX: #4 PKSM-Core PK3/Sav3
        ↓
#3 Master Vault + Banks
        ↓
#9 Summary/provenance
#6 Retro adapters
#11 modern adapter validation
#5 PKHeX Oracle
#7 Vault-driven Pokedex
#10 conversion/staging
        ↓
per-adapter safe-write work
        ↓
#20 true Move semantics
```

Supporting work:

```text
#15 artifact automation
#16 startup/branding/icon/NACP
#17 golden fixtures
#21 NRO diagnostics/reliability/performance/QoL
```

Do not let supporting polish derail the current core milestone.

---

# Controller/action safety rule

A on a focused Pokémon opens a deliberate Action Sheet. It must never silently mutate from grid focus.

B/Cancel/navigation remain non-mutating.

Controller/UI refactors must preserve the live-write hard lock.

---

# Transfer semantics

Canonical contract: `docs/TRANSFER_MODEL.md`.

```text
COPY  = source remains active; destination is created
MOVE  = real relocation after destination verification
CLONE = deliberate duplicate with clone provenance
```

Current installed-game saves remain read-only. True Move is future work under #20 and must not be enabled early.

Archival/provenance history is not the same as an active playable duplicate.

---

# Upstream-first rule

Before writing major Pokémon infrastructure from scratch, check the pinned reference stack in `docs/UPSTREAM_AUDIT.md`:

```text
PKSE
PKSM-Core
PKHeX
PKHeX-Plugins / Auto Legality
pkHouse
pkDex
PKForge
```

Record, where applicable:

```text
project
revision
file/path
language/license
DIRECT REUSE / ADAPTER / PORT / REFERENCE ONLY
tests/golden vectors
attribution requirements
```

pkHouse/pkDex remain reference/reimplementation sources under current policy; do not blindly paste incompatible licensed source.

---

# Safety rule

**LIVE INSTALLED-GAME SAVE WRITING REMAINS HARD DISABLED.**

Safe development targets include:

```text
read-only live source
Master Vault
backup file
staged save copy
exported Pokemon representation
```

A future write capability is granted **per adapter** after `docs/SAVE_SAFETY.md` gates pass. One safe game does not unlock all games.

---

# Milestone discipline

For every coherent application milestone:

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
COMMIT APPLICATION SOURCE
    ↓
PUSH
    ↓
VERIFY REMOTE SOURCE SHA
    ↓
REBUILD/PACKAGE FROM EXACT SOURCE WHEN REQUIRED
    ↓
RECORD/PRESERVE ARTIFACT
    ↓
PHYSICAL TEST
    ↓
RECORD EXACT PASS/FAIL
```

Do not accumulate large unpushed sessions when a smaller safe checkpoint exists.

If documentation/status commits follow an application-source commit, distinguish them explicitly.

---

# Artifact discipline

For every device-test `.nro` record:

```text
application source full SHA
embedded/visible abbreviated SHA when available
filename
size
SHA-256
host-test result
sanitizer result
native-build result
device-tested YES/NO
actual device result when tested
```

Do not claim a pre-commit binary was built from the later commit merely because its source tree looked similar.

Do not leave the only device binary in a temporary runtime.

Issue #15 tracks durable artifact automation.

---

# Documentation and README freshness rule

Whenever a milestone changes any of the following:

```text
current application source
physical device status
major known failure
current execution order
product semantics
supported capability
safety posture
public-facing app identity
new major issue/spec
```

review and update, where affected:

```text
PROJECT_STATUS.md
docs/BUILD_RECORD.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
README.md
relevant issue body/comment
relevant permanent prompt/spec
```

The README must not advertise stale claims such as:

```text
NOT DEVICE TESTED after a physical test exists
old application source as current
closed issue as current work
unimplemented live Move/write support
old branding state after a verified replacement milestone
```

Do **not** churn README wording for every tiny internal refactor. Update it when the public project state or roadmap meaningfully changes.

Historical reports/logs must remain historical rather than being rewritten to pretend later results existed earlier.

---

# NRO quality rule

Use `docs/NRO_QUALITY_ROADMAP.md` as the supporting backlog.

Prefer roughly:

```text
70% core functionality
20% physical testing / regression fixing
10% polish / infrastructure
```

after the second UI/device milestone, adjusting when a reliability blocker justifies more attention.

High-leverage quality items may be pulled into the milestone that naturally owns them (for example Vault recovery with Vault transactions, large-grid virtualization with Vault/Dex, or startup diagnostics with issue #16).

---

# Verification vocabulary

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

For hardware evidence also record:

```text
DEVICE TESTED — PASS
DEVICE TESTED — PARTIAL PASS
DEVICE TESTED — FAIL
```

Never mark `DEVICE TESTED` without a human physical report tied to the exact binary/hash.

---

# End-of-session procedure

1. Stop at a coherent checkpoint.
2. Run relevant host/sanitizer/diff/native verification.
3. Preserve application-source identity before docs-only commits.
4. Push to `origin`, never upstream.
5. Verify remote SHA.
6. Preserve/hash any device artifact.
7. Update status/build/issues.
8. Apply the README freshness rule.
9. Leave one explicit next action/prompt.

If the session is interrupted before all steps complete, report exactly which stage was reached. Do not invent missing artifact hashes or verification states.

---

# Compact generic continuation prompt

```text
POKEBANK NX — CONTINUE FROM VERIFIED STATE

Open/fetch GlitchedZeus/PokeBank-NX.
PKSE is upstream only; never push PokeBank NX changes there.

Before destructive git commands, inspect status/branches/reflog/stash/worktrees for interrupted work.

Read PROJECT_STATUS.md, docs/NEXT_SESSION_PLAN.md, docs/SESSION_RUNBOOK.md, docs/BUILD_RECORD.md, and the current task specs.

Keep live installed-save writing hard disabled unless the current task names an explicitly approved adapter/write gate.

Continue only the highest-priority unfinished milestone.

For stable application milestones:
test -> sanitize -> diff check -> native build -> commit application source -> push -> verify source SHA -> preserve/hash artifact -> physical test -> record result.

Keep README/status/roadmap accurate when public project state changes.

Do not mark DEVICE TESTED without the user's physical report for that exact binary.
Start working immediately.
```