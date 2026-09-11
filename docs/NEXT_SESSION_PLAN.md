# PokeBank NX — Next Session Plan

Last updated: 2026-09-11
Status: **RBY IMPLEMENTED / HOST VERIFIED / NRO BUILT / WAITING FOR PHYSICAL TEST**

## Preserved baseline

```text
FireRed: DEVICE ACCEPTED YES
LeafGreen: DEVICE ACCEPTED YES
Ruby: DEVICE ACCEPTED YES
Sapphire: DEVICE ACCEPTED YES
Emerald: DEVICE ACCEPTED YES
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED
```

## Frozen RBY runtime

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
RomFS: 3289 files
```

RBY implementation state:

```text
Red: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Blue: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Yellow: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Full host suite: PASS
FRLG regression: PASS
RSE regression: PASS
ASan: PASS
UBSan: PASS
Native devkitA64 FINAL LINK: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
```

## Exact physical-test artifact

```text
PokeBank-NX-RBY-Retest-d9077e2d.nro
bytes: 159741909
SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

Successful build/package run: `34566567906`.

Actions artifact:

```text
RBY-Retest-d9077e2d
artifact id: 10186285817
digest: sha256:6310e55c2fca1ebe160490f16fdc54374266aa1b12fe775f598621bb3cba9367
```

The artifact was downloaded and independently verified after the successful workflow run.

## Next action — physical Switch test only

The user should run the exact NRO above and test Pokémon Red, Blue and Yellow through the intended read-only flow.

Until the user reports the result:

```text
Red: DEVICE TESTED = NO / DEVICE ACCEPTED = NO
Blue: DEVICE TESTED = NO / DEVICE ACCEPTED = NO
Yellow: DEVICE TESTED = NO / DEVICE ACCEPTED = NO
```

If the physical test passes, record acceptance using the exact filename/hash. If it exposes a defect, investigate only the observed device defect while preserving accepted Gen III behavior and source-write safety.

Do **not** begin Gold/Silver/Crystal, DS, 3DS, Vault/Banks, RetroArch profile isolation, Admin Mode, transfer work, editor work, legality expansion, events or live writing before the RBY physical result.

Canonical handoff: `docs/NEXT_CODEX_PROMPT.md`.
