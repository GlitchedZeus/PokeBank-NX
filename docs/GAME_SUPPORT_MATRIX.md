# PokeBank NX — Game Support / Verification Matrix

Last updated: 2026-09-11

This matrix separates identity/source integration, host/native read-engine verification, and physical device acceptance.

## State meanings

- **RETROARCH SOURCE** — normal emulator battery-save discovery is integrated.
- **NATIVE SOURCE** — installed Switch title can be discovered through the Switch source path.
- **READ ENGINE HOST+NATIVE** — read-only parser/model is verified on host and through the native Switch build path.
- **DEVICE TESTED** — a human physically ran the relevant exact artifact.
- **DEVICE ACCEPTED** — that physical test passed the current milestone contract.
- **PLANNED** — not yet implemented for the production path.

No current adapter is approved for live installed-game or RetroArch source writing.

## Current primary identities

| Stable ID | Game | Platform | Gen | Source status | Parser / engine | Device status |
|---|---|---:|---:|---|---|---|
| `red_gb` | Red | GB | 1 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **DEVICE ACCEPTED** |
| `blue_gb` | Blue | GB | 1 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **DEVICE ACCEPTED** |
| `yellow_gb` | Yellow | GB | 1 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **DEVICE ACCEPTED** |
| `gold_gbc` | Gold | GBC | 2 | planned legacy source | PLANNED — NEXT MILESTONE | NOT DEVICE TESTED |
| `silver_gbc` | Silver | GBC | 2 | planned legacy source | PLANNED — NEXT MILESTONE | NOT DEVICE TESTED |
| `crystal_gbc` | Crystal | GBC | 2 | planned legacy source | PLANNED — NEXT MILESTONE | NOT DEVICE TESTED |
| `ruby_gba` | Ruby | GBA | 3 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **DEVICE ACCEPTED** |
| `sapphire_gba` | Sapphire | GBA | 3 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **DEVICE ACCEPTED** |
| `emerald_gba` | Emerald | GBA | 3 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **DEVICE ACCEPTED** |
| `firered_gba` | FireRed | GBA | 3 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **DEVICE ACCEPTED** |
| `leafgreen_gba` | LeafGreen | GBA | 3 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **DEVICE ACCEPTED** |
| `firered_switch` | FireRed | Switch | 3 | NATIVE SOURCE | production validation pending | NOT FULLY ACCEPTED |
| `leafgreen_switch` | LeafGreen | Switch | 3 | NATIVE SOURCE | production validation pending | NOT FULLY ACCEPTED |
| `letsgo_pikachu_switch` | Let's Go, Pikachu! | Switch | 7 | NATIVE SOURCE | production read validation pending | PARTIAL SOURCE PATH DEVICE TESTED |
| `letsgo_eevee_switch` | Let's Go, Eevee! | Switch | 7 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `sword_switch` | Sword | Switch | 8 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `shield_switch` | Shield | Switch | 8 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `brilliant_diamond_switch` | Brilliant Diamond | Switch | 8 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `shining_pearl_switch` | Shining Pearl | Switch | 8 | NATIVE SOURCE | production read validation pending | PARTIAL SOURCE PATH DEVICE TESTED |
| `legends_arceus_switch` | Legends: Arceus | Switch | 8 | NATIVE SOURCE | defensive read path hardened | PARTIAL PATH DEVICE TESTED |
| `scarlet_switch` | Scarlet | Switch | 9 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `violet_switch` | Violet | Switch | 9 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `legends_za_switch` | Legends: Z-A | Switch | 9 | NATIVE SOURCE | production read validation pending | PARTIAL PATH DEVICE TESTED |

FireRed/LeafGreen GBA and FireRed/LeafGreen Switch remain deliberately separate identities.

## Accepted Generation I RBY milestone

Accepted runtime and device artifact:

```text
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
Verification run: 34576027301
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Device/package run: 34576781488
Embedded RomFS: 3289/3289 PASS

PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
bytes: 159754197
SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
```

```text
Red: IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / DEVICE ACCEPTED
Blue: IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / DEVICE ACCEPTED
Yellow: IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / DEVICE ACCEPTED
GEN I RBY LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Accepted RBY capability is bounded normal RetroArch battery-save discovery, strict Gen I validation, international/Japanese handling, Trainer, Party, PC Boxes, PK1/Pokémon details, Bag, PC Items, dedicated Gen I item naming, GB platform identity, malformed-optional-inventory isolation and source immutability.

Historical first device-test runtime `d9077e2d` remains recorded because it exposed the Items/category and GB/GBA label defects before the accepted ItemsFix retest.

## Accepted Generation III GBA milestone

FireRed, LeafGreen, Ruby, Sapphire and Emerald are physically accepted for the current read-only legacy milestone.

Accepted RSE runtime:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
```

Accepted RSE NRO SHA-256:

```text
34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

Do not downgrade or reopen accepted Gen I/III baselines without new physical evidence.

## Current safety policy

```text
installed Switch source       READ ONLY
RetroArch / legacy source     READ ONLY
backup/staged workspace       separate from installed source
Master Vault                  NOT IMPLEMENTED YET
live installed save writing   HARD DISABLED
live RetroArch save writing   HARD DISABLED
true Move                     NOT IMPLEMENTED
```

Malformed/unsupported sources must fail safely and must never be silently repaired or overwritten.

## Next legacy milestone

Generation II Gold/Silver/Crystal (`gold_gbc`, `silver_gbc`, `crystal_gbc`) on Game Boy Color / RetroArch is next. Production GSC implementation has not started yet.
