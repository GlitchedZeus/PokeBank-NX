# PokeBank NX — Build / Artifact Record

Last updated: 2026-09-11

This file distinguishes application source, build verification, artifact identity, packaging infrastructure, and physical-device acceptance.

## CURRENT RBY PHYSICAL-TEST ARTIFACT — READY / NOT DEVICE TESTED

```text
Version: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Host verification run: 34559821944
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Recovery archive bytes: 153651200
Recovery archive SHA-256: 43ca238b1c8402f7cdc17ba9a0ca58ec87bc68895faf354403db2673a9f419fa
Expected RomFS: 3289 files

Packaging infrastructure commit: 214e5a3b8a96d340d20229b0ba948bff608a3d60
Device/package run: 34566567906

Artifact: PokeBank-NX-RBY-Retest-d9077e2d.nro
Artifact bytes: 159741909
Artifact SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664

ZIP: PokeBank-NX-RBY-Retest-d9077e2d.zip
ZIP bytes: 152768842
ZIP SHA-256: cec41809905b1d758c4c3f995dcf316ffe11f178bf7571b7c5a4d822ccf58e5c

Packaging manifest: PokeBank-NX-RBY-Retest-d9077e2d.nro.manifest.txt
Packaging manifest bytes: 869
Packaging manifest SHA-256: 2cb61bc65b84527e8aee2545667ac34ef61daf0bb5e39cf71f0abf10a794a33c

BUILD_MANIFEST.json bytes: 2329
BUILD_MANIFEST.json SHA-256: c6080ba90c0a21ca4d635bf9fb123775e5b526e60ac5263506f5cedb7962a6fc

SHA256SUMS.txt bytes: 915
SHA256SUMS.txt SHA-256: 5fa82074ac5786bcfcb34994b76cca277c346c35465849212b40d8dc0e26b11c

Actions artifact name: RBY-Retest-d9077e2d
Actions artifact id: 10186285817
Actions artifact digest: sha256:6310e55c2fca1ebe160490f16fdc54374266aa1b12fe775f598621bb3cba9367

Focused RBY: PASS
Full host suite: PASS
FRLG regression: PASS
RSE regression: PASS
Source mutation/write policy: PASS
ASan: PASS
UBSan: PASS
git diff --check: PASS
Device asset preflight: PASS
Native devkitA64 compile: PASS
Native devkitA64 FINAL LINK: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED

Red DEVICE TESTED: NO
Red DEVICE ACCEPTED: NO
Blue DEVICE TESTED: NO
Blue DEVICE ACCEPTED: NO
Yellow DEVICE TESTED: NO
Yellow DEVICE ACCEPTED: NO
```

The successful Actions artifact was downloaded into an independent session/workspace and rehashed. The retrieved NRO SHA-256 is exactly `b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664`, matching the packaging manifest, `BUILD_MANIFEST.json`, and `SHA256SUMS.txt`.

### Packaging defect resolved

Failed run `34562021925`, job `103146558801`, completed native build and NRO creation but failed in post-package bookkeeping. `SHA256SUMS.txt` lines are written as `<digest>  <filename>`, while the inline parser built `dict(digest, filename)` and then indexed it by filename. This caused:

```text
KeyError: 'PokeBank-NX-RBY-Retest-d9077e2d.nro'
```

Packaging commit `214e5a3b8a96d340d20229b0ba948bff608a3d60` reversed the parsed pair to a filename-to-digest lookup only. No runtime source or assets changed.

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

A binary is not identified by filename alone. Every device artifact record must preserve:

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

## Current stop condition

Stop for the user's physical Red/Blue/Yellow test of `PokeBank-NX-RBY-Retest-d9077e2d.nro`. Do not mark RBY accepted until that test happens.
