# PokeBank NX — Game Support / Verification Matrix

Last updated: 2026-09-14

This matrix separates source/read support, staged editing, and physical acceptance. `DEVICE ACCEPTED` applies only to the exact milestone that was physically tested.

No current adapter is approved for live installed-game or emulator-source writing.

## Main current identities

| Stable ID | Game | Platform | Gen | Source / read state | Pokémon editing state | Device state |
|---|---|---:|---:|---|---|---|
| `red_gb` | Red | GB | 1 | RetroArch read engine | boxed staged editor accepted | READ + EDIT DEVICE ACCEPTED |
| `blue_gb` | Blue | GB | 1 | RetroArch read engine | boxed staged editor accepted | READ + EDIT DEVICE ACCEPTED |
| `yellow_gb` | Yellow | GB | 1 | RetroArch read engine | boxed staged editor accepted | READ + EDIT DEVICE ACCEPTED |
| `gold_gbc` | Gold | GBC | 2 | RetroArch read engine | shared staged editor implemented on PR #68 | READ DEVICE ACCEPTED / NEW EDIT UI HARDWARE PENDING |
| `silver_gbc` | Silver | GBC | 2 | RetroArch read engine | shared staged editor implemented on PR #68 | READ DEVICE ACCEPTED / NEW EDIT UI HARDWARE PENDING |
| `crystal_gbc` | Crystal | GBC | 2 | RetroArch read engine | shared staged editor implemented on PR #68 | READ DEVICE ACCEPTED / NEW EDIT UI HARDWARE PENDING |
| `ruby_gba` | Ruby | GBA | 3 | RetroArch read engine | shared Pokémon editor not yet current milestone | READ DEVICE ACCEPTED |
| `sapphire_gba` | Sapphire | GBA | 3 | RetroArch read engine | shared Pokémon editor not yet current milestone | READ DEVICE ACCEPTED |
| `emerald_gba` | Emerald | GBA | 3 | RetroArch read engine | shared Pokémon editor not yet current milestone | READ DEVICE ACCEPTED |
| `firered_gba` | FireRed | GBA | 3 | RetroArch read engine | shared Pokémon editor not yet current milestone | READ DEVICE ACCEPTED |
| `leafgreen_gba` | LeafGreen | GBA | 3 | RetroArch read engine | shared Pokémon editor not yet current milestone | READ DEVICE ACCEPTED |
| `firered_switch` | FireRed | Switch | 3 | native source foundation | production validation pending | NOT FULLY ACCEPTED |
| `leafgreen_switch` | LeafGreen | Switch | 3 | native source foundation | production validation pending | NOT FULLY ACCEPTED |
| `letsgo_pikachu_switch` | Let's Go, Pikachu! | Switch | 7 | native source foundation | inherited/editor capability exists; production validation pending | PARTIAL SOURCE PATH TESTED |
| `letsgo_eevee_switch` | Let's Go, Eevee! | Switch | 7 | native source foundation | production validation pending | NOT DEVICE ACCEPTED |
| `sword_switch` | Sword | Switch | 8 | native source foundation | production validation pending | NOT DEVICE ACCEPTED |
| `shield_switch` | Shield | Switch | 8 | native source foundation | production validation pending | NOT DEVICE ACCEPTED |
| `brilliant_diamond_switch` | Brilliant Diamond | Switch | 8 | native source foundation | production validation pending | NOT DEVICE ACCEPTED |
| `shining_pearl_switch` | Shining Pearl | Switch | 8 | native source foundation | production validation pending | PARTIAL SOURCE PATH TESTED |
| `legends_arceus_switch` | Legends: Arceus | Switch | 8 | defensive read foundation | production validation pending | PARTIAL PATH TESTED |
| `scarlet_switch` | Scarlet | Switch | 9 | native source foundation | production validation pending | NOT DEVICE ACCEPTED |
| `violet_switch` | Violet | Switch | 9 | native source foundation | production validation pending | NOT DEVICE ACCEPTED |
| `legends_za_switch` | Legends: Z-A | Switch | 9 | native source foundation | production validation pending | PARTIAL PATH TESTED |

FireRed/LeafGreen GBA and FireRed/LeafGreen Switch remain deliberately separate identities.

## Accepted Generation I milestone

Read-only RBY is physically accepted, including Trainer, Party, Boxes, Pokémon details, Bag/PC Items, strict save validation, bounded RetroArch discovery, and source immutability.

The boxed staged Pokémon editor is also physically accepted:

```text
source 69668bc81629228ef25c1bdada7c7ce1aed9b666
NRO PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
SHA-256 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
```

PR #67 later unified passive Party/Storage/action-sheet View Pokémon and was hardware accepted before merge.

## Accepted Generation II read-only milestone

Gold, Silver, and Crystal are physically accepted for the legacy read-only path.

Accepted semantics include:

```text
Gold Trainer gender: Male
Silver Trainer gender: Male
Crystal Trainer gender: save-derived
Gen II SID: does not exist
```

Party/Boxes/Trainer/Items/PK2 browsing are accepted. This does not imply the new shared PR #68 editor UI is hardware accepted.

## Current Generation II editor milestone

Focused branch:

```text
feature/gen2-shared-pokemon-editor-20260914
PR #68 — OPEN / DRAFT / NOT MERGED
implementation head 43b8563d4177b58ae0f341c1ebd58f86e2ee4fdb
```

Current implementation includes exact-game G/S vs Crystal move compatibility, passive View compatibility, shared Create/Edit, authentic DVs and derived HP DV, six-stat battle presentation, Held Item, Friendship, Pokérus, Crystal caught/met handling, gender/shiny DV semantics, and transactional Edit sessions.

Final exact-head host/sanitizer/native/package validation and physical Gold/Silver/Crystal testing are still required.

## Accepted Generation III GBA milestone

FireRed, LeafGreen, Ruby, Sapphire, and Emerald are physically accepted for current legacy read-only support. Strict rotating-sector/signature/counter/checksum validation, Trainer, Party, Boxes, PK3 browsing, inventory, bounded discovery, and source immutability are preserved.

## Current safety policy

```text
installed Switch source       READ ONLY unless separately approved later
RetroArch / legacy source     READ ONLY unless separately approved later
staged workspace              separate
live installed save writing   HARD DISABLED
live emulator-source writing  HARD DISABLED
true Move                     NOT IMPLEMENTED
```

Malformed/unsupported sources must fail safely and must never be silently repaired or overwritten.
