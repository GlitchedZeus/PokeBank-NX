# PokeBank NX — Authoritative Next Codex Prompt

> **FRLG PHYSICAL RETEST FAILED — FIX ONLY THE TWO DEVICE-PROVEN BLOCKERS BELOW**

Use HIGH reasoning.

## Starting remote state

The branch was previously gated for physical testing at:

```text
Documentation head: 02e0367cf8ae4af8f6a741fd0778639c0c1f7646
Application source: 5d3e5e23f352dda4900ae42b4f396d9d4a4b8b8e
Commit: gen3: complete FRLG read-only source browsing
Artifact: PokeBank-NX-FRLG-Complete-5d3e5e23.nro
NRO size: 156249001 bytes
NRO SHA-256: 8dd94277e609c96f37e82b0b0b47928bad50ce36fdb380520bd2521ab18ec78a
```

That exact artifact has now been physically tested.

```text
DEVICE TESTED: YES
DEVICE ACCEPTED: NO
```

Do not describe the 5d3e5e23 artifact as untested or accepted.

Before syncing, resetting, cleaning, restoring, switching refs, rebasing, changing worktrees, or doing anything that could discard local state, inspect and preserve all local/uncommitted/recovery work.

Preserve parked RSE recovery work including `b5ef83b`, `1a921515`, or equivalent refs if present. Do not merge, resume, reimplement, or push RSE in this session.

The README is human-facing product/roadmap information only and must not expand this task.

Push coherent checkpoints only to:

```text
origin/feature/pokebank-playable
```

Never push custom PokeBank NX code upstream to PKSE.

# Physical observations from 5d3e5e23

## BLOCKER 1 — PokeBank is still reading an OLD FRLG save

The user's current FireRed/LeafGreen gameplay save contains **2 Pokémon**.

PokeBank instead still shows an older save state containing the previously observed **level 6 Charmander**.

Therefore the claimed active-root/refresh fix is not physically correct yet. Host tests proving configured-root precedence are insufficient if the device still resolves the wrong physical battery save.

This must be debugged from the actual source-selection path, not papered over by changing the displayed trainer/Pokémon data.

### Required outcome

For a live RetroArch FRLG battery save, PokeBank must open the exact physical `.sav` / `.srm` file RetroArch is currently using for that game.

Do not silently choose:

- a stale copy in another directory;
- a fallback-root copy when the configured active root is usable;
- an older duplicate inside an overlapping route;
- a cached parsed model after the physical file changed;
- an alias whose canonical target is not the active file.

### Required investigation

Inspect the complete RetroArch source-resolution chain on Switch, including where appropriate:

- `sdmc:/retroarch/retroarch.cfg` parsing;
- configured `savefile_directory` semantics;
- quotes/whitespace/special/default values;
- relative or special RetroArch path values if supported by the existing implementation;
- per-core/per-content directory behavior already represented in the repository;
- `.sav` versus `.srm` candidates;
- directory traversal and duplicate/alias handling;
- candidate ordering;
- cache keys and refresh invalidation;
- parent-open refresh;
- manual `X — Refresh Saves`;
- whether the selected child keeps an obsolete model after refresh;
- whether multiple valid FRLG files inside the authoritative root are being mislabeled or auto-selected.

Do not invent behavior unsupported by RetroArch. If the active file cannot be uniquely inferred from configuration alone, preserve multiple genuine physical save instances as children and make the UI identify them truthfully rather than silently selecting an arbitrary one.

### Mandatory on-device diagnostics for the next artifact

The next physical-test build must make it possible to identify the exact source PokeBank opened. At minimum expose in an existing diagnostics/source-details surface, or a minimal non-invasive equivalent:

```text
provider = RetroArch
normalized physical path
file size
modification time/state where available
stable source identity
short content fingerprint/hash
selected game identity
trainer name if decoded
party count
```

Do not clutter the normal Pokémon UI permanently if an existing diagnostics/details surface can carry this information.

For the user's current save, the next test must allow us to prove that the file opened by PokeBank is the same current save that contains 2 Pokémon, not the old level 6 Charmander save.

### Selection rules

- configured usable active root remains authoritative;
- conventional `sdmc:/retroarch/cores/savefiles` remains fallback-only;
- never scan both additively merely because both exist;
- multiple genuinely distinct valid saves inside the authoritative root remain distinct child instances;
- aliases of the same underlying physical file collapse;
- do not dedupe by trainer name or Pokémon-content hash alone;
- do not automatically relabel an old copy as the current Main Save;
- refresh must invalidate and reread the selected physical source at a safe navigation boundary;
- deleted/missing sources disappear safely;
- all behavior remains strictly read-only.

