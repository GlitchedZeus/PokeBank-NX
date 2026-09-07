# PokeBank NX — Coding Session Runbook

Last updated: 2026-09-07

Use this at the start and end of every coding-agent session.

## Source of truth

```text
Repository:         GlitchedZeus/PokeBank-NX
Writable remote:    origin
PKSE remote:        upstream
Development branch: feature/pokebank-playable
```

**Never push PokeBank NX changes to PKSE upstream.**

Current-state files:

```text
CURRENT_STATUS.md              short authoritative engineering handoff
PROJECT_STATUS.md              broader current project summary
docs/NEXT_SESSION_PLAN.md      current immediate plan
docs/NEXT_CODEX_PROMPT.md      full next-session coding instructions
```

The branch may contain documentation commits newer than the engineering/application source being discussed. Never confuse branch HEAD with the source SHA used for a device artifact or engine checkpoint.

---

## One-line session launcher

The user should normally only need to send:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

Do not ask the user to paste the full prompt again unless repository access genuinely fails.

---

## Start-of-session recovery check

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
git diff
git diff --cached
git submodule status --recursive
```

Look for uncommitted modifications, untracked files, local-only commits, recovery branches, stashes and secondary worktrees.

Do not blindly run:

```text
git reset --hard
git clean -fd
git restore .
force push
destructive rebase
checkout that overwrites unknown changes
```

Preserve legitimate newer work first.

---

## Baseline verification

For ordinary engine/source work:

```bash
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
```

Run the native build when the changed code participates in the Switch application:

```bash
make -j1
```

Do not rebuild/package the large sprite/device artifact unless a user-visible runtime change actually requires physical testing.

---

## Checkpoint discipline

Do not hold an entire long session only in the workspace.

As soon as a coherent milestone passes its relevant tests:

1. commit it;
2. push it to `origin/feature/pokebank-playable`;
3. verify GitHub can fetch the exact SHA;
4. verify CI when applicable;
5. only then continue to the next coherent subtask.

Prefer two clean pushed checkpoints over one giant uncommitted session that can be lost to a timeout.

If ordinary HTTPS Git credentials are unavailable, use the authenticated GitHub integration before the session ends.

---

## Verification vocabulary

Use these terms precisely:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

`DEVICE TESTED` means a human physically ran the exact recorded `.nro`/SHA-256. Never inherit device-tested status from an older binary after application source changes.

---

## Safety rules

- Live installed-game save writing remains hard disabled unless a specific future adapter passes its own safety gate.
- Read-only parser/source milestones must not auto-repair, resign or overwrite source files.
- Explicit backup/staged workspaces remain distinct from installed sources.
- Legacy inherited Storage is app-owned compatibility storage, not Master Vault.
- Do not weaken the low-level live-write lock to make an unrelated feature easier.
- Never push custom code upstream.

---

## Scope discipline

Before coding, read `docs/NEXT_CODEX_PROMPT.md` and obey its explicit `DO NOT START` boundaries.

Do not turn a narrow implementation milestone into a broad research or UI session. Existing research/reference docs should answer most architecture questions; open external repositories only for a concrete blocker.

---

## End-of-session procedure

Before stopping:

```bash
git status --short
git diff --check
```

Run the relevant host/sanitizer/native tests, then commit and push coherent work.

Update only the current state/issues that materially changed:

```text
CURRENT_STATUS.md
PROJECT_STATUS.md
docs/NEXT_SESSION_PLAN.md
docs/NEXT_CODEX_PROMPT.md when the next task changes
relevant GitHub issue(s)
```

End report should include exact implementation SHA(s), tests, native result, CI result, remaining blocker and exact next coding task.
