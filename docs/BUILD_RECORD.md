# PokeBank NX — Build / Artifact Record

Last updated: 2026-09-11

This file distinguishes application source, build verification, artifact identity, packaging infrastructure, and physical-device acceptance.

## CURRENT ACCEPTED RBY ARTIFACT — PHYSICALLY ACCEPTED

```text
Version: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
Commit: gen1: add read-only RBY inventory support
Host/sanitizer verification run: 34576027301
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Recovery archive SHA-256: 43ca238b1c8402f7cdc17ba9a0ca58ec87bc68895faf354403db2673a9f419fa
Expected RomFS: 3289 files
Device/package run: 34576781488

Artifact: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
Artifact bytes: 159754197
Artifact SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b

ZIP: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.zip
ZIP bytes: 152773612
ZIP SHA-256: f6dbdf2a76df1598e37ea0d1a771ac7a1eaa1864a29294c9101a6c6c09e2ecb6

NRO manifest SHA-256: 4b43aee6f5e68db8a3733449df4627af1c5e2a4c82f19bb038ceb5917014b126
BUILD_MANIFEST.json SHA-256: d4c93a7c24240ec444f9a105039b7920e2ac82545a8866caba97a963fa2e8f6d
SHA256SUMS.txt SHA-256: cb0930d99fe60339eca103bf82c80d61dba0ba4ac6153ffde536675d16ef2894

Actions artifact name: RBY-ItemsFix-Retest-50dac31f
Actions artifact id: 10190135145
Actions archive bytes: 305759538
Actions artifact digest: sha256:1a9fa5a4e43ac4c00d06e89c96b5effbe4450958fc41f4ad3f71e39fc8fe4901

Focused RBY inventory: PASS
RBY parser/oracle: PASS
RBY discovery: PASS
RBY source browser: PASS
RBY read-only bridge: PASS
Malformed inventory nonfatal: PASS
Full host suite: PASS
FRLG regression: PASS
RSE regression: PASS
Source mutation/write policy: PASS
ASan: PASS
UBSan: PASS
git diff --check: PASS
Gen III protected paths: PASS
Device asset preflight: PASS
Native devkitA64 compile: PASS
Native devkitA64 FINAL LINK: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED

Red DEVICE TESTED: YES
Red DEVICE ACCEPTED: YES
Blue DEVICE TESTED: YES
Blue DEVICE ACCEPTED: YES
Yellow DEVICE TESTED: YES
Yellow DEVICE ACCEPTED: YES
RBY Items DEVICE TESTED: YES
RBY Items DEVICE ACCEPTED: YES
```

The canonical Actions artifact was downloaded and independently verified. The outer Actions ZIP matched GitHub's digest; the inner ZIP passed integrity testing; the standalone NRO was byte-identical to the NRO inside the package ZIP; and the independently computed NRO hash matched the package records.

### Physical acceptance result

The user physically tested the exact `PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro` binary/hash on a real Nintendo Switch. RBY opening, Trainer, Party, Boxes, Pokémon details, general browsing, Items, Bag, PC Items, inventory navigation and the GB platform label all passed. No `Invalid category`, modern `Medicines` pouch fallback, fabricated SID or fabricated modern Trainer fields remained.

## HISTORICAL FIRST RBY DEVICE TEST — DEFECT DISCOVERED

Preserve this prior record because it explains the accepted ItemsFix:

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Host verification run: 34559821944
Device/package run: 34566567906
Artifact: PokeBank-NX-RBY-Retest-d9077e2d.nro
Artifact bytes: 159741909
Artifact SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

That exact binary passed Trainer, Party, Boxes, Pokémon details and general browsing but exposed the Items/category defect and the incorrect GBA label for Yellow. It was **DEVICE TESTED** but not accepted. Production Yellow identity detection was not weakened; the later ItemsFix corrected the actual optional inventory/UI/platform behavior.

### Historical packaging-only defect

Failed run `34562021925`, job `103146558801`, completed native build and NRO creation but failed in post-package bookkeeping because the inline `SHA256SUMS.txt` parser reversed filename/digest lookup. Packaging commit `214e5a3b8a96d340d20229b0ba948bff608a3d60` fixed that packaging-only lookup; runtime source/assets did not change.

## CURRENT ACCEPTED GEN III BASELINE

FireRed, LeafGreen, Ruby, Sapphire and Emerald remain physically accepted for the current read-only legacy milestone.

Accepted RSE application source:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
```

Accepted RSE artifact:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
bytes: 158120837
SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

```text
FireRed GBA: DEVICE ACCEPTED YES
LeafGreen GBA: DEVICE ACCEPTED YES
Ruby GBA: DEVICE ACCEPTED YES
Sapphire GBA: DEVICE ACCEPTED YES
Emerald GBA: DEVICE ACCEPTED YES
```

## Build record rule

A binary is not identified by filename alone. Every device artifact record must preserve application source SHA, build verification, native final-link result, device asset preflight, exact artifact filename/bytes/SHA-256, embedded identity/RomFS verification and physical device result. A docs-only or CI-only follow-up SHA must never be mislabeled as the NRO application source.

## Next build milestone

Generation II Gold/Silver/Crystal is the next legacy read-only milestone. No GSC device artifact exists yet and no GSC implementation was started in this closure session.
