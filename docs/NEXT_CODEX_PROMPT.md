# PokeBank NX — Next Codex Prompt

Use this file as the authoritative task prompt for the next coding session.

## Launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

## Current verified source

```text
f6a3052daeffe7cd30d7acceba81a5dfda7615ee
gen3: expose RetroArch FRLG game sources
```

The strict FireRed/LeafGreen GBA route is complete through bounded RetroArch discovery,
the UIManager-owned catalog, normal Game Sources cards, and read-only Party / Boxes.
GitHub Actions run #184 passes all twelve host suites.

Do not redo the PKSM-Core oracle, exception-free native backend, FRLG scanner, source
cards, read-only view bridge, accepted UI, Left Stick work or PLA hardening.

## First: preserve and verify

Inspect and preserve useful local work before changing refs:

```text
git status
git status --short
git branch -avv
git remote -v
git log --all --oneline --decorate --graph -30
git reflog -20
git stash list
git worktree list
git diff
git diff --cached
git submodule status --recursive
```

Never reset, clean, restore or overwrite unexplained work. Push custom code only to
`origin/feature/pokebank-playable`; `kiasta/PKSE` remains upstream-only.

Run the baseline:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

## First gate: device feedback

If an exact FRLG browser artifact/device report is available, read it first and fix only
a genuine blocker. Never claim DEVICE TESTED without the exact artifact being run on a
physical Switch.

## Single coding mission: strict RSE production reads

Extend the proven read-only Gen III legacy pipeline to:

```text
ruby_gba
sapphire_gba
emerald_gba
```

Required path:

```text
bounded RetroArch discovery
        -> strict RSE save validation
        -> exact release + GBA identity
        -> existing Game Sources screen
        -> strict read-only Party / Boxes
```

Reuse the current PokeBank-owned Gen III API where sound. Do not leak PKSM-Core types
into UI code or reparse a selected file through older permissive Trainer logic. Keep
ambiguous or invalid saves unavailable rather than guessing identity. Preserve source
path, game ID, platform, Pokémon locations and original bytes.

## Read-only safety

Legacy files and installed titles remain immutable sources. Do not implement editing,
clone-to-save, delete, move, injection, repair/resign, writeback, conversion, Master
Vault/Banks or installed-title writes. View may operate on a PokeBank-owned read model.
Disabled actions must return safely and never fall through.

## Tests

Preserve all twelve existing host suites. Add focused coverage for RSE structure,
identity, malformed/truncated input, Party/Box mapping, source immutability and blocked
mutation operations. Keep FRLG regression coverage green.

Verify:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

## Checkpoint and stop

Commit and push coherent application source early to
`origin/feature/pokebank-playable`. Update only the status/handoff files and relevant
issue materially affected.

Stop after one coherent, pushed read-only RSE production-source checkpoint. Do not start
Gen I/II, Gen IV+, GameCube, DS/3DS, Master Vault/Banks, conversion, Right Stick, final
UI polish or live writes.

End report: starting SHA, implementation SHA(s), exact identities, validation and slot
behavior, browser path, read-only controls, Party/Boxes behavior, immutability proof,
host tests, sanitizers, diff check, native build, CI, NRO size impact, remaining blocker
and exact next coding task.
