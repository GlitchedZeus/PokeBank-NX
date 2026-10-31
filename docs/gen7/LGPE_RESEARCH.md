# Let's Go Pikachu / Eevee (SAV7b / PB7): format research note

Status: **research only, no code.** Thread 3, 2026-10-15. Let's Go is its own
save family. It is a Switch title, not a 3DS one, and shares neither the SAV7
(SM/USUM) layout nor its MemeCrypto signing. Every constant below cites the pinned
PKHeX revision `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` (abbreviated *PKHeX*),
PKSM-Core `aa22d7a4` (`vendor/PKSM-Core`), or code already in this repository.

## 1. Save file

| Fact | Value | Source |
|---|---|---|
| Installed save file size | `0x100000` (`SIZE_G7GG`); only the first `0xB8800` bytes hold block data | PKHeX `Saves/Util/SaveUtil.cs`; `SAV7b(Memory) : base(data, 0xB8800)` in `Saves/SAV7b.cs` |
| Block metadata ("BEEF" footer) | at `0xB8800 - 0x200 = 0xB8600` | PKHeX `Saves/Access/SaveBlockAccessor7b.cs` (`boGG`) |
| Per-block checksum | `u16` at `0xB8600 + 0x14 + id*8 + 6 = 0xB861A + id*8` | PKHeX `Saves/Blocks/BlockInfo3DS.cs` |
| Checksum algorithm | `BlockInfo7b` → `Checksums.CRC16NoInvert` (CRC-16/ARC, reflected 0xA001, init 0, no final invert) | PKHeX `Saves/Blocks/BlockInfo3DS.cs`, `Saves/Util/Checksums.cs` |
| Signature | **none**. `SAV7b` has no MemeCrypto step, unlike `SAV7` (`GetFinalData` → `MemeCrypto.SignInPlace` exists only in `Saves/SAV7.cs`) | PKHeX `Saves/SAV7b.cs` |
| Identity | `SaveUtil.IsG7LGPE`: size `SIZE_G7GG`, BEEF footer inside the first `0xB8800` bytes, and the metadata entry at `0xB8600 + 0xB0` (block 19's id) must read `0x13`; version GP=42 / GE=43 | PKHeX `SaveUtil.cs`, `Game/Enums/GameVersion.cs` |

### Block table (21 blocks, `SaveBlockAccessor7b.BlockInfoGG`)

| id | offset | length | use (PKHeX accessor name) |
|---|---|---|---|
| 0 | 0x00000 | 0x00D90 | MyItem |
| 1 | 0x00E00 | 0x00200 | Coordinates |
| 2 | 0x01000 | 0x00168 | MyStatus |
| 3 | 0x01200 | 0x01800 | (unnamed in this note) |
| 4 | 0x02A00 | 0x020E8 | Zukan |
| 5 | 0x04C00 | 0x00930 | Misc |
| 6 | 0x05600 | 0x00004 | ConfigSave |
| 7 | 0x05800 | 0x00130 | (unnamed in this note) |
| 8 | 0x05A00 | 0x00012 | PokeListHeader |
| 9 | 0x05C00 | 0x3F7A0 | PokeListPokemon (storage) |
| 10 | 0x45400 | 0x00008 | PlayTime |
| 11–20 | 0x45600 … 0xB7C00 | see accessor | event work, records, fashion, GO Park, geolocation, etc. |

The repository's `include/Save/LGPEReadValidation.h` already checks blocks 0, 2,
4, 5, 8, 9 and 10 with the same CRC-16/ARC and `CHECKSUM_BASE = 0xB861A`. Its
offsets and lengths match the PKHeX table above. A native Gen VII-lane parser
would verify **all 21** blocks plus the BEEF magic, as the SAV6/SAV7 parsers do.

## 2. Storage model (different from every 3DS game)

- One shared storage array of **1000 slots** in block 9. Each slot is a full **260-byte
  (party-format) PB7**: `SIZE_BOXSLOT = SIZE_6PARTY`, "party format in boxes!".
  PKHeX presents it as 40 boxes × 25 slots (`BoxSlotCount = 25`, `BoxCount = 40`).
  Source: PKHeX `Saves/SAV7b.cs`.
- The **party is not a separate array**. `PokeListHeader` (block 8) holds 6 u16
  party pointers into the 1000-slot storage, a u16 starter pointer (index 6),
  and the count. Empty is `SLOT_EMPTY = 1001` and the maximum is `MAX_SLOTS = 1000`.
  Source: PKHeX `Saves/Substructures/Gen7/LGPE/PokeListHeader.cs`.
- Consequences for staging:
  - Party edits are edits of storage slots.
  - Removing a party member requires pointer compaction (`RemovePartySlot`).
  - PKHeX compacts storage before writing (`FixStoragePreWrite` → `CompressStorage`).

  None of this may be guessed. A staged design must whitelist the pointer bytes
  explicitly and preserve the starter pointer.

## 3. PB7 record

| Fact | Value | Source |
|---|---|---|
| Size | 260 bytes (`SIZE`), same for party and boxed | PKHeX `PKM/PB7.cs` |
| Crypto | `PokeCrypto.Decrypt67` (the PK6/PK7 shuffle + LCG) | PKHeX `Saves/SAV7b.cs` `DecryptPKM` |
| Species / item / nature | 0x08 / 0x0A / 0x1C (as PK6/PK7) | PKHeX `PB7.cs` |
| Awakening values | AV_HP..AV_SPD at 0x24..0x29 | PKHeX `PB7.cs` |
| Cached stats | level 0xEC, HP current 0xF0, HP max..SpD 0xF2..0xFC, **CP 0xFE**, mega 0x100/0x101 | PKHeX `PB7.cs` |
| Stat formula | `LoadStats` differs from Gen VI/VII: no EVs; adds AVs; HP = AV + (IV+2B)·L/100 + 10 + L; others = AV + friendship scalar `((friend/255/10)+1)*100` × nature-amplified base / 100; hyper training applies | PKHeX `PB7.cs` `LoadStats` |
| Personal table | `PersonalTable.GG` | PKHeX `PB7.cs` |

PKSM-Core has an independent `PB7` implementation (`include/pkx/PB7.hpp`, with the
same `hyperTrainLookup {0,1,2,5,3,4}`) that can serve as the differential reference.

## 4. What the repository already covers

| File | Coverage |
|---|---|
| `include/Pokemon/Pokemon7LGPE.h`, `src/Pokemon/Pokemon7LGPE.cpp` | PB7 accessor class (260 bytes), with AV/CP/height/weight accessors |
| `include/Encryption/Encryption7LGPE.h` | PB7 shuffle + LCG crypt |
| `include/Save/LGPEReadValidation.h` | size gate (`0xB8800` active / `0x100000` full), CRC-16/ARC on the 7 consumed blocks |
| `include/Trainer/Trainer7LGPE.h`, `include/Trainer/Inventory7LGPE.h` | block indexes (MyItem 0, MyStatus 2, Zukan 4, Misc 5, PokeListHeader 8, PokeListPokemon 9, PlayTime 10), trainer/inventory readers |
| `src/Save/GetSaveFileContents.cpp` | load path, plus an existing `writeBlocksToSaveData7LGPE` writer used by the legacy flow |
| `include/UI/ExactSaveCapabilities.h` | `letsgo_pikachu_switch` / `letsgo_eevee_switch` → `SaveFormat::PB7`, `StatModel::IVAwakening` |

These files belong to other owners (shared `Pokemon/`, `Save/`, `Trainer/`, `UI/`).
Thread 3 has not changed them.

## 5. Gaps and risks before any Let's Go staging work

1. **Installed-game rule.** Let's Go is a Switch title. Its save is read from the
   installed game, so the coordinator rule for installed Switch saves applies: reading
   is read-only and the only output is an exported working copy. The legacy
   `writeBlocksToSaveData7LGPE` path must be audited against that rule before any new
   work reuses it (owner decision).
2. **Full-block validation.** The current validator checks 7 of the 21 blocks; a strict
   parser should check all 21 plus the BEEF magic.
3. **Storage pointers.** Party/starter pointers and the 1000-slot compaction semantics
   need their own whitelist and tests (section 2).
4. **Stats.** The PB7 recomputation differs (AVs, friendship scalar, CP at 0xFE). The CP
   formula is not covered by this note and must be cited before CP is ever written.
5. **Bag.** MyItem block 0 pouch layout (`MyItem7b` / `PlayerBag7b` in PKHeX) is not
   yet researched here.

## 6. Proposed order (for coordinator approval)

1. A strict `Gen7bSave` parser in a new `Integration/Gen7b` lane: all 21 CRCs, BEEF,
   GP/GE version, 1000-slot storage, PokeListHeader, read-only Trainer/Party/Boxes.
   Differential against PKSM-Core `SavLGPE` (`chkofs`/`chklen`).
2. A staged edit of non-stat PB7 fields with the same mask-whitelist design, export
   through `VerifiedStagedExport`, and installed-save reads only.
3. Stats/AV/CP recomputation once the CP formula is cited.
