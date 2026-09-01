# PokeBank NX — Game Support / Verification Matrix

Last updated: 2026-09-01

This matrix prevents "listed", "detected", "readable", "writable", and "device tested" from being confused with one another.

## State meanings

- **IDENTITY TESTED** — canonical ID exists and registry tests pass.
- **NATIVE SOURCE** — installed Switch title can be mapped by title ID in the recovered PokeBank layer.
- **PLANNED SOURCE** — identity exists, but source discovery/parser integration is not implemented in the recovered tree.
- **UPSTREAM FOUNDATION** — PKSE contains relevant save/Pokémon support, but PokeBank-specific source/safety integration is not yet fully validated.
- **READ ONLY** — PokeBank adapter has passed read-only integration tests.
- **STAGED WRITE** — offline working-copy mutation/validation exists; this does not mean live save writing.
- **LIVE WRITE** — only used after the gates in `SAVE_SAFETY.md` are met for that exact adapter.
- **DEVICE TESTED** — a human physically tested the exact relevant build on Nintendo Switch hardware.

No current game adapter is approved for PokeBank NX live installed-save writing.

---

## Current 23-identity target registry

| Stable ID | Game | Platform | Gen | Current source status | Parser/handler status | Device status |
|---|---|---:|---:|---|---|---|
| `red_gb` | Red | Game Boy | 1 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK1/Sav1 | NOT DEVICE TESTED |
| `blue_gb` | Blue | Game Boy | 1 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK1/Sav1 | NOT DEVICE TESTED |
| `yellow_gb` | Yellow | Game Boy | 1 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK1/Sav1 | NOT DEVICE TESTED |
| `gold_gbc` | Gold | Game Boy Color | 2 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK2/Sav2 | NOT DEVICE TESTED |
| `silver_gbc` | Silver | Game Boy Color | 2 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK2/Sav2 | NOT DEVICE TESTED |
| `crystal_gbc` | Crystal | Game Boy Color | 2 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK2/Sav2 | NOT DEVICE TESTED |
| `ruby_gba` | Ruby | Game Boy Advance | 3 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK3/Sav3 | NOT DEVICE TESTED |
| `sapphire_gba` | Sapphire | Game Boy Advance | 3 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK3/Sav3 | NOT DEVICE TESTED |
| `emerald_gba` | Emerald | Game Boy Advance | 3 | PLANNED SOURCE | Not recovered; audit PKSM-Core PK3/Sav3 | NOT DEVICE TESTED |
| `firered_gba` | FireRed | Game Boy Advance | 3 | PLANNED SOURCE | First PKSM-Core PK3/Sav3 spike target | NOT DEVICE TESTED |
| `leafgreen_gba` | LeafGreen | Game Boy Advance | 3 | PLANNED SOURCE | First PKSM-Core PK3/Sav3 spike target | NOT DEVICE TESTED |
| `firered_switch` | FireRed | Nintendo Switch | 3 | NATIVE SOURCE | PKSE foundation present; outer container needs PokeBank validation | NOT DEVICE TESTED |
| `leafgreen_switch` | LeafGreen | Nintendo Switch | 3 | NATIVE SOURCE | PKSE foundation present; outer container needs PokeBank validation | NOT DEVICE TESTED |
| `letsgo_pikachu_switch` | Let's Go Pikachu | Nintendo Switch | 7 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `letsgo_eevee_switch` | Let's Go Eevee | Nintendo Switch | 7 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `sword_switch` | Sword | Nintendo Switch | 8 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `shield_switch` | Shield | Nintendo Switch | 8 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `brilliant_diamond_switch` | Brilliant Diamond | Nintendo Switch | 8 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `shining_pearl_switch` | Shining Pearl | Nintendo Switch | 8 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `legends_arceus_switch` | Legends Arceus | Nintendo Switch | 8 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `scarlet_switch` | Scarlet | Nintendo Switch | 9 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `violet_switch` | Violet | Nintendo Switch | 9 | NATIVE SOURCE | PKSE foundation present; read-only adapter validation pending | NOT DEVICE TESTED |
| `legends_za_switch` | Legends Z-A | Nintendo Switch | 9 | NATIVE SOURCE | PKSE foundation present; pkHouse/PKHeX comparison pending | NOT DEVICE TESTED |

All 23 identities are part of the host-tested registry. The four FireRed/LeafGreen GBA/Switch identities are deliberately distinct.

---

## Current Switch title IDs in the PokeBank registry

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

These are detection identifiers, not proof that every game/save version has been fully parsed or safely validated.

---

# Adapter verification matrix

Each source adapter should eventually track these gates independently.

| Adapter/family | Detect | Read party | Read boxes | Raw Pokémon export | Untouched round trip | Stage mutation | Validate stage | Live write | Device tested |
|---|---|---|---|---|---|---|---|---|---|
| GB R/B/Y | ID only | — | — | — | — | — | — | **NO** | NO |
| GBC G/S/C | ID only | — | — | — | — | — | — | **NO** | NO |
| GBA R/S/E | ID only | — | — | — | — | — | — | **NO** | NO |
| GBA FR/LG | ID only | — | — | — | — | — | — | **NO** | NO |
| Switch FR/LG | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| LGPE | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| SwSh | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| BDSP | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| PLA | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| SV | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |
| Z-A | title-ID mapping | upstream foundation | upstream foundation | upstream foundation | pending | disabled | pending | **HARD DISABLED** | NO |

`upstream foundation` means the imported PKSE code contains relevant machinery; it does not claim that the PokeBank-specific path has passed the listed gate.

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

## RetroArch / GB/GBC/GBA

Planned source types:

```text
RetroArch save directories
manual imported save
backup file
```

Discovery should produce a source record containing:

```text
canonical game ID
platform
source kind
path
save fingerprint
parser/adapter
read/write capabilities
```

Do not infer game identity from filename alone when save structure/content can verify it.

---

# First test fixtures

The first useful fixture set should cover:

1. `firered_gba`
   - valid copied save
   - both active-slot arrangements
   - Pokémon near a sector payload boundary
   - empty/occupied first and last box slots
2. `firered_switch`
   - copied title save file/container from a non-critical test environment
   - compare inner Gen III behavior with `firered_gba`
   - verify platform provenance remains Switch
3. one modern SCBlock title
   - preferably Scarlet or Z-A
   - known party/box samples
4. LGPE
   - flat storage + party-pointer cases
5. BDSP
   - at least one known supported save-size/version fixture

Fixtures must be generated/owned/redistributable appropriately; do not commit copyrighted game ROMs or unrelated proprietary data.

---

# Promotion checklist for one game

A game can move from identity-only toward real support in stages:

```text
IDENTITY TESTED
      ↓
SOURCE DETECTED
      ↓
READ PARTY
      ↓
READ BOXES
      ↓
EXPORT TO VAULT
      ↓
ROUND-TRIP VALIDATED
      ↓
STAGED MUTATION
      ↓
STAGED VALIDATION
      ↓
DEVICE READ TEST
      ↓
DISPOSABLE-SAVE WRITE TEST (future)
      ↓
LIVE WRITE CAPABILITY (per adapter, much later)
```

Do not skip directly from "PKSE supports this game" to "PokeBank supports live transfers."
