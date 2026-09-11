# NEXT CODEX PROMPT — RBY PHYSICAL TEST WAIT STATE

Continue PokeBank NX on `feature/pokebank-playable`.

Use MEDIUM reasoning unless an actual physical-test defect requires deeper diagnosis.

## THIS IS NOT A DEVELOPMENT MILESTONE

Generation I Red/Blue/Yellow read-only implementation is complete through host/native build and packaging. **Do not reopen implementation unless the user reports a concrete physical Switch defect.**

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

## FROZEN RBY APPLICATION

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Expected RomFS: 3289 files
```

This exact runtime has already passed:

```text
RBY parser/oracle: PASS
RBY discovery: PASS
RBY source-card/binding: PASS
RBY Trainer: PASS
RBY Party: PASS
RBY Boxes: PASS
RBY Pokémon View: PASS
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
```

Clean host gate run recorded by the final build manifest: `34559821944`.

## PACKAGING RECORD

A packaging-only SHA256SUMS parser defect was fixed in:

```text
214e5a3b8a96d340d20229b0ba948bff608a3d60
```

Successful build/package run:

```text
34566567906
```

Actions artifact:

```text
name: RBY-Retest-d9077e2d
artifact id: 10186285817
digest: sha256:6310e55c2fca1ebe160490f16fdc54374266aa1b12fe775f598621bb3cba9367
```

Exact physical-test package:

```text
NRO: PokeBank-NX-RBY-Retest-d9077e2d.nro
bytes: 159741909
SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664

ZIP: PokeBank-NX-RBY-Retest-d9077e2d.zip
bytes: 152768842
SHA-256: cec41809905b1d758c4c3f995dcf316ffe11f178bf7571b7c5a4d822ccf58e5c

Packaging manifest SHA-256: 2cb61bc65b84527e8aee2545667ac34ef61daf0bb5e39cf71f0abf10a794a33c
BUILD_MANIFEST.json SHA-256: c6080ba90c0a21ca4d635bf9fb123775e5b526e60ac5263506f5cedb7962a6fc
SHA256SUMS.txt SHA-256: 5fa82074ac5786bcfcb34994b76cca277c346c35465849212b40d8dc0e26b11c
```

The Actions artifact was downloaded after the successful run and independently hashed. The NRO hash agrees with all packaging records.

## CURRENT RBY TRUTH

```text
Red: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Blue: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Yellow: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST

Red DEVICE TESTED = NO
Blue DEVICE TESTED = NO
Yellow DEVICE TESTED = NO

Red DEVICE ACCEPTED = NO
Blue DEVICE ACCEPTED = NO
Yellow DEVICE ACCEPTED = NO
```

Only the user's physical Switch test may change those values.

## REQUIRED NEXT ACTION

Wait for the user to physically test `PokeBank-NX-RBY-Retest-d9077e2d.nro` with Red, Blue and Yellow.

When the user returns:

- if all three pass, update the acceptance record for the exact filename/hash and only then discuss the next authorized milestone;
- if one or more fail, diagnose only the reported device behavior and preserve the frozen baseline everywhere not implicated by evidence.

## HARD STOP

Do not start:

```text
Gold / Silver / Crystal
DS
3DS
Vault / Banks
RetroArch profile isolation
Admin Mode
transfer work
editor work
legality expansion
events
live writing
```

Expected state before the user's test remains:

```text
GEN III ACCEPTED BASELINE: PRESERVED
RBY APPLICATION SOURCE: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
RBY NRO BUILDS: YES
DEVICE TESTED FOR RBY: NO
DEVICE ACCEPTED FOR RBY: NO
STOP FOR PHYSICAL RED / BLUE / YELLOW SWITCH TEST
```
