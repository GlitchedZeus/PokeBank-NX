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

Physical testing proved Party/Boxes/View basically work, but this artifact is **NOT DEVICE-ACCEPTED** because of five integration/UI blockers/requirements.

## Preserve first

Before syncing/resetting/cleaning/restoring/changing refs, preserve useful local state. A previous interrupted RSE checkpoint may exist locally at `1a921515`; preserve it if present but do not resume/merge/push RSE in this session.

Push custom code only to:

```text
origin / feature/pokebank-playable
```

Never push custom code upstream to `kiasta/PKSE`.

## Product hierarchy and refresh requirements

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
       -> refresh current save-instance catalog
       -> Save Instances
            -> WILL — Main Save
            -> WILL — Save State 0   [future]
            -> WILL — Save State 1   [future]
            -> WILL — Save State 2   [future]
            -> WILL — Save State 3   [future]
            -> WILL — Backup         [future]
       -> choose one
       -> trainer/source view
       -> Party / Boxes
```

Likewise one LeafGreen parent card.

Current production support remains **BATTERY_SAVE only** (`.sav` / `.srm`). Do not implement `.state#` parsing in this blocker-fix session. The hierarchy must be future-proof for SAVE_STATE slots 0-3 and BACKUP children.

## Single mission: fix all five FRLG device blockers/requirements

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

### Blocker C — FireRed/LeafGreen game artwork missing

Observed on physical Switch: the RetroArch FireRed and LeafGreen top-level game cards did not show the expected game artwork.

Required behavior:

- FireRed GBA RetroArch parent card shows the correct FireRed game artwork using the project's existing asset/card-art system;
- LeafGreen GBA RetroArch parent card shows the correct LeafGreen game artwork;
- do not accidentally use the separate official Switch FireRed/LeafGreen artwork/identity if those assets differ;
- preserve `firered_gba` / `leafgreen_gba` as the source identities driving artwork lookup;
- use existing asset infrastructure rather than an ad-hoc loader;
- missing artwork fails gracefully, but the next device build must resolve both FRLG GBA artworks.

### Blocker D — legacy catalog can become stale / needs refresh

Observed during physical use: after playing in RetroArch and checking another save/state context, the visible PokeBank data can represent an older snapshot. The current UIManager/session catalog must not behave as if all legacy sources are immutable for the lifetime of the app.

Current scope is still `.sav` / `.srm` BATTERY_SAVE only, but implement refresh semantics now so the architecture works for future state slots 0-3.

Required behavior:

1. keep normal bounded scan at app startup;
2. when a RetroArch game parent card is opened, perform a lightweight refresh of the approved active source root for that game before showing child save instances;
3. expose a manual **Refresh Saves** / **Rescan Sources** action in the game-card/save-instance view;
4. changed child sources invalidate cached read models using stable source identity plus useful file metadata such as normalized path, size and modification state where available;
5. after refresh, selecting a changed child rereads/revalidates it through the same strict FRLG path before Party/Boxes is exposed;
6. do not hot-swap the selected source while already inside Party/Boxes; refresh only at a clear boundary or explicit action;
7. deleted/missing sources disappear safely after refresh and stale selections fail gracefully;
8. all refresh behavior remains read-only.

Important product semantics:

- a refresh does **not** make an old RetroArch save state newer;
- future Save State 0/1/2/3 children are frozen snapshots and may legitimately differ from the main battery save;
- if the user overwrites a state slot in RetroArch, a future SAVE_STATE adapter should detect the replacement after refresh;
- actual `.state#` parsing remains out of scope for this session.

### Blocker E — scanner can surface stale saves RetroArch no longer uses

Physical observation: PokeBank appears to surface old FRLG save copies the user does not currently load in RetroArch.

Code inspection confirms current `discoverConfiguredRetroArchFRLGSaves()` behavior adds both:

```text
savefile_directory from sdmc:/retroarch/retroarch.cfg
+ sdmc:/retroarch/cores/savefiles when that conventional folder exists
```

This is the likely source of stale historical copies and duplicate-looking saves.

Required root-precedence behavior:

1. a valid, non-default configured `savefile_directory` is authoritative for BATTERY_SAVE discovery;
2. `sdmc:/retroarch/cores/savefiles` is fallback-only when there is no usable configured save directory;
3. do not scan the conventional path additively merely because it exists;
4. preserve bounded traversal beneath the selected active root;
5. retain normalized source path and useful metadata for diagnostics and refresh;
6. during host/device verification, report which physical source path was selected so stale-copy bugs are diagnosable;
7. never silently prefer an older copy outside the active configured root.

This root-selection fix is distinct from future `.state0`-`.state3` parsing.

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

This remains strictly read-only. Do not implement/enable RetroArch writeback, repair/resign, installed-title writes, edit-to-source, clone-to-save, Move, conversion, Master Vault/Banks, RSE, Gen I/II, events/mystery gifts, physical-link hardware, or arbitrary save-state parsing.

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
- configured `savefile_directory` wins over an existing stale conventional root;
- conventional root is used only as fallback when configured root is absent/unusable;
- genuinely separate save files in the active root remain separate children;
- FireRed/LeafGreen GBA artwork lookup resolves correctly;
- opening a parent card refreshes current `.sav` / `.srm` child discovery;
- manual Refresh Saves/Rescan Sources works;
- a changed battery save invalidates/rebuilds its cached read model;
- deleted/missing sources disappear/fail safely;
- selected child diagnostics expose the physical path/metadata needed to verify the source;
- FireRed and LeafGreen identities remain exact and distinct from Switch releases;
- Party/Boxes/View still work;
- source bytes remain unchanged;
- blocked actions remain blocked.

## Checkpoint and device artifact

Commit/push coherent source work early to `origin/feature/pokebank-playable` only.

Suggested commit concept:

```text
gen3: fix RetroArch source selection grouping refresh and artwork
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
active RetroArch save root selected
FRLG GBA artwork lookup/result
refresh/rescan behavior
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
-> only saves from RetroArch's active configured battery-save root are listed
-> open game card
-> child save list refreshes from disk
-> manual Refresh Saves works
-> choose the desired Main Save child
-> confirm trainer/Party/Boxes match the save RetroArch actually uses
-> Party works
-> Boxes 1-14 work
-> View Pokémon works
-> Edit/Clone/Transfer/Move/Save/writeback blocked
```

Do not begin Ruby/Sapphire/Emerald until the user physically accepts the new artifact.