Add deterministic host tests for whatever real bug is found, including a fixture/layout with a stale old FRLG copy and a newer/current FRLG copy so the regression cannot return.

# BLOCKER 2 — GBA legacy saves are visible under BOTH Nintendo profiles

Physical result:

- the user's FRLG GBA legacy saves appear under the user's Nintendo profile;
- the same GBA legacy saves also appear under the user's niece's Nintendo profile.

This is not the desired product behavior.

The previous phrase **"RetroArch/file sources are app-global"** must now be interpreted correctly:

```text
physical discovery/catalog = app-global
profile visibility/ownership binding = profile-scoped
```

RetroArch itself is not a Nintendo user, but a discovered legacy save must not automatically appear in every profile's normal game-source library.

## Required profile/source model

Maintain one shared physical source catalog, then bind legacy save instances to a PokeBank/Nintendo profile.

Conceptually:

```text
shared RetroArch discovery
        ↓
physical save identity
        ↓
profile source binding
        ↓
only assigned profile sees it in normal Game Sources
```

Do NOT infer ownership from trainer name, TID, Pokémon contents, or save hash.

A physical save may eventually be explicitly Shared, but shared visibility must be deliberate rather than the default.

## Immediate FRLG behavior required

Implement the smallest clean profile-binding behavior consistent with the existing architecture and future profile-scoped Vault design.

Required properties:

- source discovery remains shared/app-global so the filesystem is not rescanned independently per Nintendo account;
- binding/visibility is keyed by stable Nintendo/PokeBank profile identity plus stable physical source identity;
- a bound FRLG save appears only for the profile it is assigned to;
- switching to the niece's profile must not show the user's FRLG saves by default;
- switching back to the user's profile restores the user's assigned FRLG sources;
- bindings survive application restart;
- path aliases for the same physical source do not create multiple bindings;
- genuinely separate physical saves can be assigned independently;
- installed Switch-title save account scoping must not regress;
- no direct cross-profile game-save access is introduced;
- no Vault/Gift/Trade feature is implemented here.

If an explicit first-assignment prompt/flow is needed because a filesystem save cannot intrinsically identify its Nintendo-profile owner, implement a minimal safe assignment flow rather than guessing ownership. New/unassigned legacy saves must not become visible to every profile merely because they exist on the SD card.

A future shared-source option may be represented in the data model, but do not build broad sharing UI in this milestone.

Add host tests covering at least two Nintendo/PokeBank profiles and proving a legacy FRLG source bound to Profile A is absent from Profile B's normal source list.

# Preserve the parts that did pass

Do not regress or broadly rewrite:

- strict FRLG slot/sector/checksum validation;
- Party browsing;
- all 14 Boxes;
- Pokémon View;
- trainer name/gender/TID/SID/ID32/money decoding;
- all six read-only inventory pouches;
- FireRed/LeafGreen separate parent identity;
- FireRed/LeafGreen GBA card artwork;
- canonical alias dedupe;
- manual refresh control;
- malformed-save safety;
- source byte immutability;
- native `-fno-exceptions` build;
- installed-title live-write lock;
- RetroArch writeback lock.

# Strictly out of scope

Do NOT begin:

- Ruby/Sapphire/Emerald;
- Master/Profile Vault implementation beyond the minimal legacy-source binding needed for this blocker;
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
- UI redesign.

# Verification

Run:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Native app remains `-fno-exceptions`.

Specifically prove:

1. stale-copy regression fixture selects/exposes the correct authoritative physical source behavior;
2. configured root is truly authoritative and fallback-only behavior remains correct;
3. refresh rebuilds from changed physical bytes;
4. next artifact exposes enough source diagnostics to identify exact path/fingerprint/party count;
5. Profile A legacy binding is not visible in Profile B;
6. bindings persist across restart/reload;
7. aliases do not duplicate bindings;
8. installed-title account scoping is unaffected;
9. trainer/items/party/boxes still pass;
10. source bytes remain unchanged;
11. all source writes remain blocked.

# Checkpoint / packaging

Push a coherent application checkpoint early after the two fixes and tests are green.

Then restore/verify the pinned full artwork set and build a new exact physical-test NRO.

Final report must include:

```text
starting remote SHA
application/source SHA
what caused the stale level-6-Charmander source to be selected
exact corrected source-selection rule
profile-binding storage/model used
profile A / profile B visibility test result
host tests
ASan/UBSan
git diff --check
native build
CI
asset counts
NRO filename
NRO size
NRO SHA-256
DEVICE TESTED: NO
```

Then STOP for physical testing.

Do not begin RSE until the user physically confirms both:

```text
A. current FRLG save with 2 Pokémon is the save PokeBank opens
B. user's GBA legacy saves are absent from niece's profile unless deliberately assigned/shared
```
