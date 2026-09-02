# PokeBank NX — Game Support / Verification Matrix

Last updated: 2026-09-02

This matrix prevents **targeted**, **identity-tested**, **detected**, **readable**, **writable**, and **device-tested** from being confused.

## State meanings

- **V1 TARGET** — game is explicitly in the intended v1.0 catalog, but may not exist in source yet.
- **IDENTITY TESTED** — canonical ID exists in source and registry tests pass.
- **NATIVE SOURCE** — installed Switch title can be mapped by title ID in the recovered PokeBank layer.
- **PLANNED SOURCE** — source identity/path/parser integration is planned but not complete.
- **UPSTREAM FOUNDATION** — inherited/reference code contains relevant save/Pokémon machinery but PokeBank-specific validation is incomplete.
- **READ ONLY** — PokeBank adapter passed its read-only integration gate.
- **STAGED WRITE** — isolated working-copy mutation/validation exists; this is not live save writing.
- **LIVE WRITE** — only after the exact adapter passes `SAVE_SAFETY.md`.
- **DEVICE TESTED** — a human physically tested the exact relevant `.nro`/hash on Switch.

No current game adapter is approved for PokeBank NX live installed-save writing.

---

# Catalog summary

The earlier 23-game list was an **implementation checkpoint**, not the complete product goal.

```text
Current host-tested source registry: 23 identities
Planned Nintendo DS additions:       9 identities
Planned Nintendo 3DS additions:      8 identities
V1 main-series target catalog:      40 identities
```

Issues:

```text
#30 add DS/3DS identities + source discovery
#31 read-only Nintendo DS Gen IV/V adapters
#32 read-only Nintendo 3DS Gen VI/VII adapters
```

Do not call all 40 identities `IDENTITY TESTED` until #30 adds the 17 new IDs to source and host tests pass.

---

# Current 23 host-tested source identities

| Stable ID | Game | Platform | Gen | Current source status | Parser/handler status | Device status |
|---|---|---:|---:|---|---|---|
| `red_gb` | Red | Game Boy | 1 | PLANNED SOURCE | audit PKSM-Core PK1/Sav1 | NOT DEVICE TESTED |
| `blue_gb` | Blue | Game Boy | 1 | PLANNED SOURCE | audit PKSM-Core PK1/Sav1 | NOT DEVICE TESTED |
| `yellow_gb` | Yellow | Game Boy | 1 | PLANNED SOURCE | audit PKSM-Core PK1/Sav1 | NOT DEVICE TESTED |
| `gold_gbc` | Gold | Game Boy Color | 2 | PLANNED SOURCE | audit PKSM-Core PK2/Sav2 | NOT DEVICE TESTED |
| `silver_gbc` | Silver | Game Boy Color | 2 | PLANNED SOURCE | audit PKSM-Core PK2/Sav2 | NOT DEVICE TESTED |
| `crystal_gbc` | Crystal | Game Boy Color | 2 | PLANNED SOURCE | audit PKSM-Core PK2/Sav2 | NOT DEVICE TESTED |
| `ruby_gba` | Ruby | Game Boy Advance | 3 | PLANNED SOURCE | audit PKSM-Core PK3/Sav3 | NOT DEVICE TESTED |
| `sapphire_gba` | Sapphire | Game Boy Advance | 3 | PLANNED SOURCE | audit PKSM-Core PK3/Sav3 | NOT DEVICE TESTED |
| `emerald_gba` | Emerald | Game Boy Advance | 3 | PLANNED SOURCE | audit PKSM-Core PK3/Sav3 | NOT DEVICE TESTED |
| `firered_gba` | FireRed | Game Boy Advance | 3 | PLANNED SOURCE | first PKSM-Core PK3/Sav3 spike target | NOT DEVICE TESTED |
| `leafgreen_gba` | LeafGreen | Game Boy Advance | 3 | PLANNED SOURCE | first PKSM-Core PK3/Sav3 spike target | NOT DEVICE TESTED |
| `firered_switch` | FireRed | Nintendo Switch | 3 | NATIVE SOURCE | PKSE foundation; outer container validation pending | NOT DEVICE TESTED |
| `leafgreen_switch` | LeafGreen | Nintendo Switch | 3 | NATIVE SOURCE | PKSE foundation; outer container validation pending | NOT DEVICE TESTED |
| `letsgo_pikachu_switch` | Let's Go, Pikachu! | Nintendo Switch | 7 | NATIVE SOURCE | read-only adapter validation pending | NOT DEVICE TESTED |
| `letsgo_eevee_switch` | Let's Go, Eevee! | Nintendo Switch | 7 | NATIVE SOURCE | read-only adapter validation pending | NOT DEVICE TESTED |
| `sword_switch` | Sword | Nintendo Switch | 8 | NATIVE SOURCE | read-only adapter validation pending | NOT DEVICE TESTED |
| `shield_switch` | Shield | Nintendo Switch | 8 | NATIVE SOURCE | read-only adapter validation pending | NOT DEVICE TESTED |
| `brilliant_diamond_switch` | Brilliant Diamond | Nintendo Switch | 8 | NATIVE SOURCE | read-only adapter validation pending | NOT DEVICE TESTED |
| `shining_pearl_switch` | Shining Pearl | Nintendo Switch | 8 | NATIVE SOURCE | read-only adapter validation pending | NOT DEVICE TESTED |
| `legends_arceus_switch` | Legends: Arceus | Nintendo Switch | 8 | NATIVE SOURCE | read-only adapter validation pending; old-save crash #24 | PARTIAL PATH DEVICE TESTED |
| `scarlet_switch` | Scarlet | Nintendo Switch | 9 | NATIVE SOURCE | read-only adapter validation pending | NOT DEVICE TESTED |
| `violet_switch` | Violet | Nintendo Switch | 9 | NATIVE SOURCE | read-only adapter validation pending | NOT DEVICE TESTED |
| `legends_za_switch` | Legends: Z-A | Nintendo Switch | 9 | NATIVE SOURCE | pkHouse/PKHeX comparison pending | PARTIAL PATH DEVICE TESTED |

