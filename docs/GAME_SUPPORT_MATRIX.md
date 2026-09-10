# PokeBank NX — Game Support / Verification Matrix

Last updated: 2026-09-10

This matrix separates identity, source discovery, parser/runtime state and physical device acceptance.

## State meanings

- **IDENTITY TESTED** — stable release/platform identity exists and registry tests pass.
- **NATIVE SOURCE** — installed Switch title can be discovered through the Switch source path.
- **RETROARCH SOURCE** — normal emulator battery-save discovery is integrated.
- **READ ENGINE HOST+NATIVE** — read-only parser/model is verified on host and Switch-native backend.
- **DEVICE TESTED** — a human physically ran the relevant exact artifact.
- **DEVICE ACCEPTED** — that physical test passed the current milestone contract.
- **PLANNED** — not yet implemented for the current production path.

No current adapter is approved for live installed-game or RetroArch source writing.

---

## Current primary identities

| Stable ID | Game | Platform | Gen | Source status | Parser / engine | Device status |
|---|---|---:|---:|---|---|---|
| `red_gb` | Red | GB | 1 | **NEXT: legacy source** | PLANNED | NOT DEVICE TESTED |
| `blue_gb` | Blue | GB | 1 | **NEXT: legacy source** | PLANNED | NOT DEVICE TESTED |
| `yellow_gb` | Yellow | GB | 1 | **NEXT: legacy source** | PLANNED | NOT DEVICE TESTED |
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

The user confirmed during final RSE acceptance testing that the existing Switch save paths still opened normally. This is regression evidence for the current shell/source path; it does not upgrade every modern Switch title to full production acceptance.

---

## Accepted Gen III GBA read-only milestone

FRLG acceptance checkpoint:

```text
8172ebd9c067bd69df63815dbe865207f905eac6
```

Final accepted RSE runtime:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
application tree: 559202d16f0affc13a9e1c521beff4834585b1e7
```

Accepted RSE NRO:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

Physical acceptance covers discovery/open, Trainer, Party, Boxes, Pokémon browsing, Items, game-card artwork, Refresh/current battery-save behavior and read-only source safety for Ruby/Sapphire/Emerald; FRLG additionally exercised profile assignment persistence/isolation in detail.

Host verification run `34453208654` and native build run `34454555232` passed. Embedded RomFS verification was `3286/3286`.

---

## Planned Nintendo DS identities — V1 target

| Stable ID | Game | Gen | Planned source |
|---|---|---:|---|
| `diamond_ds` | Diamond | 4 | emulator/file/manual import |
| `pearl_ds` | Pearl | 4 | emulator/file/manual import |
| `platinum_ds` | Platinum | 4 | emulator/file/manual import |
| `heartgold_ds` | HeartGold | 4 | emulator/file/manual import |
| `soulsilver_ds` | SoulSilver | 4 | emulator/file/manual import |
| `black_ds` | Black | 5 | emulator/file/manual import |
| `white_ds` | White | 5 | emulator/file/manual import |
| `black2_ds` | Black 2 | 5 | emulator/file/manual import |
| `white2_ds` | White 2 | 5 | emulator/file/manual import |

---

## Planned Nintendo 3DS identities — V1 target

| Stable ID | Game | Gen | Planned source |
|---|---|---:|---|
| `x_3ds` | X | 6 | decrypted save/emulator/manual import |
| `y_3ds` | Y | 6 | decrypted save/emulator/manual import |
| `omega_ruby_3ds` | Omega Ruby | 6 | decrypted save/emulator/manual import |
| `alpha_sapphire_3ds` | Alpha Sapphire | 6 | decrypted save/emulator/manual import |
| `sun_3ds` | Sun | 7 | decrypted save/emulator/manual import |
| `moon_3ds` | Moon | 7 | decrypted save/emulator/manual import |
| `ultra_sun_3ds` | Ultra Sun | 7 | decrypted save/emulator/manual import |
| `ultra_moon_3ds` | Ultra Moon | 7 | decrypted save/emulator/manual import |

3DS Virtual Console GB/GBC sources should initially preserve original game identity with VC provenance unless concrete evidence requires distinct stable IDs.

---

## Planned later legacy-console identities

| Stable ID | Game | Platform | Target |
|---|---|---|---|
| `colosseum_gc` | Pokémon Colosseum | GameCube | later expansion |
| `xd_gale_of_darkness_gc` | Pokémon XD: Gale of Darkness | GameCube | later expansion |
| `stadium_n64` | Pokémon Stadium | N64 | stretch / archival |
| `stadium2_n64` | Pokémon Stadium 2 | N64 | stretch / archival |

---

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

---

## Current next source milestone

```text
Red / Blue / Yellow
-> RetroArch normal battery-save discovery
-> strict Gen I size/layout/checksum recognition
-> structural/language variants modeled explicitly
-> Trainer / Party / PC storage / Pokémon view
-> Refresh
-> source-byte immutability
-> host + sanitizer verification
-> native Switch build
-> exact RBY physical-test artifact
```

Then, after RBY physical acceptance, continue to Gold/Silver/Crystal.

See `CURRENT_STATUS.md`, `docs/NEXT_CODEX_PROMPT.md` and issue #6.
