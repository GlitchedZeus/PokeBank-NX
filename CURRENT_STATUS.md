# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-08

This file is the short authoritative engineering handoff. The root README is human-facing product/roadmap information only. Detailed active work instructions live in `docs/NEXT_CODEX_PROMPT.md`.

## Current state — FRLG device-failure fixes are published; physical-test artifact packaging is incomplete

The previously rejected physical-test artifact remains:

```text
Application source: 5d3e5e23f352dda4900ae42b4f396d9d4a4b8b8e
Artifact: PokeBank-NX-FRLG-Complete-5d3e5e23.nro
SHA-256: 8dd94277e609c96f37e82b0b0b47928bad50ce36fdb380520bd2521ab18ec78a
DEVICE TESTED: YES
DEVICE ACCEPTED: NO
```

Device-proven failures were:

1. PokeBank opened an older FRLG save containing the previously observed level-6 Charmander instead of the user's current save containing 2 Pokémon.
2. The user's FRLG GBA legacy sources appeared under both the user's Nintendo profile and the niece's profile.

## Published corrective application checkpoint

The next coding session investigated those two failures and published this application source:

```text
ea0b806bac4acdb5619f22f9841d616ea8a237ff
legacy: bind FRLG sources to profiles and expose diagnostics
```

This commit is present on:

```text
origin/feature/pokebank-playable
```

Reported implementation in this checkpoint:

- removed the unsupported/arbitrary `Main Save` guess;
- multiple valid FRLG physical files remain distinct save instances;
- save instances expose truthful filename/source information instead of pretending the first filename-sorted entry is current;
- newest modification time is used only for deterministic initial focus when multiple genuine files exist, while all remain selectable;
- read-only Source Details expose provider, normalized physical path, size, modification state, stable identity, SHA-256 prefix/fingerprint, exact game ID, decoded trainer, and party count;
- shared physical legacy catalog is separated from profile-scoped visibility;
- unassigned legacy saves are hidden from normal per-profile source lists;
- explicit assignment flow binds a selected physical source to the currently selected Nintendo/PokeBank profile;
- profile binding is persisted atomically across restart/reload;
- a source bound to Profile A remains absent from Profile B;
- aliases do not create duplicate profile bindings;
- genuinely distinct physical saves can be assigned independently;
- installed Switch-title account scoping remains separate;
- RetroArch and installed-game source writes remain hard disabled.

The session reported the root cause of the stale-save behavior as:

```text
multiple valid FRLG files
-> filename sort
-> first child auto-focused
-> first child falsely labeled "Main Save"
```

There was no reliable evidence that the first filename-sorted valid save was RetroArch's currently used gameplay save.

## Verification already completed for ea0b806b

The interrupted session reported all of the following green before publishing `ea0b806b`:

```text
Host tests: 13 suites PASS
ASan/UBSan: PASS
git diff --check: PASS
Native Switch -fno-exceptions compile/link: PASS
```

New regressions reportedly include:

- stale old one-Pokémon FRLG copy + newer two-Pokémon FRLG copy;
- truthful source filename/party/fingerprint diagnostics;
- two-profile persistent binding where Profile A's source is absent from Profile B after reload.

Unless application source changes after `ea0b806b`, do not waste another session rerunning expensive full host sanitizer compilation merely to package the same source. Verify existing results/CI and rerun only what is materially required for the exact full-asset artifact. If code changes, rerun the required verification for those changes.

## Packaging progress when the session ran out

The source fix was already pushed. The remaining work was full-asset restoration and exact physical-test packaging.

Last reported restore progress:

```text
HD renders restored: 1392 / 3260
Type icons: 18 / 18
Fonts: 3 / 3
Canonical application source for final build: ea0b806bac4acdb5619f22f9841d616ea8a237ff
```

The restore process was reported as resumable. Do not discard or restart useful restored asset state if it still exists locally.

No new exact physical-test NRO for `ea0b806b` was completed before credits/session ended.

Therefore current status is:

```text
SOURCE FIX PUBLISHED: YES
SOFTWARE VERIFICATION REPORTED GREEN: YES
FULL ASSET RESTORE COMPLETE: NO / UNKNOWN AFTER SESSION END
NEW EXACT NRO PACKAGED: NO
DEVICE TESTED FOR ea0b806b: NO
DEVICE ACCEPTED: NO
```

## Immediate next milestone

Do not reopen the two already-fixed bugs unless verification or physical testing disproves the implementation.

Continue only from the published `ea0b806b` checkpoint and finish:

```text
preserve/resume useful local asset restore state
        ↓
restore all 3260 pinned HD renders
        ↓
verify 1025/1025 base species + complete asset/RomFS preflight
        ↓
clean exact-source native build for ea0b806b
        ↓
package NRO + optional ZIP/manifest
        ↓
record exact size + SHA-256 + embedded source identity
        ↓
STOP for physical Switch retest
```

The next physical test must prove both:

```text
A. the user's current two-Pokémon FRLG save can be identified and opened using truthful source details; the old level-6 Charmander copy is not silently presented as "Main Save"

B. a FRLG legacy source assigned to the user's profile is absent from the niece's profile, including after app restart
```

Do not begin Ruby/Sapphire/Emerald until those pass physically.

## Repository / safety

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream-only remote: kiasta/PKSE
```

Before syncing, resetting, cleaning, restoring, switching refs, rebasing, changing worktrees, or otherwise risking local state, preserve useful local/uncommitted/recovery work first.

Never push PokeBank NX custom code upstream.

Live installed-game and RetroArch writes remain HARD DISABLED.

## Parked RSE recovery

Preserve if present:

```text
b5ef83b
1a921515
```

or equivalent recovered refs.

RSE remains parked. Do not merge, resume, reimplement, or push it until FRLG physical acceptance.

## Session launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Before touching refs, preserve all local/uncommitted/recovery work. Read CURRENT_STATUS.md and docs/CODEX_SESSION.md, then execute docs/NEXT_CODEX_PROMPT.md exactly. Push coherent checkpoints only to origin/feature/pokebank-playable; never push custom code upstream.
```