All 23 IDs above exist in the current host-tested registry. GBA FireRed/LeafGreen and Switch FireRed/LeafGreen are deliberately distinct.

---

# Planned Nintendo DS identities — issue #30

These are **V1 TARGET / NOT YET IDENTITY TESTED** until added to source.

| Proposed stable ID | Game | Platform | Gen | Initial source model | Current status |
|---|---|---:|---:|---|---|
| `diamond_ds` | Diamond | Nintendo DS | 4 | emulator/file/manual import | V1 TARGET |
| `pearl_ds` | Pearl | Nintendo DS | 4 | emulator/file/manual import | V1 TARGET |
| `platinum_ds` | Platinum | Nintendo DS | 4 | emulator/file/manual import | V1 TARGET |
| `heartgold_ds` | HeartGold | Nintendo DS | 4 | emulator/file/manual import | V1 TARGET |
| `soulsilver_ds` | SoulSilver | Nintendo DS | 4 | emulator/file/manual import | V1 TARGET |
| `black_ds` | Black | Nintendo DS | 5 | emulator/file/manual import | V1 TARGET |
| `white_ds` | White | Nintendo DS | 5 | emulator/file/manual import | V1 TARGET |
| `black2_ds` | Black 2 | Nintendo DS | 5 | emulator/file/manual import | V1 TARGET |
| `white2_ds` | White 2 | Nintendo DS | 5 | emulator/file/manual import | V1 TARGET |

Read adapter milestone: **#31**.

---

# Planned Nintendo 3DS identities — issue #30

These are **V1 TARGET / NOT YET IDENTITY TESTED** until added to source.

| Proposed stable ID | Game | Platform | Gen | Initial source model | Current status |
|---|---|---:|---:|---|---|
| `x_3ds` | X | Nintendo 3DS | 6 | decrypted save/emulator/manual import | V1 TARGET |
| `y_3ds` | Y | Nintendo 3DS | 6 | decrypted save/emulator/manual import | V1 TARGET |
| `omega_ruby_3ds` | Omega Ruby | Nintendo 3DS | 6 | decrypted save/emulator/manual import | V1 TARGET |
| `alpha_sapphire_3ds` | Alpha Sapphire | Nintendo 3DS | 6 | decrypted save/emulator/manual import | V1 TARGET |
| `sun_3ds` | Sun | Nintendo 3DS | 7 | decrypted save/emulator/manual import | V1 TARGET |
| `moon_3ds` | Moon | Nintendo 3DS | 7 | decrypted save/emulator/manual import | V1 TARGET |
| `ultra_sun_3ds` | Ultra Sun | Nintendo 3DS | 7 | decrypted save/emulator/manual import | V1 TARGET |
| `ultra_moon_3ds` | Ultra Moon | Nintendo 3DS | 7 | decrypted save/emulator/manual import | V1 TARGET |

Read adapter milestone: **#32**.

### 3DS Virtual Console note

Red/Blue/Yellow/Gold/Silver/Crystal Virtual Console saves should initially be represented as **source/platform variants of the original GB/GBC game identities**, with provenance noting 3DS VC origin, unless technical evidence later requires distinct stable release IDs.

Do not duplicate identities purely because the save was exported from a 3DS.

---

# Current Switch title IDs in the PokeBank registry

