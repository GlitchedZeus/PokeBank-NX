# PokeBank NX — Game Support / Verification Matrix

Last updated: **2026-10-14**

This matrix separates implemented support from physical device acceptance.

No current adapter is approved for direct live writing to an installed-game save or emulator source. **Open draft implementation is not a feature merged into the default branch or owner-device acceptance.**

| Generation | Games | Read / source state | Staged editing state | Device state |
|---|---|---|---|---|
| I | Red / Blue / Yellow | RetroArch + validated legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| II | Gold / Silver / Crystal | RetroArch + validated legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| III | Ruby / Sapphire / Emerald / FireRed / LeafGreen | RetroArch / provider-aware legacy source model | Shared View / Create / Edit + inventory | **DEVICE ACCEPTED** |
| IV | Diamond / Pearl / Platinum / HeartGold / SoulSilver | DraStic / melonDS / remembered/manual assignment foundation | Party + Box View/Edit, Create, native field/move/form work | Earlier integrated milestone accepted; latest finalization **HARDWARE PENDING** |
| V | Black / White / Black 2 / White 2 | Experimental RetroArch/DraStic/melonDS/manual exact BW/B2W2 Save Instances, profile assignment, Primary/Backup | Experimental shared PK5 native staged View/Edit/Create/Review + app-owned export on [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) | **CI GREEN; SWITCH HARDWARE PENDING** |
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

## October 14 Gen V implementation and source state

The [Gen V experimental app PR #300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) at `58bd564ba479745f6129f57167aeac6616297753` has all six exact-head checks green and publishes a [development NRO](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38523683607). The production-owner acceptance flag is still **false**. The owner's previous Switch run could not see Gen V saves although they exist in the **same emulator directories as Gen IV**. Current source discovery, artwork and assignment fixes have **not yet been accepted on the device**.

Implemented Gen V draft coverage includes strict BW/B2W2 70/74 native checksum blocks, two independently validated save copies, real 24 PC boxes, Trainer/Party, PK5 stage/Create/Review and app-owned working-copy export, immutable original source data, manually chosen/emulator Save Instances, and an active-Switch-profile **Assign Save to This Profile** action. Reopening must validate exact game/title/copy and source content. FAT/exFAT path normalization keeps profile ownership stable across routine emulator-save writes; old content-based links may need one deliberate relink.

The stronger, separately tested [Gen V source-hardening PR #304](https://github.com/GlitchedZeus/PokeBank-NX/pull/304) includes additional persisted copy/profile and fair provider budget work; it remains a **stacked draft**, not automatically part of #300.

## October 14 Gen I–IV, touch and launch

[Claude finalization PR #306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) adds verified in-NRO RomFS packaging (10 regions, 20 trainers, game art), fair Gen IV multi-provider discovery and ROM header title verification before game launching. At the checked HEAD, a Gen IV candidate CI job was failing; this work still needs clean native/host gates and an owner device pass. The Gen IV native capture Ball `max(0x83,0x86)` correction is independently tracked in [#305](https://github.com/GlitchedZeus/PokeBank-NX/pull/305). Full touch-only workflow parity and Items/exit navigation require fresh tests.

## Legality limits

Gen I–IV evidence-aware legality is a separate five-lane research/implementation effort ([#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298), [Claude #303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303)); later-generation envelope/domain groundwork is [#301](https://github.com/GlitchedZeus/PokeBank-NX/pull/301). Neither original-game provenance coverage nor full PKHeX historical parity is complete; no production source-save write permission is implied.

See the [cross-lane release-gate snapshot](ENGINEERING_SNAPSHOT_20261014.md).
