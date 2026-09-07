# PokeBank NX — Next Codex Prompt

Use this file as the authoritative task prompt for the next coding session.

## Launcher

The user should only need to send:

```text
Recover and continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve all local work before syncing, push coherent checkpoints early, and never push custom code upstream.
```

---

## Current remote state

The current synchronized remote branch head is:

```text
3828abb8c939ab96caaa3337545b3a737a3994fc
Merge PR #45: sync current PokeBank NX development docs
```

This is a documentation/history synchronization commit. The latest verified Gen III engineering implementation below it is:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

Do **not** confuse the documentation branch head with the latest substantive engine checkpoint, and do **not** redo Session 3A or the native Gen III selective backend.

## Priority zero — recover interrupted workspace work

A previous coding session timed out after implementing additional RetroArch FireRed/LeafGreen work locally. That work may be newer than the remote branch and may be uncommitted.

Before pulling, rebasing, checking out another ref, resetting, cleaning, restoring, or editing source, inspect and preserve the workspace.

Inspect at minimum:

```text
pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git remote -v
git log --all --oneline --decorate --graph -60
git reflog -60
git stash list
git worktree list
git diff
git diff --cached
git submodule status --recursive
```

Also inspect untracked/recent files related to RetroArch, FRLG, Gen3, source discovery, `.sav`, `.srm`, `savefile_directory`, source catalogs/providers, and runtime registration.

If useful interrupted work exists, preserve it immediately on a recovery ref/branch and preserve untracked source/tests before doing anything destructive.

Do **not** run `git reset --hard`, `git clean`, `git restore .`, or otherwise discard local state until recovery is complete.

If the local workspace contains work based on an older branch head, recover the useful code first and then reconcile it with remote `3828abb8...`; do not overwrite it just because the remote documentation history moved.

Only recreate work from memory if recovery genuinely fails.

## Reported interrupted RetroArch work

The timed-out session reported:

- bounded read-only RetroArch source catalog;
- reads configured `savefile_directory`;
- only `.sav` / `.srm` candidates;
- maximum scan depth 2;
- maximum 256 candidates by default;
- full FRLG structure validation before using path/name hints;
- Party/Boxes exposed through the existing Gen III adapter;
- ambiguous valid FRLG sources remain unclassified instead of guessed.

The final reported unfinished task was to ensure the real runtime source-discovery lifecycle owns/invokes that catalog so the linker retains it because the application genuinely uses it.

## Mission after recovery

Finish the real end-to-end read-only runtime path:

```text
RetroArch savefile_directory
        -> bounded read-only catalog
        -> valid FRLG source
        -> firered_gba / leafgreen_gba when evidence is reliable
        -> native exception-free Gen III backend
        -> Party / Boxes read model
        -> existing PokeBank source/browser lifecycle
```

Use the existing source/game registry. Do not create a second browser, debug UI, or broad SD-card crawler.

## Classification rules

- Never classify a save as FireRed/LeafGreen from filename alone.
- Structural validation must pass first.
- Path/name hints may support classification after validation.
- If FireRed vs LeafGreen remains genuinely ambiguous, keep it unclassified rather than guessing.
- `firered_gba` / `leafgreen_gba` must never collide with `firered_switch` / `leafgreen_switch`.

## Safety

Everything in this milestone remains read-only.

Do not implement or enable:

- RetroArch save modification;
- save repair/resign/writeback;
- installed Switch title writes;
- true Move;
- conversion UI;
- Master Vault/Banks;
- unrelated generations;
- UI redesign;
- Right Stick work;
- app rename/branding migration in this session.

Preserve the existing live-write hard lock.

## Verification

Preserve existing Gen III coverage and add/finish focused tests where practical for:

- configured RetroArch save root;
- `.sav` and `.srm` candidates;
- depth/candidate bounds;
- missing directory/config graceful handling;
- non-Pokémon and malformed files skipped/rejected;
- valid FRLG source accepted;
- ambiguous FRLG not guessed;
- GBA/Switch FRLG identity separation;
- source bytes unchanged;
- actual runtime/source registration path invokes the provider/catalog where host-testable.

Run:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Native build must remain `-fno-exceptions`. Do not reintroduce full PKSM-Core into the native application.

## Checkpoint policy

As soon as recovered work is coherent enough to preserve, commit and push it to:

```text
origin / feature/pokebank-playable
```

Never upstream.

Do not wait until the final minutes. Prefer an early recovery checkpoint followed by a second runtime-wiring checkpoint rather than another lost session.

Suggested implementation commit:

```text
gen3: wire RetroArch FRLG read-only sources
```

## Minimal docs/issues

Update only what materially changes:

- `CURRENT_STATUS.md`
- `PROJECT_STATUS.md` if needed
- `docs/NEXT_SESSION_PLAN.md`
- `docs/NEXT_CODEX_PROMPT.md` so the next launcher remains current
- issue #6
- issue #17 only if fixtures change

Issue #4 is the completed PKSM-Core Gen III spike and should not be reopened unless the adapter architecture itself regresses.

## Stop condition

Stop after a coherent, pushed FRLG RetroArch runtime checkpoint. Do not start RSE, Gen I/II, Vault/Banks, GameCube, DS/3DS, final UI polish, Ranch mode, or renaming/branding work in this same recovery session.

End report should include:

```text
recovery result
recovered files/ref
remote starting SHA
implementation SHA(s)
runtime registration path
catalog/provider files
RetroArch path behavior
extensions / scan bounds
FR/LG classification behavior
Party parse / Box parse
source immutability
host tests
ASan/UBSan
git diff --check
native build
GitHub CI
NRO size impact
remaining blocker
exact next coding task
```
