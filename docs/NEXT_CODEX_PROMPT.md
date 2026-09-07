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

Physical testing proved strict FRLG Party/Boxes/View basically work, but this artifact is **NOT DEVICE-ACCEPTED**. Fix the seven FRLG integration/read-model blockers below before RSE.

## Preserve first

Before syncing/resetting/cleaning/restoring/changing refs, preserve useful local state. A previous interrupted RSE checkpoint may exist locally at:

```text
1a921515
```

Preserve it if present, but do not resume/merge/push/reimplement RSE in this session.

Push custom code only to:

```text
origin / feature/pokebank-playable
```

Never push PokeBank NX custom code upstream to `kiasta/PKSE`.

Read and follow:

```text
docs/RETROARCH_SOURCE_NAMING.md
```

Current production support remains BATTERY_SAVE only (`.sav` / `.srm`). Future RetroArch Save State slots are **0, 1, 2, 3** and belong as labeled children beneath the game card, but actual `.state#` parsing is out of scope for this blocker-fix session.

# Single mission — fix all seven FRLG device blockers

## A — RetroArch is wrongly Nintendo-user/profile scoped

Observed: the user must switch to a RetroArch-like profile to see FRLG legacy sources.

Required architecture:

```text
installed Switch title save -> Nintendo user scoped when required
RetroArch/file/legacy save   -> app-global
```

RetroArch must not be modeled as a Switch user. Legacy cards must be visible under the user's normal Nintendo profile. Preserve installed-title account scoping.

## B — wrong top-level hierarchy / duplicate game cards

Observed:

```text
2 FireRed top-level cards
3 LeafGreen top-level cards
```

Required UX:

```text
Game Sources
  -> Pokémon FireRed
       Game Boy Advance · RetroArch
       -> Save Instances
            -> WILL — Main Save
            -> WILL — Save State 0   [future]
            -> WILL — Save State 1   [future]
            -> WILL — Save State 2   [future]
            -> WILL — Save State 3   [future]
            -> WILL — Backup         [future]
       -> choose one
       -> trainer/source view
       -> Party / Boxes / Items / Trainer
```

Likewise exactly one LeafGreen parent card.

For current `.sav/.srm` children:

- collapse aliases/duplicate discovery of the same underlying file;
- preserve genuinely separate physical save files as separate children even if bytes match;
- never dedupe children merely by game ID, trainer name, or raw hash alone;
- normalize/deduplicate aliases/overlapping routes where appropriate;
- strict FRLG validation must happen before a child becomes selectable;
- use reliable trainer name in the child label when available; never invent it.

## C — FireRed/LeafGreen GBA artwork missing

Observed: RetroArch FireRed and LeafGreen top-level cards have no expected game artwork.

Required:

- `firered_gba` resolves to correct FireRed GBA card artwork;
- `leafgreen_gba` resolves to correct LeafGreen GBA card artwork;
- keep GBA identities/assets distinct from the separate official Switch FRLG identities;
- use the existing card-art/asset system, not an ad-hoc RetroArch image loader;
- next full-asset device build must visibly resolve both artworks.

## D — legacy catalog becomes stale / needs refresh

Legacy sources cannot be an immutable app-start snapshot.

Required:

1. bounded scan at app start;
2. lightweight refresh of the active approved source root when opening a RetroArch game parent card;
3. manual **Refresh Saves** / **Rescan Sources** action in the parent/save-instance view;
4. changed child source invalidates the cached parsed/read model using stable source identity + useful file metadata such as normalized path, size and modification state;
5. selecting a changed child rereads and strictly revalidates it before exposing views;
6. do not hot-swap a source while already inside Party/Boxes/Items/Trainer; refresh at a clear boundary/action;
7. deleted/missing children disappear safely and stale selection fails gracefully;
8. all refresh behavior remains read-only.

Refresh does not make an old save-state snapshot newer. Future state-slot replacement detection belongs to the future SAVE_STATE adapter.

## E — scanner surfaces stale `.sav/.srm` copies RetroArch no longer uses

Code inspection confirms current discovery scans both the configured `savefile_directory` and `sdmc:/retroarch/cores/savefiles` additively.

Required precedence:

1. usable non-default `savefile_directory` from `sdmc:/retroarch/retroarch.cfg` is authoritative for BATTERY_SAVE discovery;
2. `sdmc:/retroarch/cores/savefiles` is fallback-only when no usable configured save directory exists;
3. do not scan the conventional path additively just because it exists;
4. preserve bounded traversal under the selected active root;
5. retain normalized physical path and useful metadata for diagnostics/refresh;
6. device/host report should identify which physical save path/root was selected;
7. never silently prefer an older copy outside the active configured root.

## F — FRLG trainer info is currently deliberately blank/zero

Physical observation in Trainer view:

```text
Name: (none)
Money: 0
Gender: correct
Trainer ID: missing/zero
Full TID: zero
Full SID: zero
```

Code inspection confirms `FRLGReadOnlyTrainer` currently clears trainer name and explicitly sets money, ID32, TID, SID, TID16 and SID16 to zero. The public `ReadOnlySave` metadata currently has no trainer-info record. This is a missing read-model feature, not corrupted device data.

Expand the **PokeBank-owned strict Gen III read-only API** so the validated save exposes the trainer fields needed by the existing UI without routing the source back through mutable/permissive `Trainer3FRLG` parsing.

Required FRLG read-only trainer values:

```text
trainer name (proper Gen III text decode)
gender
TID16
SID16
combined 32-bit trainer ID semantics / ID32 as used by the existing Trainer model
money (correctly de-obfuscated with the Gen III security key)
exact source game identity
```

