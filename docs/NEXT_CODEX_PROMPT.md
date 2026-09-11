# NEXT CODEX PROMPT — RBY ITEMSFIX PHYSICAL RETEST WAIT STATE

Continue PokeBank NX on `feature/pokebank-playable`.

Use MEDIUM reasoning unless an actual physical-test defect requires deeper diagnosis.

## THIS IS NOT A NEW DEVELOPMENT MILESTONE

Generation I Red/Blue/Yellow read-only support has already been physically tested once. Runtime `d9077e2d` passed Trainer, Party, Boxes, Pokemon details and general browsing but exposed the Items/category defect and the incorrect GBA label for Yellow.

The RBY ItemsFix is now implemented, host/sanitizer verified, native-built, packaged and independently hashed. **Do not reopen implementation unless the user reports a concrete defect from the exact ItemsFix retest artifact below.**

Do not start Gold/Silver/Crystal or any later roadmap work.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

## VERIFIED GEN III BASELINE — PRESERVE

```text
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Ruby GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Sapphire GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Emerald GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
```

Do not reopen accepted Gen III work without new device evidence of a defect.

## RBY PHYSICAL-TEST HISTORY

Previous physically tested runtime:

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
NRO: PokeBank-NX-RBY-Retest-d9077e2d.nro
SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

Physical result:

```text
Trainer: PASS
Party: PASS
Boxes: PASS
Pokemon details: PASS
General RBY browsing: PASS
Items: FAIL
Yellow platform label: FAIL — shown as GBA instead of GB
RBY DEVICE TESTED = YES
RBY DEVICE ACCEPTED = NO
```

## VERIFIED ITEMSFIX APPLICATION

```text
Pre-fix feature/docs head: 0e92cee6ae743d5156dd8f7aaf55750b70dde5e5
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
Commit: gen1: add read-only RBY inventory support
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Expected RomFS: 3289 files
Verification run: 34576027301
```

This exact runtime passed:

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

The malformed-inventory bridge fixture was corrected to use the Red-family identity matching its synthetic source. Production Yellow detection was not weakened. Malformed optional inventory remains nonfatal to an otherwise-readable Trainer/Party/Boxes model, and source bytes remain immutable.

## ITEMSFIX DEVICE BUILD / PACKAGING RECORD

Successful build/package run:

```text
GitHub Actions run: 34576721503
Device asset preflight: PASS
Native devkitA64 compile: PASS
Native devkitA64 FINAL LINK: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
```

Actions artifact:

```text
name: RBY-ItemsFix-Retest-50dac31f
artifact id: 10190023094
archive bytes: 305759595
digest: sha256:07e2ba8dba82bd6edb780d1bcf0aae626e3ef134fb3415ab4af9b525228489f3
```

Exact physical-retest package:

```text
NRO: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
bytes: 159754197
SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b

ZIP: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.zip
bytes: 152773612
SHA-256: e6c0af2915c2315ce132b10b844f0d14f4951e5b8845f801697ed27aef3a578e

NRO manifest SHA-256: 4b43aee6f5e68db8a3733449df4627af1c5e2a4c82f19bb038ceb5917014b126
BUILD_MANIFEST.json SHA-256: 5302cedbf5a61244de213aea9948035ab6cc99003c055fb2a339f1206a175b66
SHA256SUMS.txt SHA-256: ca650014d5f06976df96f920066c5fc405bb584a727e6638f9df782140689fb7
```

The downloaded Actions archive independently matches GitHub's digest. The standalone NRO is byte-identical to the NRO inside the inner ZIP, and the NRO hash agrees with the NRO manifest, `BUILD_MANIFEST.json`, `SHA256SUMS.txt`, and `embedded-romfs.json`.

## CURRENT RBY TRUTH

```text
Red: IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / AWAITING ITEMSFIX RETEST
Blue: IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / AWAITING ITEMSFIX RETEST
Yellow: IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / AWAITING ITEMSFIX RETEST

RBY DEVICE TESTED = YES
RBY ITEMSFIX DEVICE TESTED = NO
RBY DEVICE ACCEPTED = NO
```

Do not mark the ItemsFix device-tested or RBY accepted until the user physically runs the exact filename/hash above.

## REQUIRED NEXT ACTION

Wait for the user to physically test `PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro` on the Nintendo Switch, especially the Items view/navigation and GB platform label.

When the user returns:

- if the exact ItemsFix artifact passes, record the exact filename/hash as physically accepted and only then discuss the next authorized milestone;
- if it fails, diagnose only the reported device behavior and preserve every verified/accepted path not implicated by evidence.

## HARD STOP

Do not start:

```text
Gold / Silver / Crystal
DS
3DS
Vault / Banks
RetroArch profile isolation
Admin Mode
RetroArch profile bridge
transfer work
editor work
legality expansion
events
live writing
```

Expected state before the user's retest remains:

```text
GEN III ACCEPTED BASELINE: PRESERVED
RBY ITEMSFIX APPLICATION SOURCE: 50dac31f53907143f48884681056f8d582813b76
RBY ITEMSFIX NRO BUILDS: YES
RBY DEVICE TESTED: YES
RBY ITEMSFIX DEVICE TESTED: NO
RBY DEVICE ACCEPTED: NO
STOP FOR PHYSICAL ITEMSFIX SWITCH RETEST
```
