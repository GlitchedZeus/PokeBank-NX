# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-11

## CURRENT STATE — RBY NRO READY FOR PHYSICAL TEST

Active development branch: `feature/pokebank-playable`.

The accepted Generation III read-only baseline is preserved. FireRed, LeafGreen, Ruby, Sapphire and Emerald remain physically accepted on a real Nintendo Switch.

Live installed-game writing and live RetroArch writing remain **HARD DISABLED**.

## GEN III — PHYSICALLY ACCEPTED BASELINE

```text
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Ruby GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Sapphire GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Emerald GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
GEN III LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Accepted RSE runtime remains `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954`; accepted RSE NRO remains `PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro` with SHA-256 `34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e`.

Do not reopen accepted Gen III behavior without new physical evidence of a defect.

## GEN I RBY — IMPLEMENTED / HOST TESTED / NRO BUILDS

Frozen application identity:

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
```

Verified RBY state:

```text
Red: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Blue: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Yellow: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Focused RBY coverage: PASS
Full host suite: PASS
FRLG regression: PASS
RSE regression: PASS
Source mutation/write policy: PASS
ASan: PASS
UBSan: PASS
git diff --check: PASS
DEVICE TESTED FOR RBY: NO
DEVICE ACCEPTED FOR RBY: NO
```

Clean host gate recorded by the final build manifest: GitHub Actions run `34559821944`.

## Exact RBY physical-test build

Recovery snapshot:

```text
5bfc27a10de1eeaf52cb92316c1453df3d4fb613
archive bytes: 153651200
archive SHA-256: 43ca238b1c8402f7cdc17ba9a0ca58ec87bc68895faf354403db2673a9f419fa
RomFS files: 3289
```

Successful packaging/device build:

```text
Packaging infrastructure commit: 214e5a3b8a96d340d20229b0ba948bff608a3d60
GitHub Actions run: 34566567906
Native devkitA64 compile: PASS
Native devkitA64 FINAL LINK: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
Packaging: PASS
Artifact upload: PASS
```

Exact artifact:

```text
PokeBank-NX-RBY-Retest-d9077e2d.nro
bytes: 159741909
SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664

PokeBank-NX-RBY-Retest-d9077e2d.zip
bytes: 152768842
SHA-256: cec41809905b1d758c4c3f995dcf316ffe11f178bf7571b7c5a4d822ccf58e5c

Packaging manifest SHA-256: 2cb61bc65b84527e8aee2545667ac34ef61daf0bb5e39cf71f0abf10a794a33c
BUILD_MANIFEST.json SHA-256: c6080ba90c0a21ca4d635bf9fb123775e5b526e60ac5263506f5cedb7962a6fc
SHA256SUMS.txt SHA-256: 5fa82074ac5786bcfcb34994b76cca277c346c35465849212b40d8dc0e26b11c
```

Actions artifact:

```text
name: RBY-Retest-d9077e2d
artifact id: 10186285817
digest: sha256:6310e55c2fca1ebe160490f16fdc54374266aa1b12fe775f598621bb3cba9367
```

The artifact was downloaded after the successful run and independently hashed. The NRO hash agrees with the package manifest, `BUILD_MANIFEST.json`, and `SHA256SUMS.txt`.

## STOP STATE

Do **not** begin Gold/Silver/Crystal or any later roadmap work. The next required action is the user's physical Nintendo Switch test of Red, Blue and Yellow using the exact NRO above.

Only that physical test may change RBY to `DEVICE TESTED = YES` or `DEVICE ACCEPTED = YES`.

## Repository authority

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX work upstream.
