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
| Red GB | ✅ **Read-only physically accepted** |
| Blue GB | ✅ **Read-only physically accepted** |
| Yellow GB | ✅ **Read-only physically accepted** |
| Gen I RBY RetroArch read-only milestone | ✅ **Physically accepted** |
| FireRed GBA | ✅ **Physically accepted** |
| LeafGreen GBA | ✅ **Physically accepted** |
| Ruby GBA | ✅ **Physically accepted** |
| Sapphire GBA | ✅ **Physically accepted** |
| Emerald GBA | ✅ **Physically accepted** |
| Gen III RetroArch read-only milestone | ✅ **Physically accepted** |
| Gen II Gold/Silver/Crystal | ▶️ **Next read-only milestone** |
| DS / 3DS mainline games | ⬜ Planned |
| Profile-scoped Vaults | ⬜ Planned |
| Named Banks | ⬜ Planned |
| Transfers / editor / legality expansion | ⬜ Planned |
| Live save writing | 🔒 **HARD DISABLED** |

Development branch: `feature/pokebank-playable`.

## Generation I — Red / Blue / Yellow accepted on real Switch hardware

The read-only Game Boy RBY source path is physically accepted for normal RetroArch battery saves. The accepted capability includes bounded `.sav`/`.srm` discovery, strict supported save recognition, international/Japanese layout handling, Trainer, Party, PC Boxes/storage, PK1/Pokémon details, Bag, PC Items, dedicated Gen I item naming, Refresh/source rebinding behavior, GB platform identity and source-byte immutability.

Accepted application identity:

```text
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
```

Exact accepted device artifact:

```text
PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
159754197 bytes
SHA-256 b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
```

Physical result:

```text
Red: DEVICE TESTED YES / DEVICE ACCEPTED YES
Blue: DEVICE TESTED YES / DEVICE ACCEPTED YES
Yellow: DEVICE TESTED YES / DEVICE ACCEPTED YES
Items / Bag / PC Items: PASS
Trainer / Party / Boxes / Pokémon details: PASS
Yellow GB platform label: PASS
```

The first RBY device-test runtime, `d9077e2d`, is retained in project history because it exposed the Items/category defect and the incorrect GBA label for Yellow. The corrected `50dac31f...` runtime fixed those issues and passed the physical retest.

This acceptance is **read-only browsing acceptance only**. It does not imply live writing, editing, transfers, Vault support, legality support or conversion support for RBY.

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

## Next legacy milestone

Generation II Gold/Silver/Crystal on Game Boy Color / RetroArch is next. It remains **read-only** and is not yet implemented in the production path.

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
