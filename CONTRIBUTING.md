# Contributing to PokeBank NX

Thanks for helping. PokeBank NX edits people's game saves, so the bar is simple: **never risk an original save,
and never claim something works on a Switch unless it was tested on one.** The full rules for every contributor
(human or agent) are in [docs/ENGINEERING_PROTOCOL.md](docs/ENGINEERING_PROTOCOL.md); this page is the short
version.

## Picking something to work on

1. Read [docs/STATUS.md](docs/STATUS.md) to see what is in progress and what is paused.
2. Browse [open issues](https://github.com/GlitchedZeus/PokeBank-NX/issues). Useful labels: `good first issue`,
   `help wanted`, `gen-i-v`, `hardware-qa`, `vault`, `bug`. Comment on the issue before starting larger work so
   it is not done twice.
3. Gen I–V is the current product. Gen VI–IX (`gen-vi-ix`) and the legality engine (`legality`) are **paused**;
   please ask before opening PRs there.
4. Hardware test reports are always welcome: use the bug-report template and name the exact NRO.

## Building

### Host tests (no Switch needed)

You need a C++20 compiler (GCC or Clang), GNU Make and Python 3. Fetch the PKSM-Core submodule first; it is a
test oracle only and is not compiled into the app.

```sh
git submodule update --init --recursive
make -f Makefile.host -j"$(nproc)" host-test          # full host suite
make -f Makefile.host -j"$(nproc)" host-sanitize      # ASan/UBSan
make -f Makefile.host -j"$(nproc)" CXX=clang++ host-test
make -f Makefile.host qa                              # fuzz smoke, export-safety contracts, Gen I-V matrix
for t in tests/test_*.py; do python3 "$t" || echo "FAIL $t"; done
git diff --check
```

`Makefile.host` includes the `Makefile.host.*` fragments; add a new test to the fragment for its area. The host
suite is large, so while iterating you can build a single target, e.g.
`make -f Makefile.host build-host/test_gen5_save_diagnostics`.
The QA lane is described in [docs/qa/README.md](docs/qa/README.md).

### Switch NRO

Requires [devkitPro](https://devkitpro.org/) with devkitA64, libnx and the Switch SDL2 portlibs; `DEVKITPRO` must be
set. The output is `PokeBankNX.nro`.

```sh
make -j2
```

The complete artwork set is not all committed as loose files. CI restores it from the pinned recovery snapshot
before building (see `.github/workflows/native-pr-build.yml`): it checks out the commit in `RECOVERY_SHA` into a
separate directory, runs `python3 tools/recover_workspace.py` there, then
`tools/prepare_release_romfs.sh <that directory>/romfs` fills gaps in `romfs/` without overwriting branch assets.
CI then checks the packaged NRO with `python3 tools/verify_nro_romfs.py PokeBankNX.nro`. The reproducible
devkitA64 container image is pinned in the same workflow.

Hardware candidates are built by CI on an exact commit, not locally; see
[docs/RELEASES.md](docs/RELEASES.md).

## Save-safety rules (do not break these)

These are enforced by code and by tests. A PR that weakens one will not be merged.

- **Original saves are immutable.** Emulator saves and installed-game saves are read-only sources. Do not add a
  code path that opens one for writing. `LIVE_SAVE_WRITES_ENABLED` in `include/Safety/WritePolicy.h` stays a
  compile-time `false`, and `SourceMutationPolicy` only lets staged copies and app-owned storage change.
- **Edits are staged, then exported as a separate verified copy.** Exports go under
  `sdmc:/switch/PokeBank-NX/exports/`, keep an exact backup of the original, read back and hash-check what they
  wrote, strictly reparse the edited save (checksums included), never overwrite an existing export, and leave
  nothing partial on failure. New export paths must join the contracts in
  [docs/qa/SAVE_SAFETY_CHAIN.md](docs/qa/SAVE_SAFETY_CHAIN.md).
- **Unknown means refuse.** Ambiguous game identity, unknown formats and unverifiable sources fail closed. Never
  guess a game from a filename when the save can decide.
- **Japanese Gen I/II saves stay read-only** (`include/Integration/Common/JapaneseEditingGate.h`) until the owner
  enables them.
- **No cross-game Move and no direct injection** into an emulator or installed game.
- **Every format rule needs a source.** Offsets, checksums, encodings and game rules come from the pret
  disassemblies, PKHeX or another cited reference, never from guesswork. Missing legality evidence is
  "Unknown", not "Invalid", and the app never edits a Pokémon automatically because of a legality finding.
- **Never weaken a test** to get CI green.
- **Never commit** real saves, ROMs, keys, console data or personal information. Test fixtures are synthetic.

## Branches and pull requests

- `main` holds the Gen I–V application. Changes land through reviewed PRs.
- Round repair work happens on `fix/round<N>-gen15-repairs`; Gen VI–IX on `development/gen6-plus` (paused).
- For your own work, branch from `main` with a short descriptive name (`fix/gen3-bag-sort`,
  `docs/install-guide`). Unfinished work goes on `wip/<topic>-<yyyymmdd>`.
- Never force-push, rebase a shared branch or rewrite history. Reconcile with a merge.
- Keep each PR to one coherent change and fill in the
  [pull request template](.github/pull_request_template.md): what changed, which tests you ran, and whether a
  build was device tested.
- Commit messages follow the style already in the log: `area: what changed` or `fix(gen4): …`, in the
  imperative, with the reason in the body.

## Status words

Use these exactly in issues, PRs and docs:

| Word | Means |
|---|---|
| Implemented | The code is written. |
| Tested | Host tests were run and passed; say which command. |
| Hardware verified | The owner tested that exact NRO on a Switch and recorded PASS. Only the owner sets this. |

A green CI run is never a hardware result.

## License

By contributing you agree that your contribution is licensed under the
[GNU Affero General Public License v3.0](LICENSE), the project's license. Keep the notices of reused code
(PKSE, PKSM-Core, NanoVG and others) intact.
