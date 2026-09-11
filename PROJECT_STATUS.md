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

Generation III legacy read-only support remains physically accepted. Generation I Red/Blue/Yellow read-only support is implemented, host-tested, native-built, packaged and **ready for physical testing**, but it is **not device tested or accepted yet**.

| Game | State |
|---|---|
| FireRed GBA | DEVICE ACCEPTED |
| LeafGreen GBA | DEVICE ACCEPTED |
| Ruby GBA | DEVICE ACCEPTED |
| Sapphire GBA | DEVICE ACCEPTED |
| Emerald GBA | DEVICE ACCEPTED |
| Red GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST |
| Blue GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST |
| Yellow GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST |

## Frozen RBY runtime

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Clean host gate run: 34559821944
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Expected embedded RomFS: 3289 files
```

This exact runtime passed focused RBY verification, the full host suite, FRLG and RSE regressions, source mutation/write policy checks, ASan, UBSan, native devkitA64 compile/final link, embedded application identity and complete embedded RomFS verification.

## RBY physical-test artifact

A packaging-only SHA256SUMS lookup defect was fixed without touching application runtime source.

```text
Packaging infrastructure commit: 214e5a3b8a96d340d20229b0ba948bff608a3d60
Successful Actions run: 34566567906
Artifact name: RBY-Retest-d9077e2d
Artifact id: 10186285817
Artifact digest: sha256:6310e55c2fca1ebe160490f16fdc54374266aa1b12fe775f598621bb3cba9367
```

```text
NRO: PokeBank-NX-RBY-Retest-d9077e2d.nro
NRO bytes: 159741909
NRO SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664

ZIP: PokeBank-NX-RBY-Retest-d9077e2d.zip
ZIP bytes: 152768842
ZIP SHA-256: cec41809905b1d758c4c3f995dcf316ffe11f178bf7571b7c5a4d822ccf58e5c

Packaging manifest SHA-256: 2cb61bc65b84527e8aee2545667ac34ef61daf0bb5e39cf71f0abf10a794a33c
BUILD_MANIFEST.json SHA-256: c6080ba90c0a21ca4d635bf9fb123775e5b526e60ac5263506f5cedb7962a6fc
SHA256SUMS.txt SHA-256: 5fa82074ac5786bcfcb34994b76cca277c346c35465849212b40d8dc0e26b11c
```

The Actions artifact was downloaded and independently verified. The NRO SHA-256 matches all three packaging records.

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
- Live installed-game writing is HARD DISABLED.
- Live RetroArch writing is HARD DISABLED.
- Gen III accepted behavior must remain preserved.

## Current stop condition

```text
Red: DEVICE TESTED = NO / DEVICE ACCEPTED = NO
Blue: DEVICE TESTED = NO / DEVICE ACCEPTED = NO
Yellow: DEVICE TESTED = NO / DEVICE ACCEPTED = NO
```

The next action is **physical Red / Blue / Yellow testing on the user's Nintendo Switch with the exact d9077e2d NRO**.

Do not start Gold/Silver/Crystal, DS, 3DS, Vault/Banks, profile isolation work, Admin Mode, transfers, editor/legality expansion, events or live writing before that test result.
