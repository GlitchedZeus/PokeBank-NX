# PokeBank NX — Game Support / Verification Matrix

Last updated: **2026-09-29**

This matrix separates implemented support from physical device acceptance.

No current adapter is approved for direct live writing to an installed-game save or emulator source.

| Generation | Games | Read / source state | Staged editing state | Device state |
|---|---|---|---|---|
| I | Red / Blue / Yellow | RetroArch + validated legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| II | Gold / Silver / Crystal | RetroArch + validated legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| III | Ruby / Sapphire / Emerald / FireRed / LeafGreen | RetroArch / provider-aware legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| IV | Diamond / Pearl / Platinum / HeartGold / SoulSilver | DraStic / melonDS / remembered/manual assignment foundation | Party + Box View/Edit, Create, native field/move/form work | Base milestone accepted; full G4-04 **HARDWARE PENDING** |
| V | Black / White / Black 2 / White 2 | Not started | Not started | NOT SUPPORTED |
| 3DS | X/Y, ORAS, SM/USUM | Provider/research planning only | Not started | NOT SUPPORTED |
| Modern Switch | LGPE, SWSH, BDSP, PLA, SV, Z-A and other tracked identities | validation/source foundation varies by title | production editor/write support not advertised | NOT GENERALLY ACCEPTED |

FireRed/LeafGreen GBA and any separately tracked Switch release identities remain distinct game/platform identities.

## Generation I

Hardware-accepted current foundation includes:

- Trainer / Party / Boxes;
- generation-correct Pokémon data;
- Bag/PC Items;
- strict source validation;
- staged shared View/Create/Edit;
- exact-game move handling;
- source immutability.

Current integrated development also exposes real per-save Pokédex progress and grounded trainer presentation in the Product Home path. That newer presentation still requires integrated hardware testing.

## Generation II

Hardware-accepted current foundation includes:

- Trainer / Party / Boxes;
- Held Item, Friendship and Pokérus;
- DVs / Stat Exp;
- gender/shiny semantics;
- Crystal-specific native fields;
- staged shared View/Create/Edit;
- inventory;
- source immutability.

Gold/Silver do not fabricate a modern SID. Crystal-specific data is only shown where the exact format stores it.

## Generation III

Hardware-accepted current foundation includes:

- strict rotating-sector validation;
- Trainer / Party / Boxes;
- PK3 browsing/editing;
- inventory;
- exact Gen III-native fields;
- staged shared View/Create/Edit;
- source immutability.

Current integrated development adds real per-save Pokédex progress and game/gender-aware trainer presentation to the Product Home layer.

## Generation IV

Supported identities:

- Diamond
- Pearl
- Platinum
- HeartGold
- SoulSilver

Source/provider foundation includes:

- DraStic;
- melonDS;
- remembered/manual source assignment;
- fail-closed validation before assignment/open.

The first staged Party/Box View/Edit milestone is physically accepted.

Current G4-04 implementation includes:

- Party / Box View/Edit;
- empty Box Add/Create;
- native Held Item / Language / Ball / Pokérus / Met Location;
- native Gen IV move picker with species compatibility;
- exact base PP and PP Up reset behavior on move replacement;
- Species mutation and dependent state reconciliation;
- supported Form editing and exact-game restrictions;
- editable OT / Trainer ID where currently supported;
- trainer/origin inspection;
- strict save reparse/checksum/rollback;
- external source immutability.

The integrated Product UI carries real Gen IV trainer-name propagation, real Gen IV Pokédex progress, and game/gender-aware Lucas/Dawn or Ethan/Lyra presentation where the save proves gender.

The full combined G4-04 + Product UI build is **not yet device accepted**.

## Source and launch policy

Discovery and launching are separate from write authorization.

Current provider-aware work includes RetroArch, DraStic, melonDS and manual/remembered bindings. Game launch metadata is stored by PokeBank NX rather than written into emulator save directories.

If a ROM/content target cannot be proven, the UI must request an explicit **Link Game File** rather than infer it from the save path.

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
