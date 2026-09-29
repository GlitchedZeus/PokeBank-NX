# PokeBank NX — Coding Session Runbook

Last updated: **2026-09-29**

## Source of truth

```text
Repository: GlitchedZeus/PokeBank-NX
Writable remote: origin
Reference upstream: kiasta/PKSE
Current branch / PR: read CURRENT_STATUS.md, then re-fetch GitHub
```

Never hard-code an old development branch as authority. As of this refresh the active MAIN lane is PR #92, but live GitHub always wins.

Never push PokeBank NX changes to PKSE upstream.

Authority order:

1. `CURRENT_STATUS.md`
2. `docs/CODEX_SESSION.md`
3. `docs/NEXT_CODEX_PROMPT.md`
4. `docs/NEXT_SESSION_PLAN.md`

## Start of session

1. Re-fetch the active PR/branch.
2. Preserve every newer commit.
3. Never reset/rebase backward or force-push over newer work.
4. Check exact-head Actions before calling a candidate green.
5. Read only the subsystem docs needed for the current task.

## Verification language

Use:

- **IMPLEMENTED**
- **HOST TESTED**
- **NATIVE BUILDS**
- **DEVICE TESTED / DEVICE ACCEPTED**

Device acceptance applies only to the exact physically tested NRO/application SHA.

## Safety rules

- source saves immutable;
- live installed/emulator writes disabled;
- staged app-owned editing allowed;
- ambiguous sources/content fail closed;
- Launch does not grant write access;
- cross-game True Move remains locked.

## Checkpoints

Push coherent checkpoints to the **live active origin branch**, not to a historical branch name.

Keep README human-facing. Put branch/CI/audit detail in status and engineering docs.

## Current boundary

The active integration target is one combined Gen I–IV + Product UI Actions-built NRO. Full app-wide touch parity begins only after that candidate is physically accepted.