Map these into `FRLGReadOnlyTrainer` so the existing Trainer panel shows truthful non-placeholder values. Preserve existing Gen III UI semantics: `Trainer ID` displays TID16/SID16; `Full TID` and `Full SID` must no longer be zero placeholders. Do not invent modern IDs that Gen III does not store.

Reference/cross-check the already-existing verified FRLG trainer offsets/semantics in `Trainer3FRLG` and the pinned PKSM-Core/PKHeX oracle, but keep the production source on the strict read-only adapter boundary.

Add deterministic host tests against the FRLG fixture for exact name, gender, TID/SID, ID32 and money, and prove source bytes are unchanged.

## G — Items view says `Invalid category`; FRLG inventory is not populated

Physical observation: opening the Items view for the FRLG RetroArch source displays:

```text
Invalid category
```

Code inspection confirms the Items panel requires `trainer.items[selectedCategory]`, while `FRLGReadOnlyTrainer` currently populates only Party/Boxes and leaves its item container empty.

The repo already has verified FRLG pouch definitions/offsets for:

```text
Items
Key Items
Poké Balls
TM Case
Berry Pouch
PC Items
```

Expand the strict Gen III read-only adapter to expose inventory records needed by the UI. Do not parse the raw selected file through the older mutable trainer path as a shortcut.

Required behavior:

- exactly the six FRLG containers above are available in the expected tab/category order;
- item IDs are exact Gen III item IDs expected by the existing FRLG naming path;
- item counts are correctly decoded;
- keyed bag pouch counts use the low 16 bits of the FRLG security key as required;
- PC Items remain handled according to their plaintext/non-keyed format;
- zero/empty slots do not become fake items;
- malformed/out-of-range inventory data fails safely;
- Items is strictly read-only for RetroArch legacy sources;
- `updateItemBlock()` remains inert and no source serializer/writeback is introduced.

Cross-check the already-verified `Inventory3FRLG` pouch definitions and existing `Trainer3FRLG::parseItems()` behavior against the pinned host oracle. Add fixture tests for pouch count/order, selected known item IDs/counts, security-key decoding and source immutability.

# Preserve what already works

Do not redo/regress:

- pinned PKSM-Core Gen III host oracle;
- exception-free native Gen III backend;
- strict slot/sector/checksum selection;
- bounded `.sav/.srm` scanning;
- strict FRLG structural validation;
- exact `firered_gba` / `leafgreen_gba` identity separation from Switch FRLG;
- Party and all 14 Boxes read model;
- exact validated catalog-entry routing;
- read-only source mutation policy;
- accepted broad UI identity.

# Safety

This entire milestone remains read-only.

Do **not** implement/enable:

- RetroArch writeback;
- repair/resign;
- installed-title writes;
- trainer/item editing back into source;
- clone-to-save;
- Move;
- conversion;
- Master Vault/Banks;
- RSE;
- Gen I/II;
- events/mystery gifts;
- physical-link hardware;
- arbitrary `.state#` parsing.

Disabled mutation actions must remain blocked and never fall through.

# Verification

Run:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Native application remains `-fno-exceptions`.

Specifically prove where practical:

- legacy sources visible independent of Nintendo user selection;
- installed-title user scoping unaffected;
- one FireRed parent + one LeafGreen parent when present;
- configured active save root wins and fallback is fallback-only;
- aliases do not duplicate child save instances;
- genuine separate files in active root remain distinct children;
- parent opening + manual Refresh Saves reread disk state;
- changed/deleted battery saves invalidate safely;
- selected physical path/root is diagnosable;
- FireRed/LeafGreen GBA artwork resolves;
- trainer name/gender/TID16/SID16/ID32/money match deterministic fixture expectations;
- Items exposes six FRLG containers in correct order;
- representative item IDs/counts decode correctly including keyed counts;
- Party/Boxes/View remain correct;
- source bytes remain byte-identical after Trainer/Items/Party/Boxes reads;
- all mutation actions remain blocked.

# Checkpoint and device artifact

Commit/push coherent source work early to `origin/feature/pokebank-playable` only.

Suggested application commit concept:

```text
gen3: complete FRLG read-only source browsing
```

Update issue #6 and minimal status/handoff docs after verification.

Then package a **new exact full-asset device-test NRO** using the complete pinned 3260-render set.

Report:

```text
starting remote SHA
preserved local RSE ref/status
application/source SHA(s)
active RetroArch save root selected
parent/child grouping behavior
refresh/rescan behavior
FRLG GBA artwork lookup/result
trainer metadata mapping/result
FRLG inventory pouch mapping/result
host tests
ASan/UBSan
git diff --check
native build
CI/status
NRO filename
NRO size
NRO SHA-256
asset render count
DEVICE TESTED: NO
```

Then **STOP for physical retest**.

Required retest:

```text
launch under normal Nintendo user
-> FireRed / LeafGreen RetroArch cards visible
-> exactly one parent card per game
-> correct FireRed / LeafGreen artwork
-> only active configured battery-save-root children listed
-> open game card / child list refreshes
-> manual Refresh Saves works
-> choose Main Save
-> Trainer: correct name, gender, TID/SID, full-ID fields and money
-> Items: opens normally; all six FRLG containers browse correctly
-> Party works
-> Boxes 1-14 work
-> View Pokémon works
-> Edit/Clone/Transfer/Move/Save/writeback remain blocked
```

Do not begin Ruby/Sapphire/Emerald until the user physically accepts the new artifact.