| Stable ID | Title ID |
|---|---|
| `firered_switch` | `0100554023408000` |
| `leafgreen_switch` | `010034D02340E000` |
| `letsgo_pikachu_switch` | `010003F003A34000` |
| `letsgo_eevee_switch` | `0100187003A36000` |
| `sword_switch` | `0100ABF008968000` |
| `shield_switch` | `01008DB008C2C000` |
| `brilliant_diamond_switch` | `0100000011D90000` |
| `shining_pearl_switch` | `010018E011D92000` |
| `legends_arceus_switch` | `01001F5010DFA000` |
| `scarlet_switch` | `0100A3D008C5C000` |
| `violet_switch` | `01008F6008C5E000` |
| `legends_za_switch` | `0100F43008C44000` |

These are detection identifiers, not proof that every save version is safely validated.

---

# Adapter verification matrix

Each adapter/family tracks gates independently.

| Adapter/family | Detect/source | Read party | Read boxes | Raw Pokémon export | Untouched round trip | Stage mutation | Validate stage | Live write | Device tested |
|---|---|---|---|---|---|---|---|---|---|
| GB R/B/Y | ID only | — | — | — | — | — | — | **NO** | NO |
| GBC G/S/C | ID only | — | — | — | — | — | — | **NO** | NO |
| GBA R/S/E | ID only | — | — | — | — | — | — | **NO** | NO |
| GBA FR/LG | ID only | — | — | — | — | — | — | **NO** | NO |
| NDS Gen IV D/P/Pt/HG/SS | planned file source | — | — | — | — | — | — | **NO** | NO |
| NDS Gen V B/W/B2/W2 | planned file source | — | — | — | — | — | — | **NO** | NO |
| 3DS Gen VI X/Y/ORAS | planned decrypted/file source | — | — | — | — | — | — | **NO** | NO |
| 3DS Gen VII S/M/US/UM | planned decrypted/file source | — | — | — | — | — | — | **NO** | NO |
| Switch FR/LG | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| LGPE | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| SwSh | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| BDSP | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| PLA | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | PARTIAL PATH TESTED |
| SV | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| Z-A | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | PARTIAL PATH TESTED |

`upstream foundation` means inherited code contains relevant machinery; it is not proof of a PokeBank NX adapter gate.

---

# Source discovery plan

## Nintendo Switch

Source type:

```text
installed title save
```

Detection:

```text
exact title ID -> canonical PokeBank game ID
```

Current behavior must remain read-only from the PokeBank safety layer.

## GB / GBC / GBA

Planned source types:

```text
RetroArch save directories
manual imported save
backup file
```

## Nintendo DS

Planned source types:

```text
melonDS save files
RetroArch DS core save paths where applicable
conventional validated .sav exports
manual import
```

DS games are not native Horizon titles, so do not model them as installed Switch saves.

## Nintendo 3DS

Planned source types:

```text
decrypted Checkpoint/JKSM-style exports
Citra/Lime3DS-compatible save files/directories where accessible
manual imported main/save files
```

3DS games are not native Horizon titles, so do not model them as installed Switch saves.

Every discovered source should resolve to:

```text
canonical game ID
platform
source kind
path
save fingerprint
parser/adapter
read/write capabilities
provenance metadata
```

Do not infer game identity from filename alone when save structure/content can verify it.

---

# Integration order

The new DS/3DS catalog does **not** change the immediate Session 2.6 blocker order.

Recommended engine sequence:

```text
Session 2.6 safety/crash + device test #2
        ↓
PKSM-Core Gen III spike (#4)
        ↓
Gen III production read adapter
        ↓
Master Vault foundation
        ↓
Gen I/II legacy reads (#6)
        ↓
Nintendo DS Gen IV/V reads (#31)
        ↓
Nintendo 3DS Gen VI/VII reads (#32)
        ↓
modern Switch validation (#11)
```

Some later phases may be parallelized once the common adapter/Vault interfaces are stable.

---

# Fixture priorities

Initial fixture milestones should expand over time:

1. Gen III FireRed/LeafGreen for #4.
2. Gen I/II samples for #6.
3. one valid + malformed save per DS family for #31.
4. one valid + malformed save per 3DS family for #32.
5. Switch family fixtures for #11.

Fixtures must be generated/owned/redistributable appropriately; do not commit ROMs, keys, credentials, private user saves, or unrelated proprietary data.

---

# Promotion checklist for one game/family

```text
V1 TARGET
      ↓
IDENTITY TESTED
      ↓
SOURCE DETECTED / IMPORTED
      ↓
READ PARTY
      ↓
READ BOXES
      ↓
EXPORT/IMPORT TO VAULT
      ↓
ROUND-TRIP VALIDATED
      ↓
STAGED MUTATION
      ↓
STAGED VALIDATION
      ↓
DEVICE READ TEST
      ↓
WRITE TEST FOR THAT SOURCE/ADAPTER (future, if applicable)
      ↓
LIVE WRITE CAPABILITY (per adapter, much later)
```

Do not skip directly from “upstream supports this game” to “PokeBank NX supports live transfers.”