<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is a controller-first, local-first Pokémon storage, collection, provenance, transfer, and save-management project for **CFW Nintendo Switch**.

The long-term goal is one Switch app for legacy emulator saves and modern Switch saves, with personal Vaults, named Banks, collection tracking, safe transfers, legality/provenance tools, and eventually carefully approved staged writing while keeping the core product local and user-owned.

> **Alpha safety rule:** live installed-game and RetroArch save writing is **HARD DISABLED**. Current legacy milestones are read-only. Writing may only be enabled later per adapter after backup, staging, validation, readback, rollback, corruption testing, and physical-device acceptance.

## Project status at a glance

| Area | Status |
|---|---|
| Native Switch `.nro` | ✅ Working on real hardware |
| Controller-first UI | ✅ Working |
| HD Pokémon artwork | ✅ 3,260/3,260 verified |
| FireRed GBA | ✅ **Physically accepted** |
| LeafGreen GBA | ✅ **Physically accepted** |
| Ruby GBA | ✅ **Physically accepted** |
| Sapphire GBA | ✅ **Physically accepted** |
| Emerald GBA | ✅ **Physically accepted** |
| Gen III RetroArch read-only milestone | ✅ **Physically accepted** |
| Red GB | 🧪 Implemented / host tested / NRO builds / ready for physical test |
| Blue GB | 🧪 Implemented / host tested / NRO builds / ready for physical test |
| Yellow GB | 🧪 Implemented / host tested / NRO builds / ready for physical test |
| Gen II Gold/Silver/Crystal | ⏸️ Parked until RBY physical result |
| DS / 3DS mainline games | ⬜ Planned |
| Profile-scoped Vaults | ⬜ Planned |
| Named Banks | ⬜ Planned |
| Transfers / editor / legality expansion | ⬜ Planned |
| Live save writing | 🔒 **HARD DISABLED** |

Development branch: `feature/pokebank-playable`.

## Generation I — Red / Blue / Yellow physical-test build

The first read-only Game Boy RBY source path is implemented for normal RetroArch battery saves. It includes bounded discovery, strict supported save recognition, Trainer, Party, Boxes/storage, Gen I-aware Pokémon browsing, Refresh/source rebinding behavior, and source-byte immutability checks.

Frozen application identity:

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
```

Verification state:

```text
Red: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Blue: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Yellow: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Full host suite: PASS
FRLG regression: PASS
RSE regression: PASS
ASan: PASS
UBSan: PASS
Native devkitA64 compile/final link: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
DEVICE TESTED FOR RBY: NO
DEVICE ACCEPTED FOR RBY: NO
```

Exact physical-test artifact:

```text
PokeBank-NX-RBY-Retest-d9077e2d.nro
159,741,909 bytes
SHA-256 b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

Companion package:

```text
PokeBank-NX-RBY-Retest-d9077e2d.zip
152,768,842 bytes
SHA-256 cec41809905b1d758c4c3f995dcf316ffe11f178bf7571b7c5a4d822ccf58e5c
```

Build provenance:

```text
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Packaging infrastructure commit: 214e5a3b8a96d340d20229b0ba948bff608a3d60
Successful Actions run: 34566567906
Actions artifact: RBY-Retest-d9077e2d
Artifact id: 10186285817
Artifact digest: sha256:6310e55c2fca1ebe160490f16fdc54374266aa1b12fe775f598621bb3cba9367
```

The uploaded Actions artifact was downloaded again and independently hashed. Its NRO SHA-256 matches the package manifest, `BUILD_MANIFEST.json`, and `SHA256SUMS.txt`.

## Generation III — accepted on real Switch hardware

The read-only legacy path is physically accepted for all five mainline Generation III GBA releases:

```text
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Ruby GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Sapphire GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Emerald GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
```

The accepted path covers normal battery-save discovery/opening, Trainer, Party, PC Boxes, Pokémon browsing, inventory where implemented, Refresh/current-save behavior and read-only source safety. Profile-scoped assignment/persistence/isolation was physically exercised in the accepted FRLG path.

## Save safety

Current product contract:

```text
installed Switch source        READ ONLY
RetroArch / legacy source      READ ONLY
backup/staged workspace        separate from live source
live installed save writing    HARD DISABLED
live RetroArch save writing    HARD DISABLED
true Move                       NOT IMPLEMENTED
```

Source-byte immutability, malformed/truncated rejection and strict parser boundaries are part of the current regression gates.

## Current stop point

The project is intentionally stopped for the user's physical **Red / Blue / Yellow** test of `PokeBank-NX-RBY-Retest-d9077e2d.nro`.

Do not start Gold/Silver/Crystal or later roadmap work until the user reports the RBY physical result. Only that physical test may mark RBY `DEVICE TESTED` or `DEVICE ACCEPTED`.

## Developer handoff

Start with:

```text
CURRENT_STATUS.md
↓
docs/CODEX_SESSION.md
↓
docs/NEXT_CODEX_PROMPT.md
```

Keep coherent source/test/docs checkpoints on `origin/feature/pokebank-playable`. Never push custom PokeBank NX code to the PKSE upstream reference.

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.
