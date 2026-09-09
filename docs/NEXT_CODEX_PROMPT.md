# PokeBank NX — Authoritative Next Codex Prompt

> **FRLG FIX SOURCE AND FULL ASSET GATE ARE COMPLETE — FINISH ONLY THE EXACT DEVICE-TEST BUILD/PACKAGE**

Use HIGH reasoning.

## Canonical application source

The two device-proven FRLG failures have already been fixed in source and published as:

```text
ea0b806bac4acdb5619f22f9841d616ea8a237ff
legacy: bind FRLG sources to profiles and expose diagnostics
```

Canonical tree:

```text
ed5912093886384894c44538d569fe4955fd2e47
```

This exact application source is the target for the next physical-test artifact unless a real build/package defect forces an application-code change.

The old rejected artifact remains:

```text
5d3e5e23f352dda4900ae42b4f396d9d4a4b8b8e
PokeBank-NX-FRLG-Complete-5d3e5e23.nro
SHA-256 8dd94277e609c96f37e82b0b0b47928bad50ce36fdb380520bd2521ab18ec78a
DEVICE TESTED: YES
DEVICE ACCEPTED: NO
```

Do not call that old artifact untested or accepted.

## Preserve first

Before syncing, resetting, cleaning, restoring, switching refs, rebasing, replacing worktrees, deleting generated assets, or doing anything that could discard local state:

- inspect all worktrees;
- inspect `git status`;
- preserve useful uncommitted/generated/recovery/build state;
- preserve the completed/restored pinned asset tree if it survived;
- preserve parked RSE refs including `b5ef83b`, `1a921515`, or equivalents.

Do not merge, resume, reimplement, or push RSE.

The README is human-facing roadmap/product information only and must NOT expand the active task.

Push only to:

```text
origin/feature/pokebank-playable
```

Never push custom PokeBank NX code upstream to PKSE.

# Already completed — DO NOT REDO WITHOUT CAUSE

## Source fix

`ea0b806b` reportedly includes:

- removal of the false/arbitrary `Main Save` label;
- distinct physical FRLG files retained as distinct save instances;
- truthful filename/source information;
- deterministic newest-mtime initial focus only, without claiming that file is the active RetroArch save;
- Source Details with provider, normalized path, size, modification state, stable identity, short SHA-256/fingerprint, exact game ID, trainer, and party count;
- shared physical discovery/catalog with profile-scoped visibility;
- unassigned legacy saves hidden from normal per-profile lists;
- explicit assignment to the current Nintendo/PokeBank profile;
- persistent source/profile bindings;
- Profile A source absent from Profile B;
- aliases not duplicating bindings;
- installed Switch-title account scoping preserved;
- all source writes still hard disabled.

Reported stale-save root cause:

```text
multiple valid FRLG files
-> filename sort
-> first child auto-focused
-> first child falsely labeled "Main Save"
```

## Verification already completed for unchanged ea0b806b

The exact published application source already has this reported verification:

```text
Host tests: 13 suites PASS
ASan/UBSan: PASS
git diff --check: PASS
Native Switch -fno-exceptions compile/link: PASS
GitHub CI run #245: PASS
```

A later continuation reconstructed a fresh isolated build tree from GitHub and verified all native `include/`, `src/`, and Makefile content against canonical tree `ed591209...` with **zero source mismatches**. Its host regression suite also passed.

Do NOT burn another session rerunning full sanitizer/PKSM-Core verification merely because the session restarted.

If application source remains exactly `ea0b806b`:

- reuse the verification record above;
- do not re-debug the stale-save/profile-binding fixes;
- do not rerun expensive ASan/UBSan/PKSM-Core unless a real defect makes it necessary;
- perform only the build/package/integrity work required for the exact artifact.

If application code changes after `ea0b806b`, then rerun the verification appropriate to the changed source before packaging.

# Full pinned asset gate is already complete

The previous continuation reported successful completion of the asset restore/preflight:

```text
HD Pokémon renders: 3260 / 3260
Base species: 1025 / 1025
Type icons: 18 / 18
Fonts: 3 / 3
Shiny/form coverage: PASS
FireRed/LeafGreen GBA card artwork: PASS
Asset preflight: PASS
```

The clean exact-source devkitA64 `-fno-exceptions` build with embedded commit `ea0b806b` was started when credits ended.

Therefore:

- inspect whether that exact build completed or left reusable outputs;
- inspect whether the complete asset tree survived;
- resume/reuse those outputs when trustworthy;
- do not delete/re-download/re-restore all 3,260 renders simply because a new session started;
- if workspace maintenance removed them, restore only what is actually missing using the established pinned process.

# Single mission — finish exact physical-test artifact

Do only this:

1. Preserve surviving build, asset, and recovery state.
2. Verify the application source used for the artifact is exactly `ea0b806bac4acdb5619f22f9841d616ea8a237ff`.
3. Reuse the completed 3,260-render asset tree if present; otherwise restore only missing/corrupt assets.
4. Finish or restart only the clean exact-source devkitA64 `-fno-exceptions` native build.
5. Verify embedded source/version identity is `ea0b806b`.
6. Verify the built RomFS is byte-identical to the intended complete asset tree using the repository's established method.
7. Produce the exact physical-test `.nro`.
8. Produce ZIP fallback and build manifest if supported by the established release flow.
9. Calculate exact byte sizes and SHA-256 hashes.
10. Record minimal engineering handoff documentation with exact artifact identity.
11. STOP for physical device testing.

Do not do new feature development.

# Preserve existing functionality

Do not regress or broadly rewrite:

- strict FRLG save validation;
- FireRed / LeafGreen identities and GBA artwork;
- Party and all 14 Boxes;
- Pokémon View;
- trainer metadata and decrypted money;
- six read-only inventory pouches;
- refresh/rescan;
- alias dedupe;
- truthful Source Details diagnostics;
- persistent per-profile source bindings;
- installed-title account scoping;
- malformed-source safety;
- source-byte immutability;
- installed-game and RetroArch live-write locks.

# Strictly out of scope

Do NOT begin:

- Ruby/Sapphire/Emerald;
- Master/Profile Vault beyond the already-published minimal source binding;
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
canonical tree SHA = ed5912093886384894c44538d569fe4955fd2e47
application commit message
whether application source changed during packaging
verification record reused and/or rerun
CI status
HD render count
base species count
asset preflight result
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
4. The old level-6 Charmander copy is not silently presented as "Main Save"; if it remains as a genuine separate file, it is truthfully identifiable as a separate child.
5. Restart PokeBank NX and confirm the binding persists.
6. Switch to the niece's profile and confirm the user's assigned FRLG save is absent.
7. Switch back to the user's profile and confirm it remains assigned/visible.
8. Trainer, Items, Party, Boxes 1-14, Pokémon View, refresh, and read-only locks still work.
```

Do not begin RSE until the user physically accepts both current-save selection behavior and profile isolation.
