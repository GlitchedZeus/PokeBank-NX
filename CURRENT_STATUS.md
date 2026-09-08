# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-08

This file is the short authoritative engineering handoff. Long-form product ideas and human-readable roadmap information live elsewhere under `docs/` and in the README.

## Current state — FRLG physical retest FAILED

The completed FRLG read-only browser application source was:

```text
Application source: 5d3e5e23f352dda4900ae42b4f396d9d4a4b8b8e
Commit: gen3: complete FRLG read-only source browsing
Embedded version: 0.1.0-alpha (5d3e5e23)
Artifact: PokeBank-NX-FRLG-Complete-5d3e5e23.nro
Artifact size: 156249001 bytes
Artifact SHA-256: 8dd94277e609c96f37e82b0b0b47928bad50ce36fdb380520bd2521ab18ec78a
```

Physical status:

```text
DEVICE TESTED: YES
DEVICE ACCEPTED: NO
```

Do not describe this artifact as untested or accepted.

## Device-proven blocker 1 — stale/old FRLG source still opens

The user's current FRLG save contains **2 Pokémon**.

The 5d3e5e23 build still opens an older FRLG save containing the previously observed **level 6 Charmander**.

Therefore the source-selection/active-root/refresh work is not physically correct yet despite host verification.

Required next result:

- identify the exact physical `.sav` / `.srm` PokeBank opens;
- correct the real source-selection bug rather than masking its contents;
- keep configured usable RetroArch `savefile_directory` authoritative;
- keep the conventional `sdmc:/retroarch/cores/savefiles` path fallback-only;
- preserve genuinely distinct saves as distinct children;
- collapse only aliases of the same physical file;
- refresh changed files into a new strict read model;
- expose enough diagnostics in the next test build to prove normalized path, useful metadata, fingerprint, trainer/game identity, and party count for the selected source;
- remain strictly read-only.

The next physical test must prove that the selected source is the user's current save with 2 Pokémon rather than the old level 6 Charmander save.

## Device-proven blocker 2 — GBA legacy saves leak across Nintendo profiles

Physical result:

- the user's FRLG GBA sources appear under the user's profile;
- the same sources also appear under the user's niece's profile.

This is not accepted behavior.

Correct architecture:

```text
physical RetroArch/file discovery = shared/app-global
profile visibility/ownership binding = profile-scoped
```

RetroArch itself is not a Nintendo user, but a legacy source must not automatically appear in every profile's normal game-source list.

Required next result:

- maintain one shared physical source catalog;
- persist a binding from stable physical source identity to the selected Nintendo/PokeBank profile;
- bound user FRLG sources appear only for that user;
- niece profile does not see the user's FRLG saves by default;
- bindings survive restart;
- aliases do not create duplicate bindings;
- separate physical saves can be assigned independently;
- do not infer owner from trainer name, TID, Pokémon contents, or save hash;
- if assignment cannot be inferred safely, use a minimal explicit first-assignment flow instead of guessing;
- installed Switch-title account scoping must remain intact;
- no Vault/Gift/Trade implementation is part of this blocker fix.

## What remains verified from 5d3e5e23

Software-side verification for the rejected physical-test candidate was:

```text
Host tests: 12 suites PASS
ASan/UBSan: PASS
git diff --check: PASS
Native Switch -fno-exceptions build: PASS
Application CI: PASS — run #238
Documentation CI: PASS — run #240
HD renders: 3260
Base species: 1025 / 1025
Embedded RomFS: 3283 / 3283 files byte-identical
```

The following functionality should be preserved while fixing the two device blockers:

- strict FRLG rotating-slot/sector/checksum validation;
- FireRed and LeafGreen separate parent identities;
- FireRed/LeafGreen GBA artwork;
- canonical alias dedupe;
- Party browsing;
- all 14 Boxes;
- Pokémon View;
- trainer name, gender, TID16, SID16, ID32, decrypted money and exact game identity;
- all six read-only FRLG inventory pouches;
- malformed-save/inventory safety;
- source-byte immutability;
- manual Refresh Saves control;
- installed-title and RetroArch live writes hard-disabled.

## Repository / branch

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream-only remote: kiasta/PKSE
```

Never push PokeBank NX custom code upstream.

Before syncing, resetting, cleaning, restoring, switching refs, rebasing, changing worktrees, or otherwise risking local state, preserve useful local/uncommitted/recovery work first.

## Parked RSE recovery

RSE remains parked until FRLG passes physical acceptance.

Preserve if present:

```text
b5ef83b
1a921515
```

or equivalent recovered refs.

Do not merge, resume, reimplement, or push RSE yet.

## Immediate next milestone

Fix only the two physical-test failures:

```text
wrong/stale FRLG physical source
+
legacy source profile visibility/binding
        ↓
host + sanitizer + native verification
        ↓
new exact full-asset NRO
        ↓
STOP for physical retest
```

Do not begin Ruby/Sapphire/Emerald or later roadmap work before physical acceptance.

The detailed authoritative task is `docs/NEXT_CODEX_PROMPT.md`.

## Session launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Before touching refs, preserve all local/uncommitted/recovery work. Read CURRENT_STATUS.md and docs/CODEX_SESSION.md, then execute docs/NEXT_CODEX_PROMPT.md exactly. Push coherent checkpoints only to origin/feature/pokebank-playable; never push custom code upstream.
```
