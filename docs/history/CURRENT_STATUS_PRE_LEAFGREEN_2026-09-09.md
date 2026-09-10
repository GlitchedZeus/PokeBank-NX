# PokeBank NX — Current Verified Engineering State

## Corrected FRLG physical-test artifact — READY FOR DEVICE TEST

```text
Starting recovery/documentation SHA: c7e410d4d168d1173c9bd9e3ceb38489c865d404
Canonical application source: ea0b806bac4acdb5619f22f9841d616ea8a237ff
Canonical application tree: ed5912093886384894c44538d569fe4955fd2e47
Application commit: legacy: bind FRLG sources to profiles and expose diagnostics
Application source changed during packaging: NO
Embedded version/source: 0.1.0-alpha / ea0b806b

Artifact: PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro
Artifact byte size: 156592377
Artifact SHA-256: 396f8ff9f4da53b5449aeb46b8d1237ca9358a0ac94998680017575916b6b1ee
ZIP: PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.zip
ZIP byte size: 149672676
ZIP SHA-256: 255ec20cd0f7965c9b25123cb83eedd0cebc83d10fd88313bcff672a0c1ced67
Manifest: PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro.manifest.txt
Manifest byte size: 900
Manifest SHA-256: a79ede73124f74ce4a6c85d6c151bf7f3e9ca732747a332caa68750626058952
SHA256SUMS: SHA256SUMS.txt
SHA256SUMS byte size: 352
SHA256SUMS SHA-256: d28ee7266c58b3cab2716211a0425b0af6562dab3f9d6768ac299b77d8342f55

HD renders: 3260 / 3260
Base species: 1025 / 1025
Type icons: 18 / 18
Fonts: 3 / 3
FRLG GBA cards: PASS
Asset preflight: PASS
Embedded RomFS: PASS — 3283 / 3283 files byte-identical, 148076683 bytes
Native devkitA64 -fno-exceptions build: PASS
Previously verified host tests: PASS — 13 suites
Previously verified ASan/UBSan: PASS
Previously verified git diff --check: PASS
GitHub CI for application source: PASS — run #245
GitHub prerelease published: NO — unavailable authentication/tooling
DEVICE TESTED: NO
DEVICE ACCEPTED: NO
```

The exact NRO, ZIP, manifest and checksum record were preserved outside the temporary build tree. Large artifacts were not committed to Git history. Parked RSE checkpoint `b5ef83b` remains isolated and untouched; `1a921515` was unavailable locally, with `b5ef83b` retained as the verified equivalent recovery state.


## Emergency save-mode checkpoint — 2026-09-09

```text
Canonical application source: ea0b806bac4acdb5619f22f9841d616ea8a237ff
Canonical tree: ed5912093886384894c44538d569fe4955fd2e47
Exact-source staging tree: reconstructed and verified clean
Pinned HD renders restored so far: 2672 / 3260
Final NRO packaged: NO
GitHub prerelease published: NO
DEVICE TESTED: NO
DEVICE ACCEPTED: NO
```

The resumable sprite generator was restoring only missing pinned renders when save mode was requested. Preserve/reuse the generated files; do not delete or restart them. Finish 3260/3260, run asset preflight, build exact `ea0b806b`, compare embedded RomFS byte-for-byte, then package fresh NRO/ZIP/manifest hashes. Parked RSE checkpoint `b5ef83b` remains isolated and untouched.


Last updated: 2026-09-08

This file is the short authoritative engineering handoff. The root README is human-facing product/roadmap information only. Detailed active work instructions live in `docs/NEXT_CODEX_PROMPT.md`.

## Current state — FRLG source fix published; assets complete; exact device artifact build incomplete

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

Both failures were addressed in source and published as:

```text
ea0b806bac4acdb5619f22f9841d616ea8a237ff
legacy: bind FRLG sources to profiles and expose diagnostics
```

This is the canonical application source for the next physical-test artifact.

Reported implementation in `ea0b806b`:

