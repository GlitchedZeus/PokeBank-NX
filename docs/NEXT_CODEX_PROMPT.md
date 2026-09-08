# PokeBank NX — Authoritative Next Codex Prompt

> **FRLG FIX SOURCE IS ALREADY PUBLISHED — FINISH ONLY THE EXACT PHYSICAL-TEST ARTIFACT**

Use HIGH reasoning.

## Starting remote application checkpoint

The two device-proven FRLG failures have already been addressed in source and published to:

```text
ea0b806bac4acdb5619f22f9841d616ea8a237ff
legacy: bind FRLG sources to profiles and expose diagnostics
```

This is the canonical application source for the next physical-test artifact unless a real packaging/build defect forces an application-source change.

The previous rejected artifact remains:

```text
5d3e5e23f352dda4900ae42b4f396d9d4a4b8b8e
PokeBank-NX-FRLG-Complete-5d3e5e23.nro
SHA-256 8dd94277e609c96f37e82b0b0b47928bad50ce36fdb380520bd2521ab18ec78a
DEVICE TESTED: YES
DEVICE ACCEPTED: NO
```

Do not call the old artifact untested or accepted.

## Preserve first

Before syncing, resetting, cleaning, restoring, switching refs, rebasing, changing worktrees, deleting generated assets, or doing anything that could discard local state:

- inspect all worktrees;
- inspect `git status`;
- preserve useful uncommitted/generated/recovery state;
- preserve any partially restored pinned artwork because the restore is resumable;
- preserve parked RSE refs including `b5ef83b`, `1a921515`, or equivalents.

Do not merge, resume, reimplement, or push RSE.

The README is human-facing roadmap/product information only and must NOT expand the active task.

Push only to:

```text
origin/feature/pokebank-playable
```

Never push custom PokeBank NX code upstream to PKSE.

# What was already completed before the previous session ran out

The previous session reported and published the following application behavior in `ea0b806b`:

## Stale/old-save correction

Root cause found:

```text
multiple valid FRLG files
-> filename sorting
-> first child auto-focused
-> first child incorrectly called "Main Save"
```

That was not proof of RetroArch's active/current gameplay save.

Published correction:

- do not invent `Main Save`;
- keep genuinely separate valid physical saves as separate children;
- expose truthful filename/source information;
- expose Source Details with provider, normalized path, size, modification state, stable identity, short SHA-256/content fingerprint, exact game ID, decoded trainer, and party count;
- use newest mtime only for deterministic initial focus when multiple genuine saves exist, while preserving all children for explicit selection;
- refresh remains read-only and rebuilds changed source models safely.

A deterministic stale-copy regression reportedly uses an older one-Pokémon save plus a newer two-Pokémon save and passes.

## Profile leakage correction

Published model:

```text
physical legacy discovery/catalog = shared/app-global
normal source visibility/binding = profile-scoped
```

Published correction reportedly provides:

- persistent source-to-profile bindings;
- unassigned legacy saves hidden from normal profile source lists;
- explicit assignment to the current Nintendo/PokeBank profile;
- Profile A source absent from Profile B;
- bindings survive reload/restart;
- aliases do not duplicate bindings;
- distinct physical saves can be assigned independently;
- installed Switch-title account scoping unaffected;
- no Vault/Gift/Trade implementation;
- no source writes.

## Verification already completed

The previous session reported all green for the exact `ea0b806b` application source:

```text
Host tests: 13 suites PASS
ASan/UBSan: PASS
git diff --check: PASS
Native Switch -fno-exceptions compile/link: PASS
```

DO NOT burn another session rerunning expensive full sanitizer/PKSM-Core work solely to package the unchanged `ea0b806b` source.

First verify that the source tree used for packaging is exactly `ea0b806b` and that no application-code changes are needed.

If application source remains byte-for-byte/commit-identical to `ea0b806b`:

- reuse the already completed verification record;
- check relevant CI status if available;
- perform only packaging/build verification materially required to create the exact artifact.

If application code changes after `ea0b806b`, rerun the appropriate host/sanitizer/native/diff verification before packaging.

# Single mission — finish the exact physical-test artifact

The previous session ran out during the pinned full-artwork restore.

Last reported progress:

```text
HD renders present: 1392 / 3260
Type icons: 18 / 18
Fonts: 3 / 3
Restore process: resumable
Canonical source: ea0b806bac4acdb5619f22f9841d616ea8a237ff
```

Do not assume those exact local counts still exist after workspace maintenance; inspect safely first. Resume useful existing restored state rather than deleting/restarting it.

Required completion:

1. restore the complete pinned **3,260 HD Pokémon render** set;
2. verify **1,025 / 1,025 base species**;
3. run the existing asset preflight/integrity checks;
4. ensure the build is tied to canonical application source `ea0b806b`;
5. perform a clean exact-source native Switch build with the complete assets;
6. verify the embedded RomFS against the intended source asset tree using the repository's established verification method;
7. package the exact physical-test `.nro`;
8. produce ZIP fallback and build manifest if the established release flow supports them;
9. calculate exact byte sizes and SHA-256 hashes;
10. record the embedded version/source SHA;
11. update minimal engineering handoff docs with exact artifact identity;
12. STOP for physical device testing.

Do not do additional feature development during this session.

# Preserve existing ea0b806b functionality

Do not regress or broadly rewrite:

- strict FRLG save validation;
- FireRed / LeafGreen identities and GBA artwork;
- Party and all 14 Boxes;
- Pokémon View;
- truthful trainer data and decrypted money;
- six read-only inventory pouches;
- refresh/rescan;
- alias dedupe;
- truthful physical-source diagnostics;
- persistent per-profile legacy-source bindings;
- installed-title account scoping;
- malformed-source safety;
- source-byte immutability;
- installed-game and RetroArch live-write locks.

# Strictly out of scope

Do NOT begin:

- Ruby/Sapphire/Emerald;
- Master/Profile Vault implementation beyond the already-published source binding;
- Banks;
- Gen I/II;
- DS/3DS;
- Colosseum/XD;
- Stadium;
- Android;
- Friends/Mailbox;
- Gift/Trade;
- events;
- save writing;
- conversion;
- Move;
- arbitrary RetroArch save-state parsing;
- broad UI redesign.

# Final artifact report

Report exactly:

```text
starting remote/documentation SHA
canonical application source SHA = ea0b806bac4acdb5619f22f9841d616ea8a237ff
application commit message
whether any application source changed during packaging
verification record reused and/or rerun
CI status if checked
HD render count
base species count
RomFS verification result
NRO filename
NRO exact byte size
NRO SHA-256
ZIP filename/size/SHA-256 if produced
build manifest filename if produced
embedded version/source identity
preserved RSE refs/status
DEVICE TESTED: NO
DEVICE ACCEPTED: NO
```

Then STOP.

# Required physical retest after artifact delivery

The user must verify on Switch:

```text
1. Under the user's profile, assign/select the correct FRLG physical save.
2. Source Details identify the exact physical path/fingerprint/trainer/party count.
3. The current save with 2 Pokémon opens correctly.
4. The old level-6 Charmander copy is not silently presented as "Main Save"; if still present as a genuine separate file, it is truthfully identifiable as a separate child.
5. Restart PokeBank NX and confirm the source binding persists.
6. Switch to the niece's profile and confirm the user's assigned FRLG save is absent.
7. Switch back to the user's profile and confirm it remains assigned/visible.
8. Trainer, Items, Party, Boxes 1-14, Pokémon View, refresh, and read-only locks still work.
```

Do not begin RSE until the user physically accepts both the current-save selection behavior and profile isolation.
