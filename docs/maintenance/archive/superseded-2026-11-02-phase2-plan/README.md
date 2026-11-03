# Superseded: November 2 Phase 1/2 branch-cleanup plan (archive)

**Do not run anything in this folder.** These files were built from the November 2, 2026 snapshot of
324 remote branches. The owner later completed the branch cleanup separately: 331 → 88 branches, with all
15 identified unique-history tips restored. The authoritative record is
[`../../BRANCH_CLEANUP_STATE_2026-11-02.md`](../../BRANCH_CLEANUP_STATE_2026-11-02.md), and the verified
pre-prune Git bundle is in the `maintenance/pre-branch-prune-20261102-bbc9dd69` release.

| File | What it was |
| --- | --- |
| `REPO_CLEANUP_AUDIT_2026-11-02.md` | Phase 1 audit report (counts as of the 324-branch snapshot) |
| `BRANCH_ARCHIVE_MANIFEST.md`, `BRANCH_ARCHIVE_MANIFEST_PHASE2.md` | Per-branch plans rendered from the JSON |
| `branch_audit_2026-11-02.json`, `branch_audit_2026-11-02_phase2.json` | Machine-readable plans |
| `branch_audit.py`, `render_branch_manifest.py` | Read-only audit and render tools; they still work on a fresh inventory |
| `prune_branches.sh` | Owner-run deletion script. **Disabled**: it exits immediately with an error |
| `BRANCH_DELETION_LOG.md` | Empty log of that script, never used |

A future cleanup must start from a fresh live inventory and follow `PROJECT_RULES.md` §1. It must never
reuse these plans.
