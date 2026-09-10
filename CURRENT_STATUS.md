# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-10

## CURRENT STATE — GEN III READ-ONLY MILESTONE ACCEPTED

Active development branch: `feature/pokebank-playable`.

FireRed, LeafGreen, Ruby, Sapphire and Emerald legacy GBA support is now physically accepted on a real Nintendo Switch for the current read-only browsing/source-assignment milestone.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

## FRLG — PHYSICALLY ACCEPTED

```text
Acceptance checkpoint: 8172ebd9c067bd69df63815dbe865207f905eac6
Accepted runtime source: d78b76503f02ae26309855970fc5ce0b35c12bcb
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
FRLG MILESTONE: PHYSICALLY ACCEPTED
```

Physical verification includes:

- normal RetroArch battery-save discovery;
- correct save opening and current-save refresh;
- trainer, Items, Party and all Boxes;
- profile-scoped legacy-source assignment;
- assignment persistence across full app restart;
- profile isolation and switching back to the original assignment;
- rereading a save after normal in-game RetroArch changes;
- original source save remaining healthy/playable.

Do not reopen FRLG work without new device evidence of a defect.

## RSE — PHYSICALLY ACCEPTED

Final accepted application source:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
application tree: 559202d16f0affc13a9e1c521beff4834585b1e7
```

Exact accepted artifact:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
bytes: 158120837
SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

The user physically tested this NRO and reported Ruby, Sapphire and Emerald all work as intended, matching the accepted FRLG browsing experience. The existing Switch save paths also continued to open normally during this test.

```text
Ruby GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Sapphire GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Emerald GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
RSE MILESTONE: PHYSICALLY ACCEPTED
GEN III LEGACY READ-ONLY MILESTONE: PHYSICALLY ACCEPTED
```

Physical RSE acceptance covers:

- save discovery/open;
- trainer data;
- Party and PC Boxes;
- Pokémon browsing;
- Items/inventory;
- Ruby/Sapphire/Emerald game-card artwork;
- Refresh/current battery-save behavior;
- read-only source safety.

### RSE defects fixed during acceptance

1. Initial RSE build opened real saves but had empty Items and missing R/S/E artwork.
2. RSE inventory layouts were added for Ruby/Sapphire and Emerald, including Emerald keyed bag quantities and plaintext PC Items.
3. R/S/E game-card artwork was added and permanently included in recovery assets.
4. A stale `FRLGReadOnlyTrainer::populate()` bridge guard caused populated RSE inventory to reject otherwise-valid RSE opens; this was removed.
5. Inventory-specific model failure is now nonfatal to the whole save while critical Gen III sector/signature/counter/checksum/family validation remains strict.
6. Focused regressions now prove Ruby/Sapphire/Emerald open paths and nonfatal inventory-submodel failure behavior.

## Verification for accepted RSE runtime

Clean host verification:

```text
checkpoint: e5df0e237ba1b7ad0dd78e37a1e1aac609071c3f
GitHub Actions run: 34453208654
full host suite: PASS
focused Ruby open: PASS
focused Sapphire open: PASS
focused Emerald open: PASS
valid Ruby inventory: PASS
valid Sapphire inventory: PASS
valid Emerald inventory: PASS
inventory failure rejects whole save: NO
FRLG regression: PASS
source mutation/write policy: PASS
ASan: PASS
UBSan: PASS
git diff --check: PASS
```

Exact native/device build:

```text
GitHub Actions run: 34454555232
recovery restore: PASS
exact application source/tree verification: PASS
device asset preflight: PASS
native devkitA64 compile: PASS
native devkitA64 final link: PASS
embedded application identity: PASS
embedded RomFS: 3286/3286 PASS
packaging: PASS
artifact upload: PASS
```

Preserved Actions artifact:

```text
name: RSE-OpenFix-Retest-a2df4c1a
artifact id: 10143216443
digest: sha256:2daecd936c60719364f5b35b66fa41500be6b5a607146fab70535cbbe5e0fec7
```

Companion package:

```text
ZIP: PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.zip
bytes: 151195235
SHA-256: 847788db890523d1ebce070d1427f6230fd89a60ca14986525fd6d2b634d0788

Packaging manifest SHA-256: 3dfa38be0fabde6f45edfb1ee52d87d2f9b1513ec89ba3fa576b0b0ed8da55f9
BUILD_MANIFEST.json SHA-256: 335c1a62b45c2b7c3f2282c424c7c2cd089a6b1178c92fec2bccb027401070ee
SHA256SUMS.txt SHA-256: 33f461dbb17fc2ddbd1ab116d682fcb9871e49e33f0c77465cbed7cff489f664
```

## Permanent recovery baseline

Current committed full RomFS snapshot:

```text
2321fa488668e32392de25afed84e38919fbd21f
```

Verified baseline:

```text
HD renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
FireRed artwork: PASS
LeafGreen artwork: PASS
Ruby artwork: PASS
Sapphire artwork: PASS
Emerald artwork: PASS
RomFS files: 3286
```

Normal recovery remains the deterministic GitHub snapshot path. Do not redownload/regenerate the sprite library during ordinary coding sessions.

## NEXT DEVELOPMENT TARGET — GEN I RBY READ-ONLY

The next narrow milestone is:

```text
Red / Blue / Yellow
Game Boy
RetroArch normal battery saves
READ ONLY
```

Use the existing legacy-source/provider architecture and pinned correctness references. The first Gen I milestone should include strict save recognition/validation, Red/Blue/Yellow identity, trainer, Party, Boxes/storage, Pokémon view, Refresh, source diagnostics and source-byte immutability. Savestates are not canonical save sources.

Do **not** start Gen II in the same session unless the RBY milestone is complete and the user explicitly authorizes continuing. Do not enable live writing.

After RBY physical acceptance, the intended next legacy target is Gold/Silver/Crystal.

## Repository authority

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX work upstream.
