# PokeBank NX — Game Support / Verification Matrix

Last updated: 2026-09-07

This matrix distinguishes identity, source discovery, parser support, native-runtime support and physical device validation.

## State meanings

- **IDENTITY TESTED** — stable release/platform ID exists and registry tests pass.
- **NATIVE SOURCE** — installed Switch title can be discovered through the existing Switch source path.
- **RETROARCH SOURCE IN PROGRESS** — bounded emulator/file discovery is being wired into the runtime.
- **READ ENGINE HOST+NATIVE** — the read-only parser/model is proven both through the host oracle and the Switch-native backend.
- **READ ENGINE PLANNED** — parser/adapter work not yet implemented.
- **DEVICE TESTED** — a human physically tested the exact relevant `.nro`/hash.
- **V1 TARGET** — planned core release identity/source.
- **V1 STRETCH** — worthwhile but may slip if disproportionately expensive.

No current game adapter is approved for live installed-game save writing.

---

## Catalog summary

```text
Current host-tested source registry:       23 identities
Planned Nintendo DS additions:              9
Planned Nintendo 3DS additions:             8
Planned GameCube additions:                 2
Planned N64 Stadium stretch additions:      2
Total target catalog:                      44
```

---

## Current 23 host-tested identities

| Stable ID | Game | Platform | Gen | Source status | Parser / engine status | Device status |
|---|---|---:|---:|---|---|---|
| `red_gb` | Red | GB | 1 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `blue_gb` | Blue | GB | 1 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `yellow_gb` | Yellow | GB | 1 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `gold_gbc` | Gold | GBC | 2 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `silver_gbc` | Silver | GBC | 2 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `crystal_gbc` | Crystal | GBC | 2 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `ruby_gba` | Ruby | GBA | 3 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `sapphire_gba` | Sapphire | GBA | 3 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `emerald_gba` | Emerald | GBA | 3 | planned legacy source | READ ENGINE PLANNED | NOT DEVICE TESTED |
| `firered_gba` | FireRed | GBA | 3 | RETROARCH SOURCE IN PROGRESS | **READ ENGINE HOST+NATIVE** | NOT DEVICE TESTED |
| `leafgreen_gba` | LeafGreen | GBA | 3 | RETROARCH SOURCE IN PROGRESS | **READ ENGINE HOST+NATIVE** | NOT DEVICE TESTED |
| `firered_switch` | FireRed | Switch | 3 | NATIVE SOURCE | inherited foundation; production validation pending | NOT DEVICE TESTED |
| `leafgreen_switch` | LeafGreen | Switch | 3 | NATIVE SOURCE | inherited foundation; production validation pending | NOT DEVICE TESTED |
| `letsgo_pikachu_switch` | Let's Go, Pikachu! | Switch | 7 | NATIVE SOURCE | production read validation pending | PARTIAL SOURCE PATH DEVICE TESTED |
| `letsgo_eevee_switch` | Let's Go, Eevee! | Switch | 7 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `sword_switch` | Sword | Switch | 8 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `shield_switch` | Shield | Switch | 8 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `brilliant_diamond_switch` | Brilliant Diamond | Switch | 8 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `shining_pearl_switch` | Shining Pearl | Switch | 8 | NATIVE SOURCE | production read validation pending | PARTIAL SOURCE PATH DEVICE TESTED |
| `legends_arceus_switch` | Legends: Arceus | Switch | 8 | NATIVE SOURCE | defensive read path hardened | **PARTIAL PATH DEVICE TESTED** |
| `scarlet_switch` | Scarlet | Switch | 9 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `violet_switch` | Violet | Switch | 9 | NATIVE SOURCE | production read validation pending | NOT DEVICE TESTED |
| `legends_za_switch` | Legends: Z-A | Switch | 9 | NATIVE SOURCE | production read validation pending | **PARTIAL PATH DEVICE TESTED** |

FireRed/LeafGreen GBA and FireRed/LeafGreen Switch are deliberately separate identities and must never be collapsed.

### Verified FRLG GBA engine checkpoints

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a  PKSM-Core host oracle / adapter
43f3a9f90a3314725979d59afdd68f19ee159009  exception-free Switch-native Gen III backend
```

Current FRLG source limitation: parser/runtime engine is ready, but the bounded RetroArch source catalog still needs recovery + real runtime registration before FRLG GBA is considered a complete user-facing source path.

---

## Planned Nintendo DS identities — V1 TARGET / NOT IDENTITY TESTED

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

Issues #30 / #31.

---

## Planned Nintendo 3DS identities — V1 TARGET / NOT IDENTITY TESTED

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

Issues #30 / #32.

3DS Virtual Console GB/GBC titles should initially preserve the original game identity with source/provenance noting 3DS VC origin unless later evidence requires distinct stable IDs.

---

## Planned GameCube identities — V1 TARGET

| Stable ID | Game | Gen | Planned source |
|---|---|---:|---|
| `colosseum_gc` | Pokémon Colosseum | 3 | `.gci`, memory-card image, Dolphin/manual import |
| `xd_gale_of_darkness_gc` | Pokémon XD: Gale of Darkness | 3 | `.gci`, memory-card image, Dolphin/manual import |

Issue #33. Initial support is read-only.

---

## Planned N64 Stadium identities — V1 STRETCH

| Stable ID | Game | Gen context | Planned source |
|---|---|---:|---|
| `stadium_n64` | Pokémon Stadium | Gen I storage | emulator/N64 save/manual import |
| `stadium2_n64` | Pokémon Stadium 2 | Gen II storage | emulator/N64 save/manual import |

Issue #34. Stadium should not block v1 if it becomes disproportionately expensive.

---

## Current safety policy

```text
installed Switch source       READ ONLY
RetroArch / legacy source     READ ONLY during parser milestones
backup/staged workspace       separate from installed source
legacy PKSE Storage           app-owned compatibility storage
Master Vault                  NOT IMPLEMENTED YET
live installed save writing   HARD DISABLED
true Move                     NOT IMPLEMENTED
```

Malformed/unsupported sources must fail safely and must never be auto-repaired or overwritten.

---

## Current next source milestone

```text
recover interrupted RetroArch FRLG catalog
-> register it in the real source-discovery lifecycle
-> validate .sav/.srm candidates
-> safely classify FireRed/LeafGreen when evidence is sufficient
-> Party / Boxes through native Gen III adapter
-> physical Switch test of the exact resulting artifact when warranted
```

See `CURRENT_STATUS.md`, `docs/NEXT_SESSION_PLAN.md`, `docs/NEXT_CODEX_PROMPT.md` and issue #6.
