# PokeBank NX — Recovery Contract

Last updated: 2026-09-09

## The rule

When the user says **RECOVERY**, recovery is an operation, not an archaeology session.

The normal recovery path is:

```text
origin/feature/pokebank-playable
        ↓
python3 tools/recover_workspace.py
        ↓
DEVICE ASSET PREFLIGHT: PASS
        ↓
RECOVERY COMPLETE
        ↓
continue the active task
```

Do **not** start by reading reflogs, walking every worktree, searching old sessions, rebuilding roadmaps, or checking dozens of historical refs.

Forensic recovery is an **exception path only**. Enter it only when the normal GitHub recovery path fails, the requested remote commit is missing, or the user explicitly asks to recover unsaved local-only work.

## What GitHub must contain

Every piece of project-authored work that cannot be reproduced automatically must be committed and pushed to `origin/feature/pokebank-playable` before a session is considered safely saved:

- source code;
- tests;
- build scripts;
- save-format rules;
- sprite/form mappings;
- asset-generation scripts;
- pinned upstream revisions;
- UI artwork created for PokeBank NX;
- manual corrections/overrides that are legally safe to redistribute;
- documentation and research distilled into the repo;
- recovery state/manifests.

No meaningful fix is allowed to exist only in:

```text
romfs/
build/
/mnt/data/
a temporary Codex worktree
an unpushed commit
```

If a fix is made by manually changing generated output, that fix must be promoted into a tracked generator/mapping/transform/override before the session is called saved.

## Complete private GitHub RomFS snapshot

The repository is private, so routine recovery should ultimately be independent of a temporary Codex cache **and** independent of redownloading 3,260 assets from the internet.

After the first full preflight-passing recovery, run:

```bash
python3 tools/pack_recovery_snapshot.py
git add -f recovery/assets_snapshot/
git commit -m "recovery: snapshot complete verified RomFS"
git push origin feature/pokebank-playable
```

The packer creates a deterministic tar of the complete `romfs/` tree and splits it into 80 MiB GitHub-safe pieces under:

```text
recovery/assets_snapshot/
```

No individual part exceeds GitHub's normal 100 MiB single-file limit.

Once `recovery/assets_snapshot/manifest.json` and all listed parts are committed, `tools/recover_workspace.py` **prefers the full committed GitHub snapshot**. Pinned source regeneration becomes a fallback, not the normal path.

This gives us the intended recovery model:

```text
private GitHub
  source/code/tests/docs
  + complete verified RomFS snapshot
        ↓
one recovery command
        ↓
working build context
```

Do not commit a partial snapshot. The packer runs the device asset preflight first and refuses to snapshot an incomplete tree.

## Generated third-party assets and project-authored fixes

Normal build output under `romfs/` remains generated/gitignored on the development branch. That prevents ordinary coding commits from accidentally mixing thousands of generated binaries with source changes.

GitHub still contains both:

1. the complete verified recovery snapshot under `recovery/assets_snapshot/` once populated; and
2. the exact regeneration recipe as a fallback:
   - pinned Pokémon sprite source revision;
   - generator code;
   - expected sprite counts;
   - pinned type source information;
   - font source information;
   - project-owned game-card art;
   - tracked project-authored overrides;
   - verification script;
   - last known artifact identity/hash.

Any future non-reproducible manual asset correction must also be represented under:

```text
assets/recovery_overrides/
```

or encoded as a tracked deterministic transform. Never leave the only corrected copy in generated `romfs/`.

## Current known full visual baseline

Historical full visual baseline:

```text
Application source: ea0b806bac4acdb5619f22f9841d616ea8a237ff
Artifact: PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro
Size: 156,592,377 bytes
SHA-256: 396f8ff9f4da53b5449aeb46b8d1237ca9358a0ac94998680017575916b6b1ee
HD renders: 3,260 / 3,260
Base species: 1,025 / 1,025
Type icons: 18 / 18
Fonts: 3 / 3
Embedded RomFS: 3,283 / 3,283 files
```

Current application source awaiting a new physical-test artifact:

```text
92bde34d1586990aaa82adc4f60d42d7bc6b5bdf
legacy: safely replace profile binding database on Switch
```

The user's copy of the historical NRO is a useful independent fallback, but normal recovery must not depend on the user re-uploading it.

## Routine RECOVERY algorithm

When a new coding session starts and the user says RECOVERY:

1. Use `origin/feature/pokebank-playable` as the normal source of truth.
2. Read only `CURRENT_STATUS.md`, this contract, and `docs/NEXT_CODEX_PROMPT.md`.
3. Run:

```bash
python3 tools/recover_workspace.py
```

4. The tool first attempts the complete committed GitHub RomFS snapshot.
5. If the snapshot has not been populated yet, it uses pinned deterministic regeneration as the fallback.
6. If it prints `RECOVERY COMPLETE`, continue the authorized task immediately.
7. Do not rerun expensive tests merely because the workspace is new. Run only verification required by changed source or by the current prompt.
8. If recovery fails, report the exact failed stage and then use the smallest forensic action necessary.

## Forensic exception path

Only if normal recovery fails:

```text
remote commit missing
tracked recovery recipe inconsistent
committed snapshot corrupt/missing parts AND pinned fallback unavailable
local-only work explicitly needs rescue
Git history divergence prevents normal checkout
```

then inspect status/reflog/worktrees/bundles as appropriate.

Do not make forensic archaeology the default definition of recovery.

## Session-save contract

Before a coding session is reported as safely saved:

```text
project-authored changes committed
        ↓
pushed to origin/feature/pokebank-playable
        ↓
remote SHA verified
        ↓
recovery recipe/state still valid
        ↓
complete recovery snapshot refreshed when generated assets materially change
        ↓
no manual fix exists only in ignored/generated output
```

A statement such as “preserved in downloadable storage” is **not** equivalent to GitHub recovery unless that storage is explicitly persistent and future sessions can retrieve it. Do not use that wording as a substitute for a remote checkpoint.

## Artifact rule

Git source and a built `.nro` are different things.

GitHub must always preserve enough project-authored information and the private RomFS recovery snapshot needed to recreate the build. Binary NRO/ZIP preservation is useful but must not be treated as the only copy of source/build logic.

If a binary cannot be uploaded to GitHub from the active environment, record its exact filename/size/SHA-256 in GitHub and state clearly where the binary actually lives. Never call temporary session storage “durable” without verification.

## Success wording

Use this only after the normal path succeeds:

```text
RECOVERY COMPLETE
GitHub source: <sha>
Recovery source: committed GitHub RomFS snapshot OR pinned fallback
Generated asset preflight: PASS
Active task can continue: YES
```

If it does not succeed, use:

```text
RECOVERY BLOCKED
Stage: <exact stage>
Reason: <exact reason>
```

No vague “build context is gone” report without first running the deterministic recovery path.
