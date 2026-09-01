# PokeBank NX — Coding Session Runbook

Last updated: 2026-09-01

Use this at the start and end of future coding-agent sessions. Recovery is complete; do not repeat the old forensic recovery workflow unless the verified GitHub state is genuinely missing.

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

Verified remote safety milestone:

```text
c618bd5
safety: hard-lock live game save writes
```

Recovery branch:

```text
recovery/interrupted-2026-09-01
```

Verified recovery checkpoint:

```text
3101fc0
```

---

## Recovery status

Recovery is complete.

The old requested checkpoint `1932cf0` was not found in the available object database, refs, reflogs, stashes, unreachable objects, workspace archives, or GitHub during the completed recovery session.

The surviving interrupted 23-game identity work was preserved and published.

Do **not** spend a new session re-running the entire recovery investigation unless the actual GitHub branch/commits above cannot be fetched.

---

# Start-of-session procedure

## 1. Inspect repository state

```bash
pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git remote -v
git log --all --oneline --decorate --graph -30
```

## 2. Fetch without destroying local work

```bash
git fetch --all --prune
```

Before switching/resetting anything, preserve any useful uncommitted/newer local work.

Do not:

```text
reset --hard over unknown work
clean -fd
force push
rebase away recoverable commits
discard untracked project files
```

## 3. Verify known remote state

Confirm:

```text
feature/pokebank-playable exists
c618bd5 exists in remote history
```

If there is a newer valid remote feature commit, use the newer state and update `PROJECT_STATUS.md` accordingly.

## 4. Read project documentation

Read before major implementation:

```text
PROJECT_STATUS.md
README.md
docs/UPSTREAM_AUDIT.md
docs/MASTER_VAULT_SPEC.md
docs/SAVE_SAFETY.md
docs/TRANSFER_MODEL.md
docs/POKEDEX_SPEC.md
docs/PKHEX_ORACLE.md
docs/DEVICE_TEST_CHECKLIST.md
```

## 5. Run baseline host verification

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

If baseline tests fail before your change, diagnose/document the baseline failure before piling new work on top.

## 6. Build native `.nro`

Use the environment recorded in `PROJECT_STATUS.md`, then:

```bash
make -j1
```

Record:

```text
output filename
size
SHA-256
source commit
```

---

# Current development order

Unless `PROJECT_STATUS.md` records a newer priority, proceed in this order:

```text
1. A-button Pokémon action sheet
2. upstream reuse audit/integration spike
3. Master Vault v1
4. named banks
5. professional summary + provenance
6. RetroArch/source discovery
7. read-only Gen I-III adapters
8. PKHeX Oracle + golden tests
9. Vault-driven Pokédex
10. conversion/transfer engine
11. staged save validation
12. live writes only after safety gates
```

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

Particularly:

- audit PKSM-Core before rebuilding Gen I–VIII Pokémon/save logic
- use PKHeX as the primary technical oracle/reference
- keep pkHouse as reference/reimplementation, not pasted source
- keep pkDex as Pokédex UX/data reference, not ownership authority
- adapt PKForge Vault/transaction concepts to native C++

---

# pkHouse policy

Insektaure encouraged use of pkHouse as a reference and recommended reimplementation rather than pure copy/paste.

Therefore:

```text
REFERENCE ONLY
```

Do not paste pkHouse GPLv2 source into PokeBank NX.

When pkHouse informs an implementation, record:

```text
pkHouse commit
source file/path
behavior/fact being studied
independent cross-check if available
PokeBank implementation/tests
```

If an implementation detail is ambiguous and important, prepare a concise technical question for Insektaure.

---

# Current safety rule

**LIVE GAME SAVE WRITING REMAINS HARD DISABLED.**

Do not remove/bypass the safety lock merely to make a feature demonstration work.

A useful feature can operate against:

```text
read-only live source
Vault
backup file
staged save
exported Pokémon file
```

before live write support exists.

Read `docs/SAVE_SAFETY.md` before any save mutation work.

---

# A-button rule

Pressing A on a Pokémon must open a deliberate action sheet.

Target:

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

A single A press must never silently move, delete, edit, clone, or write a Pokémon.

Unavailable options may be disabled or return a clear not-yet-supported result.

---

# Milestone discipline

For every meaningful coherent milestone:

```text
IMPLEMENT
   ↓
HOST TEST
   ↓
SANITIZERS
   ↓
BUILD .NRO
   ↓
UPDATE PROJECT_STATUS.md
   ↓
COMMIT
   ↓
PUSH
   ↓
VERIFY REMOTE SHA
```

Do not continue accumulating major changes without a remote checkpoint.

---

# Verification vocabulary

Use only:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

Do not say `DEVICE TESTED` until the user physically runs the exact build/commit on a Switch.

---

# Build artifact discipline

Temporary coding runtimes can disappear. A successfully built `.nro` is not safely preserved merely because it existed under `/mnt/data`.

For testable milestones:

1. record the exact source commit
2. record `.nro` size and SHA-256
3. provide/export the `.nro` to the user during the session
4. when release-ready enough, publish a GitHub prerelease artifact

Do not commit large build outputs to normal source history unless the repository deliberately adopts that policy.

---

# End-of-session procedure

Before the session ends:

## 1. Stop at a coherent point

Do not begin a huge subsystem if there is not enough time to reach a safe checkpoint.

## 2. Run verification

```bash
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
make -j1

git diff --check
```

## 3. Update status

Record:

```text
what changed
what passed
what builds
what is device tested
exact commit
.nro hash/size if built
known blockers
next task
```

## 4. Commit and push

Use a meaningful milestone commit.

Verify the exact remote commit SHA after publishing.

## 5. Preserve user-test artifact

If the build is intended for physical testing, ensure the user actually receives an accessible `.nro` artifact before the temporary workspace disappears.

## 6. Leave the next task explicit

`PROJECT_STATUS.md` must end the session with a concrete highest-priority next task.

---

# Compact prompt for a fresh coding session

```text
POKEBANK NX — CONTINUE FROM VERIFIED GITHUB STATE

Open/fetch GlitchedZeus/PokeBank-NX.
PKSE is upstream only; never push PokeBank NX changes there.

Recovery is already complete. Do not repeat the old recovery procedure unless the verified GitHub state is genuinely missing.

Read, in order:
- PROJECT_STATUS.md
- docs/SESSION_RUNBOOK.md
- docs/UPSTREAM_AUDIT.md
- the feature-specific spec(s) relevant to the current task

Verify feature/pokebank-playable and preserve anything newer than the last recorded milestone.
Run host tests, sanitizers, and a native .nro build before starting new work.

Before rewriting major Pokémon/save/legality/conversion infrastructure, audit the tracked upstream/reference projects first.

Keep live game-save writing hard disabled.

Continue the highest-priority unfinished task in PROJECT_STATUS.md.

For every stable milestone:
test → sanitize → build .nro → update PROJECT_STATUS.md → commit → push → verify remote SHA.

If producing a device-test build, preserve/provide the actual .nro artifact and record its SHA-256 before the session ends.

Start working immediately.
```
