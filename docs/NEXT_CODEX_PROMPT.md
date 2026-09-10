# PokeBank NX — Authoritative Next Codex Prompt

Use MEDIUM reasoning.

## CURRENT STOP STATE — WAIT FOR PHYSICAL RSE OPEN-FIX RETEST

Do **not** continue roadmap development until the user supplies physical Ruby/Sapphire/Emerald results for the exact NRO below.

This is not a recovery task and not a new implementation milestone. Do not regenerate assets, redownload sprites, redo FRLG, redo RSE parsing, or start another generation merely because a new session began.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

## FRLG remains accepted

```text
FRLG acceptance checkpoint: 8172ebd9c067bd69df63815dbe865207f905eac6
accepted runtime source: d78b76503f02ae26309855970fc5ce0b35c12bcb
FireRed GBA: DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE ACCEPTED = YES
```

Do not reopen FRLG without new device evidence of a defect.

## RSE physical state

RSE has been physically tested, including artwork. Current state:

```text
Ruby artwork = PASS
Sapphire artwork = PASS
Emerald artwork = PASS
RSE DEVICE TESTED = YES
RSE DEVICE ACCEPTED = NO
```

The prior `1c96df25` artifact failed to open Ruby/Sapphire/Emerald after RSE inventory became populated. Root cause was the stale inventory guard in `FRLGReadOnlyTrainer::populate()`. That runtime bridge defect is fixed.

Critical Gen III save validation remains strict. Inventory-specific submodel failure is optional/nonfatal: a structurally valid RSE save still opens with trainer/party/boxes usable while inventory reports unavailable.

## Exact RSE open-fix runtime source

```text
application source: a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
application tree: 559202d16f0affc13a9e1c521beff4834585b1e7
tests-only checkpoint: 82364e32b1b82a4313a092c7aa54f44479bfade0
clean host checkpoint: e5df0e237ba1b7ad0dd78e37a1e1aac609071c3f
```

Clean host run `34453208654` passed:

```text
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

## Permanent recovery snapshot — unchanged

```text
2321fa488668e32392de25afed84e38919fbd21f
HD renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
FireRed artwork: PASS
LeafGreen artwork: PASS
Ruby artwork: PASS
Sapphire artwork: PASS
Emerald artwork: PASS
RomFS: 3286 files
```

Do not repack this snapshot or redownload sprites.

## Exact physical-retest artifact — READY

The first open-fix build run `34453854069` had no runtime failure; it stopped because its workflow grepped for stale focused-test output text. CI-only commit `ef8795134ec1243dd2600c33694621b0074592d1` corrected that check while preserving application source `a2df4c1a...`.

Successful exact build:

```text
GitHub Actions run: 34454555232
host/focused/sanitizer gate: PASS
recovery restore: PASS
exact application source/tree verification: PASS
device asset preflight: PASS
native devkitA64 compile: PASS
native devkitA64 final link: PASS
embedded application identity: PASS
embedded RomFS: PASS 3286/3286
packaging: PASS
artifact upload: PASS
```

Preserved Actions artifact:

```text
name: RSE-OpenFix-Retest-a2df4c1a
artifact id: 10143216443
Actions artifact digest: sha256:2daecd936c60719364f5b35b66fa41500be6b5a607146fab70535cbbe5e0fec7
```

Exact files:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
bytes: 158120837
SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e

PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.zip
bytes: 151195235
SHA-256: 847788db890523d1ebce070d1427f6230fd89a60ca14986525fd6d2b634d0788

PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro.manifest.txt
SHA-256: 3dfa38be0fabde6f45edfb1ee52d87d2f9b1513ec89ba3fa576b0b0ed8da55f9

BUILD_MANIFEST.json
SHA-256: 335c1a62b45c2b7c3f2282c424c7c2cd089a6b1178c92fec2bccb027401070ee

SHA256SUMS.txt
SHA-256: 33f461dbb17fc2ddbd1ab116d682fcb9871e49e33f0c77465cbed7cff489f664
```

The artifact was independently downloaded/extracted and the NRO hash matches both `BUILD_MANIFEST.json` and `SHA256SUMS.txt`.

## What to do when the user returns with the physical result

First verify the user tested this exact filename/hash:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

Record Ruby, Sapphire and Emerald results for:

- save opens and stays in the game UI;
- trainer/party/boxes remain usable;
- valid Items inventory renders naturally;
- known Emerald Potion x3 renders if still present in the real save;
- Ruby/Sapphire/Emerald artwork remains visible;
- read-only behavior leaves source saves normal/unchanged.

If all pass, record `DEVICE ACCEPTED FOR RSE = YES`. If something fails, diagnose only that exact observed defect from application source `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954`.

Do not infer device acceptance from host/native tests alone.

## Out of scope while RSE acceptance is pending

Do not start Gen I/II, DS/3DS, modern Switch expansion, Vault/Banks, transfers, legality/editor/Create Pokémon, events/gifts, RetroArch per-user changes, or live save writing.

## STOP

Stop and wait for the user's physical Switch test of `PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro`.