- removed the unsupported/arbitrary `Main Save` guess;
- multiple valid FRLG physical files remain distinct save instances;
- source children expose truthful filename/source information;
- newest modification time is used only for deterministic initial focus when several genuine files exist;
- Source Details expose provider, normalized physical path, size, modification state, stable identity, SHA-256 prefix/fingerprint, exact game ID, decoded trainer, and party count;
- physical legacy discovery/catalog remains shared/app-global;
- normal source visibility is profile-scoped;
- unassigned legacy saves stay hidden from normal per-profile source lists;
- explicit assignment binds a selected source to the current Nintendo/PokeBank profile;
- bindings persist atomically across restart/reload;
- Profile A's bound source remains absent from Profile B;
- aliases do not duplicate bindings;
- genuinely distinct physical saves can be assigned independently;
- installed Switch-title account scoping remains separate;
- RetroArch and installed-game source writes remain hard disabled.

Reported stale-save root cause:

```text
multiple valid FRLG files
-> filename sort
-> first child auto-focused
-> first child falsely labeled "Main Save"
```

There was no evidence that the first filename-sorted save was RetroArch's active gameplay save.

## Verification already completed for ea0b806b

Reported green for the exact application source:

```text
Host tests: 13 suites PASS
ASan/UBSan: PASS
git diff --check: PASS
Native Switch -fno-exceptions compile/link: PASS
GitHub CI run #245: PASS
```

Regression coverage reportedly includes:

- old one-Pokémon FRLG copy + newer two-Pokémon FRLG copy;
- truthful filename/party/fingerprint source diagnostics;
- two-profile persistent source binding where Profile A's source is absent from Profile B after reload.

Unless application source changes after `ea0b806b`, do not rerun expensive full sanitizer/PKSM-Core verification solely to package the same source. Reuse the completed verification record and only rerun what is materially required to create/verify the exact artifact. If application code changes, rerun the required verification for the changed source.

## Latest packaging progress before credits ended

A later continuation reconstructed an isolated build tree from canonical GitHub source and reported:

```text
Canonical application source: ea0b806bac4acdb5619f22f9841d616ea8a237ff
Canonical tree: ed5912093886384894c44538d569fe4955fd2e47
Native include/src/Makefile source mismatches: 0
GitHub CI run #245: PASS
Host regression suite: PASS
```

It intentionally did not repeat the already-recorded expensive sanitizer pass.

The full pinned asset gate then completed:

```text
HD renders: 3260 / 3260
Base species: 1025 / 1025
Type icons: 18 / 18
Fonts: 3 / 3
Shiny/form coverage: PASS
FRLG GBA card artwork: PASS
Asset preflight: PASS
```

The clean devkitA64 `-fno-exceptions` build tied to embedded commit `ea0b806b` was started, but credits ended before the final NRO/package identity was reported.

Therefore current status is:

```text
SOURCE FIX PUBLISHED: YES
SOURCE STAGING VERIFIED AGAINST CANONICAL GITHUB TREE: YES
SOFTWARE VERIFICATION REPORTED GREEN: YES
FULL PINNED ASSET RESTORE COMPLETE: YES
ASSET PREFLIGHT: PASS
EXACT ea0b806b NATIVE BUILD STARTED: YES
NEW EXACT NRO PACKAGED/RECORDED: NO
DEVICE TESTED FOR ea0b806b: NO
DEVICE ACCEPTED: NO
```

## Immediate next milestone

Do not reopen the already-fixed bugs or redo completed asset restoration unless current local state proves it was lost.

Continue only from `ea0b806b` and finish:

```text
preserve surviving recovery/build/asset state
        ↓
reuse completed 3260-render asset tree if present
        ↓
finish or restart only the clean exact-source native build
        ↓
verify embedded RomFS against intended asset tree
        ↓
package exact NRO + ZIP fallback/manifest if supported
        ↓
record exact sizes + SHA-256 + embedded source identity
        ↓
update minimal handoff docs
        ↓
STOP for physical Switch retest
```

Do not rerun full ASan/UBSan/PKSM-Core merely because a new session starts. Only rerun it if application source changes or an actual verification defect is discovered.

The next physical test must prove:

```text
A. the user's current two-Pokémon FRLG save can be identified and opened using truthful Source Details; the old level-6 Charmander copy is not silently presented as "Main Save"

B. a FRLG legacy source assigned to the user's profile is absent from the niece's profile, including after application restart
```

Do not begin Ruby/Sapphire/Emerald until both pass physically.

## Repository / safety

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream-only remote: kiasta/PKSE
```

Before any sync/reset/clean/restore/ref/worktree change, preserve useful local/uncommitted/recovery/generated state first.

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
