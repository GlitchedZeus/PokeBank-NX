# PokeBank NX — Recovery Contract

Last updated: **2026-09-29**

Recovery means restoring a usable workspace from the **live authoritative GitHub state**, not returning to an old checkpoint.

## Rule

1. Read `CURRENT_STATUS.md`.
2. Re-fetch the live active PR/branch.
3. Preserve newer commits.
4. Use tracked recovery tooling/assets only as needed to reconstruct generated build context.
5. Never reset/rebase backward because an old recovery note names a historical branch.

The old `feature/pokebank-playable` branch and historical artifacts remain evidence, not automatic recovery targets.

## What GitHub must contain

Every non-reproducible project-authored change required to continue or rebuild must be committed and pushed: source, tests, workflows, save-format rules, mappings/generators, project-owned assets/overrides, documentation and recovery metadata.

A meaningful fix must not exist only in generated output, `build/`, `/mnt/data/`, a temporary worktree or an unpushed commit.

## Procedure

1. fetch the repository;
2. checkout/fetch the live branch named by `CURRENT_STATUS.md`;
3. initialize required submodules;
4. run tracked recovery/asset tooling needed by the current build;
5. verify asset preflight;
6. continue from the live head.

Use forensic Git archaeology only if remote recovery is missing required work or the user explicitly asks to rescue local-only changes.

Recovery must never weaken source-save immutability, live-write locks, provenance, or exact artifact identity.
