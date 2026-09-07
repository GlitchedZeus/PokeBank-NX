# PokeBank NX — Next Codex Prompt

Use this file as the authoritative task prompt for the next coding session.

## Launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

## Current verified application source

```text
f6a3052daeffe7cd30d7acceba81a5dfda7615ee
gen3: expose RetroArch FRLG game sources
```

Device-tested artifact that exposed the current blockers:

```text
NRO size:    155174825 bytes
NRO SHA-256: 809c94c842a3c385d23907c61e5ecfa201b24f08e8ac935e87a4add60770282d
Assets:      3260 HD renders / 1025 base species / 3281 RomFS files verified
```

Physical testing proved Party/Boxes/View basically work, but this artifact is **NOT DEVICE-ACCEPTED** because of three integration/UI blockers.

## Preserve first

Before syncing/resetting/cleaning/restoring/changing refs, preserve useful local state. A previous interrupted RSE checkpoint may exist locally at `1a921515`; preserve it if present but do not resume/merge/push RSE in this session.

Push custom code only to:

```text
origin / feature/pokebank-playable
```

Never push custom code upstream to `kiasta/PKSE`.

## Product hierarchy requirement

Read and follow:

```text
docs/RETROARCH_SOURCE_NAMING.md
```

The intended UX is **one top-level RetroArch card per game identity/source family**, with save instances beneath that card.

Required shape:

```text
Game Sources
  -> Pokémon FireRed
       Game Boy Advance · RetroArch
       -> Save Instances
            -> WILL — Main Save
            -> WILL — Save State 1   [future]
            -> WILL — Save State 2   [future]
            -> WILL — Save State 3   [future]
            -> WILL — Backup         [future]
       -> choose one
       -> trainer/source view
       -> Party / Boxes
```

Likewise one LeafGreen parent card.

Current production support remains **BATTERY_SAVE only** (`.sav` / `.srm`). Do not implement `.state#` parsing in this blocker-fix session. The hierarchy must be future-proof for SAVE_STATE/BACKUP children.

## Single mission: fix all three FRLG device blockers

### Blocker A — RetroArch is wrongly user/profile scoped

Observed on physical Switch: the user must switch to a RetroArch-like account/profile to see FRLG sources.

Required architecture:

```text
installed Switch save source
    -> Nintendo user/account scoped when required

RetroArch / file / legacy source
    -> app-global
    -> visible regardless of selected Nintendo user
```

Do not model RetroArch itself as a Switch user. Preserve installed-title account handling.

### Blocker B — duplicate top-level FireRed/LeafGreen cards

Observed:

```text
2 FireRed cards
3 LeafGreen cards
```

Desired model:

```text
one FireRed RetroArch parent card
one LeafGreen RetroArch parent card
```

Distinct validated `.sav` / `.srm` save instances belong **inside** the matching parent card.

Rules:

- collapse aliases/duplicate discovery of the same underlying file inside the child list;
- preserve genuinely separate save files as separate children, even if bytes match;
- never dedupe children merely by game ID, trainer name, or file hash alone;
- normalize/deduplicate overlapping roots/path aliases where appropriate;
- keep strict FRLG validation before a child becomes selectable;
- if trainer name is reliably available, use it in the child label; otherwise do not invent one.

Examples:

```text
Pokémon FireRed
Game Boy Advance · RetroArch
  -> WILL — Main Save
  -> WILL — Backup 1
```

Future save-state children must fit naturally beneath the same card, but actual `.state#` parsing is out of scope now.

### Blocker C — FireRed/LeafGreen game artwork missing

Observed on physical Switch: the RetroArch FireRed and LeafGreen top-level game cards did not show the expected game artwork.

Required behavior:

- FireRed GBA RetroArch parent card shows the correct FireRed game artwork using the project's existing asset/card-art system;
- LeafGreen GBA RetroArch parent card shows the correct LeafGreen game artwork;
- do not accidentally use the separate official Switch FireRed/LeafGreen artwork/identity if those assets differ;
- preserve `firered_gba` / `leafgreen_gba` as the source identities driving artwork lookup;
- use existing asset infrastructure rather than adding an ad-hoc image loader solely for RetroArch;
- missing artwork must fail gracefully, but the new device-test build should include/resolve both required FRLG GBA artworks.

Add a focused test or deterministic lookup check where practical so the GBA identities resolve to non-empty/valid art keys/assets.

## Preserve what already works

Do not redo or regress:

- PKSM-Core Gen III host oracle;
- exception-free native Gen III backend;
- bounded `.sav` / `.srm` scanning;
- strict FRLG structural validation;
- GBA vs Switch FRLG identity separation;
- Party and all 14 Boxes read model;
- exact validated catalog-entry routing;
- read-only action policy;
- accepted broad UI identity.

## Safety

This remains strictly read-only. Do not implement/enable RetroArch writeback, repair/resign, installed-title writes, edit-to-source, clone-to-save, Move, conversion, Master Vault/Banks, RSE, Gen I/II, events/mystery gifts, or physical-link hardware.

## Verification

Run:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Native application must remain `-fno-exceptions`.

Specifically prove where practical:

- legacy sources are visible independent of Nintendo user selection;
- installed-title user scoping still works;
- exactly one top-level FireRed RetroArch parent card and one LeafGreen parent card are produced when present;
- overlapping roots/path aliases do not duplicate child save instances;
- genuinely separate save files remain separate children;
- FireRed/LeafGreen GBA artwork lookup resolves correctly;
- FireRed and LeafGreen identities remain exact and distinct from Switch releases;
- Party/Boxes/View still work;
- source bytes remain unchanged;
- blocked actions remain blocked.

## Checkpoint and device artifact

Commit/push coherent source work early to `origin/feature/pokebank-playable` only.

Suggested commit concept:

```text
gen3: fix RetroArch grouping scope and artwork
```

Update issue #6 and minimal status/handoff docs after verification.

Then package a **new exact full-asset device-test NRO** using the complete pinned 3260-render set.

Report:

```text
application/source SHA
NRO filename
NRO size
NRO SHA-256
asset render count
FRLG GBA artwork lookup/result
host tests
ASan/UBSan
git diff --check
native build
CI/status
DEVICE TESTED: NO
```

Then STOP for physical retest.

Required retest:

```text
launch under normal Nintendo user
-> FireRed / LeafGreen RetroArch game cards visible
-> exactly one top-level card per game
-> correct FireRed / LeafGreen artwork visible
-> open game card
-> choose the desired save instance
-> Party works
-> Boxes 1-14 work
-> View Pokémon works
-> Edit/Clone/Transfer/Move/Save/writeback blocked
```

Do not begin Ruby/Sapphire/Emerald until the user physically accepts the new artifact.
