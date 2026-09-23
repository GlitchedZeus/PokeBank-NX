# PokeBank NX — Game Support / Verification Matrix

Last updated: **2026-09-23**

This matrix separates source/read support, staged editing, and physical acceptance.

No current adapter is approved for live installed-game or emulator-source writing.

| Stable ID | Game | Platform | Gen | Read/source state | Pokémon editing state | Device state |
|---|---|---:|---:|---|---|---|
| `red_gb` | Red | GB | 1 | RetroArch read engine | shared boxed staged editor | READ + EDIT DEVICE ACCEPTED |
| `blue_gb` | Blue | GB | 1 | RetroArch read engine | shared boxed staged editor | READ + EDIT DEVICE ACCEPTED |
| `yellow_gb` | Yellow | GB | 1 | RetroArch read engine | shared boxed staged editor | READ + EDIT DEVICE ACCEPTED |
| `gold_gbc` | Gold | GBC | 2 | RetroArch read engine | shared staged editor | READ + EDIT DEVICE ACCEPTED |
| `silver_gbc` | Silver | GBC | 2 | RetroArch read engine | shared staged editor | READ + EDIT DEVICE ACCEPTED |
| `crystal_gbc` | Crystal | GBC | 2 | RetroArch read engine | shared staged editor | READ + EDIT DEVICE ACCEPTED |
| `ruby_gba` | Ruby | GBA | 3 | RetroArch read engine | shared staged PK3 editor | READ + EDIT DEVICE ACCEPTED |
| `sapphire_gba` | Sapphire | GBA | 3 | RetroArch read engine | shared staged PK3 editor | READ + EDIT DEVICE ACCEPTED |
| `emerald_gba` | Emerald | GBA | 3 | RetroArch read engine | shared staged PK3 editor | READ + EDIT DEVICE ACCEPTED |
| `firered_gba` | FireRed | GBA | 3 | RetroArch read engine | shared staged PK3 editor | READ + EDIT DEVICE ACCEPTED |
| `leafgreen_gba` | LeafGreen | GBA | 3 | RetroArch read engine | shared staged PK3 editor | READ + EDIT DEVICE ACCEPTED |
| `firered_switch` | FireRed | Switch | 3 | native source foundation | production validation pending | NOT FULLY ACCEPTED |
| `leafgreen_switch` | LeafGreen | Switch | 3 | native source foundation | production validation pending | NOT FULLY ACCEPTED |
| `letsgo_pikachu_switch` | Let's Go, Pikachu! | Switch | 7 | native source foundation | validation pending | PARTIAL SOURCE PATH TESTED |
| `letsgo_eevee_switch` | Let's Go, Eevee! | Switch | 7 | native source foundation | validation pending | NOT DEVICE ACCEPTED |
| `sword_switch` | Sword | Switch | 8 | native source foundation | validation pending | NOT DEVICE ACCEPTED |
| `shield_switch` | Shield | Switch | 8 | native source foundation | validation pending | NOT DEVICE ACCEPTED |
| `brilliant_diamond_switch` | Brilliant Diamond | Switch | 8 | native source foundation | validation pending | NOT DEVICE ACCEPTED |
| `shining_pearl_switch` | Shining Pearl | Switch | 8 | native source foundation | validation pending | PARTIAL SOURCE PATH TESTED |
| `legends_arceus_switch` | Legends: Arceus | Switch | 8 | defensive read foundation | validation pending | PARTIAL PATH TESTED |
| `scarlet_switch` | Scarlet | Switch | 9 | native source foundation | validation pending | NOT DEVICE ACCEPTED |
| `violet_switch` | Violet | Switch | 9 | native source foundation | validation pending | NOT DEVICE ACCEPTED |
| `legends_za_switch` | Legends: Z-A | Switch | 9 | native source foundation | validation pending | PARTIAL PATH TESTED |

FireRed/LeafGreen GBA and FireRed/LeafGreen Switch are deliberately separate identities.

## Exact accepted Gen III editor evidence

```text
source:
996e6aa40c96e4408282f3d55476dae8e64968b2

tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

NRO:
PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro

SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

Actions:
35825830004

Artifact:
10735208869

Status:
DEVICE ACCEPTED
```

Accepted Gen III editor behavior includes shared View/Create/Edit, exact-game fields and move lists, staged source-safe mutation, shiny species preview/commit/cancel, EXP numeric editing, controller-stick parity, readability/theme work, and readable PID/read-only presentation.

## Safety policy

```text
installed Switch source       READ ONLY
RetroArch / legacy source     READ ONLY
other emulator source         READ ONLY
staged workspace              mutable
live installed writing        HARD DISABLED
live emulator writing         HARD DISABLED
true cross-store Move         NOT APPROVED
```

The current next engineering phase is issue #69: storage/transfer/conversion audit and durability hardening before Master Vault and broader game support.
