# PokeBank NX Project Status

Last updated: 2026-09-11

For the shortest engineering handoff, read `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md`.

## Project identity and safety

```text
Product: PokeBank NX
Version: 0.1.0-alpha
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED
```

Verification vocabulary:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
DEVICE ACCEPTED
```

`DEVICE ACCEPTED` is only used after the user physically tests the exact relevant artifact and reports the milestone working.

## Current headline

Generation III legacy read-only support remains physically accepted. Generation I Red/Blue/Yellow was physically tested with runtime `d9077e2d`; Trainer, Party, Boxes, Pokemon details and general browsing passed, while the Items view exposed a device defect. The RBY ItemsFix is now implemented, fully host/sanitizer verified, native-built and packaged as a new exact artifact. RBY remains **DEVICE ACCEPTED = NO** until that ItemsFix artifact is physically retested.

| Game | State |
|---|---|
| FireRed GBA | DEVICE TESTED / DEVICE ACCEPTED |
| LeafGreen GBA | DEVICE TESTED / DEVICE ACCEPTED |
| Ruby GBA | DEVICE TESTED / DEVICE ACCEPTED |
| Sapphire GBA | DEVICE TESTED / DEVICE ACCEPTED |
| Emerald GBA | DEVICE TESTED / DEVICE ACCEPTED |
| Red GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / AWAITING ITEMSFIX PHYSICAL RETEST |
| Blue GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / AWAITING ITEMSFIX PHYSICAL RETEST |
| Yellow GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / AWAITING ITEMSFIX PHYSICAL RETEST |

## RBY physical-test history

Previous physically tested runtime:

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Artifact: PokeBank-NX-RBY-Retest-d9077e2d.nro
NRO bytes: 159741909
NRO SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

Physical result:

```text
Trainer: PASS
Party: PASS
Boxes: PASS
Pokemon details: PASS
General RBY browsing: PASS
Items: FAIL — device-discovered category/inventory defect
Platform label: FAIL — Yellow was shown as GBA instead of GB
RBY DEVICE TESTED: YES
RBY DEVICE ACCEPTED: NO
```

## Verified RBY ItemsFix runtime

Pre-fix feature/docs head:

```text
0e92cee6ae743d5156dd8f7aaf55750b70dde5e5
```

Verified runtime:

```text
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
Commit: gen1: add read-only RBY inventory support
Clean verification run: 34576027301
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Expected embedded RomFS: 3289 files
```

Verification gates:

```text
Focused RBY inventory: PASS
RBY parser/oracle: PASS
RBY discovery: PASS
RBY source browser: PASS
RBY read-only bridge: PASS
Malformed inventory nonfatal test: PASS
Full host suite: PASS
FRLG regression: PASS
RSE regression: PASS
Source mutation/write policy: PASS
ASan: PASS
UBSan: PASS
git diff --check: PASS
Gen III protected paths: PASS
```

The ItemsFix preserves the strict read-only policy. Generation I inventory is modeled as the two real stores, Bag and PC Items; malformed optional inventory does not invalidate an otherwise-readable Trainer/Party/Boxes save. Red/Blue/Yellow use the GB platform identity. Accepted Gen III runtime paths remain protected.

## RBY ItemsFix physical-retest artifact

Successful native/device build:

```text
GitHub Actions run: 34576721503
Actions artifact: RBY-ItemsFix-Retest-50dac31f
Artifact id: 10190023094
Actions archive bytes: 305759595
Actions archive SHA-256: 07e2ba8dba82bd6edb780d1bcf0aae626e3ef134fb3415ab4af9b525228489f3
Device asset preflight: PASS
Native devkitA64 FINAL LINK: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
```

Exact physical-retest package:

```text
NRO: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
NRO bytes: 159754197
NRO SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b

ZIP: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.zip
ZIP bytes: 152773612
ZIP SHA-256: e6c0af2915c2315ce132b10b844f0d14f4951e5b8845f801697ed27aef3a578e

NRO manifest SHA-256: 4b43aee6f5e68db8a3733449df4627af1c5e2a4c82f19bb038ceb5917014b126
BUILD_MANIFEST.json SHA-256: 5302cedbf5a61244de213aea9948035ab6cc99003c055fb2a339f1206a175b66
SHA256SUMS.txt SHA-256: ca650014d5f06976df96f920066c5fc405bb584a727e6638f9df782140689fb7
```

The Actions artifact was downloaded and independently verified. The outer Actions ZIP matches GitHub's artifact digest. The standalone NRO is byte-identical to the NRO inside the package ZIP, and its independently computed SHA-256 agrees with the NRO manifest, `BUILD_MANIFEST.json`, `SHA256SUMS.txt`, and `embedded-romfs.json`.

## Accepted Generation III baseline

```text
FireRed GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
LeafGreen GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Ruby GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Sapphire GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Emerald GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
GEN III LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Accepted RSE application source remains `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954`; exact accepted RSE NRO SHA-256 remains `34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e`.

## Current architecture / safety rules

- Legacy source identity remains provider/path/save-container based.
- Savestates are not canonical battery-save sources.
- Original source bytes remain untouched during read-only milestones.
- Unknown save variants fail safely instead of being guessed writable.
- Malformed optional RBY inventory remains nonfatal to otherwise-valid Trainer/Party/Boxes access.
- Live installed-game writing is HARD DISABLED.
- Live RetroArch writing is HARD DISABLED.
- Gen III accepted behavior must remain preserved.

## Current stop condition

```text
RBY DEVICE TESTED = YES
RBY ITEMSFIX HOST VERIFIED = YES
RBY ITEMSFIX NRO BUILDS = YES
RBY ITEMSFIX DEVICE TESTED = NO
RBY DEVICE ACCEPTED = NO
```

The next action is **physical Red / Blue / Yellow Items testing on the user's Nintendo Switch with the exact `PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro` binary and SHA-256 `b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b`**.

Do not start Gold/Silver/Crystal, DS, 3DS, Vault/Banks, profile isolation work, Admin Mode, RetroArch profile bridge, transfers, editor/legality expansion, events or live writing before that physical retest result.
