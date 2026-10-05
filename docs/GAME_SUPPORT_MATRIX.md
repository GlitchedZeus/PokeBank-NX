# PokeBank NX — Game Support / Verification Matrix

Last updated: **2026-10-05**

This matrix separates implemented support from physical device acceptance.

No current adapter is approved for direct live writing to an installed-game save or emulator source.

| Generation | Games | Read / source state | Staged editing state | Device state |
|---|---|---|---|---|
| I | Red / Blue / Yellow | RetroArch + validated legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| II | Gold / Silver / Crystal | RetroArch + validated legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| III | Ruby / Sapphire / Emerald / FireRed / LeafGreen | RetroArch / provider-aware legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| IV | Diamond / Pearl / Platinum / HeartGold / SoulSilver | DraStic / melonDS / remembered/manual assignment foundation | Party + Box View/Edit, Create, native field/move/form work | Base milestone accepted; full G4-04 **HARDWARE PENDING** |
| V | Black / White / Black 2 / White 2 | Not started as a product workflow | Not started | **NOT SUPPORTED** |
| 3DS | X/Y, ORAS, SM/USUM | Provider/research planning only | Not started | **NOT SUPPORTED** |
| Modern Switch | LGPE, SWSH, BDSP, PLA, SV, Z-A and other tracked identities | validation/source foundation varies by title | production editor/write support not advertised | **NOT GENERALLY ACCEPTED** |

FireRed/LeafGreen GBA and separately tracked Switch release identities remain distinct game/platform identities.

## Generation I

Hardware-accepted current foundation includes:

- Trainer / Party / Boxes;
- generation-correct Pokémon data;
- Bag/PC Items;
- strict source validation;
- staged shared View/Create/Edit;
- exact-game move handling;
- source immutability.

Current Product Home/Games integration adds source assignment, sorting/favorites foundations, real per-save Pokédex progress, and grounded trainer presentation on top of this accepted editor foundation. Those newer shell changes remain part of the current integrated hardware pass.

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

Current integrated development adds real per-save Pokédex progress, game/gender-aware trainer presentation, source reassignment from the Games browser, and installed-forwarder launch preference where a matching title can be proven.

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
- provider-neutral setup/assignment;
- fail-closed validation before assignment/open.

The first staged Party/Box View/Edit milestone is physically accepted.

Current G4-04 implementation includes:

- Party / Box View/Edit;
- empty Box Add/Create;
- native Held Item / Language / Ball / Pokérus / Met Location;
- native Gen IV move picker with species compatibility;
- exact base PP and PP Up reset behavior on move replacement;
- Species mutation and dependent-state reconciliation;
- supported Form editing and exact-game restrictions;
- editable OT / Trainer ID where currently supported;
- trainer/origin inspection;
- strict save reparse/checksum/rollback;
- external source immutability.

The integrated Product UI carries real Gen IV trainer-name propagation, real Gen IV Pokédex progress, game/gender-aware trainer presentation, Games/source assignment, current sorting/favorites work, and direct-launch routing.

The full current G4-04 + Product UI head is **automated-green but not yet device accepted**.

## Product Home / Games / launch integration

Current integrated application work includes:

- full Games artwork browser as a game/save/profile assignment surface;
- `X = Save / Source` source assignment flow;
- provider-neutral Gen IV setup;
- sorting and Release Date ordering foundation;
- Favorites foundation;
- region-scene Product Home presentation with readability treatment;
- neutral idle destination controls with focused blue/cyan state;
- installed HOME-forwarder preference for matching GBA/DS launch targets;
- exact-release matching to reduce similar-name collisions;
- provider/file/link fallback when the target cannot be proven.

The current GBA/DS forwarder-preference and latest Quick Games/Product Home recovery fixes still require physical verification.

## Read-only legality analysis

A separate PR #103 lane now implements a substantial **Gen I–IV evidence-aware legality foundation**.

Current evidence work includes exact source-game profiles, move/species ceilings and compatibility, Gen I/II encounter/history evidence, extensive Gen III event/PID/IV/GameCube evidence, Gen III→IV transfer evidence, and extensive Gen IV wild/static/trade/event/RNG/form/origin evidence.

This does **not** change the support table above into a blanket legality guarantee. The analysis reports one of:

- **Invalid** when available evidence proves a contradiction;
- **No problems found** when the checks that actually ran find no contradiction;
- **Incomplete** when evidence coverage is insufficient.

The legality lane is analysis-only: no auto-fix, no source writes, no staged mutation, and no write-permission changes.

## Audit / hardening state

The full forensic remediation package from PR #101 was merged into the active PR #92 application lane on 2026-10-05.

Final disposition remains:

- **43 VERIFIED / FIXED**
- **1 DEFERRED WITH JUSTIFICATION — AUDIT-043**
- **0 OPEN**

The frozen forensic checkpoint remains historical evidence and is not rewritten by this integration.

## Source and launch policy

Discovery and launching are separate from write authorization.

Current provider-aware work includes RetroArch, DraStic, melonDS and manual/remembered bindings. Game launch metadata is stored by PokeBank NX rather than written into emulator save directories.

For supported GBA/DS identities, the current application can prefer a matching installed HOME forwarder before emulator fallback. If a ROM/content/installed target cannot be proven, the UI must request an explicit source/content selection or **Link Game File** rather than infer it from a save path.

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
source injection              LOCKED
```

Malformed or unsupported sources must never be silently repaired, normalized, reassigned, or overwritten.
