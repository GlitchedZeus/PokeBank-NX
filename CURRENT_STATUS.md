# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-11

## CURRENT STATE — RBY ITEMSFIX NRO READY FOR PHYSICAL RETEST

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

## GEN I RBY — DEVICE TESTED / ITEMSFIX HOST VERIFIED / AWAITING RETEST

The previous RBY physical-test runtime was:

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Artifact: PokeBank-NX-RBY-Retest-d9077e2d.nro
SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

That exact binary was physically tested. Trainer, Party, Boxes, Pokemon details and general RBY browsing passed. The Items view exposed the device defect, and Yellow was incorrectly labeled as GBA rather than GB. Therefore RBY is **DEVICE TESTED = YES** but **DEVICE ACCEPTED = NO**.

Verified ItemsFix runtime:

```text
Pre-fix feature/docs head: 0e92cee6ae743d5156dd8f7aaf55750b70dde5e5
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
Commit: gen1: add read-only RBY inventory support
Verification run: 34576027301
```

Verification status:

```text
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
RBY DEVICE TESTED: YES
RBY ITEMSFIX DEVICE TESTED: NO
RBY DEVICE ACCEPTED: NO
```

The malformed-inventory bridge test now uses the game identity matching its Red-family synthetic fixture. Production Yellow detection was not weakened. Malformed optional inventory leaves the otherwise-valid Trainer/Party/Boxes model readable and source bytes immutable.

## Exact RBY ItemsFix physical-retest build

Recovery snapshot:

```text
5bfc27a10de1eeaf52cb92316c1453df3d4fb613
archive SHA-256: 43ca238b1c8402f7cdc17ba9a0ca58ec87bc68895faf354403db2673a9f419fa
RomFS files: 3289
```

Successful native/device build:

```text
GitHub Actions run: 34576781488
Device asset preflight: PASS
Native devkitA64 compile: PASS
Native devkitA64 FINAL LINK: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
Packaging: PASS
Artifact upload: PASS
```

Exact artifact:

```text
PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
bytes: 159754197
SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b

PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.zip
bytes: 152773612
SHA-256: f6dbdf2a76df1598e37ea0d1a771ac7a1eaa1864a29294c9101a6c6c09e2ecb6

NRO manifest SHA-256: 4b43aee6f5e68db8a3733449df4627af1c5e2a4c82f19bb038ceb5917014b126
BUILD_MANIFEST.json SHA-256: d4c93a7c24240ec444f9a105039b7920e2ac82545a8866caba97a963fa2e8f6d
SHA256SUMS.txt SHA-256: cb0930d99fe60339eca103bf82c80d61dba0ba4ac6153ffde536675d16ef2894
```

Actions artifact:

```text
name: RBY-ItemsFix-Retest-50dac31f
artifact id: 10190135145
archive bytes: 305759538
digest: sha256:1a9fa5a4e43ac4c00d06e89c96b5effbe4450958fc41f4ad3f71e39fc8fe4901
```

The canonical Actions artifact for run `34576781488` was downloaded and independently hashed. The outer artifact exactly matches GitHub's current digest. The standalone NRO is byte-identical to the NRO inside the inner ZIP, and the NRO hash agrees with the package manifest, `BUILD_MANIFEST.json`, and `SHA256SUMS.txt`. The inner ZIP passes integrity testing and `embedded-romfs.json` records runtime `50dac31f...` with `3289/3289` RomFS verification.

## STOP STATE

Do **not** begin Gold/Silver/Crystal or any later roadmap work.

The next required action is the user's physical Nintendo Switch retest of RBY Items using exactly:

```text
PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
```

Only that physical test may change RBY ItemsFix to `DEVICE TESTED = YES` or RBY to `DEVICE ACCEPTED = YES`.

## Repository authority

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX work upstream.
