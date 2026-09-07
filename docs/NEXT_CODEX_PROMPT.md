# PokeBank NX — Next Codex Prompt

Use this file as the authoritative task prompt for the next coding session.

## Launcher

The user should only need to send:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

---

## Mission

Continue from the verified remote checkpoint:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

Do **not** redo Session 3A or the native Gen III selective backend.

First recover any interrupted/uncommitted RetroArch FireRed/LeafGreen work that may still exist in the workspace. Inspect `git status`, branches, reflog, stashes, worktrees, diffs and untracked files before editing. Preserve useful local work on a recovery ref before changing it.

Reported interrupted work after `43f3a9f9...`:

- bounded read-only RetroArch source catalog;
- reads configured `savefile_directory`;
- only `.sav` / `.srm` candidates;
- maximum scan depth 2;
- maximum 256 candidates by default;
- full FRLG structure validation before using path/name hints;
- Party/Boxes exposed through the existing Gen III adapter;
- ambiguous valid FRLG sources remain unclassified instead of guessed.

The unfinished task was to ensure the real runtime source-discovery lifecycle owns/invokes the catalog so it is not dead code discarded by the linker.

## Required end-to-end path

```text
RetroArch savefile_directory
        -> bounded read-only catalog
        -> valid FRLG source
        -> firered_gba / leafgreen_gba when evidence is reliable
        -> native exception-free Gen III backend
        -> Party / Boxes read model
        -> existing PokeBank source/browser lifecycle
```

Use the existing source/game registry. Do not create a second browser, debug UI or broad SD-card crawler.

## Classification rules

- Never classify a save as FR/LG from filename alone.
- Structural validation must pass first.
- Path/name hints may support classification after validation.
- If FireRed vs LeafGreen remains genuinely ambiguous, keep it unclassified rather than guessing.
- `firered_gba` / `leafgreen_gba` must never collide with `firered_switch` / `leafgreen_switch`.

## Safety

Everything in this milestone is read-only.

Do not implement or enable:

- RetroArch save modification;
- save repair/resign/writeback;
- installed Switch title writes;
- true Move;
- conversion UI;
- Master Vault/Banks;
- unrelated generations;
- UI redesign or Right Stick work.

Preserve the existing live-write hard lock.

## Verification

Preserve the existing Gen III tests and add focused tests where practical for:

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

Native build must remain `-fno-exceptions`. Do not reintroduce full PKSM-Core into the native build.

## Checkpoint policy

As soon as the recovered/runtime-wired RetroArch work is coherent, commit and push it to:

```text
origin / feature/pokebank-playable
```

Never upstream.

Do not wait until the final minutes. If credentials fail, use the authenticated GitHub connection before continuing.

Suggested commit:

```text
gen3: wire RetroArch FRLG read-only sources
```

## Minimal docs/issues

Update only what materially changes:

- `CURRENT_STATUS.md`
- `PROJECT_STATUS.md` if needed
- `docs/NEXT_SESSION_PLAN.md`
- issue #6
- issue #17 only if fixtures change

Issue #4 is the completed PKSM-Core Gen III spike and should not be reopened for this runtime wiring unless the adapter architecture itself regresses.

## Stop condition

Stop after a coherent FRLG RetroArch runtime checkpoint. Do not start Gen I/II, RSE, Vault/Banks, GameCube, DS/3DS or UI polish in the same session.

End report should include:

```text
recovered files/ref
starting SHA
implementation SHA
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
