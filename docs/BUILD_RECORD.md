# PokeBank NX — Build / Artifact Record

Last updated: 2026-09-10

This file distinguishes application source, build verification, artifact identity and physical-device acceptance.

## CURRENT ACCEPTED GEN III RSE ARTIFACT

```text
Version: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source: a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
Application tree: 559202d16f0affc13a9e1c521beff4834585b1e7
Host verification checkpoint: e5df0e237ba1b7ad0dd78e37a1e1aac609071c3f
Host verification run: 34453208654
Device build run: 34454555232
Recovery snapshot: 2321fa488668e32392de25afed84e38919fbd21f

Artifact: PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
Artifact bytes: 158120837
Artifact SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e

ZIP: PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.zip
ZIP bytes: 151195235
ZIP SHA-256: 847788db890523d1ebce070d1427f6230fd89a60ca14986525fd6d2b634d0788

Packaging manifest SHA-256: 3dfa38be0fabde6f45edfb1ee52d87d2f9b1513ec89ba3fa576b0b0ed8da55f9
BUILD_MANIFEST.json SHA-256: 335c1a62b45c2b7c3f2282c424c7c2cd089a6b1178c92fec2bccb027401070ee
SHA256SUMS.txt SHA-256: 33f461dbb17fc2ddbd1ab116d682fcb9871e49e33f0c77465cbed7cff489f664

Actions artifact name: RSE-OpenFix-Retest-a2df4c1a
Actions artifact id: 10143216443
Actions digest: sha256:2daecd936c60719364f5b35b66fa41500be6b5a607146fab70535cbbe5e0fec7

Full host suite: PASS
Focused Ruby/Sapphire/Emerald open: PASS
Valid R/S/E inventory: PASS
Inventory-only failure rejects whole save: NO
FRLG regression: PASS
Source mutation/write policy: PASS
ASan/UBSan: PASS
git diff --check: PASS
Native devkitA64 compile/final link: PASS
Device asset preflight: PASS
Embedded application identity: PASS
Embedded RomFS: 3286/3286 PASS
Live writing: HARD DISABLED

DEVICE TESTED: YES
DEVICE RESULT: PASS
DEVICE ACCEPTED: YES
```

Physical Switch result reported by the user:

- Ruby opens and browses correctly.
- Sapphire opens and browses correctly.
- Emerald opens and browses correctly.
- RSE Items behave as intended.
- R/S/E artwork behaves as intended.
- FRLG remains healthy.
- Existing Switch save paths remain healthy in the tested app.

Therefore Ruby/Sapphire/Emerald are physically accepted for the current read-only legacy milestone.

---

## FRLG accepted milestone

```text
Acceptance checkpoint: 8172ebd9c067bd69df63815dbe865207f905eac6
Accepted runtime source: d78b76503f02ae26309855970fc5ce0b35c12bcb
FireRed GBA: DEVICE ACCEPTED YES
LeafGreen GBA: DEVICE ACCEPTED YES
```

Physical acceptance included normal RetroArch battery-save discovery, Trainer/Items/Party/Boxes, assignment persistence, profile isolation, Refresh after in-game save changes and continued health of original sources.

Important earlier FRLG artifacts remain historical evidence only and must not be relabelled as the current accepted RSE source.

---

## Permanent asset/recovery baseline

```text
Snapshot: 2321fa488668e32392de25afed84e38919fbd21f
HD renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
FR/LG/R/S/E card art: PASS
RomFS files: 3286
```

The recovery snapshot is stored in private GitHub history and restored through the deterministic recovery tooling. Do not rebuild/redownload the full asset set during normal coding sessions.

---

## Selected historical artifact checkpoints

| Milestone | Application source | Physical state |
|---|---|---|
| First controller/theme device build | `3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a` | DEVICE TESTED / partial pass |
| Accepted red UI identity | `af2acf043a15dbf48b8195880a80cc5de562fced` | later behavior accepted in combined builds |
| FRLG complete browser | `5d3e5e23f352dda4900ae42b4f396d9d4a4b8b8e` | historical pre-fix artifact |
| FRLG profile/source diagnostics | `ea0b806bac4acdb5619f22f9841d616ea8a237ff` | physical FireRed pass / LeafGreen persistence defect found |
| FRLG accepted runtime | `d78b76503f02ae26309855970fc5ce0b35c12bcb` | DEVICE ACCEPTED |
| Initial strict RSE runtime | `46e0c1617fb642f45c0cd1d4b07a9bcc89c01909` | physical core pass; Items/art missing |
| RSE inventory/art retest | `1c96df2543cba339cec3dc88e20f8c6ca4fe82bb` | artwork pass; save-open bridge regression found |
| Final RSE open-fix runtime | `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954` | **DEVICE ACCEPTED** |

Git history and archived status documents contain the deeper per-session history. This record keeps the currently relevant artifact chain unambiguous.

---

## Build record rule

A binary is not identified by filename alone. For every device artifact record:

```text
Application source full SHA
Application tree when relevant
Build run/environment
Host/sanitizer result
Native final-link result
Device asset preflight
Artifact filename + bytes + SHA-256
Embedded identity/RomFS verification
DEVICE TESTED YES/NO
DEVICE RESULT PASS/PARTIAL/FAIL
```

A docs-only or CI-only follow-up SHA must never be mislabeled as the NRO application source.
