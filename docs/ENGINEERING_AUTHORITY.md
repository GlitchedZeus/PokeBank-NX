# PokeBank NX — Engineering Authority

This is the small live routing file for engineering work. **GitHub is authoritative.** Do not trust a branch name, PR head, milestone, workflow result, or "current" statement copied from an older document without re-fetching live repository state first.

## Start every engineering session here

1. Re-fetch the live repository and the PR/branch named by the owner or current task.
2. Preserve every newer commit. Never reset/rebase backward, force-push, or discard forward work.
3. Read the current task-specific handoff or remediation matrix after live state is known.
4. Treat dated prompts, session plans, research notes, roadmaps, and historical status snapshots as context—not branch authority.

## Current lane structure

- **MAIN development/integration:** PR #92 / its live head at the time of work. Re-fetch before use.
- **Audit remediation:** draft PR #101 on `fix/full-audit-remediation-20260929`. Keep OPEN / DRAFT / NOT MERGED until owner approval.
- **Forensic evidence:** `audit/full-repository-line-by-line-20260928` frozen at `143c5e5c341d4f85af30e013808a37d6719560fe`. Do not develop on or rewrite this evidence branch.
- Product/UI work may advance in parallel. Do not preemptively merge/cherry-pick it into remediation unless a specific finding requires the overlap.

## Permanent safety boundaries

Original/external source saves remain immutable. Live writes to installed titles and emulator sources remain disabled. Unknown or ambiguous sources fail closed. Cross-game True Move and source injection remain locked. Launch capability never grants save-write permission. Do not begin Gen V or Master Vault persistence as a side effect of unrelated work.

## What other docs mean

- `docs/AUDIT_REMEDIATION_MATRIX.md`: authoritative finding disposition for the current remediation phase.
- `docs/NEXT_SESSION_PLAN.md`: dated planning snapshot; useful only after live GitHub re-fetch.
- `CURRENT_STATUS.md` / `PROJECT_STATUS.md`: descriptive snapshots, not substitutes for live GitHub state.
- `docs/PROJECT_RESOURCE_INDEX.md` / `docs/RESEARCH_CURRENT_INDEX.md`: navigation/reference maps only.
- old `NEXT_CODEX_PROMPT.md`, session prompts/runbooks, dated audits, and research notes: historical/reference material unless the owner explicitly reactivates them.
