# PokeBank NX — Game Support / Verification Matrix

Last updated: **2026-09-29**

This matrix separates implemented support from physical device acceptance.

No current adapter is approved for direct live writing to an installed-game save or emulator source.

| Generation | Games | Read / source state | Staged editing state | Device state |
|---|---|---|---|---|
| I | Red / Blue / Yellow | RetroArch + validated legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| II | Gold / Silver / Crystal | RetroArch + validated legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| III | Ruby / Sapphire / Emerald / FireRed / LeafGreen | RetroArch / provider-aware legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| IV | Diamond / Pearl / Platinum / HeartGold / SoulSilver | DraStic / melonDS / remembered/manual assignment | Party + Box View/Edit, Create and native field/move/form work | Base milestone accepted; full integrated G4-04 **HARDWARE PENDING** |
| V | Black / White / Black 2 / White 2 | Not started | Not started | NOT SUPPORTED |
| 3DS | X/Y, ORAS, SM/USUM | Research/provider planning only | Not started | NOT SUPPORTED |
| Modern Switch | LGPE, SWSH, BDSP, PLA, SV, Z-A and tracked identities | validation/source foundation varies by title | production editor/write support not generally advertised | NOT GENERALLY ACCEPTED |

FireRed/LeafGreen GBA and separately tracked Switch release identities remain distinct game/platform identities.

## Generation I

Hardware-accepted foundation includes Trainer / Party / Boxes, generation-correct Pokémon data, Bag/PC Items, strict source validation, shared View/Create/Edit, exact-game move handling and source immutability.

The integrated Product UI now exposes real Gen I Pokédex progress and trainer presentation; that newer shell is part of the combined hardware-pending candidate.

## Generation II

Hardware-accepted foundation includes Trainer / Party / Boxes, Held Item, Friendship, Pokérus, DVs / Stat Exp, native gender/shiny semantics, Crystal-only fields where stored, shared View/Create/Edit, inventory and source immutability.

Gold/Silver do not fabricate a modern SID. The integrated Product UI exposes real Gen II Pokédex progress.

## Generation III

Hardware-accepted foundation includes rotating-sector validation, Trainer / Party / Boxes, PK3 browsing/editing, inventory, Gen III-native fields, shared View/Create/Edit and source immutability.

The integrated Product UI exposes real Gen III Pokédex progress and game/gender-aware trainer presentation.

## Generation IV

Supported identities: Diamond, Pearl, Platinum, HeartGold, SoulSilver.

Source/provider foundation includes DraStic, melonDS and remembered/manual assignment with strict validation before open.

The first staged Party/Box View/Edit milestone is physically accepted.

Current G4-04 implementation includes Party/Box View/Edit, empty Box Add/Create, native Held Item/Language/Ball/Pokérus/Met Location handling, native move selection with species compatibility, exact PP behavior, Species mutation, supported Form editing, trainer identity handling, strict save reparse/checksum/rollback, and external source immutability.

The integrated Product UI carries real Gen IV trainer names, real Gen IV Pokédex progress, Party sprites, and game/gender-grounded trainer portraits.

The full combined G4-04 + Product UI build is **not yet device accepted**.

## Source and launch policy

Discovery and launching are separate from write authorization.

Current provider-aware work includes RetroArch, DraStic, melonDS and manual/remembered bindings. Launch metadata is stored in PokeBank-owned configuration rather than emulator save directories.

If a ROM/content target cannot be proven, the UI requests **Link Game File** instead of inferring it from the save path.

Direct DraStic/melonDS launch handoff remains an integration-verification item for the combined hardware candidate.

## Permanent safety policy

```text
installed Switch source       READ ONLY unless separately approved later
RetroArch / emulator source   READ ONLY unless separately approved later
staged workspace              app-owned
live installed save writing   HARD DISABLED
live emulator-source writing  HARD DISABLED
launch permission             DOES NOT GRANT WRITE ACCESS
ambiguous source/content      FAIL CLOSED
cross-game True Move          LOCKED
```

Malformed or unsupported sources must never be silently repaired, normalized, reassigned, or overwritten.
