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

Current documentation head before this handoff update:

```text
be7db52c6e8b3caa4f10d9de27ef15058088ec42
docs: record FRLG read-only browser milestone
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

## Hard gate: exact FRLG device artifact

The application code milestone is complete, but it is **NOT DEVICE TESTED**.

The previous session intentionally did not release a device artifact because asset
preflight found only 1,200 Pokémon renders in the transient sprite cache instead of the
verified complete 3,260-render pinned set.

Before beginning Ruby/Sapphire/Emerald work:

1. restore/reconstruct the project-approved complete pinned 3,260-render sprite set using
   the existing documented asset workflow/reference; do not silently accept the incomplete
   1,200-render cache;
2. preserve/verify asset hashes/manifests as the project workflow requires;
3. build/package an exact device-test `.nro` from application source `f6a3052d...` (or an
   explicitly documented no-code-change packaging descendant whose application tree is
   identical);
4. record artifact filename, source SHA, size and SHA-256;
5. stop and report the exact artifact for physical Switch testing unless an exact physical
   device report for that artifact is already available.

Do **not** call FRLG `DEVICE TESTED` until that exact artifact is physically run on Switch.

Required physical flow to verify:

```text
Game Sources
  -> FireRed GBA / LeafGreen GBA (RETROARCH)
  -> select source
  -> trainer/source view
  -> Party
  -> Boxes 1-14
  -> View Pokémon
```

Confirm that GBA sources cannot be confused with the separate Switch FireRed/LeafGreen
identities and that Edit/Clone/Transfer/Move/Save/writeback remain blocked.

If the device test exposes a genuine blocker, fix only that blocker, rerun all verification,
package a new exact artifact, and report it for another physical test.

## Next coding mission AFTER FRLG physical acceptance: strict RSE production reads

Only after the exact FRLG browser artifact is physically accepted, extend the proven
read-only Gen III legacy pipeline to:

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

## Tests for RSE when that phase begins

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

For the immediate next session, the stop condition is the exact FRLG device-test artifact
unless that exact artifact has already been physically accepted.

After physical FRLG acceptance, a later coding session may implement one coherent, pushed
read-only RSE production-source checkpoint.

Do not start Gen I/II, Gen IV+, GameCube, DS/3DS, Master Vault/Banks, conversion, events,
physical-link hardware, Right Stick, final UI polish or live writes.

Immediate-session end report should include:

```text
starting SHA
asset restoration result
verified render count
asset manifest/hash result
exact application/source SHA
NRO filename
NRO size
NRO SHA-256
host tests
ASan/UBSan
git diff --check
native build
CI/status if applicable
physical test status (must remain NO until user tests exact artifact)
remaining blocker
exact next action for physical test
```

After physical acceptance, the RSE session end report should include:

```text
starting SHA
implementation SHA(s)
exact identities
validation and slot behavior
browser path
read-only controls
Party/Boxes behavior
immutability proof
host tests
sanitizers
diff check
native build
CI
NRO size impact
remaining blocker
exact next coding task
```
