# PokeBank NX — Game Support / Verification Matrix

Last updated: 2026-09-11

This matrix separates identity/source integration, host/native read-engine verification, and physical device acceptance.

## State meanings

- **RETROARCH SOURCE** — normal emulator battery-save discovery is integrated.
- **NATIVE SOURCE** — installed Switch title can be discovered through the Switch source path.
- **READ ENGINE HOST+NATIVE** — read-only parser/model is verified on host and through the native Switch build path.
- **READY FOR PHYSICAL TEST** — exact NRO exists and passed build/packaging verification, but the user has not yet run it on hardware.
- **DEVICE TESTED** — a human physically ran the relevant exact artifact.
- **DEVICE ACCEPTED** — that physical test passed the current milestone contract.
- **PLANNED** — not yet implemented for the production path.

No current adapter is approved for live installed-game or RetroArch source writing.

## Current primary identities

| Stable ID | Game | Platform | Gen | Source status | Parser / engine | Device status |
|---|---|---:|---:|---|---|---|
| `red_gb` | Red | GB | 1 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **READY FOR PHYSICAL TEST / NOT DEVICE TESTED** |
| `blue_gb` | Blue | GB | 1 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **READY FOR PHYSICAL TEST / NOT DEVICE TESTED** |
| `yellow_gb` | Yellow | GB | 1 | **RETROARCH SOURCE** | **READ ENGINE HOST+NATIVE** | **READY FOR PHYSICAL TEST / NOT DEVICE TESTED** |
| `gold_gbc` | Gold | GBC | 2 | planned legacy source | PLANNED | NOT DEVICE TESTED |
| `silver_gbc` | Silver | GBC | 2 | planned legacy source | PLANNED | NOT DEVICE TESTED |
| `crystal_gbc` | Crystal | GBC | 2 | planned legacy source | PLANNED | NOT DEVICE TESTED |
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

## Generation I RBY build record

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Host gate run: 34559821944
Device/package run: 34566567906
Native devkitA64 compile/final link: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
```

Exact physical-test NRO:

```text
PokeBank-NX-RBY-Retest-d9077e2d.nro
bytes: 159741909
SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

```text
Red: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Blue: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
Yellow: IMPLEMENTED / HOST TESTED / NRO BUILDS / READY FOR PHYSICAL TEST
DEVICE TESTED FOR RBY: NO
DEVICE ACCEPTED FOR RBY: NO
```

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

Do not downgrade or reopen this accepted baseline without new physical evidence.

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

## Current stop state

The next required action is the user's physical Red/Blue/Yellow Switch test of `PokeBank-NX-RBY-Retest-d9077e2d.nro`.

Gold/Silver/Crystal and later roadmap work remain parked until that result is reported.
